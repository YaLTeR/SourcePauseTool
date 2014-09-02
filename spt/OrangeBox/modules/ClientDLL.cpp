#include "stdafx.hpp"

#include <future>

#include "..\cvars.hpp"
#include "..\modules.hpp"
#include "..\patterns.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include <SPTLib\memutils.hpp>
#include <SPTLib\detoursutils.hpp>
#include <SPTLib\hooks.hpp>
#include "ClientDLL.hpp"

using std::uintptr_t;
using std::size_t;

void __cdecl ClientDLL::HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h)
{
	return clientDLL.HOOKED_DoImageSpaceMotionBlur_Func(view, x, y, w, h);
}

//bool __fastcall ClientDLL::HOOKED_CheckJumpButton(void* thisptr, int edx)
//{
//	return Hooks::getInstance().clientDLL.HOOKED_CheckJumpButton_Func(thisptr, edx);
//}

void __stdcall ClientDLL::HOOKED_HudUpdate(bool bActive)
{
	return clientDLL.HOOKED_HudUpdate_Func(bActive);
}

int __fastcall ClientDLL::HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState)
{
	return clientDLL.HOOKED_GetButtonBits_Func(thisptr, edx, bResetState);
}

void __fastcall ClientDLL::HOOKED_AdjustAngles(void* thisptr, int edx, float frametime)
{
	return clientDLL.HOOKED_AdjustAngles_Func(thisptr, edx, frametime);
}

