#include "stdafx.h"
#include "..\cvars.hpp"
#include "..\modules.hpp"

#include "..\..\sptlib-wrapper.hpp"
#include <SPTLib\memutils.hpp>
#include <SPTLib\hooks.hpp>
#include "ServerDLL.hpp"
#include "..\patterns.hpp"
#include "..\overlay\overlays.hpp"
#include "..\..\utils\savestate.hpp"

using std::uintptr_t;
using std::size_t;

bool __fastcall ServerDLL::HOOKED_CheckJumpButton(void* thisptr, int edx)
{
	return serverDLL.HOOKED_CheckJumpButton_Func(thisptr, edx);
}

void __fastcall ServerDLL::HOOKED_FinishGravity(void* thisptr, int edx)
{
	return serverDLL.HOOKED_FinishGravity_Func(thisptr, edx);
}

void __fastcall ServerDLL::HOOKED_PlayerRunCommand(void* thisptr, int edx, void* ucmd, void* moveHelper)
{
	return serverDLL.HOOKED_PlayerRunCommand_Func(thisptr, edx, ucmd, moveHelper);
}

int __fastcall ServerDLL::HOOKED_CheckStuck(void* thisptr, int edx)
{
	return serverDLL.HOOKED_CheckStuck_Func(thisptr, edx);
}

void __fastcall ServerDLL::HOOKED_AirAccelerate(void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel)
{
	return serverDLL.HOOKED_AirAccelerate_Func(thisptr, edx, wishdir, wishspeed, accel);
}

void __fastcall ServerDLL::HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove)
{
	return serverDLL.HOOKED_ProcessMovement_Func(thisptr, edx, pPlayer, pMove);
}

float __fastcall ServerDLL::HOOKED_TraceFirePortal(void* thisptr, int edx, bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest)
{
	return serverDLL.HOOKED_TraceFirePortal_Func(thisptr, edx, bPortal2, vTraceStart, vDirection, tr, vFinalPosition, qFinalAngles, iPlacedBy, bTest);
}

void __fastcall ServerDLL::HOOKED_SlidingAndOtherStuff(void* thisptr, int edx, void* a, void* b)
{
	return serverDLL.HOOKED_SlidingAndOtherStuff_Func(thisptr, edx, a, b);
}

int __fastcall ServerDLL::HOOKED_CRestore__ReadAll(void * thisptr, int edx, void * pLeafObject, datamap_t * pLeafMap)
{
	return serverDLL.ORIG_CRestore__ReadAll(thisptr, edx, pLeafObject, pLeafMap);
}

int __fastcall ServerDLL::HOOKED_CRestore__DoReadAll(void * thisptr, int edx, void * pLeafObject, datamap_t * pLeafMap, datamap_t * pCurMap)
{
	utils::AddDatamap(pLeafMap, pLeafObject);
	return serverDLL.ORIG_CRestore__DoReadAll(thisptr, edx, pLeafObject, pLeafMap, pCurMap);
}

int __cdecl ServerDLL::HOOKED_DispatchSpawn(void * pEntity)
{
	return serverDLL.ORIG_DispatchSpawn(pEntity);
}

