#include <cstddef>
#include <cstdint>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "..\cvars.hpp"
#include "..\..\spt.hpp"
#include "..\..\memutils.hpp"
#include "..\..\detoursutils.hpp"
#include "..\..\patterns.hpp"
#include "..\..\hooks.hpp"
#include "ServerDLL.hpp"

using std::uintptr_t;
using std::size_t;

bool __fastcall ServerDLL::HOOKED_CheckJumpButton(void* thisptr, int edx)
{
	return Hooks::getInstance().serverDLL.HOOKED_CheckJumpButton_Func(thisptr, edx);
}

void __fastcall ServerDLL::HOOKED_FinishGravity(void* thisptr, int edx)
{
	return Hooks::getInstance().serverDLL.HOOKED_FinishGravity_Func(thisptr, edx);
}

void ServerDLL::Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	MemUtils::ptnvec_size ptnNumber;

	// CheckJumpButton
	EngineDevMsg("SPT: [server dll] Searching for CheckJumpButton...\n");

	uintptr_t pCheckJumpButton = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsServerCheckJumpButton, &pCheckJumpButton);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
		EngineDevMsg("SPT: [server dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsServerCheckJumpButton[ptnNumber].build.c_str());

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
		}
	}
	else
	{
		EngineDevWarning("SPT: [server dll] Could not find CheckJumpButton!\n");
		EngineWarning("SPT: [server dll] y_spt_autojump has no effect.\n");
	}

	// FinishGravity
	EngineDevMsg("SPT: [server dll] Searching for FinishGravity...\n");

	uintptr_t pFinishGravity = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsFinishGravity, &pFinishGravity);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_FinishGravity = (_FinishGravity)pFinishGravity;
		EngineDevMsg("SPT: [server dll] Found FinishGravity at %p (using the build %s pattern).\n", pFinishGravity, Patterns::ptnsFinishGravity[ptnNumber].build.c_str());

		switch (ptnNumber)
		{
		case 0:
			off1M_bDucked = 1;
			off2M_bDucked = 2128;
			break;
		}
	}
	else
	{
		EngineDevWarning("SPT: [server dll] Could not find FinishGravity!\n");
		EngineWarning("SPT: [server dll] y_spt_additional_abh has no effect.\n");
	}

	AttachDetours(moduleName, 4,
		&ORIG_CheckJumpButton, HOOKED_CheckJumpButton,
		&ORIG_FinishGravity, HOOKED_FinishGravity);
}

void ServerDLL::Unhook()
{
	DetachDetours(moduleName, 4,
		&ORIG_CheckJumpButton, HOOKED_CheckJumpButton,
		&ORIG_FinishGravity, HOOKED_FinishGravity);

	Clear();
}

void ServerDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_CheckJumpButton = nullptr;
	ORIG_FinishGravity = nullptr;
	off1M_nOldButtons = NULL;
	off2M_nOldButtons = NULL;
	cantJumpNextTime = false;
	insideCheckJumpButton = false;
	off1M_bDucked = NULL;
	off2M_bDucked = NULL;
}

bool __fastcall ServerDLL::HOOKED_CheckJumpButton_Func(void* thisptr, int edx)
{
	const int IN_JUMP = (1 << 1);

	int *pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	//CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t *)thisptr + off1M_nOldButtons));
	//EngineDevMsg("SPT: (x, y, z) %.8f %.8f %.8f\n", mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, mv->GetAbsOrigin().z);

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
			//EngineDevMsg( "SPT: Con jump prevented!\n" );
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
		if (y_spt_autojump_ensure_legit.GetBool())
		{
			cantJumpNextTime = true; // Prevent consecutive jumps.
		}

		//EngineDevMsg( "SPT: Jump!\n" );
	}

	//EngineDevMsg( "SPT: Engine call: [server dll] CheckJumpButton() => %s\n", (rv ? "true" : "false") );

	return rv;
}

void __fastcall ServerDLL::HOOKED_FinishGravity_Func(void* thisptr, int edx)
{
	if (insideCheckJumpButton && y_spt_additional_abh.GetBool())
	{
		CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t *)thisptr + off1M_nOldButtons));
		bool ducked = *(bool*)(*((uintptr_t *)thisptr + off1M_bDucked) + off2M_bDucked);

		// <stolen from gamemovement.cpp>
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
		if (flNewSpeed > flMaxSpeed)
		{
			flSpeedAddition -= flNewSpeed - flMaxSpeed;
		}

		if (mv->m_flForwardMove < 0.0f)
			flSpeedAddition *= -1.0f;

		// Add it on
		VectorAdd((vecForward*flSpeedAddition), mv->m_vecVelocity, mv->m_vecVelocity);
		// </stolen from gamemovement.cpp>
	}

	return ORIG_FinishGravity(thisptr, edx);
}