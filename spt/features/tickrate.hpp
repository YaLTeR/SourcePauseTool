#pragma once
#include "..\feature.hpp"

// For getting tickrate stuff
class TickrateMod : public FeatureWrapper<TickrateMod>
{
public:
	float GetTickrate();
	void SetTickrate(float tickrate);

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	uintptr_t ORIG_MiddleOfSV_InitGameDLL = 0;
	float* pIntervalPerTick = nullptr;
};

extern TickrateMod spt_tickrate;
