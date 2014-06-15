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

void ServerDLL::Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	MemUtils::ptnvec_size ptnNumber;

	// CheckJumpButton
	EngineDevLog("SPT: [server dll] Searching for CheckJumpButton...\n");

	uintptr_t pCheckJumpButton = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsServerCheckJumpButton, &pCheckJumpButton);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
		EngineLog("SPT: [server dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsServerCheckJumpButton[ptnNumber].build.c_str());

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
		}
	}
	else
	{
		EngineWarning("SPT: [server dll] Could not find CheckJumpButton!\n");
		EngineWarning("SPT: [server dll] y_spt_autojump has no effect.\n");
	}

	AttachDetours(moduleName, 2,
		&ORIG_CheckJumpButton, HOOKED_CheckJumpButton);
}

void ServerDLL::Unhook()
{
	DetachDetours(moduleName, 2,
		&ORIG_CheckJumpButton, HOOKED_CheckJumpButton);

	Clear();
}

void ServerDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_CheckJumpButton = nullptr;
	off1M_nOldButtons = NULL;
	off2M_nOldButtons = NULL;
	cantJumpNextTime = false;
}

bool __fastcall ServerDLL::HOOKED_CheckJumpButton_Func(void* thisptr, int edx)
{
	/*
	CheckJumpButton calls FinishGravity() if and only if we jumped.
	*/
	const int IN_JUMP = (1 << 1);

	int *pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons = (int *)(*((uintptr_t *)thisptr + off1M_nOldButtons) + off2M_nOldButtons);
		// EngineLog("thisptr: %p, pM_nOldButtons: %p, difference: %x\n", thisptr, pM_nOldButtons, (pM_nOldButtons - thisptr));
		origM_nOldButtons = *pM_nOldButtons;

		if (!cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
		else
		{
			//EngineDevLog( "SPT: Con jump prevented!\n" );
		}
	}

	cantJumpNextTime = false;

	bool rv = ORIG_CheckJumpButton(thisptr, edx); // This function can only change the jump bit.

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

		//EngineDevLog( "SPT: Jump!\n" );
	}

	//EngineDevLog( "SPT: Engine call: [server dll] CheckJumpButton() => %s\n", (rv ? "true" : "false") );

	return rv;
}