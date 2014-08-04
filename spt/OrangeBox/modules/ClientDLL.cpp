#include "stdafx.h"

#include "..\cvars.hpp"
#include "..\..\spt.hpp"
#include "..\..\memutils.hpp"
#include "..\..\detoursutils.hpp"
#include "..\..\patterns.hpp"
#include "..\..\hooks.hpp"
#include "ClientDLL.hpp"

using std::uintptr_t;
using std::size_t;

void __cdecl ClientDLL::HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h)
{
	return Hooks::getInstance().clientDLL.HOOKED_DoImageSpaceMotionBlur_Func(view, x, y, w, h);
}

//bool __fastcall ClientDLL::HOOKED_CheckJumpButton(void* thisptr, int edx)
//{
//	return Hooks::getInstance().clientDLL.HOOKED_CheckJumpButton_Func(thisptr, edx);
//}

void __stdcall ClientDLL::HOOKED_HudUpdate(bool bActive)
{
	return Hooks::getInstance().clientDLL.HOOKED_HudUpdate_Func(bActive);
}

void __stdcall ClientDLL::HOOKED_CreateMove(int sequence_number, float input_sample_frametime, bool active)
{
	return Hooks::getInstance().clientDLL.HOOKED_CreateMove_Func(sequence_number, input_sample_frametime, active);
}

void ClientDLL::Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	bool detected5135 = false;

	MemUtils::ptnvec_size ptnNumber;

	// DoImageSpaceMotionBlur
	EngineDevMsg("SPT: Searching for DoImageSpaceMotionBlur...\n");

	uintptr_t pDoImageSpaceMotionBlur = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsDoImageSpaceMotionBlur, &pDoImageSpaceMotionBlur);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_DoImageSpaceMorionBlur = (_DoImageSpaceMotionBlur)pDoImageSpaceMotionBlur;
		EngineDevMsg("SPT: Found DoImageSpaceMotionBlur at %p (using the build %s pattern).\n", pDoImageSpaceMotionBlur, Patterns::ptnsDoImageSpaceMotionBlur[ptnNumber].build.c_str());

		switch (ptnNumber)
		{
		case 0:
			pgpGlobals = *(uintptr_t **)(pDoImageSpaceMotionBlur + 132);
			detected5135 = true;
			break;

		case 1:
			pgpGlobals = *(uintptr_t **)(pDoImageSpaceMotionBlur + 153);
			break;

		case 2:
			pgpGlobals = *(uintptr_t **)(pDoImageSpaceMotionBlur + 129);
			break;

		case 3:
			pgpGlobals = *(uintptr_t **)(pDoImageSpaceMotionBlur + 171);
			break;
		}

		EngineDevMsg("SPT: pgpGlobals is %p.\n", pgpGlobals);
	}
	else
	{
		EngineDevWarning("SPT: Could not find DoImageSpaceMotionBlur!\n");
		EngineWarning("SPT: y_spt_motion_blur_fix has no effect.\n");
	}

	// CheckJumpButton
	// Commented out because used only for client prediction, when we're playing on a server.
	// SPT doesn't support multiplayer games yet so that's not a problem.

	//EngineDevMsg("SPT: [client dll] Searching for CheckJumpButton...\n");

	//uintptr_t pCheckJumpButton = NULL;
	//ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsClientCheckJumpButton, &pCheckJumpButton);
	//if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	//{
	//	ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
	//	EngineDevMsg("SPT: [client dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsClientCheckJumpButton[ptnNumber].build.c_str());

	//	switch (ptnNumber)
	//	{
	//	case 0:
	//		off1M_nOldButtons = 2;
	//		off2M_nOldButtons = 40;
	//		break;

	//	case 1:
	//		off1M_nOldButtons = 1;
	//		off2M_nOldButtons = 40;
	//		break;

	//	case 2:
	//		off1M_nOldButtons = 2;
	//		off2M_nOldButtons = 40;
	//		break;

	//	case 3:
	//		off1M_nOldButtons = 2;
	//		off2M_nOldButtons = 40;
	//		break;

	//	case 4:
	//		off1M_nOldButtons = 2;
	//		off2M_nOldButtons = 40;
	//		break;
	//	}
	//}
	//else
	//{
	//	EngineDevWarning("SPT: [client dll] Could not find CheckJumpButton.\n");
	//}

	// HudUpdate
	EngineDevMsg("SPT: [client dll] Searching for CHLClient::HudUpdate...\n");

	uintptr_t pHudUpdate = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsHudUpdate, &pHudUpdate);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_HudUpdate = (_HudUpdate)pHudUpdate;
		EngineDevMsg("SPT: [client dll] Found CHLClient::HudUpdate at %p (using the build %s pattern).\n", pHudUpdate, Patterns::ptnsHudUpdate[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("SPT: [client dll] Could not find CHLClient::HudUpdate.\n");
		EngineWarning("SPT: [client dll] y_spt_afterframes has no effect.\n");
	}

	// CreateMove
	EngineDevMsg("SPT: [client dll] Trying to obtain the IBaseClientDLL interface...\n");

	CreateInterfaceFn cdllIfaceFactory = (CreateInterfaceFn)GetProcAddress(hModule, "CreateInterface");
	if (cdllIfaceFactory)
	{
		EngineDevMsg("SPT: [client dll] CreateInterface: %p\n", cdllIfaceFactory);

		if (clientDll = (uintptr_t)cdllIfaceFactory("VClient015", NULL))
		{
			EngineDevMsg("SPT: [client dll] IBaseClientDLL v15 is located at %p\n", clientDll);
			viCreateMove = 18;

			// HACK, 5135 exposes updated interfaces with the old version number
			if (detected5135)
				viCreateMove = 19;
		}
		else if (clientDll = (uintptr_t)cdllIfaceFactory("VClient017", NULL))
		{
			EngineDevMsg("SPT: [client dll] IBaseClientDLL v17 is located at %p\n", clientDll);
			viCreateMove = 21;
		}
		else
		{
			EngineDevWarning("SPT: [client dll] Could not obtain the IBaseClientDLL interface.\n");
		}

		if (clientDll)
		{
			ORIG_CreateMove = (_CreateMove)MemUtils::HookVTable(*(uintptr_t**)clientDll, viCreateMove, (uintptr_t)HOOKED_CreateMove);
			EngineDevMsg("SPT: [client dll] CreateMove is located at %p\n", ORIG_CreateMove);
		}
	}
	else
	{
		EngineDevWarning("SPT: [client dll] Could not obtain CreateInterface.\n");
	}

	AttachDetours(moduleName, 4,
		&ORIG_DoImageSpaceMorionBlur, HOOKED_DoImageSpaceMotionBlur,
		//&ORIG_CheckJumpButton, HOOKED_CheckJumpButton,
		&ORIG_HudUpdate, HOOKED_HudUpdate);
}

