#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>

using std::uintptr_t;
using std::size_t;

typedef bool(__cdecl *_SV_ActivateServer) ();
typedef void(__fastcall *_FinishRestore) (void* thisptr, int edx);
typedef void(__fastcall *_SetPaused) (void* thisptr, int edx, bool paused);
typedef void(__cdecl *__Host_RunFrame) (float time);
typedef void(__cdecl *__Host_RunFrame_Input) (float accumulated_extra_samples, int bFinalTick);
typedef void(__cdecl *__Host_RunFrame_Server) (int bFinalTick);
typedef void(__cdecl *_Cbuf_Execute) ();
typedef void*(__cdecl *_Cbuf_GetCommandBuffer) ();
typedef void(__cdecl *_Cbuf_AddText)(void* command_buffer, const char* text, int unknown);
typedef void(__cdecl *_SetSignonState)(void* thisptr, int edx, int state, int spawncount);

class EngineDLL : public IHookableNameFilter
{
public:
	EngineDLL() : IHookableNameFilter( { L"engine.dll" } ) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	static bool __cdecl HOOKED_SV_ActivateServer();
	static void __fastcall HOOKED_FinishRestore(void* thisptr, int edx);
	static void __fastcall HOOKED_SetPaused(void* thisptr, int edx, bool paused);
	static void __cdecl HOOKED__Host_RunFrame(float time);
	static void __cdecl HOOKED__Host_RunFrame_Input(float accumulated_extra_samples, int bFinalTick);
	static void __cdecl HOOKED__Host_RunFrame_Server(int bFinalTick);
	static void __cdecl HOOKED_Cbuf_Execute();
	bool __cdecl HOOKED_SV_ActivateServer_Func();
	void __fastcall HOOKED_FinishRestore_Func(void* thisptr, int edx);
	void __fastcall HOOKED_SetPaused_Func(void* thisptr, int edx, bool paused);
	void __cdecl HOOKED__Host_RunFrame_Func(float time);
	void __cdecl HOOKED__Host_RunFrame_Input_Func(float accumulated_extra_samples, int bFinalTick);
	void __cdecl HOOKED__Host_RunFrame_Server_Func(int bFinalTick);
	void __cdecl HOOKED_Cbuf_Execute_Func();
	void __cdecl HOOKED_SetSignonState(void* thisptr, int edx, int state, int spawncount);
	void __cdecl HOOKED_SetSignonState_Func(void* thisptr, int edx, int state, int spawncount);

	float GetTickrate() const;
	void SetTickrate(float value);

	int Demo_GetPlaybackTick() const;
	int Demo_GetTotalTicks() const;
	bool Demo_IsPlayingBack() const;
	bool Demo_IsPlaybackPaused() const;

protected:
	_SV_ActivateServer ORIG_SV_ActivateServer;
	_FinishRestore ORIG_FinishRestore;
	_SetPaused ORIG_SetPaused;
	__Host_RunFrame ORIG__Host_RunFrame;
	__Host_RunFrame_Input ORIG__Host_RunFrame_Input;
	_Cbuf_GetCommandBuffer Cbuf_GetCommandBuffer;
	_Cbuf_AddText Cbuf_AddText;
	__Host_RunFrame_Server ORIG__Host_RunFrame_Server;
	_Cbuf_Execute ORIG_Cbuf_Execute;

	void* pGameServer;
	bool* pM_bLoadgame;
	bool shouldPreventNextUnpause;
	float* pIntervalPerTick;
	float* pHost_Frametime;
	int* pM_State;
	int* pM_nSignonState;
	void** pDemoplayer;
};
