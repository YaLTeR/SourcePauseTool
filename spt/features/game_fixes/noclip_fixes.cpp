#include "stdafx.hpp"

#include "..\feature.hpp"
#include "..\playerio.hpp"
#include "convar.hpp"
#include "game_detection.hpp"
#include "signals.hpp"

#ifdef  OE
static void NoclipNofixCVarCallback(ConVar* pConVar, const char* pOldValue);
#else
static void NoclipNofixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue);
#endif

ConVar y_spt_noclip_nofix("y_spt_noclip_nofix",
						  "0",
						  0,
                          "Disables noclip's position fixing.",
                          NoclipNofixCVarCallback);
ConVar spt_noclip_noslowfly("spt_noclip_noslowfly", "0", 0, "Fix noclip slowfly.");

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
	void OnTick();

	uintptr_t ORIG_Server__NoclipString = 0;
	std::vector<patterns::MatchedPattern> MATCHES_Server__StringReferences;

	DECL_BYTE_REPLACE(Jump, 6);

	byte inscCompare[4] = {0x84, 0xC0, 0x0F, 0x85};
	int jmpDistance = 0x0;
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
	PATTERNS(Server__StringReferences, "1", "68");
}

bool NoclipFixesFeature::ShouldLoadFeature()
{
	return true;
}

void NoclipFixesFeature::InitHooks()
{
	FIND_PATTERN(server, Server__NoclipString);
	FIND_PATTERN_ALL(server, Server__StringReferences);
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

	for (auto match : MATCHES_Server__StringReferences)
	{
		if (*(uintptr_t*)(match.ptr + 1) != ORIG_Server__NoclipString)
			continue;

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

			if (memcmp((void*)(bytes + i), (void*)inscCompare, 4))
				continue;

			INIT_BYTE_REPLACE(Jump, (uintptr_t)(bytes + i + 2));

			/*
			* When we replace the instruction with a JMP, we replace the last byte with NOP,
			* meaning the starting point of our jump is shifted up by 1 to that byte.
			*/
			jmpDistance = jmpOff + 1;
			InitConcommandBase(y_spt_noclip_nofix);
			return;
		}

	}
}

void NoclipFixesFeature::OnTick()
{
	if (!spt_noclip_noslowfly.GetBool())
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
	DESTROY_BYTE_REPLACE(Jump);
}


void NoclipFixesFeature::ToggleNoclipNofix(bool enabled)
{
	if (PTR_Jump == 0)
		return;

	if (enabled)
	{
		MemUtils::ReplaceBytes((void*)PTR_Jump, 1, new uint8[1]{0xE9});
		MemUtils::ReplaceBytes((void*)(PTR_Jump + 1), 4, (uint8*)&jmpDistance);
		MemUtils::ReplaceBytes((void*)(PTR_Jump + 5), 1, new uint8[1]{0x90});
	}
	else
	{
		UNDO_BYTE_REPLACE(Jump);
	}
}
