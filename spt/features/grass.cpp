#include "stdafx.hpp"
#include "..\feature.hpp"

// Feature description
class GrassFeature : public FeatureWrapper<GrassFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override
	{
		return true;
	}

	virtual void InitHooks() override {}

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override {}
};

CON_COMMAND(spt_touch_grass, "crashes the game")
{
	AssertMsg(false, "Grass has been touched");
	abort();
}

static GrassFeature spt_grass;

void GrassFeature::LoadFeature()
{
	InitCommand(spt_touch_grass);
}