__declspec(naked) void ServerDLL::HOOKED_MiddleOfSlidingFunction()
{
	/**
	 * This is a hook in the middle of a function.
	 * Save all registers and EFLAGS to restore right before jumping out.
	 * Don't use local variables as they will corrupt the stack.
	 */
	__asm {
		pushad;
		pushfd;
	}

	serverDLL.HOOKED_MiddleOfSlidingFunction_Func();

	__asm {
		popfd;
		popad;
		/**
		 * It's important that nothing modifies the registers, the EFLAGS or the stack past this point,
		 * or the game won't be able to continue normally.
		 */

		jmp serverDLL.ORIG_MiddleOfSlidingFunction;
	}
}

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::server::##name);
#define GET_HOOKEDFUTURE(future_name) \
    { \
        auto pattern = f##future_name.get(); \
        if (ORIG_##future_name) { \
            DevMsg("[server dll] Found " #future_name " at %p (using the %s pattern).\n", ORIG_##future_name, pattern->name()); \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for(int i=0; true; ++i) { if(patterns::server::##future_name.at(i).name() == pattern->name()) { patternContainer.AddIndex((PVOID*)ORIG_##future_name,i); break; } } \
        } else { \
            DevWarning("[server dll] Could not find " #future_name ".\n"); \
        } \
    }

#define GET_FUTURE(future_name) \
    { \
        auto pattern = f##future_name.get(); \
        if (ORIG_##future_name) { \
            DevMsg("[server dll] Found " #future_name " at %p (using the %s pattern).\n", ORIG_##future_name, pattern->name()); \
			for(int i=0; true; ++i) { if(patterns::server::##future_name.at(i).name() == pattern->name()) { patternContainer.AddIndex((PVOID*)ORIG_##future_name,i); break; } } \
		} else { \
			DevWarning("[server dll] Could not find " #future_name ".\n"); \
		} \
}

void ServerDLL::Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept)
{
	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	uintptr_t ORIG_CHL2_Player__HandleInteraction = NULL,
		ORIG_PerformFlyCollisionResolution = NULL,
		ORIG_GetStepSoundVelocities = NULL,
		ORIG_CBaseEntity__SetCollisionGroup = NULL;
	patternContainer.Init(moduleName);

	DEF_FUTURE(FinishGravity);
	DEF_FUTURE(PlayerRunCommand);
	DEF_FUTURE(CheckStuck);
	DEF_FUTURE(MiddleOfSlidingFunction);
	DEF_FUTURE(CheckJumpButton);
	DEF_FUTURE(CHL2_Player__HandleInteraction);
	DEF_FUTURE(PerformFlyCollisionResolution);
	DEF_FUTURE(GetStepSoundVelocities);
	DEF_FUTURE(CBaseEntity__SetCollisionGroup);
	DEF_FUTURE(CreateEntityByName);
	DEF_FUTURE(CRestore__DoReadAll);
	DEF_FUTURE(DispatchSpawn);
	DEF_FUTURE(AllocPooledString);

	GET_HOOKEDFUTURE(FinishGravity);
	GET_HOOKEDFUTURE(PlayerRunCommand);
	GET_HOOKEDFUTURE(CheckStuck);
	GET_HOOKEDFUTURE(MiddleOfSlidingFunction);
	GET_HOOKEDFUTURE(CheckJumpButton);
	GET_HOOKEDFUTURE(CRestore__DoReadAll);
	GET_HOOKEDFUTURE(DispatchSpawn);
	GET_FUTURE(CHL2_Player__HandleInteraction);
	GET_FUTURE(PerformFlyCollisionResolution);
	GET_FUTURE(GetStepSoundVelocities);
	GET_FUTURE(CBaseEntity__SetCollisionGroup);
	GET_FUTURE(CreateEntityByName);
	GET_FUTURE(AllocPooledString);

	// Server-side CheckJumpButton
	if (ORIG_CheckJumpButton)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_CheckJumpButton);
		switch (ptnNumber)
		{
		case 0:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 1:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 2:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 3:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 4:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 5:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 6:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 7:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 8:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 9:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 10:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 11:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 12:
			off1M_nOldButtons = 3;
			off2M_nOldButtons = 40;
			break;

		case 13:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 14:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;
			
		case 15:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;
			
		case 16:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;
		}
	}
	else
	{
		Warning("y_spt_autojump has no effect.\n");
	}

	// FinishGravity
	if (ORIG_FinishGravity)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_FinishGravity);
		switch (ptnNumber)
		{
		case 0:
			off1M_bDucked = 1;
			off2M_bDucked = 2128;
			break;

		case 1:
			off1M_bDucked = 2;
			off2M_bDucked = 3120;
			break;
			
		case 2:
			off1M_bDucked = 2;
			off2M_bDucked = 3184;
			break;
			
		case 3:
			off1M_bDucked = 2;
			off2M_bDucked = 3376;
			break;
			
		case 4:
			off1M_bDucked = 1;
			off2M_bDucked = 3440;
			break;
			
		case 5:
			off1M_bDucked = 1;
			off2M_bDucked = 3500;
			break;
			
		case 6:
			off1M_bDucked = 1;
			off2M_bDucked = 3724;
			break;
			
		case 7:
			off1M_bDucked = 2;
			off2M_bDucked = 3112;
			break;
			
		case 8:
			off1M_bDucked = 1;
			off2M_bDucked = 3416;
			break;
		}
	}
	else
	{
		Warning("y_spt_additional_jumpboost has no effect.\n");
	}

	// PlayerRunCommand
	if (ORIG_PlayerRunCommand)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_FinishGravity);
		switch (ptnNumber)
		{
		case 0:
			offM_vecAbsVelocity = 476;
			break;

		case 1:
			offM_vecAbsVelocity = 476;
			break;

		case 2:
			offM_vecAbsVelocity = 532;
			break;

		case 3:
			offM_vecAbsVelocity = 532;
			break;

		case 4:
			offM_vecAbsVelocity = 532;
			break;

		case 5:
			offM_vecAbsVelocity = 476;
			break;

		case 6:
			offM_vecAbsVelocity = 532;
			break;

		case 7:
			offM_vecAbsVelocity = 532;
			break;

		case 8:
			offM_vecAbsVelocity = 592;
			break;
			
		case 9:
			offM_vecAbsVelocity = 556;
			break;
			
		case 10:
			offM_vecAbsVelocity = 364;
			break;
			
		case 12:
			offM_vecAbsVelocity = 476;
			break;
		}
	}
	else
	{
		DevWarning("[server dll] Could not find PlayerRunCommand!\n");
		Warning("_y_spt_getvel has no effect.\n");
	}

	if (ORIG_CHL2_Player__HandleInteraction)
	{
		offM_afPhysicsFlags = *reinterpret_cast<int*>(ORIG_CHL2_Player__HandleInteraction + 0x16);
		DevMsg("Physics flags offset is %d\n", offM_afPhysicsFlags);
	}

	if (ORIG_PerformFlyCollisionResolution)
	{
		offM_moveCollide = *reinterpret_cast<int*>(ORIG_PerformFlyCollisionResolution + 0x8);
		DevMsg("Move collide offset is %d\n", offM_moveCollide);
	}

	if (ORIG_GetStepSoundVelocities)
	{
		offM_moveType = *reinterpret_cast<int*>(ORIG_GetStepSoundVelocities + 0xB);
		DevMsg("Move type offset is %d\n", offM_moveType);
	}

	if (ORIG_CBaseEntity__SetCollisionGroup)
	{
		offM_collisionGroup = *reinterpret_cast<int*>(ORIG_CBaseEntity__SetCollisionGroup + 0x5);
		DevMsg("Collision group offset is %d\n", offM_collisionGroup);
	}

	// CheckStuck
	if (!ORIG_CheckStuck)
	{
		Warning("y_spt_stucksave has no effect.\n");
	}

	// Middle of DMoMM sliding function.
	if (ORIG_MiddleOfSlidingFunction)
	{
		ORIG_SlidingAndOtherStuff = (_SlidingAndOtherStuff)(&ORIG_MiddleOfSlidingFunction - 0x4bb);
		patternContainer.AddHook(HOOKED_SlidingAndOtherStuff, (PVOID*)ORIG_SlidingAndOtherStuff);
		DevMsg("[server.dll] Found the sliding function at %p.\n", ORIG_SlidingAndOtherStuff);
	}
	else
	{
		DevWarning("[server.dll] Could not find the sliding code!\n");
		Warning("y_spt_on_slide_pause_for has no effect.\n");
	}

	extern void* gm;
	if (gm) {
		auto vftable = reinterpret_cast<void**>(gm);
		//ORIG_AirAccelerate = reinterpret_cast<_AirAccelerate>(MemUtils::HookVTable(vftable, 17, reinterpret_cast<uintptr_t>(HOOKED_AirAccelerate)));
		ORIG_ProcessMovement = reinterpret_cast<_ProcessMovement>(MemUtils::HookVTable(vftable, 1, reinterpret_cast<void*>(HOOKED_ProcessMovement)));

		EngineDevMsg("[server dll] Hooked ProcessMovement through the vftable.\n");
	}

	// TODO: remove fixed offsets.
	SnapEyeAngles = reinterpret_cast<_SnapEyeAngles>((unsigned int)moduleBase + 0x1B92F0);
	FirePortal = reinterpret_cast<_FirePortal>((unsigned int)moduleBase + 0x442090);
	m_hPortalEnvironmentOffsetPtr = reinterpret_cast<int*>((unsigned int)FirePortal + 0xA3);
	GetActiveWeapon = reinterpret_cast<_GetActiveWeapon>((unsigned int)moduleBase + 0xCCE90);
	ORIG_TraceFirePortal = reinterpret_cast<_TraceFirePortal>((unsigned int)moduleBase + 0x441730);
	patternContainer.AddHook(HOOKED_TraceFirePortal, (PVOID*)&ORIG_TraceFirePortal);

	
	patternContainer.Hook();
}

