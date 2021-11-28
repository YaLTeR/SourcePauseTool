#include "stdafx.h"

#if defined(SSDK2007) || defined(SSDK2013)

#include "..\feature.hpp"
#include "..\utils\game_detection.hpp"
#include "..\cvars.hpp"

#include "dbg.h"

ConVar y_spt_prevent_vag_crash(
    "y_spt_prevent_vag_crash",
    "0",
    FCVAR_CHEAT | FCVAR_DONTRECORD,
    "Prevents the game from crashing from too many recursive teleports (useful when searching for vertical angle glitches).\n");

// y_spt_prevent_vag_crash
class VAG : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void* ORIG_MiddleOfTeleportTouchingEntity = nullptr;
	void* ORIG_EndOfTeleportTouchingEntity = nullptr;
	int recursiveTeleportCount = 0;

	static void HOOKED_MiddleOfTeleportTouchingEntity();
	static void __fastcall HOOKED_MiddleOfTeleportTouchingEntity_Func(void* portalPtr, void* tpStackPointer);
	static void HOOKED_EndOfTeleportTouchingEntity();
	void HOOKED_EndOfTeleportTouchingEntity_Func();
};

static VAG spt_vag;

bool VAG::ShouldLoadFeature()
{
	return utils::DoesGameLookLikePortal();
}

void VAG::InitHooks()
{
	HOOK_FUNCTION(server, MiddleOfTeleportTouchingEntity);
	HOOK_FUNCTION(server, EndOfTeleportTouchingEntity);
}

void VAG::LoadFeature()
{
	if (!ORIG_MiddleOfTeleportTouchingEntity || !ORIG_EndOfTeleportTouchingEntity)
	{
		DevWarning("[server.dll] Could not find the teleport function!\n");
	}
	recursiveTeleportCount = 0;
}

void VAG::UnloadFeature() {}

__declspec(naked) void VAG::HOOKED_MiddleOfTeleportTouchingEntity()
{
	/**
	* We want a pointer to the portal and the coords of whatever is being teleported.
	* The former is currently in ebp (which is not used here as the stack frame pointer), and the latter
	* is somewhere on the stack.
	*/
	__asm {
		pushad;
		pushfd;
		mov ecx, ebp; // first paremeter - pass portal ref
		mov edx, esp; // second parameter - pass stack pointer
		add edx, 0x24; // account for pushad/pushfd
		call VAG::HOOKED_MiddleOfTeleportTouchingEntity_Func;
		popfd;
		popad;
		jmp spt_vag.ORIG_MiddleOfTeleportTouchingEntity;
	}
}

__declspec(naked) void VAG::HOOKED_EndOfTeleportTouchingEntity()
{
	__asm {
		pushad;
		pushfd;
	}
	spt_vag.HOOKED_EndOfTeleportTouchingEntity_Func();
	__asm {
		popfd;
		popad;
		jmp spt_vag.ORIG_EndOfTeleportTouchingEntity;
	}
}

/**
* A no free edicts crash when trying to do a vag happens when the 2nd teleport places the entity
* behind the entry portal. This causes another teleport by the entry portal to be queued which
* sometimes places the entity right back to where it started, triggering another vag. This process is
* recursive, and would eventually cause a stack overflow if the game didn't crash from allocating an
* edict for a shadowclone every single teleport. This function detects when there are too many recursive
* teleports, and nudges the entity position before the teleport so that it doesn't return to exactly the
* same spot. The position vector is on the stack at this point, so we access it via a stack pointer from
* the original teleport function. This works for both players and other entities.
*/
void __fastcall VAG::HOOKED_MiddleOfTeleportTouchingEntity_Func(void* portalPtr, void* tpStackPointer)
{
	if (!spt_vag.ORIG_EndOfTeleportTouchingEntity || !y_spt_prevent_vag_crash.GetBool())
		return;
	if (spt_vag.recursiveTeleportCount++ > 2)
	{
		Msg("spt: nudging entity to prevent more recursive teleports!\n");
		Vector* entPos = (Vector*)((uint32_t*)tpStackPointer + 26);
		Vector* portalNorm = *((Vector**)portalPtr + 2505) + 2;
		DevMsg(
		    "spt: ent coords in TeleportTouchingEntity: %f %f %f, portal norm: %f %f %f, %i recursive teleports\n",
		    entPos->x,
		    entPos->y,
		    entPos->z,
		    portalNorm->x,
		    portalNorm->y,
		    portalNorm->z,
		    spt_vag.recursiveTeleportCount);
		// push entity further into the portal so it comes further out after the teleport
		entPos->x -= portalNorm->x;
		entPos->y -= portalNorm->y;
		entPos->z -= portalNorm->z;
	}
}

void VAG::HOOKED_EndOfTeleportTouchingEntity_Func()
{
	if (y_spt_prevent_vag_crash.GetBool())
		recursiveTeleportCount--;
}

#endif