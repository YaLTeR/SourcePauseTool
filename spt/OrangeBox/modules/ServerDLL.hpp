#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>

#include "engine\iserverplugin.h"
#include <SDK\hl_movedata.h>

using std::uintptr_t;
using std::size_t;
using std::ptrdiff_t;

typedef bool(__fastcall *_CheckJumpButton) (void* thisptr, int edx);
typedef void(__fastcall *_FinishGravity) (void* thisptr, int edx);
typedef void(__fastcall *_PlayerRunCommand) (void* thisptr, int edx, void* ucmd, void* moveHelper);
typedef int(__fastcall *_CheckStuck) (void* thisptr, int edx);
typedef void(__fastcall *_AirAccelerate) (void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel);
typedef void(__fastcall *_ProcessMovement) (void* thisptr, int edx, void* pPlayer, void* pMove);
typedef void(__fastcall *_SnapEyeAngles) (void* thisptr, int edx, const QAngle& viewAngles);
typedef float(__fastcall *_FirePortal) (void* thisptr, int edx, bool bPortal2, Vector* pVector, bool bTest);
typedef float(__fastcall *_TraceFirePortal) (void* thisptr, int edx, bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest);
typedef void*(__fastcall *_GetActiveWeapon) (void* thisptr);

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
	static int __fastcall HOOKED_CheckStuck(void* thisptr, int edx);
	static void __fastcall HOOKED_AirAccelerate(void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel);
	static void __fastcall HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove);
	static float __fastcall HOOKED_TraceFirePortal(void* thisptr, int edx, bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest);
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __fastcall HOOKED_FinishGravity_Func(void* thisptr, int edx);
	void __fastcall HOOKED_PlayerRunCommand_Func(void* thisptr, int edx, void* ucmd, void* moveHelper);
	int __fastcall HOOKED_CheckStuck_Func(void* thisptr, int edx);
	void __fastcall HOOKED_AirAccelerate_Func(void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel);
	void __fastcall HOOKED_ProcessMovement_Func(void* thisptr, int edx, void* pPlayer, void* pMove);
	float __fastcall HOOKED_TraceFirePortal_Func(void* thisptr, int edx, bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest);

	const Vector& GetLastVelocity() const { return lastVelocity; }

	void StartTimer() { timerRunning = true; }
	void StopTimer() { timerRunning = false; }
	void ResetTimer() { ticksPassed = 0; timerRunning = false; }
	unsigned int GetTicksPassed() const { return ticksPassed; }

	_SnapEyeAngles SnapEyeAngles;
	_FirePortal FirePortal;
	double lastTraceFirePortalDistanceSq;
	Vector lastTraceFirePortalNormal;
	_GetActiveWeapon GetActiveWeapon;

protected:
	_CheckJumpButton ORIG_CheckJumpButton;
	_FinishGravity ORIG_FinishGravity;
	_PlayerRunCommand ORIG_PlayerRunCommand;
	_CheckStuck ORIG_CheckStuck;
	_AirAccelerate ORIG_AirAccelerate;
	_ProcessMovement ORIG_ProcessMovement;
	_TraceFirePortal ORIG_TraceFirePortal;

	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	bool cantJumpNextTime;
	bool insideCheckJumpButton;
	ptrdiff_t off1M_bDucked;
	ptrdiff_t off2M_bDucked;
	ptrdiff_t offM_vecAbsVelocity;

	Vector lastVelocity;

	unsigned ticksPassed;
	bool timerRunning;
};
