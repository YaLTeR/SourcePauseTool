#pragma once

#include "..\feature.hpp"
#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

#include "SDK\orangebox\public\vphysics_interface.h"
#include "spt\utils\ivp_maths.hpp"

typedef int(__thiscall* _GetShadowPosition)(void* thisptr, Vector* worldPosition, QAngle* angles);

typedef void(__thiscall* _beam_object_to_new_position)(void* thisptr,
                                                       const IvpQuat* quat,
                                                       const IvpPoint* pos,
                                                       bool optimizeForRepeatedCalls);

// This feature allows access to the Havok hitbox location (aka physics shadow (aka vphysics bbox))
class ShadowPosition : public FeatureWrapper<ShadowPosition>
{
protected:
	static int __fastcall HOOKED_GetShadowPosition(void* thisptr, int _, Vector* worldPosition, QAngle* angles);
	_GetShadowPosition ORIG_GetShadowPosition = nullptr;

	_beam_object_to_new_position ORIG_beam_object_to_new_position = nullptr;

	Vector PlayerHavokPos;
	QAngle PlayerHavokAngles;

	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override {}

public:
	/*
	* There's something fucky going on. Setting one angle and then getting it again doesn't
	* always result in the same value. It mostly works if you only care about one axis
	* (although ang.x by itself seems to be limited to +-90 or something), but if you try
	* mixing angles you'll probably get weird results. This could very well be a problem
	* with my conversion from qangles to quats, but since I only care about the shadow roll
	* (and I doubt these functions will get more use any time soon) I'm just not gonna
	* worry about it - uncrafted.
	*/

	void GetPlayerHavokPos(Vector* worldPosition, QAngle* angles);
	void SetPlayerHavokPos(const Vector& worldPosition, const QAngle& angles);

	/*
	* Returns the player's IPhysicsPlayerController. NOTE: because of multiple inheritence
	* this does NOT point to a CPlayerController. If you wanted it do, you have to subtract
	* 4 bytes from this pointer.
	*/
	IPhysicsPlayerController* GetPlayerController();
};

extern ShadowPosition spt_player_shadow;
