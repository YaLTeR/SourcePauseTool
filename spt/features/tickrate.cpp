#include "stdafx.hpp"
#include "..\spt-serverplugin.hpp"
#include "tickrate.hpp"
#include "convar.hpp"
#include "dbg.h"

TickrateMod spt_tickrate;

bool TickrateMod::ShouldLoadFeature()
{
	return true;
}

namespace patterns
{
	PATTERNS(
	    MiddleOfSV_InitGameDLL,
	    "4104",
	    "8B 0D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? FF ?? D9 15 ?? ?? ?? ?? DD 05 ?? ?? ?? ?? DB F1 DD 05 ?? ?? ?? ?? 77 08 D9 CA DB F2 76 1F D9 CA",
	    "4044",
	    "8B 0D ?? ?? ?? ?? 8B 01 83 C4 14 FF 50 24 D9 15 ?? ?? ?? ?? DC 1D ?? ?? ?? ?? DF E0 F6 C4 05 7B 13");
}

void TickrateMod::InitHooks()
{
	FIND_PATTERN(engine, MiddleOfSV_InitGameDLL);
}

float TickrateMod::GetTickrate()
{
	if (pIntervalPerTick)
		return *pIntervalPerTick;

	return 0.0f;
}

void TickrateMod::SetTickrate(float tickrate)
{
	if (pIntervalPerTick)
		*pIntervalPerTick = tickrate;
}

void TickrateMod::UnloadFeature() {}

CON_COMMAND(_y_spt_tickrate, "Get or set the tickrate. Usage: _y_spt_tickrate [tickrate]")
{
	switch (args.ArgC())
	{
	case 1:
		Msg("Current tickrate: %f\n", spt_tickrate.GetTickrate());
		break;

	case 2:
		spt_tickrate.SetTickrate(atof(args.Arg(1)));
		break;

	default:
		Msg("Usage: _y_spt_tickrate [tickrate]\n");
	}
}

void TickrateMod::LoadFeature()
{
	// interval_per_tick
	if (ORIG_MiddleOfSV_InitGameDLL)
	{
		int ptnNumber = GetPatternIndex((void**)&ORIG_MiddleOfSV_InitGameDLL);

		switch (ptnNumber)
		{
		case 0: // 4104
			pIntervalPerTick = *reinterpret_cast<float**>(ORIG_MiddleOfSV_InitGameDLL + 18);
			break;
		case 1: // 4044
			pIntervalPerTick = *reinterpret_cast<float**>(ORIG_MiddleOfSV_InitGameDLL + 16);
			break;
		default:
			pIntervalPerTick = nullptr;
			break;
		}

		DevMsg("Found interval_per_tick at %p.\n", pIntervalPerTick);

		if (pIntervalPerTick)
			InitCommand(_y_spt_tickrate);
	}
}
