#include "stdafx.h"
#include "saveloads.hpp"
#include "..\sptlib-wrapper.hpp"
#include "signals.hpp"
#include "convar.hpp"
#include "..\features\afterticks.hpp"
#include <format>

CON_COMMAND(y_spt_saveloads, "Enables and begins automated save/load segment creation.\n\n\
Arguments: y_spt_saveloads <segment name> <start index> <end index> [<ticks to wait>] [<extra commands>].\n \
	- <segment name> is the segment name, which will be used to name the saves and demos (format: <segment name>-<index>)\n\
	- <start index> is the index from which the saves and demos will be named from.\n\
	- <end index> is the index up to which SPT will save/load.\n\
	- OPTIONAL: <ticks to wait> is the number of ticks to wait after a save is loaded before saving again.\n\n\
	- OPTIONAL: <extra commands> is a string containing extra commands to be executed every save/load \n\n\
Usage: \n\
	- Enter in the command.\n\
	- Load the save from which the segment should begin from. The save/load process will begin automatically.\n\
	- Use \"y_spt_saveloads_stop\" at any time to stop the process. This will also be bound to \"L\"")
{
	if (args.ArgC() < 4)
	{
		ConWarning("Incorrect number of arguments! Do \"help\" y_spt_saveloads for information.\n");
		return;	
	}

	spt_saveloads.Begin(args.Arg(1),
	                    atoi(args.Arg(2)),
	                    atoi(args.Arg(3)),
	                    args.ArgC() == 4 ? 0 : atoi(args.Arg(4)),
						args.ArgC() == 5 ? nullptr : args.Arg(5));
}

CON_COMMAND(y_spt_saveloads_stop, "Stops the current save/load'ing process.") 
{
	spt_saveloads.Stop();
}

namespace patterns
{
	PATTERNS(
		Engine__SignOnState,
		"OrangeBox-and-older",
		"80 3D ?? ?? ?? ?? 00 74 06 B8 ?? ?? ?? ?? C3 83 3D ?? ?? ?? ?? 02 B8 ?? ?? ?? ??",
		"BMS-Retail",
		"A1 ?? ?? ?? ?? 85 C0 75 ?? B8 ?? ?? ?? ??",
		"Source-2009-and-Portal-2",
		"74 ?? 8B 74 87 04 83 7E 18 00 74 2D 8B 0D ?? ?? ?? ?? 8B 49 18");
};

bool SaveloadsFeature::ShouldLoadFeature() 
{
	return true;
}

void SaveloadsFeature::InitHooks() 
{
	AddMatchAllPattern(patterns::Engine__SignOnState,
	                   "engine",
	                   "Engine__SignOnState",
	                   &signOnStateMatches);
}


void SaveloadsFeature::LoadFeature()
{
	if (!spt_afterticks.Works || signOnStateMatches.empty())
		return;

	auto match = signOnStateMatches[0]; 
	switch (match.ptnIndex)
	{
	case 0:
		ORIG_SignOnState = *(uintptr_t*)(match.ptr + 17);
		break;
	case 1:
		ORIG_SignOnState = *(uintptr_t*)(match.ptr + 1);
		break;
	case 2:
		ORIG_SignOnState = **(uintptr_t**)(match.ptr + 14) + 0x70;
		break;
	default: 
		ORIG_SignOnState = NULL;
		return;
	}

	//FinishRestoreSignal.Connect(this, &SaveloadsFeature::FinishRestore);
	InitCommand(y_spt_saveloads);
	InitCommand(y_spt_saveloads_stop);

	FrameSignal.Connect(this, &SaveloadsFeature::Update);
}


void SaveloadsFeature::Begin(const char* segName_, int startIndex_, int endIndex_, int ticksToWait_, const char* extra) 
{
	this->prefixName.assign(segName_);
	this->startIndex = startIndex_;
	this->endIndex = endIndex_;
	this->ticksToWait = ticksToWait_;
	this->extraCommands.assign(extra == nullptr ? "" : extra);

	execute = true;

	ConMsg(
		"\nSave/load process:\n\
	- Segment name: \"%s\"\n\
	- From index %i to index %i (%i save/loads)\n\
	- Wait time before save: %i\n\
	- Extra commands: \"%s\"\n\n\
Please load the save from which save/loading should begin.\n",
	    segName_,
	    startIndex_,
	    endIndex_,
	    endIndex_ - startIndex_ + 1,
	    ticksToWait_,
	    this->extraCommands.c_str());

	EngineConCmd("bind L y_spt_saveloads_stop");
}

void SaveloadsFeature::Stop() 
{
	if (!execute)
		return;

	this->extraCommands.assign("");
	execute = false;;
	spt_afterticks.ResetAfterticksQueue();
	ConMsg("Save/load process stopped.\n");
}

int SaveloadsFeature::GetSignOnState() 
{
	return *(int*)ORIG_SignOnState;
}

void SaveloadsFeature::Update()
{
	int last = lastSignOnState;
	int cur = GetSignOnState();
	lastSignOnState = cur;

	if (!execute)
		return;

	if (last == cur || cur != 6)
		return;

	std::string segName = std::format("{}-{:03d}", prefixName, startIndex);
	std::string command = std::format(
		"save {0}; \
record {0}; _y_spt_afterticks 20 \"echo #SAVE#\"; _y_spt_afterticks 25 \"stop\"; \
_y_spt_afterticks 30 \"load {0}\"", 
		segName, 
		extraCommands);

	if (ticksToWait > 0)
	{
		afterticks_entry_t entry;
		entry.command.assign(command);
		entry.ticksLeft = ticksToWait;

		spt_afterticks.AddAfterticksEntry(entry);
	}
	else 
	{
		EngineConCmd(command.c_str());
	}
		

	startIndex++;
	if (startIndex > endIndex)
	{
		execute = false;
		ConMsg("Save/load process finished.\n");
	}
}
