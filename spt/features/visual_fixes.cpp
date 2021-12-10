#include "stdafx.h"
#include "..\feature.hpp"
#include "convar.hpp"
#include "dbg.h"

typedef void(__cdecl* _DoImageSpaceMotionBlur)(void* view, int x, int y, int w, int h);
typedef void(__fastcall* _CViewEffects__Fade)(void* thisptr, int edx, void* data);
typedef void(__fastcall* _CViewEffects__Shake)(void* thisptr, int edx, void* data);
typedef void(__cdecl* _ResetToneMapping)(float value);

ConVar y_spt_motion_blur_fix("y_spt_motion_blur_fix", "0", FCVAR_ARCHIVE, "Fixes motion blur for startmovie.");
ConVar y_spt_disable_fade("y_spt_disable_fade", "0", FCVAR_ARCHIVE, "Disables all fades.");
ConVar y_spt_disable_shake("y_spt_disable_shake", "0", FCVAR_ARCHIVE, "Disables all shakes.");
ConVar y_spt_disable_tone_map_reset(
    "y_spt_disable_tone_map_reset",
    "0",
    FCVAR_ARCHIVE,
    "Prevents the tone map getting reset (during each load), useful for keeping colors the same between demos.");

// Misc visual fixes
class VisualFixes : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	uintptr_t* pgpGlobals = nullptr;
	_DoImageSpaceMotionBlur ORIG_DoImageSpaceMotionBlur = nullptr;
	_CViewEffects__Fade ORIG_CViewEffects__Fade = nullptr;
	_CViewEffects__Shake ORIG_CViewEffects__Shake = nullptr;
	_ResetToneMapping ORIG_ResetToneMapping = nullptr;

	static void __cdecl HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h);
	static void __fastcall HOOKED_CViewEffects__Fade(void* thisptr, int edx, void* data);
	static void __fastcall HOOKED_CViewEffects__Shake(void* thisptr, int edx, void* data);
	static void __cdecl HOOKED_ResetToneMapping(float value);
};

static VisualFixes spt_visual_fixes;

bool VisualFixes::ShouldLoadFeature()
{
	return true;
}

void VisualFixes::InitHooks()
{
	HOOK_FUNCTION(client, DoImageSpaceMotionBlur);
	HOOK_FUNCTION(client, CViewEffects__Fade);
	HOOK_FUNCTION(client, CViewEffects__Shake);
	HOOK_FUNCTION(client, ResetToneMapping);
}

void VisualFixes::LoadFeature()
{
	if (ORIG_DoImageSpaceMotionBlur)
	{
		int ptnNumber = GetPatternIndex((void**)&ORIG_DoImageSpaceMotionBlur);

		switch (ptnNumber)
		{
		case 0:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 132);
			break;

		case 1:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 153);
			break;

		case 2:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 129);
			break;

		case 3:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 171);
			break;

		case 4:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 177);
			break;

		case 5:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 128);
			break;
		}

		DevMsg("[client dll] pgpGlobals is %p.\n", pgpGlobals);
		InitConcommandBase(y_spt_motion_blur_fix);
	}

	if (ORIG_CViewEffects__Fade)
		InitConcommandBase(y_spt_disable_fade);

	if (ORIG_CViewEffects__Shake)
		InitConcommandBase(y_spt_disable_shake);

	if (ORIG_ResetToneMapping)
		InitConcommandBase(y_spt_disable_tone_map_reset);
}

void VisualFixes::UnloadFeature() {}

void __cdecl VisualFixes::HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h)
{
	uintptr_t origgpGlobals = NULL;

	/*
	Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
	so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
	function does not use anything apart from gpGlobals->realtime from gpGlobals,
	so we can do such a replace to make it use gpGlobals->curtime instead without
	breaking anything else.
	*/
	if (spt_visual_fixes.pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			origgpGlobals = *spt_visual_fixes.pgpGlobals;
			*spt_visual_fixes.pgpGlobals = *spt_visual_fixes.pgpGlobals + 12;
		}
	}

	spt_visual_fixes.ORIG_DoImageSpaceMotionBlur(view, x, y, w, h);

	if (spt_visual_fixes.pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			*spt_visual_fixes.pgpGlobals = origgpGlobals;
		}
	}
}

void __fastcall VisualFixes::HOOKED_CViewEffects__Fade(void* thisptr, int edx, void* data)
{
	if (!y_spt_disable_fade.GetBool())
		spt_visual_fixes.ORIG_CViewEffects__Fade(thisptr, edx, data);
}

void __fastcall VisualFixes::HOOKED_CViewEffects__Shake(void* thisptr, int edx, void* data)
{
	if (!y_spt_disable_shake.GetBool())
		spt_visual_fixes.ORIG_CViewEffects__Shake(thisptr, edx, data);
}

void __cdecl VisualFixes::HOOKED_ResetToneMapping(float value)
{
	if (!y_spt_disable_tone_map_reset.GetBool())
		spt_visual_fixes.ORIG_ResetToneMapping(value);
}
