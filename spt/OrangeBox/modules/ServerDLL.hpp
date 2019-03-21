#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>

#include "engine\iserverplugin.h"
#include <SDK\hl_movedata.h>
#include "..\..\utils\patterncontainer.hpp"

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
typedef void(__fastcall *_SlidingAndOtherStuff) (void* thisptr, int edx, void* a, void* b);
typedef CBaseEntity*(__cdecl *_CreateEntityByName) (const char* name, int forceEdictIndex);
typedef int(__fastcall *_CRestore__ReadAll) (void* thisptr, int edx, void* pLeafObject, datamap_t* pLeafMap);
typedef int(__fastcall *_CRestore__DoReadAll) (void* thisptr, int edx, void* pLeafObject, datamap_t* pLeafMap, datamap_t* pCurMap);
typedef int(__cdecl *_DispatchSpawn) (void* pEntity);
typedef string_t(__cdecl * _AllocPooledString) (const char *pszValue, int trash);

class ServerDLL : public IHookableNameFilter
{
public:
	ServerDLL() : IHookableNameFilter({ L"server.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static void __fastcall HOOKED_FinishGravity(void* thisptr, int edx);
	static void __fastcall HOOKED_PlayerRunCommand(void* thisptr, int edx, void* ucmd, void* moveHelper);
	static int __fastcall HOOKED_CheckStuck(void* thisptr, int edx);
	static void __fastcall HOOKED_AirAccelerate(void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel);
	static void __fastcall HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove);
	static float __fastcall HOOKED_TraceFirePortal(void* thisptr, int edx, bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest);
	static void __fastcall HOOKED_SlidingAndOtherStuff(void* thisptr, int edx, void* a, void* b);
	static int __fastcall HOOKED_CRestore__ReadAll(void* thisptr, int edx, void* pLeafObject, datamap_t* pLeafMap);
	static int __fastcall HOOKED_CRestore__DoReadAll(void* thisptr, int edx, void* pLeafObject, datamap_t* pLeafMap, datamap_t* pCurMap);
	static int __cdecl HOOKED_DispatchSpawn(void* pEntity);
	static void HOOKED_MiddleOfSlidingFunction();
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __fastcall HOOKED_FinishGravity_Func(void* thisptr, int edx);
	void __fastcall HOOKED_PlayerRunCommand_Func(void* thisptr, int edx, void* ucmd, void* moveHelper);
	int __fastcall HOOKED_CheckStuck_Func(void* thisptr, int edx);
	void __fastcall HOOKED_AirAccelerate_Func(void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel);
	void __fastcall HOOKED_ProcessMovement_Func(void* thisptr, int edx, void* pPlayer, void* pMove);
	float __fastcall HOOKED_TraceFirePortal_Func(void* thisptr, int edx, bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest);
	void __fastcall HOOKED_SlidingAndOtherStuff_Func(void* thisptr, int edx, void* a, void* b);
	void HOOKED_MiddleOfSlidingFunction_Func();

	const Vector& GetLastVelocity() const { return lastVelocity; }

	void StartTimer() { timerRunning = true; }
	void StopTimer() { timerRunning = false; }
	void ResetTimer() { ticksPassed = 0; timerRunning = false; }
	unsigned int GetTicksPassed() const { return ticksPassed; }
	int GetPlayerPhysicsFlags() const;
	int GetPlayerMoveType() const;
	int GetPlayerMoveCollide() const;
	int GetPlayerCollisionGroup() const;
	int GetEnviromentPortalHandle() const;

	_SnapEyeAngles SnapEyeAngles;
	_FirePortal FirePortal;
	double lastTraceFirePortalDistanceSq;
	Vector lastTraceFirePortalNormal;
	_GetActiveWeapon GetActiveWeapon;
	_CreateEntityByName ORIG_CreateEntityByName;
	int* m_hPortalEnvironmentOffsetPtr;
	_CRestore__ReadAll ORIG_CRestore__ReadAll;
	_CRestore__DoReadAll ORIG_CRestore__DoReadAll;
	_DispatchSpawn ORIG_DispatchSpawn;
	_AllocPooledString ORIG_AllocPooledString;
	Gallant::Signal0<void> JumpSignal;

protected:
	PatternContainer patternContainer;
	_CheckJumpButton ORIG_CheckJumpButton;
	_FinishGravity ORIG_FinishGravity;
	_PlayerRunCommand ORIG_PlayerRunCommand;
	_CheckStuck ORIG_CheckStuck;
	_AirAccelerate ORIG_AirAccelerate;
	_ProcessMovement ORIG_ProcessMovement;
	_TraceFirePortal ORIG_TraceFirePortal;
	_SlidingAndOtherStuff ORIG_SlidingAndOtherStuff;
	void* ORIG_MiddleOfSlidingFunction;

	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	bool cantJumpNextTime;
	bool insideCheckJumpButton;
	ptrdiff_t off1M_bDucked;
	ptrdiff_t off2M_bDucked;
	ptrdiff_t offM_vecAbsVelocity;
	ptrdiff_t offM_afPhysicsFlags;
	ptrdiff_t offM_moveType;
	ptrdiff_t offM_moveCollide;
	ptrdiff_t offM_collisionGroup;

	Vector lastVelocity;

	unsigned ticksPassed;
	bool timerRunning;

	bool sliding;
	bool wasSliding;
};
