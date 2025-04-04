#include "stdafx.hpp"

#include "..\feature.hpp"
#include "..\playerio.hpp"
#include "convar.hpp"
#include "game_detection.hpp"
#include "signals.hpp"
#include "..\autojump.hpp"
#include "SDK\hl_movedata.h"
#include "..\visualizations\imgui\imgui_interface.hpp"

#ifdef OE
static void NoclipNofixCVarCallback(ConVar* pConVar, const char* pOldValue);
#else
static void NoclipNofixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue);
#endif

ConVar y_spt_noclip_nofix("y_spt_noclip_nofix",
                          "0",
                          0,
                          "Stops noclip from fixing player's position after disabling.",
                          NoclipNofixCVarCallback);
ConVar spt_noclip_noslowfly("spt_noclip_noslowfly", "0", 0, "Fix noclip slowfly.");
ConVar spt_noclip_persist(
    "spt_noclip_persist",
    "0",
    0,
    "When enabled, enabling noclip will make the player indefinitely float in the same direction and speed of movement.");

// Noclip related fixes
class NoclipFixesFeature : public FeatureWrapper<NoclipFixesFeature>
{
public:
    void ToggleNoclipNofix(bool enabled);

protected:
    virtual bool ShouldLoadFeature() override;

    virtual void InitHooks() override;

    virtual void LoadFeature() override;

    virtual void UnloadFeature() override;

    // value replacements and single instruction overrides
    // we'll just write the bytes ourselves...
private:
    void OnTick(bool simulating);

    // position fixing prevention
    uintptr_t ORIG_Server__NoclipString = 0;
    std::vector<patterns::MatchedPattern> MATCHES_Server__StringReferences;
    DECL_BYTE_REPLACE(nofixJumpInsc, 6);
    byte nofixInscCompare[4] = {0x84, 0xC0, 0x0F, 0x85};
    int nofixJumpDistance = 0x0;

    // velocity persistence
    uintptr_t ORIG_Server__gpGlobalsTargetString = 0;
    CGlobalVarsBase* servergpGlobals = nullptr;
    DECL_HOOK_THISCALL(void, CGameMovement__FullNoClipMove, void*, float factor, float maxacceleration);
};

static NoclipFixesFeature spt_noclipfixes;

#ifdef OE
static void NoclipNofixCVarCallback(ConVar* pConVar, const char* pOldValue)
#else
static void NoclipNofixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue)
#endif
{
    spt_noclipfixes.ToggleNoclipNofix(((ConVar*)pConVar)->GetBool());
}

namespace patterns
{
    PATTERNS(Server__NoclipString, "1", "6E 6F 63 6C 69 70 20 4F 46 46 0A 00");
    PATTERNS(Server__gpGlobalsTargetString,
             "1",
             "5B 25 30 33 64 5D 20 46 6F 75 6E 64 3A 20 25 73 2C 20 66 69 72 69 6E 67 20 28 25 73 29 0A 00");
    PATTERNS(Server__StringReferences, "1", "68");

    PATTERNS(CGameMovement__FullNoClipMove,
             "BMS 0.9",
             "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B ?? 89 6C 24 ?? 8B EC 83 EC 6C 56 8B F1 8B 0D ?? ?? ?? ??",
             "HL2 5135",
             "83 EC ?? A1 ?? ?? ?? ?? D9 40 2C 56 D8 4C 24 ?? 8B F1 8D 4C 24 ?? 51 8B 4E 08");
} // namespace patterns

bool NoclipFixesFeature::ShouldLoadFeature()
{
    return true;
}

void NoclipFixesFeature::InitHooks()
{
    FIND_PATTERN(server, Server__NoclipString);
    FIND_PATTERN(server, Server__gpGlobalsTargetString);
    FIND_PATTERN_ALL(server, Server__StringReferences);

    HOOK_FUNCTION(server, CGameMovement__FullNoClipMove);
}