void ServerDLL::Unhook()
{
	extern void* gm;
	if (gm) {
		auto vftable = reinterpret_cast<void**>(gm);
		//MemUtils::HookVTable(vftable, 17, reinterpret_cast<uintptr_t>(ORIG_AirAccelerate));
		MemUtils::HookVTable(vftable, 1, reinterpret_cast<void*>(ORIG_ProcessMovement));

		EngineDevMsg("[server dll] Unhooked ProcessMovement through the vftable.\n");
	}


	patternContainer.Unhook();
	Clear();
}

void ServerDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_CheckJumpButton = nullptr;
	ORIG_FinishGravity = nullptr;
	ORIG_PlayerRunCommand = nullptr;
	ORIG_CheckStuck = nullptr;
	ORIG_AirAccelerate = nullptr;
	ORIG_SlidingAndOtherStuff = nullptr;
	ORIG_MiddleOfSlidingFunction = nullptr;
	off1M_nOldButtons = 0;
	off2M_nOldButtons = 0;
	cantJumpNextTime = false;
	insideCheckJumpButton = false;
	off1M_bDucked = 0;
	off2M_bDucked = 0;
	offM_vecAbsVelocity = 0;
	lastVelocity.Init();
	ticksPassed = 0;
	timerRunning = false;
	sliding = false;
	wasSliding = false;
}

