#include "stdafx.h"
#include "..\feature.hpp"
#include "..\utils\game_detection.hpp"

#include <functional>
#include <memory>

#include "convar.hpp"

#if defined(SSDK2007) || defined(SSDK2013)

// This feature enables the ISG setting and HUD features
class ISGFeature : public Feature
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

void ISGFeature::LoadFeature() {}

void ISGFeature::UnloadFeature() {}

bool IsISGActive()
{
	if (spt_isg.isgFlagPtr)
		return *spt_isg.isgFlagPtr;
	else
		return false;
}
#else

bool IsISGActive()
{
	return false;
}

#endif