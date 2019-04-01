#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\..\utils\patterncontainer.hpp"


#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

using std::uintptr_t;
using std::size_t;

typedef bool(__cdecl *_SV_ActivateServer) ();
typedef void(__fastcall *_FinishRestore) (void* thisptr, int edx);
typedef void(__fastcall *_SetPaused) (void* thisptr, int edx, bool paused);
typedef void(__cdecl *__Host_RunFrame) (float time);
typedef void(__cdecl *__Host_RunFrame_Input) (float accumulated_extra_samples, int bFinalTick);
typedef void(__cdecl *__Host_RunFrame_Server) (int bFinalTick);
typedef void(__cdecl *_Cbuf_Execute) ();
typedef void(__fastcall *_VGui_Paint) (void* thisptr, int edx, int mode);
typedef int(__fastcall *DemoPlayer__Func)(void* thisptr);
typedef bool(__fastcall *_CEngineTrace__PointOutsideWorld)(void* thisptr, int edx, const Vector& pt);


class EngineDLL : public IHookableNameFilter
{
public:
	EngineDLL() : IHookableNameFilter({ L"engine.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	static bool __cdecl HOOKED_SV_ActivateServer();
	static void __fastcall HOOKED_FinishRestore(void* thisptr, int edx);
	static void __fastcall HOOKED_SetPaused(void* thisptr, int edx, bool paused);
	static void __cdecl HOOKED__Host_RunFrame(float time);
	static void __cdecl HOOKED__Host_RunFrame_Input(float accumulated_extra_samples, int bFinalTick);
	static void __cdecl HOOKED__Host_RunFrame_Server(int bFinalTick);
	static void __cdecl HOOKED_Cbuf_Execute();
	static void __fastcall HOOKED_VGui_Paint(void* thisptr, int edx, int mode);
	bool __cdecl HOOKED_SV_ActivateServer_Func();
	void __fastcall HOOKED_FinishRestore_Func(void* thisptr, int edx);
	void __fastcall HOOKED_SetPaused_Func(void* thisptr, int edx, bool paused);
	void __cdecl HOOKED__Host_RunFrame_Func(float time);
	void __cdecl HOOKED__Host_RunFrame_Input_Func(float accumulated_extra_samples, int bFinalTick);
	void __cdecl HOOKED__Host_RunFrame_Server_Func(int bFinalTick);
	void __cdecl HOOKED_Cbuf_Execute_Func();
	void __fastcall HOOKED_VGui_Paint_Func(void* thisptr, int edx, int mode);

	float GetTickrate() const;
	void SetTickrate(float value);

	int Demo_GetPlaybackTick() const;
	int Demo_GetTotalTicks() const;
	bool Demo_IsPlayingBack() const;
	bool Demo_IsPlaybackPaused() const;
	_CEngineTrace__PointOutsideWorld ORIG_CEngineTrace__PointOutsideWorld;

protected:
	PatternContainer patternContainer;
	_SV_ActivateServer ORIG_SV_ActivateServer;
	_FinishRestore ORIG_FinishRestore;
	_SetPaused ORIG_SetPaused;
	__Host_RunFrame ORIG__Host_RunFrame;
	__Host_RunFrame_Input ORIG__Host_RunFrame_Input;
	__Host_RunFrame_Server ORIG__Host_RunFrame_Server;
	_Cbuf_Execute ORIG_Cbuf_Execute;
	_VGui_Paint ORIG_VGui_Paint;

	void* pGameServer;
	bool* pM_bLoadgame;
	bool shouldPreventNextUnpause;
	float* pIntervalPerTick;
	float* pHost_Frametime;
	int* pM_State;
	int* pM_nSignonState;
	void** pDemoplayer;
	
	int GetPlaybackTick_Offset;
	int GetTotalTicks_Offset;
	int IsPlayingBack_Offset;
	int IsPlaybackPaused_Offset;
};