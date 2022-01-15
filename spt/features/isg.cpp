#include "stdafx.h"
#include "hud.hpp"
#include "..\feature.hpp"
#include "..\utils\game_detection.hpp"

#include <functional>
#include <memory>

#include "convar.hpp"

#if defined(SSDK2007) || defined(SSDK2013)

ConVar y_spt_hud_isg("y_spt_hud_isg", "0", FCVAR_CHEAT, "Is the ISG flag set?\n");

// This feature enables the ISG setting and HUD features
class ISGFeature : public FeatureWrapper<ISGFeature>
{
public:
	bool* isgFlagPtr = nullptr;

protected:
	uint32_t ORIG_MiddleOfRecheck_ov_element = 0;

	virtual bool ShouldLoadFeature() override
	{
		return utils::DoesGameLookLikePortal();
	}

	virtual void InitHooks() override
	{
		FIND_PATTERN(vphysics, MiddleOfRecheck_ov_element);
	}

	virtual void PreHook() override
	{
		if (ORIG_MiddleOfRecheck_ov_element)
			this->isgFlagPtr = *(bool**)(ORIG_MiddleOfRecheck_ov_element + 2);
		else
			Warning("y_spt_hud_isg 1 and y_spt_set_isg have no effect\n");
	}

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static ISGFeature spt_isg;
CON_COMMAND_F(y_spt_set_isg,
              "Sets the state of ISG in the game (1 or 0), no arguments means 1",
              FCVAR_DONTRECORD | FCVAR_CHEAT)
{
	if (spt_isg.isgFlagPtr)
		*spt_isg.isgFlagPtr = args.ArgC() == 1 || atoi(args[1]);
	else
		Warning("y_spt_set_isg has no effect\n");
}

bool IsISGActive()
{
	if (spt_isg.isgFlagPtr)
		return *spt_isg.isgFlagPtr;
	else
		return false;
}

void ISGFeature::LoadFeature()
{
	if (ORIG_MiddleOfRecheck_ov_element)
	{
		InitCommand(y_spt_set_isg);

#ifdef SSDK2007
		AddHudCallback(
		    "isg", []() { spt_hud.DrawTopHudElement(L"isg: %d", IsISGActive()); }, y_spt_hud_isg);
#endif
	}
}

void ISGFeature::UnloadFeature() {}

#else

bool IsISGActive()
{
	return false;
}

#endif