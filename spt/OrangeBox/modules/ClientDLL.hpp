#include "..\..\stdafx.hpp"
#pragma once

#include <vector>

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\spt-serverplugin.hpp"
#include "..\..\..\SDK\igamemovement.h"
#include "..\..\strafestuff.hpp"
#include "..\..\utils\patterncontainer.hpp"
#include "..\public\cdll_int.h"
#include "thirdparty\Signal.h"
#include "cmodel.h"

using std::size_t;
using std::uintptr_t;

typedef void(__cdecl* _DoImageSpaceMotionBlur)(void* view, int x, int y, int w, int h);
typedef bool(__fastcall* _CheckJumpButton)(void* thisptr, int edx);
typedef void(__stdcall* _HudUpdate)(bool bActive);
typedef int(__fastcall* _GetButtonBits)(void* thisptr, int edx, int bResetState);
typedef void(__fastcall* _AdjustAngles)(void* thisptr, int edx, float frametime);
typedef void(
    __fastcall* _CreateMove)(void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
typedef void(__fastcall* _CViewRender__OnRenderStart)(void* thisptr, int edx);
typedef void*(__cdecl* _GetLocalPlayer)();
typedef void*(__fastcall* _GetGroundEntity)(void* thisptr, int edx);
typedef void(__fastcall* _CalcAbsoluteVelocity)(void* thisptr, int edx);
typedef void(
    __fastcall* _CViewRender__RenderView)(void* thisptr, int edx, void* cameraView, int nClearFlags, int whatToDraw);
typedef void(__fastcall* _CViewRender__Render)(void* thisptr, int edx, void* rect);
typedef void*(__cdecl* _GetClientModeNormal)();
typedef void(__cdecl* _UTIL_TraceLine)(const Vector& vecAbsStart,
                                       const Vector& vecAbsEnd,
                                       unsigned int mask,
                                       ITraceFilter* pFilter,
                                       trace_t* ptr);
typedef void(__fastcall* _UTIL_TracePlayerBBox)(void* thisptr,
                                                int edx,
                                                const Vector& vecAbsStart,
                                                const Vector& vecAbsEnd,
                                                unsigned int mask,
                                                int collisionGroup,
                                                trace_t& ptr);
typedef void(__cdecl* _UTIL_TraceRay)(const Ray_t& ray,
                                      unsigned int mask,
                                      const IHandleEntity* ignore,
                                      int collisionGroup,
                                      trace_t* ptr);
typedef bool(__fastcall* _CGameMovement__CanUnDuckJump)(void* thisptr, int edx, trace_t& ptr);
typedef void(__fastcall* _CViewEffects__Fade)(void* thisptr, int edx, void* data);
typedef void(__fastcall* _CViewEffects__Shake)(void* thisptr, int edx, void* data);
typedef const Vector&(__cdecl* _MainViewOrigin)();

struct afterframes_entry_t
{
	afterframes_entry_t(long long int framesLeft, std::string command)
	    : framesLeft(framesLeft), command(std::move(command))
	{
	}
	afterframes_entry_t() {}
	long long int framesLeft;
	std::string command;
};

typedef struct
{
	float angle;
	bool set;
} angset_command_t;

class ClientDLL : public IHookableNameFilter
{
public:
	ClientDLL() : IHookableNameFilter({L"client.dll"}){};
	virtual void Hook(const std::wstring& moduleName,
	                  void* moduleHandle,
	                  void* moduleBase,
	                  size_t moduleLength,
	                  bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	static void __cdecl HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h);
	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static void __stdcall HOOKED_HudUpdate(bool bActive);
	static int __fastcall HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState);
	static void __fastcall HOOKED_AdjustAngles(void* thisptr, int edx, float frametime);
	static void __fastcall HOOKED_CreateMove(void* thisptr,
	                                         int edx,
	                                         int sequence_number,
	                                         float input_sample_frametime,
	                                         bool active);
	static void __fastcall HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx);
	static void __fastcall HOOKED_CViewRender__RenderView(void* thisptr,
	                                                      int edx,
	                                                      void* cameraView,
	                                                      int nClearFlags,
	                                                      int whatToDraw);
	static void __fastcall HOOKED_CViewRender__Render(void* thisptr, int edx, void* rect);
	static void __fastcall HOOKED_CViewEffects__Fade(void* thisptr, int edx, void* data);
	static void __fastcall HOOKED_CViewEffects__Shake(void* thisptr, int edx, void* data);
	void __cdecl HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h);
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __stdcall HOOKED_HudUpdate_Func(bool bActive);
	int __fastcall HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState);
	void __fastcall HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime);
	void __fastcall HOOKED_CreateMove_Func(void* thisptr,
	                                       int edx,
	                                       int sequence_number,
	                                       float input_sample_frametime,
	                                       bool active);
	void __fastcall HOOKED_CViewRender__OnRenderStart_Func(void* thisptr, int edx);
	void __fastcall HOOKED_CViewRender__RenderView_Func(void* thisptr,
	                                                    int edx,
	                                                    void* cameraView,
	                                                    int nClearFlags,
	                                                    int whatToDraw);
	void __fastcall HOOKED_CViewRender__Render_Func(void* thisptr, int edx, void* rect);
	static void HOOKED_HDTF_MiddleOfViewBobFuncStart();
	static void HOOKED_HDTF_MiddleOfViewRollFunc();

	void DelayAfterframesQueue(int delay);
	void AddIntoAfterframesQueue(const afterframes_entry_t& entry);
	void ResetAfterframesQueue();

	void PauseAfterframesQueue()
	{
		afterframesPaused = true;
	}
	void ResumeAfterframesQueue()
	{
		afterframesPaused = false;
	}

	void EnableDuckspam()
	{
		duckspam = true;
	}
	void DisableDuckspam()
	{
		duckspam = false;
	}

	void SetPitch(float pitch)
	{
		setPitch.angle = pitch;
		setPitch.set = true;
	}
	void SetYaw(float yaw)
	{
		setYaw.angle = yaw;
		setYaw.set = true;
	}
	void ResetPitchYawCommands()
	{
		setYaw.set = false;
		setPitch.set = false;
	}
	Strafe::MovementVars GetMovementVars();
	Strafe::PlayerData GetPlayerData();
	Vector GetPlayerVelocity();
	Vector GetPlayerEyePos();
	Vector GetCameraOrigin();
	int GetPlayerFlags();
	bool GetFlagsDucking();
	double GetDuckJumpTime();
	bool CanUnDuckJump(trace_t& ptr);

	Gallant::Signal0<void> FrameSignal;
	Gallant::Signal0<void> AfterFramesSignal;
	Gallant::Signal0<void> TickSignal;
	Gallant::Signal1<bool> OngroundSignal;
	bool renderingOverlay;
	void* screenRect;
	void* cinput_thisptr;
	_GetClientModeNormal ORIG_GetClientModeNormal;
	_UTIL_TraceRay ORIG_UTIL_TraceRay;
	bool IsGroundEntitySet();

