#include "stdafx.h"
#if defined(SSDK2007)
#include "vag_searcher.hpp"
#include "interfaces.hpp"
#include "property_getter.hpp"
#include "ent_utils.hpp"
#include "playerio.hpp"
#include "signals.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\cvars.hpp"

VagSearcher spt_vag_searcher;

ConVar y_spt_vag_search_portal("y_spt_vag_search_portal",
                               "blue",
                               FCVAR_CHEAT,
                               "Chooses the portal for the VAG search. Valid options are blue/orange/portal index. This is the portal you enter.\n");

CON_COMMAND(y_spt_vag_search, "Search VAG")
{
	spt_vag_searcher.StartSearch();
}

void VagSearcher::StartSearch()
{
	if (IsIterating())
		return;
	
	enter_portal = getPortal(y_spt_vag_search_portal.GetString(), false);
	if (!enter_portal)
	{
		Msg("Entry portal not found, maybe try using index.\n");
		return;
	}
		
	exit_portal = GetLinkedPortal(enter_portal);
	if (!exit_portal)
	{
		Msg("Exit portal not found, maybe try using index.\n");
		return;
	}
		
	entry_index = enter_portal->entindex();
	exit_index = exit_portal->entindex();
	
	if (utils::GetProperty<int>(entry_index - 1, "m_bActivated") == 0
	    || utils::GetProperty<int>(exit_index - 1, "m_bActivated") == 0)
	{
		Msg("Portal not activated.\n");
		return;
	}
	
	StartIterations();
}

bool VagSearcher::ShouldLoadFeature()
{
	return true;
}

void VagSearcher::InitHooks() {}

void VagSearcher::LoadFeature()
{
	InitCommand(y_spt_vag_search);
	InitConcommandBase(y_spt_vag_search_portal);
	if (TickSignal.Works)
	{
		TickSignal.Connect(this, &VagSearcher::OnTick);
	}
}

void VagSearcher::UnloadFeature() {}

void VagSearcher::VagCrashTriggered()
{
	crash = true;
}

void VagSearcher::StartIterations()
{
	if (!y_spt_prevent_vag_crash.GetBool())
	{
		Msg("Auto enable y_spt_prevent_vag_crash.\n");
		y_spt_prevent_vag_crash.SetValue(1);
	}
    
	entry_origin = utils::GetPortalPosition(enter_portal);
	auto angle = utils::GetPortalAngles(enter_portal);
	// angles_to_vec
	angle *= (M_PI_F / 180);
	entry_norm = Vector(std::cos(angle[1]) * std::cos(angle[0]),
	                    std::sin(angle[1]) * std::cos(angle[0]),
	                    std::sin(-angle[0]));
	exit_origin = utils::GetPortalPosition(exit_portal);
	is_crouched = (utils::GetProperty<int>(0, "m_fFlags") & 2) != 0;
	// change z pos so player center is where the portal center is
	player_half_height = is_crouched ? 18 : 36;
	player_setpos = entry_origin;
	player_setpos.z -= player_half_height;

	// save only component of the portal normal with the largest magnitude, we'll be moving along in that axis
	no_idx = 0;
	auto max_value = std::abs(entry_norm[0]);
	for (int i = 1; i < 3; i++)
	{
		if (std::abs(entry_norm[i]) > max_value)
		{
			max_value = std::abs(entry_norm[i]);
			no_idx = i;
		}
	}

	first_result = NONE;
	iteration = max_iteration;

	// start first iteration
	DevMsg("Iteration %d\n", max_iteration - iteration + 1);
	crash = false;
	setpos_cmd = "setpos " + std::to_string(player_setpos.x) + " " + std::to_string(player_setpos.y) + " "
	             + std::to_string(player_setpos.z);
	DevMsg("Trying: %s\n", setpos_cmd.c_str());
	EngineConCmd(setpos_cmd.c_str());
	// the player position is wacky - it doesn't seem to be valid right away
	cooldown = cooldown_ticks;
}

VagSearcher::VagSearchResult VagSearcher::RunIteration()
{
	if (crash)
	{
		Msg("This VAG would normally cause a crash, not possible here.\n");
		crash = false;
		return WOULD_CAUSE_CRASH;
	}
	auto new_player_pos = utils::GetProperty<Vector>(0, "m_vecOrigin");
	new_player_pos.z += player_half_height;

	// idk how to use this
	auto player_portal_idx = utils::GetProperty<int>(0, "m_hPortalEnvironment") & 0xfff;

	DevMsg("Player pos: %f %f %f\n", new_player_pos.x, new_player_pos.y, new_player_pos.z);
	if (player_portal_idx == entry_index)
	{
		current_result = NEXT_TO_ENTRY;
	}
	else if (player_portal_idx == exit_index)
	{
		current_result = NEXT_TO_EXIT;
	}
	else if ((new_player_pos - entry_origin).IsLengthLessThan(1.0f))
	{
		// behind portal but didn't teleport
		current_result = BEHIND_ENTRY_PLANE;
	}
	else
	{
		Msg("VAG probably worked: %s\n", setpos_cmd.c_str());
		return SUCCESS;
	}

	if (first_result == NONE && current_result != BEHIND_ENTRY_PLANE)
	{
		first_result = current_result;
	}

	if (current_result == NEXT_TO_ENTRY)
	{
		if (first_result == NEXT_TO_EXIT)
		{
			Msg("No VAG found\n");
			return FAIL;
		}
		DevMsg("Trying setpos closer to portal,\n");
		player_setpos[no_idx] = std::nextafterf(player_setpos[no_idx], entry_norm[no_idx] * -INFINITY);
	}
	else if (current_result == NEXT_TO_EXIT)
	{
		if (first_result == NEXT_TO_ENTRY)
		{
			Msg("No VAG found,\n");
			return FAIL;
		}
		DevMsg("Trying setpos further from portal.\n");
		player_setpos[no_idx] = std::nextafterf(player_setpos[no_idx], entry_norm[no_idx] * INFINITY);
	}
	else
	{
		DevMsg("Behind portal plane, trying setpos further from portal.\n");
		player_setpos[no_idx] = std::nextafterf(player_setpos[no_idx], entry_norm[no_idx] * INFINITY);
	}

	iteration--;
	if (iteration <= 0)
	{
		Msg("Maximum iterations reached.\n");
		return MAX_ITERATIONS;
	}

	// next iteration
	DevMsg("Iteration %d\n", max_iteration - iteration + 1);
	crash = false;
	setpos_cmd = "setpos " + std::to_string(player_setpos.x) + " " + std::to_string(player_setpos.y) + " "
	             + std::to_string(player_setpos.z);
	DevMsg("Trying: %s\n", setpos_cmd.c_str());
	EngineConCmd(setpos_cmd.c_str());
	cooldown = cooldown_ticks;
	return ITERATING;
}

void VagSearcher::OnTick() {
	if (IsIterating())
	{
		if (cooldown)
		{
			cooldown--;
		}
		else if (RunIteration() != ITERATING)
		{
			Msg("Finished in %d iteration(s).\n", max_iteration - iteration + 1);
			iteration = 0;
		}
	}
}

#endif