bool __fastcall ServerDLL::HOOKED_CheckJumpButton_Func(void* thisptr, int edx)
{
	const int IN_JUMP = (1 << 1);

	int *pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t *)thisptr + off1M_nOldButtons));
	if (tas_log.GetBool())
		EngineDevMsg("[CheckJumpButton PRE ] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
			mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z,
			mv->m_vecVelocity.x, mv->m_vecVelocity.y, mv->m_vecVelocity.z);

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons = (int *)(*((uintptr_t *)thisptr + off1M_nOldButtons) + off2M_nOldButtons);
		// EngineMsg("thisptr: %p, pM_nOldButtons: %p, difference: %x\n", thisptr, pM_nOldButtons, (pM_nOldButtons - thisptr));
		origM_nOldButtons = *pM_nOldButtons;

		if (!cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
		else
		{
			//EngineDevMsg( "Con jump prevented!\n" );
		}
	}

	cantJumpNextTime = false;

	insideCheckJumpButton = true;
	bool rv = ORIG_CheckJumpButton(thisptr, edx); // This function can only change the jump bit.
	insideCheckJumpButton = false;

	if (y_spt_autojump.GetBool())
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		// We jumped.
		if (_y_spt_autojump_ensure_legit.GetBool())
		{
			cantJumpNextTime = true; // Prevent consecutive jumps.
		}

		//EngineDevMsg( "Jump!\n" );
	}

	//EngineDevMsg( "Engine call: [server dll] CheckJumpButton() => %s\n", (rv ? "true" : "false") );

	//CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t *)thisptr + off1M_nOldButtons));
	//DevMsg("[CJB] maxspeed = %.8f; speed = %.8f; yaw = %.8f; fmove = %.8f; smove = %.8f\n", mv->m_flMaxSpeed, mv->m_vecVelocity.Length2D(), mv->m_vecViewAngles[YAW], mv->m_flForwardMove, mv->m_flSideMove);

	if (tas_log.GetBool())
		EngineDevMsg("[CheckJumpButton POST] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
			mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z,
			mv->m_vecVelocity.x, mv->m_vecVelocity.y, mv->m_vecVelocity.z);

	return rv;
}