void ClientDLL::Unhook()
{
	DetachDetours(moduleName, 4,
		&ORIG_DoImageSpaceMorionBlur, HOOKED_DoImageSpaceMotionBlur,
		//&ORIG_CheckJumpButton, HOOKED_CheckJumpButton,
		&ORIG_HudUpdate, HOOKED_HudUpdate);

	if (ORIG_CreateMove)
		MemUtils::HookVTable(*(uintptr_t**)clientDll, viCreateMove, (uintptr_t)ORIG_CreateMove);

	Clear();
}

void ClientDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_DoImageSpaceMorionBlur = nullptr;
	//ORIG_CheckJumpButton = nullptr;
	ORIG_HudUpdate = nullptr;
	ORIG_CreateMove = nullptr;
	pgpGlobals = nullptr;
	//off1M_nOldButtons = NULL;
	//off2M_nOldButtons = NULL;
	//cantJumpNextTime = false;
	clientDll = 0;
	viCreateMove = 0;
}

void ClientDLL::AddIntoAfterframesQueue(const afterframes_entry_t& entry)
{
	afterframesQueue.push_back(entry);
}

void ClientDLL::ResetAfterframesQueue()
{
	afterframesQueue.clear();
}

void ClientDLL::OnFrame()
{
	for (auto it = afterframesQueue.begin(); it != afterframesQueue.end(); )
	{
		it->framesLeft--;
		if (it->framesLeft <= 0)
		{
			EngineConCmd(it->command.c_str());
			it = afterframesQueue.erase(it);
		}
		else
			++it;
	}
}

void __cdecl ClientDLL::HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h)
{
	uintptr_t origgpGlobals = NULL;

	/*
	Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
	so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
	function does not use anything apart from gpGlobals->realtime from gpGlobals,
	so we can do such a replace to make it use gpGlobals->curtime instead without
	breaking anything else.
	*/
	if (pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			origgpGlobals = *pgpGlobals;
			*pgpGlobals = *pgpGlobals + 12;
		}
	}

	ORIG_DoImageSpaceMorionBlur(view, x, y, w, h);

	if (pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			*pgpGlobals = origgpGlobals;
		}
	}
}

//bool __fastcall ClientDLL::HOOKED_CheckJumpButton_Func(void* thisptr, int edx)
//{
//	/*
//	Not sure if this gets called at all from the client dll, but
//	I will just hook it in exactly the same way as the server one.
//	*/
//	const int IN_JUMP = (1 << 1);
//
//	int *pM_nOldButtons = NULL;
//	int origM_nOldButtons = 0;
//
//	if (y_spt_autojump.GetBool())
//	{
//		pM_nOldButtons = (int *)(*((uintptr_t *)thisptr + off1M_nOldButtons) + off2M_nOldButtons);
//		origM_nOldButtons = *pM_nOldButtons;
//
//		if (!cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
//		{
//			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
//		}
//		else
//		{
//			// EngineDevMsg( "SPT: Con jump prevented!\n" );
//		}
//	}
//
//	cantJumpNextTime = false;
//
//	bool rv = ORIG_CheckJumpButton(thisptr, edx); // This function can only change the jump bit.
//
//	if (y_spt_autojump.GetBool())
//	{
//		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
//		{
//			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
//		}
//	}
//
//	if (rv)
//	{
//		// We jumped.
//		if (y_spt_autojump_ensure_legit.GetBool())
//		{
//			cantJumpNextTime = true; // Prevent consecutive jumps.
//		}
//
//		// EngineDevMsg( "SPT: Jump!\n" );
//	}
//
//	EngineDevMsg("SPT: Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false"));
//
//	return rv;
//}

void __stdcall ClientDLL::HOOKED_HudUpdate_Func(bool bActive)
{
	OnFrame();

	return ORIG_HudUpdate(bActive);
}

void __stdcall ClientDLL::HOOKED_CreateMove_Func(int sequence_number, float input_sample_frametime, bool active)
{
	return ORIG_CreateMove(sequence_number, input_sample_frametime, active);
}