void NoclipFixesFeature::LoadFeature()
{
    if (spt_playerio.m_surfaceFriction.Found() && spt_playerio.m_MoveType.Found() && TickSignal.Works)
    {
        TickSignal.Connect(this, &NoclipFixesFeature::OnTick);
        InitConcommandBase(spt_noclip_noslowfly);
    }

    if (!ORIG_Server__NoclipString || MATCHES_Server__StringReferences.empty())
        return;

    bool nofixFound = ORIG_Server__NoclipString == 0;
    bool gpGlobalsFound = ORIG_CGameMovement__FullNoClipMove == nullptr;
    void* serverHandle;
    void* serverBase;
    size_t serverSize = 0;
    MemUtils::GetModuleInfo(L"server.dll", &serverHandle, &serverBase, &serverSize);

    for (auto match : MATCHES_Server__StringReferences)
    {
        if (nofixFound && gpGlobalsFound)
            break;

        uintptr_t strPtr = *(uintptr_t*)(match.ptr + 1);

        if (!nofixFound && strPtr == ORIG_Server__NoclipString)
        {
            /*
            * After printing "noclip OFF\n", the game will do "if ( !TestEntityPosition( pPlayer ) )",
            * which generally translates to
            *	xx					PUSH pPlayer
            *	E8 xx xx xx xx		CALL [TestEntityPosition]
            *	~ noise ~
            *	84 c0				TEST al, al
            *	0f 85 xx xx 00 00	JNE [eof] <-- we'll asume its less than 0x10000 bytes away...
            */
            byte* bytes = (byte*)match.ptr;
            for (int i = 0; i <= 0x200; i++)
            {
                int jmpOff = *(int*)(bytes + i + 4);
                if (jmpOff > 0x10000)
                    continue;

                if (memcmp((void*)(bytes + i), (void*)nofixInscCompare, 4))
                    continue;

                INIT_BYTE_REPLACE(nofixJumpInsc, (uintptr_t)(bytes + i + 2));

                /*
                * When we replace the instruction with a JMP, we replace the last byte with NOP,
                * meaning the starting point of our jump is shifted up by 1 to that byte.
                */
                nofixJumpDistance = jmpOff + 1;
                InitConcommandBase(y_spt_noclip_nofix);
                continue;
            }
        }

        if (!gpGlobalsFound && strPtr == ORIG_Server__gpGlobalsTargetString)
        {
            /*
            * For noclip persist velocity calculations, we'll need access to server's gpGlobals->frametime,  
            * as that acts differently from client's gpGlobals.
            * 
            * We'll be relying on string references again, specifically the reference in FireTargets as
            * that one is close to a string ref.
            * DevMsg( 2, "[%03d] Found: %s, firing (%s)\n", gpGlobals->tickcount%1000, pTarget->GetDebugName(), targetName );
            * 
            * We'll start at the string push, then look back a small distance to find the mov instruction for 1000 for the
            * modulo operation. We'll assume this is right after the mov of gpGlobals
            *   A1 xx xx xx xx      MOV     eax, [gpGlobals]
            *   ?? e8 03 00 00      MOV     ??, 0x3e8
            *   ~ noise ~
            *   68 xx xx xx xx      PUSH    [string]            
            */

            // look back 100 bytes
            const int LOOK_BACK_AMOUNT = 100;
            byte* bytes = (byte*)(match.ptr - LOOK_BACK_AMOUNT);
            for (int i = LOOK_BACK_AMOUNT - 1; i >= 0; i--)
            {
                if (*(int*)(bytes + i) == 0x3e8)
                {
                    for (int j = i; j >= 0; j--)
                    {
                        if (bytes[j] == 0xA1)
                        {
                            uintptr_t candidate = *(uintptr_t*)(bytes + j + 1);
                            if (candidate >= (uintptr_t)serverBase
                                && candidate <= ((uintptr_t)serverBase + serverSize))
                            {
                                servergpGlobals = *(CGlobalVarsBase**)candidate;
                                gpGlobalsFound = true;
                                InitConcommandBase(spt_noclip_persist);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    SptImGuiGroup::QoL_Noclip.RegisterUserCallback(
        []()
        {
            SptImGui::CvarCheckbox(y_spt_noclip_nofix, "##checkbox_nofix");
            SptImGui::CvarCheckbox(spt_noclip_noslowfly, "##checkbox_noslowfly");
            SptImGui::CvarCheckbox(spt_noclip_persist, "##checkbox_persist");
#ifndef OE
            // whoever decided to put this in a different file deserves stale bread
            extern ConVar y_spt_portal_no_ground_snap;
            SptImGui::CvarCheckbox(y_spt_portal_no_ground_snap, "##checkbox_no_ground_snap");
#endif
        });
}

void NoclipFixesFeature::OnTick(bool simulating)
{
    if (!simulating || !spt_noclip_noslowfly.GetBool())
        return;

    if ((unsigned char)spt_playerio.m_MoveType.GetValue() == MOVETYPE_NOCLIP)
    {
        float* friction = spt_playerio.m_surfaceFriction.GetPtr();
        if (!friction)
            return;
        *friction = 1.0f;
    }
}

void NoclipFixesFeature::UnloadFeature()
{
    DESTROY_BYTE_REPLACE(nofixJumpInsc);
    servergpGlobals = nullptr;
}

void NoclipFixesFeature::ToggleNoclipNofix(bool enabled)
{
    if (PTR_nofixJumpInsc == 0)
        return;

    if (enabled)
    {
        uint8 inst_jmp = 0xE9, inst_nop = 0x90;
        MemUtils::ReplaceBytes((void*)PTR_nofixJumpInsc, 1, &inst_jmp);
        MemUtils::ReplaceBytes((void*)(PTR_nofixJumpInsc + 1), 4, (uint8*)&nofixJumpDistance);
        MemUtils::ReplaceBytes((void*)(PTR_nofixJumpInsc + 5), 1, &inst_nop);
    }
    else
    {
        UNDO_BYTE_REPLACE(nofixJumpInsc);
    }
}

IMPL_HOOK_THISCALL(NoclipFixesFeature, void, CGameMovement__FullNoClipMove, void*, float factor, float maxacceleration)
{
    if (!spt_noclip_persist.GetBool())
    {
        spt_noclipfixes.ORIG_CGameMovement__FullNoClipMove(thisptr, factor, maxacceleration);
        return;
    }

    // copied from original function, slightly modified to remove factor

    CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + spt_autojump.off_mv_ptr));
    Vector out;
    VectorMA(mv->GetAbsOrigin(), spt_noclipfixes.servergpGlobals->frametime, mv->m_vecVelocity, out);
    mv->SetAbsOrigin(out);

    return;
}