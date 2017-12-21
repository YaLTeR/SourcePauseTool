#include "stdafx.hpp"

#include "..\cvars.hpp"
#include "..\modules.hpp"
#include "..\patterns.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include <SPTLib\memutils.hpp>
#include <SPTLib\detoursutils.hpp>
#include <SPTLib\hooks.hpp>
#include "ServerDLL.hpp"

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

void ServerDLL::Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	MemUtils::ptnvec_size ptnNumber;

	uintptr_t pCheckJumpButton = NULL,
		pFinishGravity = NULL,
		pPlayerRunCommand = NULL,
		pCheckStuck = NULL,
		pMiddleOfSlidingFunction = NULL;

	auto fFinishGravity = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsFinishGravity, &pFinishGravity);
	auto fPlayerRunCommand = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsPlayerRunCommand, &pPlayerRunCommand);
	auto fCheckStuck = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsCheckStuck, &pCheckStuck);
	auto fMiddleOfSlidingFunction = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsMiddleOfSlidingFunction, &pMiddleOfSlidingFunction);

	// Server-side CheckJumpButton
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsServerCheckJumpButton, &pCheckJumpButton);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
		EngineDevMsg("[server dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsServerCheckJumpButton[ptnNumber].build.c_str());

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
		EngineDevWarning("[server dll] Could not find CheckJumpButton!\n");
		EngineWarning("y_spt_autojump has no effect.\n");
	}

	// FinishGravity
	ptnNumber = fFinishGravity.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_FinishGravity = (_FinishGravity)pFinishGravity;
		EngineDevMsg("[server dll] Found FinishGravity at %p (using the build %s pattern).\n", pFinishGravity, Patterns::ptnsFinishGravity[ptnNumber].build.c_str());

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
		EngineDevWarning("[server dll] Could not find FinishGravity!\n");
		EngineWarning("y_spt_additional_jumpboost has no effect.\n");
	}

	// PlayerRunCommand
	ptnNumber = fPlayerRunCommand.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_PlayerRunCommand = (_PlayerRunCommand)pPlayerRunCommand;
		EngineDevMsg("[server dll] Found PlayerRunCommand at %p (using the build %s pattern).\n", pPlayerRunCommand, Patterns::ptnsPlayerRunCommand[ptnNumber].build.c_str());

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
		EngineDevWarning("[server dll] Could not find PlayerRunCommand!\n");
		EngineWarning("_y_spt_getvel has no effect.\n");
	}

	// CheckStuck
	ptnNumber = fCheckStuck.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_CheckStuck = (_CheckStuck)pCheckStuck;
		EngineDevMsg("[server dll] Found CheckStuck at %p (using the build %s pattern).\n", pCheckStuck, Patterns::ptnsCheckStuck[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("[server dll] Could not find CheckStuck!\n");
		EngineWarning("y_spt_stucksave has no effect.\n");
	}

	// Middle of DMoMM sliding function.
	ptnNumber = fMiddleOfSlidingFunction.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_MiddleOfSlidingFunction = (void*)pMiddleOfSlidingFunction;
		ORIG_SlidingAndOtherStuff = (_SlidingAndOtherStuff)(pMiddleOfSlidingFunction - 0x4bb);
		EngineDevMsg("[server dll] Found the sliding code at %p (using the build %s pattern).\n", pMiddleOfSlidingFunction, Patterns::ptnsMiddleOfSlidingFunction[ptnNumber].build.c_str());
		EngineDevMsg("[server dll] Found the sliding function at %p.\n", ORIG_SlidingAndOtherStuff);
	}
	else
	{
		EngineDevWarning("[server dll] Could not find the sliding code!\n");
		EngineWarning("y_spt_on_slide_pause_for has no effect.\n");
	}

	extern void* gm;
	if (gm) {
		auto vftable = *reinterpret_cast<uintptr_t**>(gm);
		//ORIG_AirAccelerate = reinterpret_cast<_AirAccelerate>(MemUtils::HookVTable(vftable, 17, reinterpret_cast<uintptr_t>(HOOKED_AirAccelerate)));
		ORIG_ProcessMovement = reinterpret_cast<_ProcessMovement>(MemUtils::HookVTable(vftable, 1, reinterpret_cast<uintptr_t>(HOOKED_ProcessMovement)));

		EngineDevMsg("[server dll] Hooked ProcessMovement through the vftable.\n");
	}

	// TODO: remove fixed offsets.
	SnapEyeAngles = reinterpret_cast<_SnapEyeAngles>(moduleStart + 0x1B92F0);
	FirePortal = reinterpret_cast<_FirePortal>(moduleStart + 0x442090);
	GetActiveWeapon = reinterpret_cast<_GetActiveWeapon>(moduleStart + 0xCCE90);
	ORIG_TraceFirePortal = reinterpret_cast<_TraceFirePortal>(moduleStart + 0x441730);

	DetoursUtils::AttachDetours(moduleName, {
		{ (PVOID *)(&ORIG_CheckJumpButton), HOOKED_CheckJumpButton },
		{ (PVOID *)(&ORIG_FinishGravity), HOOKED_FinishGravity },
		{ (PVOID *)(&ORIG_PlayerRunCommand), HOOKED_PlayerRunCommand },
		{ (PVOID *)(&ORIG_CheckStuck), HOOKED_CheckStuck },
		{ (PVOID *)(&ORIG_TraceFirePortal), HOOKED_TraceFirePortal },
		{ (PVOID *)(&ORIG_SlidingAndOtherStuff), HOOKED_SlidingAndOtherStuff },
		{ &ORIG_MiddleOfSlidingFunction, HOOKED_MiddleOfSlidingFunction }
	});
}

void ServerDLL::Unhook()
{
	extern void* gm;
	if (gm) {
		auto vftable = *reinterpret_cast<uintptr_t**>(gm);
		//MemUtils::HookVTable(vftable, 17, reinterpret_cast<uintptr_t>(ORIG_AirAccelerate));
		MemUtils::HookVTable(vftable, 1, reinterpret_cast<uintptr_t>(ORIG_ProcessMovement));

		EngineDevMsg("[server dll] Unhooked ProcessMovement through the vftable.\n");
	}

	DetoursUtils::DetachDetours(moduleName, {
		{ (PVOID *)(&ORIG_CheckJumpButton), HOOKED_CheckJumpButton },
		{ (PVOID *)(&ORIG_FinishGravity), HOOKED_FinishGravity },
		{ (PVOID *)(&ORIG_PlayerRunCommand), HOOKED_PlayerRunCommand },
		{ (PVOID *)(&ORIG_CheckStuck), HOOKED_CheckStuck },
		{ (PVOID *)(&ORIG_TraceFirePortal), HOOKED_TraceFirePortal },
		{ (PVOID *)(&ORIG_SlidingAndOtherStuff), HOOKED_SlidingAndOtherStuff },
		{ &ORIG_MiddleOfSlidingFunction, HOOKED_MiddleOfSlidingFunction }
	});

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
