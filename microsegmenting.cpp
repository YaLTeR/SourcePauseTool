#include "stdafx.h"
#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"
#include <sstream>

// Port of waezone's microsegmenting AHK file
class MicroSegmenting : public FeatureWrapper<MicroSegmenting>
{
public:
	int currentSegment = 1;
	std::string runFolder = "";
	ConVar* playerName = nullptr;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static MicroSegmenting spt_microsegmenting;

bool MicroSegmenting::ShouldLoadFeature()
{
	return true;
}

void MicroSegmenting::InitHooks() {}

void MicroSegmenting::UnloadFeature()
{
	playerName = nullptr;
}

CON_COMMAND(y_spt_microsegmenting_set, "Set the current segment. Usage: y_spt_microsegmenting_set <number>")
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: y_spt_microsegmenting_set <number>\n");
		return;
	}
	spt_microsegmenting.currentSegment = std::atoi(args.Arg(1));

	Msg("########## SEGMENT %i ##########\n", spt_microsegmenting.currentSegment);
}

CON_COMMAND(y_spt_microsegmenting_previous, "Go back one segment.")
{
	spt_microsegmenting.currentSegment--;

	Msg("########## SEGMENT %i ##########\n", spt_microsegmenting.currentSegment);
}

CON_COMMAND(y_spt_microsegmenting_next, "Go up one segment.")
{
	spt_microsegmenting.currentSegment++;

	Msg("########## SEGMENT %i ##########\n", spt_microsegmenting.currentSegment);
}

CON_COMMAND(y_spt_microsegmenting_record, "Start recording a segment. If provided with an arguement, runs an alias of that name after recording.")
{
	std::ostringstream startCmd;
	startCmd << "record " << spt_microsegmenting.runFolder << spt_microsegmenting.currentSegment << "-"
	         << spt_microsegmenting.playerName->GetString() << "-";
	EngineConCmd(startCmd.str().c_str());

	if (args.ArgC() >= 2)
	{
		EngineConCmd(args.Arg(1));
	}
}

CON_COMMAND(y_spt_microsegmenting_stop, "Stop the current segment. If provided with an arguement, runs an alias of that name instead of the default commands after saving. Defaut commands: \"echo #SAVE#;sensitivity 0;wait 100;stop\"")
{
	std::ostringstream saveCmd;
	saveCmd << "save " << spt_microsegmenting.runFolder << spt_microsegmenting.currentSegment << "-"
	        << spt_microsegmenting.playerName->GetString() << "-";
	EngineConCmd(saveCmd.str().c_str());

	if (args.ArgC() >= 2)
	{
		EngineConCmd(args.Arg(1));
	}
	else
	{
		EngineConCmd("echo #SAVE#;sensitivity 0;wait 100;stop");
	}
}

CON_COMMAND(y_spt_microsegmenting_load, "Loads the current segment. If provided with one arguement, runs an alias of that name after loading instead of the default command. Default command: \"sensitivity 0\" If provided with a 2nd, uses that as the player name instead of your own.")
{
	std::ostringstream loadCmd;
	loadCmd << "load " << spt_microsegmenting.runFolder << spt_microsegmenting.currentSegment - 1 << "-"
	        << ((args.ArgC() > 2) ? args.Arg(2) : spt_microsegmenting.playerName->GetString()) << "-";
	EngineConCmd(loadCmd.str().c_str());

	if (args.ArgC() >= 1)
	{
		EngineConCmd(args.Arg(1));
	}
	else
	{
		EngineConCmd("sensitivity 0");
	}
}

CON_COMMAND(y_spt_microsegmenting_folder, "If set, resulting demos/saves will be loaded and saved from this folder. Need to create it in both the root mod directory and in the SAVE folder.")
{
	spt_microsegmenting.runFolder = "";

	if (args.ArgC() >= 2)
	{
		spt_microsegmenting.runFolder.append(args.Arg(1)).append("\\");
	}
}

void MicroSegmenting::LoadFeature()
{
	spt_microsegmenting.playerName = g_pCVar->FindVar("name");

	InitCommand(y_spt_microsegmenting_set);
	InitCommand(y_spt_microsegmenting_previous);
	InitCommand(y_spt_microsegmenting_next);
	InitCommand(y_spt_microsegmenting_stop);
	InitCommand(y_spt_microsegmenting_record);
	InitCommand(y_spt_microsegmenting_load);
	InitCommand(y_spt_microsegmenting_folder);
}