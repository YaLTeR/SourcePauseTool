#include "stdafx.h"
#include "..\feature.hpp"
#include "convar.hpp"

typedef void(__fastcall* _CInputSystem__SleepUntilInput)(void* thisptr, int edx, int nMaxSleepTimeMS);
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
	_CInputSystem__SleepUntilInput ORIG_CInputSystem__SleepUntilInput = nullptr;
	static void __fastcall HOOKED_CInputSystem__SleepUntilInput(void* thisptr, int edx, int nMaxSleepTimeMS);
};

static NoSleepFeature spt_nosleep;

bool NoSleepFeature::ShouldLoadFeature()
{
	return true;
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

void __fastcall NoSleepFeature::HOOKED_CInputSystem__SleepUntilInput(void* thisptr, int edx, int nMaxSleepTimeMS)
{
	if (y_spt_focus_nosleep.GetBool())
		nMaxSleepTimeMS = 0;
	spt_nosleep.ORIG_CInputSystem__SleepUntilInput(thisptr, edx, nMaxSleepTimeMS);
}
