#include "stdafx.h"
#pragma once

#include "..\..\IHookableNameFilter.hpp"

#include "engine\iserverplugin.h"
#include "..\..\..\SDK\hl_movedata.h"

using std::uintptr_t;
using std::size_t;
using std::ptrdiff_t;

typedef bool(__fastcall *_CheckJumpButton) (void* thisptr, int edx);
typedef void(__fastcall *_FinishGravity) (void* thisptr, int edx);
typedef void(__fastcall *_PlayerRunCommand) (void* thisptr, int edx, void* ucmd, void* moveHelper);

class ServerDLL : public IHookableNameFilter
{
public:
	ServerDLL() : IHookableNameFilter({ L"server.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static void __fastcall HOOKED_FinishGravity(void* thisptr, int edx);
	static void __fastcall HOOKED_PlayerRunCommand(void* thisptr, int edx, void* ucmd, void* moveHelper);
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __fastcall HOOKED_FinishGravity_Func(void* thisptr, int edx);
	void __fastcall HOOKED_PlayerRunCommand_Func(void* thisptr, int edx, void* ucmd, void* moveHelper);

	const Vector& GetLastVelocity() const { return lastVelocity; }

protected:
	_CheckJumpButton ORIG_CheckJumpButton;
	_FinishGravity ORIG_FinishGravity;
	_PlayerRunCommand ORIG_PlayerRunCommand;

	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	bool cantJumpNextTime;
	bool insideCheckJumpButton;
	ptrdiff_t off1M_bDucked;
	ptrdiff_t off2M_bDucked;
	ptrdiff_t offM_vecAbsVelocity;

	Vector lastVelocity;
};
