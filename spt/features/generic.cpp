#include "stdafx.h"

#include "generic.hpp"
#include "aim.hpp"
#include "playerio.hpp"
#include "ent_utils.hpp"
#include "game_detection.hpp"
#include "tas.hpp"
#include "signals.hpp"
#include "..\cvars.hpp"
#include "..\sptlib-wrapper.hpp"

#ifdef OE
ConVar y_spt_gamedir(
    "y_spt_gamedir",
    "",
    0,
    "Sets the game directory, that is used for loading tas scripts and tests. Use the full path for the folder e.g. C:\\Steam\\steamapps\\sourcemods\\hl2oe\\\n");

const char* GetGameDirectoryOE()
{
	return y_spt_gamedir.GetString();
}
#endif

GenericFeature spt_generic;

Vector GenericFeature::GetCameraOrigin()
{
	if (ORIG_MainViewOrigin)
		return ORIG_MainViewOrigin();
	else
		return Vector(0, 0, 0);
}

bool GenericFeature::ShouldLoadFeature()
{
	return true;
}

void GenericFeature::InitHooks()
{
	HOOK_FUNCTION(client, HudUpdate);
	HOOK_FUNCTION(engine, FinishRestore);
	HOOK_FUNCTION(engine, SetPaused);
	HOOK_FUNCTION(engine, SV_ActivateServer);
	HOOK_FUNCTION(client, ControllerMove);
	FIND_PATTERN(client, CHudDamageIndicator__GetDamagePosition);
}

CON_COMMAND(y_spt_build, "Returns the build number of the game")
{
	Msg("The build is: %d\n", utils::GetBuildNumber());
}

void GenericFeature::LoadFeature()
{
#ifdef OE
	InitConcommandBase(y_spt_gamedir);
#endif
	InitCommand(y_spt_build);

	if (ORIG_CHudDamageIndicator__GetDamagePosition)
	{
		int ptnNumber = GetPatternIndex((void**)&ORIG_CHudDamageIndicator__GetDamagePosition);
		switch (ptnNumber)
		{
		case 0:
			ORIG_MainViewOrigin =
			    (_MainViewOrigin)(*reinterpret_cast<int*>(ORIG_CHudDamageIndicator__GetDamagePosition + 4)
			                      + ORIG_CHudDamageIndicator__GetDamagePosition + 8);
			break;
		case 1:
			ORIG_MainViewOrigin =
			    (_MainViewOrigin)(*reinterpret_cast<int*>(ORIG_CHudDamageIndicator__GetDamagePosition + 10)
			                      + ORIG_CHudDamageIndicator__GetDamagePosition + 14);
			break;
		}
		DevMsg("[client.dll] Found MainViewOrigin at %p\n", ORIG_MainViewOrigin);
	}

	if (ORIG_CHLClient__CanRecordDemo)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHLClient__CanRecordDemo + 1);
		ORIG_GetClientModeNormal = (_GetClientModeNormal)(offset + ORIG_CHLClient__CanRecordDemo + 5);
		DevMsg("[client.dll] Found GetClientModeNormal at %p\n", ORIG_GetClientModeNormal);
	}
}

void GenericFeature::UnloadFeature() {}

void GenericFeature::PreHook()
{
	if (ORIG_HudUpdate)
		FrameSignal.Works = true;

	if (ORIG_SV_ActivateServer)
		SV_ActivateServerSignal.Works = true;

	if (ORIG_SetPaused)
		SetPausedSignal.Works = true;

	if (ORIG_FinishRestore)
		FinishRestoreSignal.Works = true;

	if (ORIG_ControllerMove)
	{
		AdjustAngles.Works = true;
		OngroundSignal.Works = true;
	}
}

void __stdcall GenericFeature::HOOKED_HudUpdate(bool bActive)
{
	FrameSignal();
	spt_generic.ORIG_HudUpdate(bActive);
}

bool __cdecl GenericFeature::HOOKED_SV_ActivateServer()
{
	bool result = spt_generic.ORIG_SV_ActivateServer();
	SV_ActivateServerSignal(result);

	return result;
}

void __fastcall GenericFeature::HOOKED_FinishRestore(void* thisptr, int edx)
{
	spt_generic.ORIG_FinishRestore(thisptr, edx);
	FinishRestoreSignal(thisptr, edx);
}

void __fastcall GenericFeature::HOOKED_SetPaused(void* thisptr, int edx, bool paused)
{
	SetPausedSignal(thisptr, edx, paused);

	if (paused == false)
	{
		if (spt_generic.shouldPreventNextUnpause)
		{
			DevMsg("Unpause prevented.\n");
			spt_generic.shouldPreventNextUnpause = false;
			return;
		}
	}

	return spt_generic.ORIG_SetPaused(thisptr, edx, paused);
}

void __fastcall GenericFeature::HOOKED_ControllerMove(void* thisptr, int edx, float frametime, void* cmd)
{
	spt_playerio.Set_cinput_thisptr(thisptr);
	spt_generic.ORIG_ControllerMove(thisptr, edx, frametime, cmd);

	if (!spt_playerio.pCmd)
	{
		return;
	}

	spt_tas.Strafe();

	if (utils::playerEntityAvailable())
	{
		OngroundSignal(spt_playerio.IsGroundEntitySet());
		AdjustAngles();
	}
}
