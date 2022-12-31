#include "stdafx.h"
#include "afterticks.hpp"
#include "generic.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\cvars.hpp"
#include "signals.hpp"
#include "dbg.h"
#include <sstream>

AfterticksFeature spt_afterticks;
static std::vector<afterticks_entry_t> afterticksQueue;
static bool afterticksPaused = false;
static int afterticksDelay = 0;

ConVar _y_spt_afterticks_await_legacy("_y_spt_afterticks_await_legacy",
                                       "0",
                                       FCVAR_TAS_RESET,
                                       "Set to 1 for backwards compatibility with old scripts.");
ConVar _y_spt_afterticks_reset_on_server_activate("_y_spt_afterticks_reset_on_server_activate", "1", FCVAR_ARCHIVE);


namespace patterns
{
	PATTERNS(
	    HostRunframe__TargetString,
	    "1",
	    "5F 48 6F 73 74 5F 52 75 6E 46 72 61 6D 65 20 28 74 6F 70 29 3A 20 20 5F 68 65 61 70 63 68 6B 28 29 20 21 3D 20 5F 48 45 41 50 4F 4B 0A 00");
	PATTERNS(
		Engine__StringReferences, 
		"1", 
		"68"
	);
}

void AfterticksFeature::AddAfterticksEntry(afterticks_entry_t entry)
{
	afterticksQueue.push_back(entry);
}

void AfterticksFeature::ResetAfterticksQueue()
{
	afterticksQueue.clear();
}

void AfterticksFeature::PauseAfterticksQueue()
{
	afterticksPaused = true;
}

void AfterticksFeature::ResumeAfterticksQueue()
{
	afterticksPaused = false;
}

void AfterticksFeature::DelayAfterticksQueue(int delay)
{
	afterticksDelay = delay;
}

int AfterticksFeature::GetTickCount() 
{
	return *(int*)ptrHostTickCount;
}

bool AfterticksFeature::ShouldLoadFeature()
{
	return true;
}

void AfterticksFeature::UnloadFeature() 
{
	ptrHostTickCount = NULL;
	Works = false;
}

void AfterticksFeature::OnFrame()
{
	int current = GetTickCount();
	int diff = current - lastTickCount;
	lastTickCount = current;

	if (afterticksPaused)
	{
		return;
	}

	if (afterticksDelay > 0)
	{
		afterticksDelay -= diff;
		return;
	}

	for (auto it = afterticksQueue.begin(); it != afterticksQueue.end();)
	{
		it->ticksLeft -= diff;
		if (it->ticksLeft <= 0)
		{
			EngineConCmd(it->command.c_str());
			if (true)
				DevWarning("afterticks: command \"%s\" fired late by %i tick(s)\n",
					it->command.c_str(), 
					it->ticksLeft * -1);

			it = afterticksQueue.erase(it);
		}
		else
			++it;
	}
}

void AfterticksFeature::SV_ActivateServer(bool result)
{
	if (_y_spt_afterticks_reset_on_server_activate.GetBool())
		ResetAfterticksQueue();
}

void AfterticksFeature::FinishRestore(void* thisptr)
{
	if (_y_spt_afterticks_await_legacy.GetBool())
		ResumeAfterticksQueue();
}

void AfterticksFeature::SetPaused(void* thisptr, bool paused)
{
	if (!paused && !_y_spt_afterticks_await_legacy.GetBool())
		ResumeAfterticksQueue();
}

CON_COMMAND(_y_spt_afterticks_wait, "Delays the afterticks queue. Usage: _y_spt_afterticks_wait <delay>")
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: _y_spt_afterticks_wait <delay>\n");
		return;
	}

	int delay = std::stoi(args.Arg(1));

	spt_afterticks.DelayAfterticksQueue(delay);
}

CON_COMMAND(_y_spt_afterticks, "Add a command into an afterticks queue. Usage: _y_spt_afterticks <count> <command>")
{
	if (args.ArgC() != 3)
	{
		Msg("Usage: _y_spt_afterticks <count> <command>\n");
		return;
	}

	afterticks_entry_t entry;

	std::istringstream ss(args.Arg(1));
	ss >> entry.ticksLeft;
	entry.command.assign(args.Arg(2));

	spt_afterticks.AddAfterticksEntry(entry);
}

#ifndef OE
CON_COMMAND(
    _y_spt_afterticks2,
    "Add everything after count as a command into the queue. Do not insert the command in quotes. Usage: _y_spt_afterticks2 <count> <command>")
{
	if (args.ArgC() < 3)
	{
		Msg("Usage: _y_spt_afterticks2 <count> <command>\n");
		return;
	}

	afterticks_entry_t entry;

	std::istringstream ss(args.Arg(1));
	ss >> entry.ticksLeft;
	const char* cmd = args.ArgS() + strlen(args.Arg(1)) + 1;
	entry.command.assign(cmd);

	spt_afterticks.AddAfterticksEntry(entry);
}
#endif

