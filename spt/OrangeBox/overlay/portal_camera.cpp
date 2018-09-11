#include "stdafx.h"

#include "portal_camera.hpp"
#include "..\modules.hpp"
#include "..\spt-serverplugin.hpp"
#include "cdll_int.h"
#include "engine\iserverplugin.h"
#include "eiface.h"
#include "tier2\tier2.h"
#include "edict.h"
#include "..\cvars.hpp"

#if SSDK2007
#include "mathlib\vmatrix.h"
#endif


void calculate_sg_position(Vector & new_player_origin, QAngle & new_player_angles)
{
	int portal_index = getPortal(_y_spt_camera_portal.GetString(), false);
	
	if (portal_index == -1)
	{
		auto& player_origin = clientDLL.GetPlayerEyePos();
		auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

		new_player_origin = player_origin;
		new_player_angles = player_angles;
		return;
	}

	auto engine_server = GetEngine();
	auto portal = engine_server->PEntityOfEntIndex(portal_index);
	if (!portal || portal->IsFree() || strcmp(portal->GetClassName(), "prop_portal") != 0) {
		auto& player_origin = clientDLL.GetPlayerEyePos();
		auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

		new_player_origin = player_origin;
		new_player_angles = player_angles;

		return;
	}

	calculate_offset_player(portal, new_player_origin, new_player_angles);
}

void calculate_offset_player(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	// Here we make sure that the eye position and the eye angles match up.
	auto& player_origin = clientDLL.GetPlayerEyePos();
	auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

	auto& matrix = *reinterpret_cast<VMatrix*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + 1072);

	auto eye_origin = player_origin;
	auto new_eye_origin = matrix * eye_origin;
	new_player_origin = new_eye_origin;

	new_player_angles = TransformAnglesToWorldSpace(player_angles, matrix.As3x4());
	new_player_angles.x = AngleNormalizePositive(new_player_angles.x);
	new_player_angles.y = AngleNormalizePositive(new_player_angles.y);
	new_player_angles.z = AngleNormalizePositive(new_player_angles.z);
}

int getPortal(const char * Arg, bool verbose)
{
	int portal_index = atoi(Arg);
	auto engine_server = GetEngine();

	if (!strcmp(Arg, "blue") || !strcmp(Arg, "orange")) {
		std::vector<int> indices;

		portal_index = -1;

		for (int i = 0; i < MAX_EDICTS; ++i) {
			auto ent = engine_server->PEntityOfEntIndex(i);

			if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal")) {
				auto is_orange_portal = *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + 1137);

				if (is_orange_portal == (Arg[0] == 'o'))
					indices.push_back(i);
			}
		}

		if (indices.size() > 1) {
			if(verbose)
				Msg("There are multiple %s portals, please use the index:\n", Arg);

			for (auto i : indices) {
				auto ent = engine_server->PEntityOfEntIndex(i);
				auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + clientDLL.offServerAbsOrigin);

				if (verbose)
					Msg("%d located at %.8f %.8f %.8f\n", i, origin.x, origin.y, origin.z);
			}
		}
		else if (indices.size() == 0) {
			if (verbose)
				Msg("There are no %s portals.\n", Arg);
		}
		else {
			portal_index = indices[0];
		}
	}

	return portal_index;
}
