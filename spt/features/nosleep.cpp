#include "stdafx.hpp"
#include "..\feature.hpp"
#include "convar.hpp"

ConVar y_spt_focus_nosleep("y_spt_focus_nosleep", "0", 0, "Improves FPS while alt-tabbed.");

// Gives the option to disable sleeping to improve FPS while alt-tabbed
class NoSleepFeature : public FeatureWrapper<NoSleepFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	DECL_HOOK_THISCALL(void, CInputSystem__SleepUntilInput, void*, int nMaxSleepTimeMS);
};

static NoSleepFeature spt_nosleep;

bool NoSleepFeature::ShouldLoadFeature()
{
	return true;
}

namespace patterns
{
	PATTERNS(CInputSystem__SleepUntilInput,
	         "5135",
	         "8B 44 24 ?? 85 C0 7D ??",
	         "5377866-BMS_Retail",
	         "55 8B EC 8B 45 ?? 83 CA FF");
}

void NoSleepFeature::InitHooks()
{
	HOOK_FUNCTION(inputsystem, CInputSystem__SleepUntilInput);
}

void NoSleepFeature::LoadFeature()
{
	if (ORIG_CInputSystem__SleepUntilInput)
	{
		InitConcommandBase(y_spt_focus_nosleep);
	}
}

void NoSleepFeature::UnloadFeature() {}

IMPL_HOOK_THISCALL(NoSleepFeature, void, CInputSystem__SleepUntilInput, void*, int nMaxSleepTimeMS)
{
	if (y_spt_focus_nosleep.GetBool())
		nMaxSleepTimeMS = 0;
	spt_nosleep.ORIG_CInputSystem__SleepUntilInput(thisptr, nMaxSleepTimeMS);
}
