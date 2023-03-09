#include "stdafx.hpp"
#include "convar.hpp"
#include "signals.hpp"
#include "..\feature.hpp"
#include "..\scripts2\srctas_reader2.hpp"

// V2 TASing
class NewTASFeature : public FeatureWrapper<NewTASFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static NewTASFeature spt_new_tas;

CON_COMMAND_AUTOCOMPLETEFILE(
    tas_experimental_load,
    "Loads and executes an .srctas script. If an extra ticks argument is given, the script is played back at maximal FPS and without rendering until that many ticks before the end of the script. Usage: tas_load_script [script] [ticks]",
    0,
    "",
    ".srctas")
{
	scripts2::LoadResult result = scripts2::LoadResult::Error;
	if (args.ArgC() == 2)
	{
		result = scripts2::g_TASReader.ExecuteScript(args.Arg(1));
	}
	else if (args.ArgC() == 3)
	{
		result = scripts2::g_TASReader.ExecuteScriptWithResume(args.Arg(1), std::stoi(args.Arg(2)));
	}
	else
		Msg("Loads and executes an .srctas script. Usage: tas_load_script [script]\n");

	if (result == scripts2::LoadResult::V1Script)
	{
		Msg("Use tas_script_load for version 1 .srctas scripts");
	}
}

bool NewTASFeature::ShouldLoadFeature()
{
	return true;
}

void NewTASFeature::InitHooks() {}

void NewTASFeature::LoadFeature() 
{
	if (FrameSignal.Works)
	{
		InitCommand(tas_experimental_load);
	}
}

void NewTASFeature::UnloadFeature() {}
