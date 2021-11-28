#include "stdafx.h"

#include "generic.hpp"
#include "playerio.hpp"
#include "ent_utils.hpp"
#include "game_detection.hpp"
#include "signals.hpp"

GenericFeature spt_generic;

void GenericFeature::AdjustAngles_hook()
{
	if (utils::playerEntityAvailable())
	{
		OngroundSignal(spt_playerio.IsGroundEntitySet());
		AdjustAngles();
	}
}

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
	FIND_PATTERN(client, CHudDamageIndicator__GetDamagePosition);
}

void GenericFeature::LoadFeature()
{
	if (ORIG_CHudDamageIndicator__GetDamagePosition)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHudDamageIndicator__GetDamagePosition + 4);
		ORIG_MainViewOrigin = (_MainViewOrigin)(offset + ORIG_CHudDamageIndicator__GetDamagePosition + 8);
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
