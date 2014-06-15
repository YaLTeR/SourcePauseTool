#ifndef __SERVERDLL_H__
#define __SERVERDLL_H__

#ifdef _WIN32
#pragma once
#endif // _WIN32

#include <cstddef>
#include <cstdint>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "..\..\IHookableNameFilter.hpp"

using std::uintptr_t;
using std::size_t;
using std::ptrdiff_t;

typedef bool(__fastcall *_CheckJumpButton) (void* thisptr, int edx);

class ServerDLL : public IHookableNameFilter
{
public:
	ServerDLL() : IHookableNameFilter({ L"server.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);

protected:
	_CheckJumpButton ORIG_CheckJumpButton;

	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	bool cantJumpNextTime;
};

#endif // __SERVERDLL_H__