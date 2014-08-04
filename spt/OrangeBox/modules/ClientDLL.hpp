#ifndef __CLIENTDLL_H__
#define __CLIENTDLL_H__

#ifdef _WIN32
#pragma once
#endif // _WIN32

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "..\..\IHookableNameFilter.hpp"

using std::uintptr_t;
using std::size_t;

typedef void(__cdecl *_DoImageSpaceMotionBlur) (void* view, int x, int y, int w, int h);
//typedef bool(__fastcall *_CheckJumpButton) (void* thisptr, int edx);
typedef void(__stdcall *_HudUpdate) (bool bActive);
typedef void(__stdcall *_CreateMove) (int sequence_number, float input_sample_frametime, bool active);

typedef struct
{
	long long int framesLeft;
	std::string command;
} afterframes_entry_t;

class ClientDLL : public IHookableNameFilter
{
public:
	ClientDLL() : IHookableNameFilter({ L"client.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	static void __cdecl HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h);
	//static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static void __stdcall HOOKED_HudUpdate(bool bActive);
	static void __stdcall HOOKED_CreateMove(int sequence_number, float input_sample_frametime, bool active);
	void __cdecl HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h);
	//bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __stdcall HOOKED_HudUpdate_Func(bool bActive);
	void __stdcall HOOKED_CreateMove_Func(int sequence_number, float input_sample_frametime, bool active);

	void AddIntoAfterframesQueue(const afterframes_entry_t& entry);
	void ResetAfterframesQueue();

protected:
	_DoImageSpaceMotionBlur ORIG_DoImageSpaceMorionBlur;
	//_CheckJumpButton ORIG_CheckJumpButton;
	_HudUpdate ORIG_HudUpdate;
	_CreateMove ORIG_CreateMove;

	uintptr_t* pgpGlobals;
	//ptrdiff_t off1M_nOldButtons;
	//ptrdiff_t off2M_nOldButtons;
	//bool cantJumpNextTime;

	uintptr_t clientDll;
	size_t viCreateMove;

	std::vector<afterframes_entry_t> afterframesQueue;

	void OnFrame();
};

#endif // __CLIENTDLL_H__