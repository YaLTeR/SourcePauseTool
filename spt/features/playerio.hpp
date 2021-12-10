#pragma once

#include "..\feature.hpp"
#include "..\strafe\strafestuff.hpp"

#ifdef OE
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

typedef void(__fastcall* _CalcAbsoluteVelocity)(void* thisptr, int edx);
typedef void*(__fastcall* _GetGroundEntity)(void* thisptr, int edx);
typedef void(
    __fastcall* _CreateMove)(void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
typedef int(__fastcall* _GetButtonBits)(void* thisptr, int edx, int bResetState);
typedef void*(__cdecl* _GetLocalPlayer)();

// This feature reads player stuff from memory and writes player stuff into memory
class PlayerIOFeature : public Feature
{
private:
	void __fastcall HOOKED_CreateMove_Func(void* thisptr,
	                                       int edx,
	                                       int sequence_number,
	                                       float input_sample_frametime,
	                                       bool active);
	static void __fastcall HOOKED_CreateMove(void* thisptr,
	                                         int edx,
	                                         int sequence_number,
	                                         float input_sample_frametime,
	                                         bool active);
	int __fastcall HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState);
	static int __fastcall HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState);

public:
	void SetTASInput(float* va, const Strafe::ProcessedFrame& out);
	Strafe::MovementVars GetMovementVars();
	bool GetFlagsDucking();
	Strafe::PlayerData GetPlayerData();
	Vector GetPlayerVelocity();
	Vector GetPlayerEyePos();
	int GetPlayerFlags();
	double GetDuckJumpTime();
	bool IsGroundEntitySet();
	bool TryJump();
	bool PlayerIOAddressesFound();
	int GetPlayerPhysicsFlags() const;
	int GetPlayerMoveType() const;
	int GetPlayerMoveCollide() const;
	int GetPlayerCollisionGroup() const;
	void Set_cinput_thisptr(void* thisptr);
	void OnTick();

	bool duckspam = false;
	bool forceJump = false;
	bool forceUnduck = false;
	bool playerioAddressesWereFound = false;
	ptrdiff_t offServerAbsOrigin = 0;
	uintptr_t pCmd = 0;

	ptrdiff_t offM_vecAbsVelocity = 0;
	ptrdiff_t offM_afPhysicsFlags = 0;
	ptrdiff_t offM_moveType = 0;
	ptrdiff_t offM_moveCollide = 0;
	ptrdiff_t offM_collisionGroup = 0;
	ptrdiff_t offM_vecPunchAngle = 0;
	ptrdiff_t offM_vecPunchAngleVel = 0;
	_CreateMove ORIG_CreateMove = nullptr;

	_GetLocalPlayer ORIG_GetLocalPlayer = nullptr;
	_CalcAbsoluteVelocity ORIG_CalcAbsoluteVelocity = nullptr;
	_GetGroundEntity ORIG_GetGroundEntity = nullptr;
	_GetButtonBits ORIG_GetButtonBits = nullptr;
	uintptr_t ORIG_MiddleOfCAM_Think = 0;
	uintptr_t ORIG_PlayerRunCommand = 0;
	Vector currentVelocity;
	Vector previousVelocity;

	ptrdiff_t offM_pCommands = 0;
	ptrdiff_t offForwardmove = 0;
	ptrdiff_t offSidemove = 0;
	ptrdiff_t offMaxspeed = 0;
	ptrdiff_t offFlags = 0;
	ptrdiff_t offAbsVelocity = 0;
	ptrdiff_t offDucking = 0;
	ptrdiff_t offDuckJumpTime = 0;
	ptrdiff_t offServerSurfaceFriction = 0;
	ptrdiff_t offServerPreviouslyPredictedOrigin = 0;
	std::size_t sizeofCUserCmd = 0;

	void EnableDuckspam()
	{
		duckspam = true;
	}
	void DisableDuckspam()
	{
		duckspam = false;
	}

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	virtual void PreHook() override;
};

extern PlayerIOFeature spt_playerio;
