#pragma once
#include "..\feature.hpp"
#include "thirdparty\Signal.h"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

typedef void(__stdcall* _HudUpdate)(bool bActive);
typedef bool(__cdecl* _SV_ActivateServer)();
typedef void(__fastcall* _FinishRestore)(void* thisptr, int edx);
typedef void(__fastcall* _SetPaused)(void* thisptr, int edx, bool paused);
typedef void(__fastcall* _CViewRender__OnRenderStart)(void* thisptr, int edx);
typedef const Vector&(__cdecl* _MainViewOrigin)();
typedef void*(__cdecl* _GetClientModeNormal)();
typedef void(__fastcall* _ControllerMove)(void* thisptr, int edx, float frametime, void* cmd);

// For hooks used by many features
class GenericFeature : public FeatureWrapper<GenericFeature>
{
public:
	_HudUpdate ORIG_HudUpdate = nullptr;
	_SetPaused ORIG_SetPaused = nullptr;
	_SV_ActivateServer ORIG_SV_ActivateServer = nullptr;
	_FinishRestore ORIG_FinishRestore = nullptr;
	_MainViewOrigin ORIG_MainViewOrigin = nullptr;
	_GetClientModeNormal ORIG_GetClientModeNormal = nullptr;
	_ControllerMove ORIG_ControllerMove = nullptr;

	bool shouldPreventNextUnpause = false;
	int signOnState;

	Vector GetCameraOrigin();
	virtual bool ShouldLoadFeature() override;

protected:
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;
	virtual void PreHook() override;

private:
	std::vector<patterns::MatchedPattern> MATCHES_Engine__SignOnState;
	uintptr_t ORIG_CHudDamageIndicator__GetDamagePosition = 0;

	static void __stdcall HOOKED_HudUpdate(bool bActive);
	static bool __cdecl HOOKED_SV_ActivateServer();
	static void __fastcall HOOKED_FinishRestore(void* thisptr, int edx);
	static void __fastcall HOOKED_SetPaused(void* thisptr, int edx, bool paused);
	static void __fastcall HOOKED_ControllerMove(void* thisptr, int edx, float frametime, void* cmd);
	DECL_HOOK_CDECL(void, SV_Frame, bool finalTick);
	DECL_HOOK_THISCALL(void, ProcessMovement, void*, void* pPlayer, void* pMove);
	DECL_HOOK_THISCALL(void, SetSignonState, void*, int state);
};

extern GenericFeature spt_generic;
