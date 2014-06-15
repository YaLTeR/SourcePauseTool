#ifndef __CLIENTDLL_H__
#define __CLIENTDLL_H__

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

typedef void(__cdecl *_DoImageSpaceMotionBlur) (void* view, int x, int y, int w, int h);
typedef bool(__fastcall *_CheckJumpButton) (void* thisptr, int edx);

class ClientDLL : public IHookableNameFilter
{
public:
	ClientDLL() : IHookableNameFilter({ L"client.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	static void __cdecl HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h);
	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	void __cdecl HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h);
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);

protected:
	_DoImageSpaceMotionBlur ORIG_DoImageSpaceMorionBlur;
	_CheckJumpButton ORIG_CheckJumpButton;

	uintptr_t* pgpGlobals;
	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	bool cantJumpNextTime;
};

#endif // __CLIENTDLL_H__