#include "stdafx.hpp"
#include "..\feature.hpp"
#include "..\cvars.hpp"
#include "signals.hpp"

ConVar y_spt_fast_loads(
    "y_spt_fast_loads",
    "1",
    0,
    "Increases FPS and turns off rendering during loads to speed up load times. Values greater than one set fps_max to the value after loading.");

// Speed up loads minimally
class FastLoads : public FeatureWrapper<FastLoads>
{
public:
	ConVar* fps_max = nullptr;
	ConVar* mat_norendering = nullptr;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void LoadFeature() override;
};

static FastLoads fast_loads;

void LevelShutdown()
{
	if (y_spt_fast_loads.GetBool())
	{
		fast_loads.fps_max->SetValue(0);
		fast_loads.mat_norendering->SetValue(1);
	}
}

void ClientActive(edict_t* pEntity)
{
	if (y_spt_fast_loads.GetBool())
	{
		if (y_spt_fast_loads.GetInt() > 1)
		{
			fast_loads.fps_max->SetValue(y_spt_fast_loads.GetInt());
		}
		else
		{
			fast_loads.fps_max->SetValue(300);
		}
		fast_loads.mat_norendering->SetValue(0);
	}
}

bool FastLoads::ShouldLoadFeature()
{
	fps_max = g_pCVar->FindVar("fps_max");
	mat_norendering = g_pCVar->FindVar("mat_norendering");

	if (fps_max && mat_norendering)
		return true;

	return false;
}

void FastLoads::LoadFeature()
{
	InitConcommandBase(y_spt_fast_loads);
	LevelShutdownSignal.Connect(LevelShutdown);
	ClientActiveSignal.Connect(ClientActive);
}