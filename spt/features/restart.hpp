#pragma once

#include "..\feature.hpp"

#if defined(OE) || defined(SSDK2013)
#define SPT_TAS_RESTART_ENABLED
#endif

// Does game restarts
class RestartFeature : public FeatureWrapper<RestartFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void LoadFeature() override;
};
