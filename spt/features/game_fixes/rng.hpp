#pragma once
#include "..\feature.hpp"

extern ConVar y_spt_set_ivp_seed_on_load;

// RNG prediction
class RNGStuff : public FeatureWrapper<RNGStuff>
{
public:
	int GetPredictionRandomSeed(int commandOffset);
	int commandNumber = 0;

	DECL_MEMBER_CDECL(void, ivp_srand, uint32_t seed);

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	DECL_HOOK_CDECL(void, SetPredictionRandomSeed, void* usercmd);
#ifdef OE
	DECL_HOOK_THISCALL(void, CBasePlayer__InitVCollision, void*);
#else
	DECL_HOOK_THISCALL(void, CBasePlayer__InitVCollision, void*, const Vector& vecAbsOrigin, const Vector& vecAbsVelocity);
#endif

	uint32_t* IVP_RAND_SEED = nullptr;
};

extern RNGStuff spt_rng;
