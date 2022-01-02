#include "stdafx.h"

#if !defined(OE)

#include "generic.hpp"
#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\cvars.hpp"
#include "..\spt-serverplugin.hpp"
#include "..\utils\ent_utils.hpp"
#include "signals.hpp"

#include <sstream>

ConVar y_spt_stucksave("y_spt_stucksave", "", FCVAR_TAS_RESET, "Automatically saves when you get stuck in a prop.\n");
ConVar y_spt_piwsave("y_spt_piwsave", "", FCVAR_TAS_RESET, "Automatically saves when you push a prop into a wall.\n");

typedef int(__fastcall* _CheckStuck)(void* thisptr, int edx);

// Implements saving when the player gets stuck, enabled with y_spt_stucksave
class Stucksave : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	_CheckStuck ORIG_CheckStuck = nullptr;

	static int __fastcall HOOKED_CheckStuck(void* thisptr, int edx);
};

static Stucksave spt_stucksave;

bool Stucksave::ShouldLoadFeature()
{
	return true;
}

void Stucksave::InitHooks()
{
	HOOK_FUNCTION(server, CheckStuck);
}

void Stucksave::LoadFeature()
{
	TickSignal.Connect(&utils::CheckPiwSave);

	// CheckStuck
	if (ORIG_CheckStuck)
	{
		InitConcommandBase(y_spt_stucksave);
	}

	if (TickSignal.Works)
	{
		InitConcommandBase(y_spt_piwsave);
	}
}

void Stucksave::UnloadFeature() {}

int __fastcall Stucksave::HOOKED_CheckStuck(void* thisptr, int edx)
{
	auto ret = spt_stucksave.ORIG_CheckStuck(thisptr, edx);

	if (ret && y_spt_stucksave.GetString()[0] != '\0')
	{
		std::ostringstream oss;
		oss << "save " << y_spt_stucksave.GetString();
		EngineConCmd(oss.str().c_str());
		y_spt_stucksave.SetValue("");
	}

	return ret;
}

#endif