void ClientDLL::Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	MemUtils::ptnvec_size ptnNumber;

	uintptr_t pHudUpdate,
		pGetButtonBits,
		pAdjustAngles;

	auto fHudUpdate = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsHudUpdate, &pHudUpdate);
	auto fGetButtonBits = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsGetButtonBits, &pGetButtonBits);
	auto fAdjustAngles = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, Patterns::ptnsAdjustAngles, &pAdjustAngles);

	// DoImageSpaceMotionBlur
	uintptr_t pDoImageSpaceMotionBlur = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsDoImageSpaceMotionBlur, &pDoImageSpaceMotionBlur);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_DoImageSpaceMorionBlur = (_DoImageSpaceMotionBlur)pDoImageSpaceMotionBlur;
		EngineDevMsg("Found DoImageSpaceMotionBlur at %p (using the build %s pattern).\n", pDoImageSpaceMotionBlur, Patterns::ptnsDoImageSpaceMotionBlur[ptnNumber].build.c_str());

		switch (ptnNumber)
		{
		case 0:
			pgpGlobals = *(uintptr_t **)(pDoImageSpaceMotionBlur + 132);
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

		EngineDevMsg("pgpGlobals is %p.\n", pgpGlobals);
	}
	else
	{
		EngineDevWarning("[client dll] Could not find DoImageSpaceMotionBlur!\n");
		EngineWarning("y_spt_motion_blur_fix has no effect.\n");
	}

	// CheckJumpButton
	// Commented out because used only for client prediction, when we're playing on a server.
	// SPT doesn't support multiplayer games yet so that's not a problem.

	//EngineDevMsg("[client dll] Searching for CheckJumpButton...\n");

	//uintptr_t pCheckJumpButton = NULL;
	//ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsClientCheckJumpButton, &pCheckJumpButton);
	//if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	//{
	//	ORIG_CheckJumpButton = (_CheckJumpButton)pCheckJumpButton;
	//	EngineDevMsg("[client dll] Found CheckJumpButton at %p (using the build %s pattern).\n", pCheckJumpButton, Patterns::ptnsClientCheckJumpButton[ptnNumber].build.c_str());

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
	//	EngineDevWarning("[client dll] Could not find CheckJumpButton.\n");
	//}

	// HudUpdate
	ptnNumber = fHudUpdate.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_HudUpdate = (_HudUpdate)pHudUpdate;
		EngineDevMsg("[client dll] Found CHLClient::HudUpdate at %p (using the build %s pattern).\n", pHudUpdate, Patterns::ptnsHudUpdate[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("[client dll] Could not find CHLClient::HudUpdate.\n");
		EngineWarning("y_spt_afterframes has no effect.\n");
	}

	// GetButtonBits
	ptnNumber = fGetButtonBits.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_GetButtonBits = (_GetButtonBits)pGetButtonBits;
		EngineDevMsg("[client dll] Found CInput::GetButtonBits at %p (using the build %s pattern).\n", pGetButtonBits, Patterns::ptnsGetButtonBits[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("[client dll] Could not find CInput::GetButtonBits.\n");
		EngineWarning("+y_spt_duckspam has no effect.\n");
	}

	// AdjustAngles
	ptnNumber = fAdjustAngles.get();
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_AdjustAngles = (_AdjustAngles)pAdjustAngles;
		EngineDevMsg("[client dll] Found CInput::AdjustAngles at %p (using the build %s pattern).\n", pAdjustAngles, Patterns::ptnsAdjustAngles[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("[client dll] Could not find CInput::AdjustAngles.\n");
		EngineWarning("_y_spt_setpitch and _y_spt_setyaw have no effect.\n");
		EngineWarning("_y_spt_pitchspeed and _y_spt_yawspeed have no effect.\n");
	}

	AttachDetours(moduleName, 8,
		&ORIG_DoImageSpaceMorionBlur, HOOKED_DoImageSpaceMotionBlur,
		//&ORIG_CheckJumpButton, HOOKED_CheckJumpButton,
		&ORIG_HudUpdate, HOOKED_HudUpdate,
		&ORIG_GetButtonBits, HOOKED_GetButtonBits,
		&ORIG_AdjustAngles, HOOKED_AdjustAngles);
}

void ClientDLL::Unhook()
{
	DetachDetours(moduleName, 8,
		&ORIG_DoImageSpaceMorionBlur, HOOKED_DoImageSpaceMotionBlur,
		//&ORIG_CheckJumpButton, HOOKED_CheckJumpButton,
		&ORIG_HudUpdate, HOOKED_HudUpdate,
		&ORIG_GetButtonBits, HOOKED_GetButtonBits,
		&ORIG_AdjustAngles, HOOKED_AdjustAngles);

	Clear();
}

void ClientDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_DoImageSpaceMorionBlur = nullptr;
	//ORIG_CheckJumpButton = nullptr;
	ORIG_HudUpdate = nullptr;
	ORIG_GetButtonBits = nullptr;
	ORIG_AdjustAngles = nullptr;

	pgpGlobals = nullptr;

	afterframesQueue.clear();
	duckspam = false;

	setPitch.set = false;
	setYaw.set = false;
	//off1M_nOldButtons = NULL;
	//off2M_nOldButtons = NULL;
	//cantJumpNextTime = false;
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
//			// EngineDevMsg( "Con jump prevented!\n" );
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
//		// EngineDevMsg( "Jump!\n" );
//	}
//
//	EngineDevMsg("Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false"));
//
//	return rv;
//}

void __stdcall ClientDLL::HOOKED_HudUpdate_Func(bool bActive)
{
	OnFrame();

	return ORIG_HudUpdate(bActive);
}

int __fastcall ClientDLL::HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState)
{
	int rv = ORIG_GetButtonBits(thisptr, edx, bResetState);

	if (bResetState == 1)
	{
		static bool duckPressed = false;
		
		if (duckspam)
		{
			if (duckPressed)
				duckPressed = false;
			else
			{
				duckPressed = true;
				rv |= (1 << 2); // IN_DUCK
			}
		}
		else
			duckPressed = false;
	}

	return rv;
}

void __fastcall ClientDLL::HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime)
{
	ORIG_AdjustAngles(thisptr, edx, frametime);

	double pitchSpeed = atof(_y_spt_pitchspeed.GetString()),
		yawSpeed = atof(_y_spt_yawspeed.GetString());

	if (setPitch.set || setYaw.set
		|| (pitchSpeed != 0.0)
		|| (yawSpeed != 0.0))
	{
		float va[3];
		EngineGetViewAngles(va);

		va[0] += pitchSpeed;
		va[1] += yawSpeed;

		if (setPitch.set)
		{
			setPitch.set = false;
			va[0] = setPitch.angle;
		}

		if (setYaw.set)
		{
			setYaw.set = false;
			va[1] = setYaw.angle;
		}

		EngineSetViewAngles(va);
	}
}
