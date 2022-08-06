#pragma once
#include "..\feature.hpp"

typedef void(__cdecl* _SetPredictionRandomSeed)(void* usercmd);
typedef void(__cdecl* _ivp_srand)(uint32_t seed);
typedef void(__fastcall* _CBasePlayer__InitVCollision)(void* thisptr,
                                                       int edx,
                                                       const Vector& vecAbsOrigin,
                                                       const Vector& vecAbsVelocity);

// RNG prediction
class RNGStuff : public FeatureWrapper<RNGStuff>
{
public:
	int GetPredictionRandomSeed(int commandOffset);
	int commandNumber = 0;

	_ivp_srand ORIG_ivp_srand = nullptr;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	_SetPredictionRandomSeed ORIG_SetPredictionRandomSeed = nullptr;
	_CBasePlayer__InitVCollision ORIG_CBasePlayer__InitVCollision = nullptr;

	static void __cdecl HOOKED_SetPredictionRandomSeed(void* usercmd);
	static void __fastcall HOOKED_CBasePlayer__InitVCollision(void* thisptr,
	                                                          int edx,
	                                                          const Vector& vecAbsOrigin,
	                                                          const Vector& vecAbsVelocity);

	uint32_t* IVP_RAND_SEED = nullptr;
};

extern RNGStuff spt_rng;