void __fastcall ServerDLL::HOOKED_FinishGravity_Func(void* thisptr, int edx)
{
	if (insideCheckJumpButton && y_spt_additional_jumpboost.GetBool())
	{
		CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t *)thisptr + off1M_nOldButtons));
		bool ducked = *(bool*)(*((uintptr_t *)thisptr + off1M_bDucked) + off2M_bDucked);

		// <stolen from gamemovement.cpp>
		{
			Vector vecForward;
			AngleVectors(mv->m_vecViewAngles, &vecForward);
			vecForward.z = 0;
			VectorNormalize(vecForward);

			// We give a certain percentage of the current forward movement as a bonus to the jump speed.  That bonus is clipped
			// to not accumulate over time.
			float flSpeedBoostPerc = (!mv->m_bIsSprinting && !ducked) ? 0.5f : 0.1f;
			float flSpeedAddition = fabs(mv->m_flForwardMove * flSpeedBoostPerc);
			float flMaxSpeed = mv->m_flMaxSpeed + (mv->m_flMaxSpeed * flSpeedBoostPerc);
			float flNewSpeed = (flSpeedAddition + mv->m_vecVelocity.Length2D());

			// If we're over the maximum, we want to only boost as much as will get us to the goal speed
			if (y_spt_additional_jumpboost.GetInt() == 1)
			{
				if (flNewSpeed > flMaxSpeed)
				{
					flSpeedAddition -= flNewSpeed - flMaxSpeed;
				}

				if (mv->m_flForwardMove < 0.0f)
					flSpeedAddition *= -1.0f;
			}

			// Add it on
			VectorAdd((vecForward*flSpeedAddition), mv->m_vecVelocity, mv->m_vecVelocity);
		}
		// </stolen from gamemovement.cpp>
	}

	return ORIG_FinishGravity(thisptr, edx);
}

void __fastcall ServerDLL::HOOKED_PlayerRunCommand_Func(void* thisptr, int edx, void* ucmd, void* moveHelper)
{
	ORIG_PlayerRunCommand(thisptr, edx, ucmd, moveHelper);

	auto prevVelocity = lastVelocity;
	lastVelocity = *(Vector*)((uintptr_t)thisptr + offM_vecAbsVelocity);
	//DevMsg("Gain: %.8f\n", lastVelocity.Length2D() - prevVelocity.Length2D());

	if (timerRunning)
		ticksPassed++;
}

int __fastcall ServerDLL::HOOKED_CheckStuck_Func(void* thisptr, int edx)
{
	auto ret = ORIG_CheckStuck(thisptr, edx);

	if (ret && y_spt_stucksave.GetString()[0] != '\0')
	{
		std::ostringstream oss;
		oss << "save " << y_spt_stucksave.GetString();
		EngineConCmd(oss.str().c_str());
		y_spt_stucksave.SetValue("");
	}

	return ret;
}

