#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>

using std::uintptr_t;
using std::size_t;

typedef bool(__cdecl *_SV_ActivateServer) ();
typedef void(__fastcall *_FinishRestore) (void* thisptr, int edx);
typedef void(__fastcall *_SetPaused) (void* thisptr, int edx, bool paused);

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
	bool __cdecl HOOKED_SV_ActivateServer_Func();
	void __fastcall HOOKED_FinishRestore_Func(void* thisptr, int edx);
	void __fastcall HOOKED_SetPaused_Func(void* thisptr, int edx, bool paused);

protected:
	_SV_ActivateServer ORIG_SV_ActivateServer;
	_FinishRestore ORIG_FinishRestore;
	_SetPaused ORIG_SetPaused;

	void* pGameServer;
	bool* pM_bLoadgame;
	bool shouldPreventNextUnpause;
};
