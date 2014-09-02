#include "..\..\stdafx.hpp"
#pragma once

#include <vector>

#include "..\..\..\SPTLib\IHookableNameFilter.hpp"

using std::uintptr_t;
using std::size_t;

typedef void(__cdecl *_DoImageSpaceMotionBlur) (void* view, int x, int y, int w, int h);
//typedef bool(__fastcall *_CheckJumpButton) (void* thisptr, int edx);
typedef void(__stdcall *_HudUpdate) (bool bActive);
typedef int(__fastcall *_GetButtonBits) (void* thisptr, int edx, int bResetState);
typedef void(__fastcall *_AdjustAngles) (void* thisptr, int edx, float frametime);

typedef struct
{
	long long int framesLeft;
	std::string command;
} afterframes_entry_t;

typedef struct 
{
	float angle;
	bool set;
} angset_command_t;

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
	static int __fastcall HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState);
	static void __fastcall HOOKED_AdjustAngles(void* thisptr, int edx, float frametime);
	void __cdecl HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h);
	//bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __stdcall HOOKED_HudUpdate_Func(bool bActive);
	int __fastcall HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState);
	void __fastcall HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime);

	void AddIntoAfterframesQueue(const afterframes_entry_t& entry);
	void ResetAfterframesQueue();

	void EnableDuckspam()  { duckspam = true; }
	void DisableDuckspam() { duckspam = false; }

	void SetPitch(float pitch) { setPitch.angle = pitch; setPitch.set = true; }
	void SetYaw(float yaw)     { setYaw.angle   = yaw;   setYaw.set   = true; }

protected:
	_DoImageSpaceMotionBlur ORIG_DoImageSpaceMorionBlur;
	//_CheckJumpButton ORIG_CheckJumpButton;
	_HudUpdate ORIG_HudUpdate;
	_GetButtonBits ORIG_GetButtonBits;
	_AdjustAngles ORIG_AdjustAngles;

	uintptr_t* pgpGlobals;

	std::vector<afterframes_entry_t> afterframesQueue;
	bool duckspam;
	angset_command_t setPitch, setYaw;

	void OnFrame();
};