protected:
	_DoImageSpaceMotionBlur ORIG_DoImageSpaceMotionBlur;
	_CheckJumpButton ORIG_CheckJumpButton;
	_HudUpdate ORIG_HudUpdate;
	_GetButtonBits ORIG_GetButtonBits;
	_AdjustAngles ORIG_AdjustAngles;
	_CreateMove ORIG_CreateMove;
	_CViewRender__OnRenderStart ORIG_CViewRender__OnRenderStart;
	_GetLocalPlayer ORIG_GetLocalPlayer;
	_GetGroundEntity ORIG_GetGroundEntity;
	_CalcAbsoluteVelocity ORIG_CalcAbsoluteVelocity;
	_CViewRender__RenderView ORIG_CViewRender__RenderView;
	_CViewRender__Render ORIG_CViewRender__Render;
	_CGameMovement__CanUnDuckJump ORIG_CGameMovement__CanUnDuckJump;
	_CViewEffects__Fade ORIG_CViewEffects__Fade;
	_CViewEffects__Shake ORIG_CViewEffects__Shake;
	_MainViewOrigin ORIG_MainViewOrigin;

	void* ORIG_HDTF_MiddleOfViewBobFuncStart;
	uintptr_t ORIG_HDTF_MiddleOfViewBobFuncEnd;
	void* ORIG_HDTF_MiddleOfViewRollFunc;
	uintptr_t ORIG_HDTF_MiddleOfViewRollFunc_JumpTo;

	uintptr_t* pgpGlobals;
	ptrdiff_t offM_pCommands;
	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	ptrdiff_t offForwardmove;
	ptrdiff_t offSidemove;
	ptrdiff_t offMaxspeed;
	ptrdiff_t offFlags;
	ptrdiff_t offAbsVelocity;
	ptrdiff_t offDucking;
	ptrdiff_t offDuckJumpTime;
	ptrdiff_t offServerSurfaceFriction;
	ptrdiff_t offServerPreviouslyPredictedOrigin;
	std::size_t sizeofCUserCmd;

public:
	ptrdiff_t offServerAbsOrigin;

protected:
	uintptr_t pCmd;

	bool tasAddressesWereFound;

	std::vector<afterframes_entry_t> afterframesQueue;
	bool afterframesPaused = false;

	bool duckspam;
	angset_command_t setPitch, setYaw;
	bool forceJump;
	bool forceUnduck;
	bool cantJumpNextTime;

	void OnFrame();

	int afterframesDelay;
	PatternContainer patternContainer;
};
