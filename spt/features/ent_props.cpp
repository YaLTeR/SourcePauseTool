#include "stdafx.h"
#include "..\feature.hpp"
#include "convar.hpp"
#include "game_detection.hpp"
#include "interfaces.hpp"
#include "ent_utils.hpp"
#include <string>

// Initializes ent utils stuff
class EntUtils : public Feature
{
public:
	virtual bool ShouldLoadFeature()
	{
#ifdef OE
		return false;
#else
		return true;
#endif
	}

protected:
};

static EntUtils spt_entutils;

CON_COMMAND(y_spt_canjb, "Tests if player can jumpbug on a given height, with the current position and speed.")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_canjb [height]\n");
		return;
	}

	float height = std::stof(args.Arg(1));
	auto can = utils::CanJB(height);

	if (can.canJB)
		Msg("Yes, projected landing height %.6f in %d ticks\n", can.landingHeight - height, can.ticks);
	else
		Msg("No, missed by %.6f in %d ticks.\n", can.landingHeight - height, can.ticks);
}

#ifndef OE
CON_COMMAND(y_spt_print_ents, "Prints all client entity indices and their corresponding classes.")
{
	utils::PrintAllClientEntities();
}

CON_COMMAND(y_spt_print_portals, "Prints all portal indexes, their position and angles.")
{
	utils::PrintAllPortals();
}

CON_COMMAND(y_spt_print_ent_props, "Prints all props for a given entity index.")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_print_ent_props [index]\n");
	}
	else
	{
		utils::PrintAllProps(std::stoi(args.Arg(1)));
	}
}
#endif