CON_COMMAND(
    _y_spt_afterticks_await_load,
    "Pause reading from the afterticks queue until the next load or changelevel. Useful for writing scripts spanning multiple maps or save-load segments.")
{
	spt_afterticks.PauseAfterticksQueue();
}

CON_COMMAND(_y_spt_afterticks_reset, "Reset the afterticks queue.")
{
	spt_afterticks.ResetAfterticksQueue();
}

void AfterticksFeature::InitHooks() 
{
	FIND_PATTERN(engine, HostRunframe__TargetString);
	AddMatchAllPattern(patterns::Engine__StringReferences,
	                   "engine",
	                   "Engine__StringReferences",
	                   &stringReferenceMatches);
}

void AfterticksFeature::LoadFeature()
{
	Works = false;

	if (!spt_generic.ORIG_HudUpdate)
		return;

	// find host_tickcount pointer
	{
		if (!ORIG_HostRunframe__TargetString || stringReferenceMatches.empty())
			return;

		void* handle = nullptr;
		void* base = nullptr;
		size_t size = 0;
		if (!MemUtils::GetModuleInfo(L"engine.dll", &handle, &base, &size))
			return;

		for (auto match : stringReferenceMatches)
		{
			auto strRefPtr = match.ptr;
			uintptr_t refLocation = *(uintptr_t*)(strRefPtr + 1);
			if (refLocation != (uintptr_t)ORIG_HostRunframe__TargetString)
				continue;

			// bytes after the string reference
			uint8* bytes = (uint8*)strRefPtr;
			// 0x700 as this function's quite long...
			for (int i = 0; i <= 0x700; i++)
			{
				// this is a jump backwards to the start of the update loop, where
				// the tick count is incremented.
				if (bytes[i] == 0x0F && bytes[i + 1] == 0x8C && bytes[i + 5] == 0xFF)
				{
					int jump = *(int*)(bytes + i + 2);
					uintptr_t loopStart = (uintptr_t)bytes + jump + i + 6;
					if (loopStart < (uintptr_t)base)
						// bogus jump
						continue; 

					uintptr_t loopEnd = (uintptr_t)bytes + i;
					for (; loopStart <= loopEnd; loopStart++)
					{
						/*
						* host_tickcount and host_currentframetick live near each other in memory,
						* and at the start of the loop are incremented one after another 
						* so when we find a candidate for host_tickcount, check the following bytes
						* if we find another pointer point to somewhere near it.
						*/
						uintptr_t candidate = *(uintptr_t*)loopStart;
						if (candidate - (uintptr_t)base > size)
							continue;

						// xx (xx) [host_tickcount]			INC [host_tickcount]
						// xx (xx) [host_currentframetick]	INC [host_currentframetick]
						// inc here could be 1 or 2 bytes long
						for (int k = 0; k <= 2; k++)
						{
							uintptr_t candidateCurFrameTick = *(uintptr_t*)(loopStart + 4 + k);
							if (candidateCurFrameTick - (uintptr_t)base > size ||
								candidateCurFrameTick - candidate > 0x8)
								continue;

							ptrHostTickCount = candidate;
							goto success;
						}
					}
				}
			}
		}

		return;

	success:;
	}
	
	InitCommand(_y_spt_afterticks);
	InitCommand(_y_spt_afterticks_reset);
	InitCommand(_y_spt_afterticks_wait);
#ifndef OE
	InitCommand(_y_spt_afterticks2);
#endif

	FrameSignal.Connect(this, &AfterticksFeature::OnFrame);

	if (SV_ActivateServerSignal.Works)
	{
		SV_ActivateServerSignal.Connect(this, &AfterticksFeature::SV_ActivateServer);
		InitConcommandBase(_y_spt_afterticks_reset_on_server_activate);
	}

	if (FinishRestoreSignal.Works || SetPausedSignal.Works)
	{
		FinishRestoreSignal.Connect(this, &AfterticksFeature::FinishRestore);
		SetPausedSignal.Connect(this, &AfterticksFeature::SetPaused);
		InitCommand(_y_spt_afterticks_await_load);
		InitConcommandBase(_y_spt_afterticks_await_legacy);

		if (!SetPausedSignal.Works)
		{
			_y_spt_afterticks_await_legacy.SetValue(1);
			Warning("_y_spt_afterticks_await_legacy 0 has no effect.\n");
		}
		else if (!FinishRestoreSignal.Works)
		{
			_y_spt_afterticks_await_legacy.SetValue(0);
			Warning("_y_spt_afterticks_await_legacy 1 has no effect.\n");
		}
	}

	Works = true;
}
