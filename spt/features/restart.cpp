#include "stdafx.h"
#include <Windows.h>
#include "..\feature.hpp"
#include "..\spt-serverplugin.hpp"
#include "..\sptlib-wrapper.hpp"
#include "SPTLib\Hooks.hpp"

// Does game restarts
class RestartFeature : public FeatureWrapper<RestartFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void LoadFeature() override;
};

static RestartFeature spt_restart;
static HMODULE spt_module = 0;

CON_COMMAND(tas_restart_game, "Restarts the game")
{
	bool gotHandle = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&spt_module, &spt_module);
	if (gotHandle)
	{
		EngineConCmd("_restart");
	}
}

bool RestartFeature::ShouldLoadFeature()
{
#if defined(OE) || defined(SSDK2013)
	return true;
#else
	return false;
#endif
}

void RestartFeature::LoadFeature()
{
	if (spt_module != 0)
	{
		FreeLibrary(spt_module); // clear old reference from restart
		spt_module = 0;
	}

	InitCommand(tas_restart_game);
}
