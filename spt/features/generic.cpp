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

CON_COMMAND(y_spt_build, "Returns the build number of the game")
{
	Msg("The build is: %d\n", utils::GetBuildNumber());
}

GenericFeature spt_generic;

namespace patterns
{
	PATTERNS(
	    SV_ActivateServer,
	    "5135",
	    "83 EC 08 57 8B 3D ?? ?? ?? ?? 68 ?? ?? ?? ?? FF D7 83 C4 04 E8 ?? ?? ?? ?? 8B 10",
	    "5339",
	    "55 8B EC 83 EC 0C 57 8B 3D ?? ?? ?? ?? 68 ?? ?? ?? ?? FF D7 83 C4 04 E8 ?? ?? ?? ?? 8B 10",
	    "2257546",
	    "55 8B EC 83 EC 0C 53 8B 1D ?? ?? ?? ?? 57 68 ?? ?? ?? ?? FF D3 83 C4 04 E8 ?? ?? ?? ?? 6A 0B",
	    "2707",
	    "6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04 E8 ?? ?? ?? ?? 8B 10 6A 0B 8B C8 FF 92",
	    "6879",
	    "55 8B EC 83 EC 08 56 57 8B 3D ?? ?? ?? ?? 68 ?? ?? ?? ?? FF D7 83 C4 04 E8 ?? ?? ?? ?? 8B 10 8B C8");
	PATTERNS(FinishRestore,
	         "5135",
	         "81 EC A4 06 00 00 33 C0 55 8B E9 8D 8C 24 20 01 00 00 89 84 24 08 01 00 00",
	         "5339",
	         "55 8B EC 81 EC A4 06 00 00 33 C0 53 8B D9 8D 8D 74 F9 FF FF 89 85 5C F9 FF FF",
	         "2257546",
	         "55 8B EC 81 EC A4 06 00 00 33 C0 53 8B D9 89 85 5C F9 FF FF 8D 8D 74 F9 FF FF",
	         "2707",
	         "55 8B EC 83 E4 F8 81 EC 8C 06 00 00 55 56 57 8B E9 80 BD ?? ?? ?? ?? 00 0F 84 C0 00 00 00",
	         "BMS-Retail",
	         "55 8B EC 81 EC A8 06 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 33 C0 53 8B D9 89 85 58 F9 FF FF",
	         "4044",
	         "55 8B EC 83 E4 F8 81 EC 8C 06 00 00 55 56 57 8B E9 80 BD ?? 01 00 00 00 0F 84 BE 00 00 00 33 C0",
	         "6879",
	         "55 8B EC 81 EC ?? ?? ?? ?? 53 8B D9 8D 8D ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 BB ?? ?? ?? ?? ?? 75 1B");
	PATTERNS(
	    SetPaused,
	    "5135",
	    "83 EC 14 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 06 8B 50 ?? 8B CE FF D2 84 C0 74 ?? 8A 44 24 1C 8B 16 8B 92 80 00 00 00",
	    "5339",
	    "55 8B EC 83 EC 14 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 06 8B 50 ?? 8B CE FF D2 84 C0 74 ?? 8A 45 08 8B 16 8B 92 80 00 00 00",
	    "4104",
	    "83 EC 14 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 06 8B 50 ?? 8B CE FF D2 84 C0 74 ?? 8A 44 24 1C 8B 16 8B 52 7C 33 C9 84 C0",
	    "2257546",
	    "55 8B EC 83 EC 14 56 8B F1 8B 06 8B 40 ?? FF D0 84 C0 74 ?? 8B 06 8B CE 8B 40 ?? FF D0 84 C0 74 ?? 8A 4D 08 33 C0 84 C9 88 4D FC 6A 00",
	    "2707",
	    "64 A1 00 00 00 00 6A FF 68 ?? ?? ?? ?? 50 64 89 25 00 00 00 00 83 EC 14 56 8B F1 8B 06 FF 50 ?? 84 C0 74 59 8B 16 8B CE FF 52",
	    "6879",
	    "55 8B EC 83 EC 14 56 8B F1 83 7E 04 03 74 0B 8B 06 8B 50 68 FF D2 84 C0 74 46 8B 06 8B 50 54 8B CE");
	PATTERNS(
	    HudUpdate,
	    "5135",
	    "51 A1 ?? ?? ?? ?? D9 40 10 56 83 EC 08 D9 54 24 0C DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? 8B 4C 24 0C 51 B9",
	    "5339",
	    "55 8B EC 51 A1 ?? ?? ?? ?? D9 40 10 56 83 EC 08 D9 55 FC DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? 8B 4D 08 51 B9",
	    "2257546",
	    "55 8B EC 51 A1 ?? ?? ?? ?? 56 83 EC 08 D9 40 10 D9 55 FC DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? FF 75 08 B9",
	    "bms",
	    "55 8B EC 51 A1 ?? ?? ?? ?? F3 0F 10 40 10 56 F3 0F 11 45 FC 83 EC 08 0F 5A C0 F2 0F 11 04 24 E8 ?? ?? ?? ?? 8B C8",
	    "2707",
	    "51 A1 ?? ?? ?? ?? 8B 48 10 89 4C 24 00 E8 ?? ?? ?? ?? D9 44 24 00 83 EC 08 DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8",
	    "dmomm",
	    "51 A1 ?? ?? ?? ?? 8B 48 10 89 0C 24 D9 04 24 83 EC 08 DD 1C 24 E8 ?? ?? ?? ?? 8B C8 E8",
	    "6879",
	    "55 8B EC 83 EC 24 A1 ?? ?? ?? ?? F3 0F 10 40 ?? 53 56 F3 0F 11 45 ?? 83 EC 08 0F 5A C0 F2 0F 11 04 24",
	    "missinginfo1_4_7",
	    "55 8B EC 83 EC 08 89 4D F8 A1 ?? ?? ?? ?? 8B 48 10 89 4D FC E8 ?? ?? ?? ?? D9 45 FC 83 EC 08 DD 1C 24",
	    "hl1movement",
	    "55 8B EC 51 A1 ?? ?? ?? ?? 56 83 EC 08 F3 0F 10 40 10 F3 0F 11 45 FC 0F 5A C0 F2 0F 11 04 24 E8 ?? ?? ?? ?? 8B C8");
	PATTERNS(ControllerMove,
	         "5135",
	         "56 8B F1 80 BE ?? ?? ?? ?? 00 57 8B 7C 24 10 75 ??",
	         "hls-220622",
	         "55 8B EC 56 8B F1 57 8B 7D ?? 80 BE ?? ?? ?? ?? 00",
	         "3420",
	         "81 EC 54 05 00 00 53 55 8B E9 80 7D ?? 00 BB 01 00 00 00");
	PATTERNS(
	    CHudDamageIndicator__GetDamagePosition,
	    "5135",
	    "83 EC 18 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 08 89 4C 24 0C 8B 50 04 6A 00 89 54 24 14 8B 40 08 6A 00 8D 4C 24 08 51 8D 54 24 18 52 89 44 24 24",
	    "1910503",
	    "55 8B EC 83 EC 18 56 8B F1 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 7E 00 6A 00 6A 00 8D 4D F4 66 0F D6 45 E8");
	PATTERNS(SV_Frame,
	         "5135",
	         "8B 0D ?? ?? ?? ?? 83 EC 08 85 C9 74 10 8B 44 24 0C 84 C0 74 08 8B 11 50 8B 42 78 FF D0 83 3D");

} // namespace patterns

void GenericFeature::InitHooks()
{
	HOOK_FUNCTION(client, HudUpdate);
	HOOK_FUNCTION(engine, FinishRestore);
	HOOK_FUNCTION(engine, SetPaused);
	HOOK_FUNCTION(engine, SV_ActivateServer);
	HOOK_FUNCTION(engine, SV_Frame);
	HOOK_FUNCTION(client, ControllerMove);
	FIND_PATTERN(client, CHudDamageIndicator__GetDamagePosition);
}

bool GenericFeature::ShouldLoadFeature()
{
	return true;
}

Vector GenericFeature::GetCameraOrigin()
{
	if (ORIG_MainViewOrigin)
		return ORIG_MainViewOrigin();
	else
		return Vector(0, 0, 0);
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

	if (ORIG_SV_Frame)
		SV_FrameSignal.Works = true;

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
	FinishRestoreSignal(thisptr);
}

void __fastcall GenericFeature::HOOKED_SetPaused(void* thisptr, int edx, bool paused)
{
	SetPausedSignal(thisptr, paused);

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

HOOK_CDECL(void, GenericFeature, SV_Frame, bool finalTick)
{
	SV_FrameSignal(finalTick);
	spt_generic.ORIG_SV_Frame(finalTick);
}