void __fastcall ServerDLL::HOOKED_AirAccelerate_Func(void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel)
{
	const double M_RAD2DEG = 180 / M_PI;

	CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t *)thisptr + off1M_nOldButtons));
	DevMsg("[AA Pre ] velocity: %.8f %.8f %.8f\n", mv->m_vecVelocity.x, mv->m_vecVelocity.y, mv->m_vecVelocity.z);
	DevMsg("[AA Pre ] speed = %.8f; wishspeed = %.8f; accel = %.8f; wishdir = %.8f; surface friction = %.8f\n", mv->m_vecVelocity.Length2D(), wishspeed, accel, atan2(wishdir->y, wishdir->x) * M_RAD2DEG, *(float*)(*(uintptr_t*)((uintptr_t)thisptr + 4) + 3812));

	ORIG_AirAccelerate(thisptr, edx, wishdir, wishspeed, accel);

	DevMsg("[AA Post] speed = %.8f\n", mv->m_vecVelocity.Length2D());
}

void __fastcall ServerDLL::HOOKED_ProcessMovement_Func(void* thisptr, int edx, void* pPlayer, void* pMove)
{
	CHLMoveData *mv = reinterpret_cast<CHLMoveData*>(pMove);
	if (tas_log.GetBool())
		EngineDevMsg("[ProcessMovement PRE ] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
			mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z,
			mv->m_vecVelocity.x, mv->m_vecVelocity.y, mv->m_vecVelocity.z);

	ORIG_ProcessMovement(thisptr, edx, pPlayer, pMove);

	if (tas_log.GetBool())
		EngineDevMsg("[ProcessMovement POST] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
			mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z,
			mv->m_vecVelocity.x, mv->m_vecVelocity.y, mv->m_vecVelocity.z);
}

float __fastcall ServerDLL::HOOKED_TraceFirePortal_Func(void* thisptr, int edx, bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest)
{
	const auto rv = ORIG_TraceFirePortal(thisptr, edx, bPortal2, vTraceStart, vDirection, tr, vFinalPosition, qFinalAngles, iPlacedBy, bTest);

	lastTraceFirePortalDistanceSq = (vFinalPosition - vTraceStart).LengthSqr();
	lastTraceFirePortalNormal = tr.plane.normal;

	return rv;
}

void __fastcall ServerDLL::HOOKED_SlidingAndOtherStuff_Func(void* thisptr, int edx, void* a, void* b)
{
	if (sliding) {
		sliding = false;
		wasSliding = true;
	} else {
		wasSliding = false;
	}

	return ORIG_SlidingAndOtherStuff(thisptr, edx, a, b);
}

void ServerDLL::HOOKED_MiddleOfSlidingFunction_Func()
{
	//EngineDevMsg("Sliding!\n");

	sliding = true;

	if (!wasSliding) {
		const auto pauseFor = y_spt_on_slide_pause_for.GetInt();
		if (pauseFor > 0) {
			EngineConCmd("setpause");

			afterframes_entry_t entry;
			entry.framesLeft = pauseFor;
			entry.command = "unpause";
			clientDLL.AddIntoAfterframesQueue(entry);
		}
	}
}

int ServerDLL::GetPlayerPhysicsFlags() const
{
	if (!serverActive())
		return -1;
	else 
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_afPhysicsFlags));
}

int ServerDLL::GetPlayerMoveType() const
{
	if (!serverActive())
		return -1;
	else 
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_moveType)) & 0xF;
}

int ServerDLL::GetPlayerMoveCollide() const
{
	if (!serverActive())
		return -1;
	else
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_moveCollide)) & 0x7;
}

int ServerDLL::GetPlayerCollisionGroup() const
{
	if (!serverActive())
		return -1;
	else
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_collisionGroup));
}

int ServerDLL::GetEnviromentPortalHandle() const
{
	if (!serverActive())
		return -1;
	else
	{
		int offset = *m_hPortalEnvironmentOffsetPtr;

		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offset));
	}
}