#include "stdafx.hpp"
#include "hud.hpp"
#include "..\feature.hpp"
#include "..\utils\game_detection.hpp"

#include <functional>
#include <memory>

#include "convar.hpp"

#if defined(SSDK2007) || defined(SSDK2013) || defined(OE)

ConVar y_spt_hud_isg("y_spt_hud_isg", "0", FCVAR_CHEAT, "Is the ISG flag set?\n");

namespace patterns
{
	PATTERNS(MiddleOfRecheck_ov_element,
	         "5135",
	         "C6 05 ?? ?? ?? ?? 01 83 EE 01 3B 74 24 28 7D D3 8B 4C 24 38",
	         "1910503",
	         "C6 05 ?? ?? ?? ?? 01 4E 3B 75 ?? 7D ??",
	         "DMoMM",
	         "C6 05 ?? ?? ?? ?? 01 83 EE 01 3B 74 24 30 7D D5");
}

// This feature enables the ISG setting and HUD features
class ISGFeature : public FeatureWrapper<ISGFeature>
{
public:
	bool* isgFlagPtr = nullptr;

protected:
	uint32_t ORIG_MiddleOfRecheck_ov_element = 0;

	virtual bool ShouldLoadFeature() override
	{
		return true;
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
			Warning("spt_hud_isg 1 and spt_set_isg have no effect\n");
	}

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static ISGFeature spt_isg;
CON_COMMAND_F(y_spt_set_isg, "Sets the state of ISG in the game (1 or 0)", FCVAR_DONTRECORD | FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Warning("%s - %s\n", y_spt_set_isg_command.GetName(), y_spt_set_isg_command.GetHelpText());
		return;
	}
	if (spt_isg.isgFlagPtr)
		*spt_isg.isgFlagPtr = atoi(args.Arg(1));
	else
		Warning("%s has no effect\n", y_spt_set_isg_command.GetName());
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

#ifdef SPT_HUD_ENABLED
		AddHudCallback(
		    "isg",
		    [](std::string) { spt_hud_feat.DrawTopHudElement(L"isg: %d", IsISGActive()); },
		    y_spt_hud_isg);
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
