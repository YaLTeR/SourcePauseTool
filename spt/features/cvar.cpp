#include "stdafx.h"
#include "..\feature.hpp"
#include "convar.hpp"
#include "..\utils\math.hpp"
#include "interfaces.hpp"
#include <string>

CON_COMMAND(y_spt_cvar, "CVar manipulation.")
{
	if (!interfaces::engine || !g_pCVar)
		return;

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar* cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	if (args.ArgC() == 2)
	{
		Msg("\"%s\" = \"%s\"\n", cvar->GetName(), cvar->GetString(), cvar->GetHelpText());
		Msg("Default: %s\n", cvar->GetDefault());

		float val;
		if (cvar->GetMin(val))
		{
			Msg("Min: %f\n", val);
		}

		if (cvar->GetMax(val))
		{
			Msg("Max: %f\n", val);
		}

		const char* helpText = cvar->GetHelpText();
		if (helpText[0] != '\0')
			Msg("- %s\n", cvar->GetHelpText());

		return;
	}

	const char* value = args.Arg(2);
	cvar->SetValue(value);
}

CON_COMMAND(y_spt_cvar_random, "Randomize CVar value.")
{
	if (!g_pCVar)
		return;

	if (args.ArgC() != 4)
	{
		Msg("Usage: y_spt_cvar_random <name> <min> <max>\n");
		return;
	}

	ConVar* cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	float min = std::stof(args.Arg(2));
	float max = std::stof(args.Arg(3));

	float r = utils::RandomFloat(min, max);
	cvar->SetValue(r);
}

#if !defined(OE)
CON_COMMAND(y_spt_cvar2, "CVar manipulation, sets the CVar value to the rest of the argument string.")
{
	if (!interfaces::engine || !g_pCVar)
		return;

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar* cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	if (args.ArgC() == 2)
	{
		Msg("\"%s\" = \"%s\"\n", cvar->GetName(), cvar->GetString(), cvar->GetHelpText());
		Msg("Default: %s\n", cvar->GetDefault());

		float val;
		if (cvar->GetMin(val))
		{
			Msg("Min: %f\n", val);
		}

		if (cvar->GetMax(val))
		{
			Msg("Max: %f\n", val);
		}

		const char* helpText = cvar->GetHelpText();
		if (helpText[0] != '\0')
			Msg("- %s\n", cvar->GetHelpText());

		return;
	}

	const char* value = args.ArgS() + strlen(args.Arg(1)) + 1;
	cvar->SetValue(value);
}

#endif
