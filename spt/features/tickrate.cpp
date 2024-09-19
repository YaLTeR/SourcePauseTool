#include "stdafx.hpp"
#include "..\spt-serverplugin.hpp"
#include "tickrate.hpp"
#include "convar.hpp"
#include "visualizations\imgui\imgui_interface.hpp"

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

	return 0.015f;
}

void TickrateMod::SetTickrate(float tickrate)
{
	if (pIntervalPerTick)
		*pIntervalPerTick = tickrate;
}

void TickrateMod::UnloadFeature() {}

CON_COMMAND(_y_spt_tickrate, "Get or set the tickrate")
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
		Msg("Usage: spt_tickrate [tickrate]\n");
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
		{
			InitCommand(_y_spt_tickrate);

			SptImGuiGroup::Cheats_Tickrate.RegisterUserCallback(
			    []()
			    {
				    ImGui::Text("Current tickrate: %g", spt_tickrate.GetTickrate());
				    static double df = DEFAULT_TICK_INTERVAL;
				    SptImGui::InputDouble("new tickrate", &df);
				    ImGui::SameLine();
				    if (ImGui::Button("set"))
				    {
					    /*
					    * KILLME: the game stores the interval internally as a float, but the
					    * comparisons in game are done with the macros which are doubles.
					    */
					    float f = (float)df;
					    if (f == (float)MAXIMUM_TICK_INTERVAL)
						    f = nexttowardf(f, -HUGE_VALL);
					    else if (f == (float)MINIMUM_TICK_INTERVAL)
						    f = nexttowardf(f, HUGE_VALL);
					    spt_tickrate.SetTickrate(f);
				    }
				    ImGui::SameLine();
				    SptImGui::CmdHelpMarkerWithName(_y_spt_tickrate_command);
				    if (df < MINIMUM_TICK_INTERVAL || df > MAXIMUM_TICK_INTERVAL)
				    {
					    ImGui::TextColored(
					        SPT_IMGUI_WARN_COLOR_YELLOW,
					        "Warning: this value is outside [%.3f,%.3f], it may crash the game.",
					        MINIMUM_TICK_INTERVAL,
					        MAXIMUM_TICK_INTERVAL);
				    }
			    });
		}
	}
}
