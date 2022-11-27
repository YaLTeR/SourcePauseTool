#include "stdafx.h"
#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"

// Port of waezone's microsegmenting AHK file
class MicroSegmenting : public FeatureWrapper<MicroSegmenting>
{
public:
	int currentSegment = 1;
	int previousSegment = 0;
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

CON_COMMAND(spt_microsegmenting_set_current_segment,
            "The current segment. Usage: spt_microsegmenting_set_current_segment <number>")
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: spt_microsegmenting_set_current_segment <number>\n");
		return;
	}
	spt_microsegmenting.currentSegment = std::atoi(args.Arg(1));
	spt_microsegmenting.previousSegment = spt_microsegmenting.currentSegment - 1;

	Msg("########## SEGMENT %i ##########\n", spt_microsegmenting.currentSegment);
}

CON_COMMAND(spt_microsegmenting_previous_segment, "Go back one segment.")
{
	spt_microsegmenting.currentSegment--;
	spt_microsegmenting.previousSegment--;

	Msg("########## SEGMENT %i ##########\n", spt_microsegmenting.currentSegment);
}

CON_COMMAND(spt_microsegmenting_next_segment, "Go up one segment.")
{
	spt_microsegmenting.currentSegment++;
	spt_microsegmenting.previousSegment++;

	Msg("########## SEGMENT %i ##########\n", spt_microsegmenting.currentSegment);
}

CON_COMMAND(spt_microsegmenting_record_segment,
            "Start recording a segment. If provided with an arguement, runs an alias of that name after recording.")
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

CON_COMMAND(
    spt_microsegmenting_stop_segment,
    "Stop the current segment. If provided with an arguement, runs an alias of that name instead of the default commands after saving.")
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

CON_COMMAND(
    spt_microsegmenting_load_segment,
    "Loads the current segment. If provided with one arguement, runs an alias of that name after loading. If provided with a 2nd, uses that as the player name instead of your own.")
{
	std::ostringstream loadCmd;
	loadCmd << "load " << spt_microsegmenting.runFolder << spt_microsegmenting.previousSegment << "-"
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

CON_COMMAND(
    spt_microsegmenting_folder,
    "If set, resulting demos/saves will be loaded and saved from this folder. Need to create it in both the root mod directory and in the SAVE folder.")
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

	InitCommand(spt_microsegmenting_set_current_segment);
	InitCommand(spt_microsegmenting_previous_segment);
	InitCommand(spt_microsegmenting_next_segment);
	InitCommand(spt_microsegmenting_stop_segment);
	InitCommand(spt_microsegmenting_record_segment);
	InitCommand(spt_microsegmenting_load_segment);
	InitCommand(spt_microsegmenting_folder);
}