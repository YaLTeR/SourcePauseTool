#pragma once

#include "..\feature.hpp"
#include "..\strafe\strafestuff.hpp"
#include "ent_props.hpp"

#ifdef OE
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

#include <string>
#include <map>

typedef void*(__fastcall* _GetGroundEntity)(void* thisptr, int edx);
typedef void(
    __fastcall* _CreateMove)(void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
typedef int(__fastcall* _GetButtonBits)(void* thisptr, int edx, int bResetState);
typedef void*(__cdecl* _GetLocalPlayer)();

// This feature reads player stuff from memory and writes player stuff into memory
class PlayerIOFeature : public FeatureWrapper<PlayerIOFeature>
{
private:
	DECL_HOOK_THISCALL(void, CreateMove, void*, int sequence_number, float input_sample_frametime, bool active);
	DECL_HOOK_THISCALL(void, DecodeUserCmdFromBuffer, void*, bf_read& buf, int sequence_number);
	DECL_HOOK_THISCALL(int, GetButtonBits, void*, int bResetState);

public:
	virtual bool ShouldLoadFeature() override;
	void GetMoveInput(float& forwardmove, float& sidemove);
	void SetTASInput(float* va, const Strafe::ProcessedFrame& out);
	Strafe::MovementVars GetMovementVars();
	bool GetFlagsDucking();
	Strafe::PlayerData GetPlayerData();
	Vector GetPlayerVelocity();
	Vector GetPlayerEyePos(bool newVersion);
	double GetDuckJumpTime();
	bool IsGroundEntitySet();
	bool TryJump();
	bool PlayerIOAddressesFound();
	void Set_cinput_thisptr(void* thisptr);
	void GetPlayerFields();
	void OnTick(bool simulating);

	bool fetchedPlayerFields = false;
	bool forceJump = false;
	bool forceUnduck = false;
	bool playerioAddressesWereFound = false;
	int spamButtons = 0;
	ptrdiff_t offServerAbsOrigin = 0;
	uintptr_t pCmd = 0;

	_GetGroundEntity ORIG_GetGroundEntity = nullptr;
	Vector currentVelocity;
	Vector previousVelocity;

	ptrdiff_t offM_pCommands = 0;
	ptrdiff_t offForwardmove = 0;
	ptrdiff_t offSidemove = 0;
	std::size_t sizeofCUserCmd = 0;

	PlayerField<bool> m_bDucking;
	PlayerField<float> m_flDuckJumpTime;
	PlayerField<float> m_flMaxspeed;
	PlayerField<float> m_surfaceFriction;
	PlayerField<int> m_afPhysicsFlags;
	PlayerField<int> m_CollisionGroup;
	PlayerField<int> m_fFlags;
	PlayerField<int> m_hGroundEntity;
	PlayerField<int> m_MoveCollide;
	PlayerField<int> m_MoveType;
	PlayerField<QAngle> m_vecPunchAngle;
	PlayerField<QAngle> m_vecPunchAngleVel;
	PlayerField<Vector> m_vecAbsOrigin;
	PlayerField<Vector> m_vecAbsVelocity;
	PlayerField<Vector> m_vecPreviouslyPredictedOrigin;
	PlayerField<Vector> m_vecViewOffset;

	void EnableSpam(int flags)
	{
		spamButtons |= flags;
	}
	void DisableSpam(int flags)
	{
		spamButtons &= ~flags;
	}

protected:
	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	virtual void PreHook() override;
};

extern PlayerIOFeature spt_playerio;
