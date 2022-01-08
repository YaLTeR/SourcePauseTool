#pragma once
#include "..\feature.hpp"

typedef void(__cdecl* _SetPredictionRandomSeed)(void* usercmd);

// RNG prediction
class RNGStuff : public FeatureWrapper<RNGStuff>
{
public:
	int GetPredictionRandomSeed(int commandOffset);
	int commandNumber = 0;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	_SetPredictionRandomSeed ORIG_SetPredictionRandomSeed = nullptr;

	static void __cdecl HOOKED_SetPredictionRandomSeed(void* usercmd);
};

extern RNGStuff spt_rng;