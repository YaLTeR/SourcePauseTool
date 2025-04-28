#include "stdafx.hpp"

#if defined(SSDK2007) || defined(SSDK2013)

#include "spt\feature.hpp"
#include "spt\features\playerio.hpp"
#include "spt\features\property_getter.hpp"
#include "spt\utils\ent_utils.hpp"
#include "spt\utils\interfaces.hpp"
#include "spt\utils\game_detection.hpp"
#include "spt\utils\signals.hpp"
#include "spt\sptlib-wrapper.hpp"
#include "spt\utils\portal_utils.hpp"

extern ConVar y_spt_prevent_vag_crash;
extern ConVar _y_spt_overlay_portal;

// VAG tester
class VagSearcher : public FeatureWrapper<VagSearcher>
{
public:
	void StartSearch();
	void VagCrashTriggered();
	bool IsIterating()
	{
		return iteration != 0;
	}
	enum VagSearchResult
	{
		ITERATING,
		SUCCESS,
		FAIL,
		MAX_ITERATIONS,
		WOULD_CAUSE_CRASH
	};
	enum SearchResult
	{
		NONE,
		NEXT_TO_ENTRY,
		NEXT_TO_EXIT,
		BEHIND_ENTRY_PLANE
	};
	void StartIterations();
	VagSearchResult RunIteration();
	void OnTick(bool simulating);

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	int cooldown_ticks = 2;
	int cooldown;
	int max_iteration = 35;
	int iteration = 0;
	bool crash;
	const utils::PortalInfo* enter_portal = NULL;
	int entry_index;
	int exit_index;
	Vector entry_origin;
	Vector entry_norm;
	Vector exit_origin;
	bool is_crouched;
	int player_half_height;
	Vector player_setpos;
	int no_idx;
	SearchResult first_result = NONE;
	SearchResult current_result;
	std::string setpos_cmd;
};

VagSearcher spt_vag_searcher;

ConVar y_spt_vag_search_portal("y_spt_vag_search_portal",
                               "overlay",
                               FCVAR_CHEAT,
                               "Chooses the portal for the VAG search. Valid options are:\n"
                               "" SPT_PORTAL_SELECT_DESCRIPTION_OVERLAY_PREFIX "" SPT_PORTAL_SELECT_DESCRIPTION);

CON_COMMAND(y_spt_vag_search, "Search VAG")
{
	spt_vag_searcher.StartSearch();
}

void VagSearcher::StartSearch()
{
	if (IsIterating())
		return;

	enter_portal = getPortal(y_spt_vag_search_portal.GetString(), true, false);

	if (!enter_portal || !enter_portal->linkedHandle.IsValid())
	{
		Msg("Entry portal not found, maybe try using a color or index.\n");
		return;
	}

	entry_index = enter_portal->handle.GetEntryIndex();

	if (!enter_portal->isOpen)
	{
		Msg("Portal not open.\n");
		return;
	}

	StartIterations();
}

bool VagSearcher::ShouldLoadFeature()
{
	return utils::DoesGameLookLikePortal();
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
	if (VagCrashSignal.Works)
	{
		VagCrashSignal.Connect(this, &VagSearcher::VagCrashTriggered);
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
		Msg("Auto enable spt_prevent_vag_crash.\n");
		y_spt_prevent_vag_crash.SetValue(1);
	}

	entry_origin = enter_portal->pos;
	auto angle = enter_portal->ang;
	AngleVectors(angle, &entry_norm);

	exit_origin = enter_portal->linkedPos;
	is_crouched = (spt_propertyGetter.GetProperty<int>(1, "m_fFlags") & 2) != 0;
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
	auto new_player_pos = spt_propertyGetter.GetProperty<Vector>(1, "m_vecOrigin");
	new_player_pos.z += player_half_height;

	auto player_portal_idx = spt_propertyGetter.GetProperty<int>(1, "m_hPortalEnvironment") & 0xfff;

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

void VagSearcher::OnTick(bool simulating)
{
	if (!simulating || !IsIterating())
		return;

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

#endif
