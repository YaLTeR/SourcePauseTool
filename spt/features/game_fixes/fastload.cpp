#include "stdafx.hpp"
#include "..\feature.hpp"
#include "..\generic.hpp"
#include "signals.hpp"

#ifdef BMS
ConVar y_spt_fast_loads(
    "y_spt_fast_loads",
    "1",
    0,
    "Increases FPS and turns off rendering during loads to speed up load times. Values greater than one set fps_max to the value after loading.");
#else
ConVar y_spt_fast_loads(
    "y_spt_fast_loads",
    "0",
    0,
    "Increases FPS and turns off rendering during loads to potentially speed up long load times. Values greater than one set fps_max to the value after loading.");
#endif

// Speed up loads minimally
class FastLoads : public FeatureWrapper<FastLoads>
{
public:
	ConVar* fps_max = nullptr;
	ConVar* mat_norendering = nullptr;
	void TurnOnFastLoads();
	void TurnOffFastLoads();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void LoadFeature() override;
	void OnLoad(void* thisptr, int state);
	int lastSignOnState;
};

static FastLoads fast_loads;

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
	if (SetSignonStateSignal.Works)
	{
		InitConcommandBase(y_spt_fast_loads);
		SetSignonStateSignal.Connect(this, &FastLoads::OnLoad);
	}
}

void FastLoads::OnLoad(void* thisptr, int state)
{
	if (state != 6 && lastSignOnState == 6)
		fast_loads.TurnOnFastLoads();
	else if (state == 6 && lastSignOnState != 6)
		fast_loads.TurnOffFastLoads();

	lastSignOnState = state;
}

void FastLoads::TurnOffFastLoads()
{
	int value = y_spt_fast_loads.GetInt();
	if (value)
	{
		if (value > 1)
		{
			fast_loads.fps_max->SetValue(value);
		}
		else
		{
			fast_loads.fps_max->SetValue(300);
		}
		fast_loads.mat_norendering->SetValue(0);
	}
}

void FastLoads::TurnOnFastLoads()
{
	if (y_spt_fast_loads.GetBool())
	{
		fast_loads.fps_max->SetValue(0);
		fast_loads.mat_norendering->SetValue(1);
	}
}
