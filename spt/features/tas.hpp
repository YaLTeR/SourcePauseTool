#pragma once
#include "..\feature.hpp"

// Enables TAS strafing and view related functionality
class TASFeature : public FeatureWrapper<TASFeature>
{
public:
	void Strafe(float* va, bool yawChanged);
	bool tasAddressesWereFound = false;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void LoadFeature() override;
};

extern TASFeature spt_tas;
