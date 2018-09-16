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

const int PORTAL_ORIGIN_OFFSET = 1180;
const int PORTAL_ANGLE_OFFSET = 1192;
const int PORTAL_LINKED_OFFSET = 1068;
const int INDEX_MASK = MAX_EDICTS - 1;


bool get_portal_index(edict_t ** portal_edict, Vector & new_player_origin, QAngle & new_player_angles)
{
	int portal_index = getPortal(_y_spt_overlay_portal.GetString(), false);
	if (portal_index == -1)
	{
		auto& player_origin = clientDLL.GetPlayerEyePos();
		auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

		new_player_origin = player_origin;
		new_player_angles = player_angles;
		return false;
	}

	auto engine_server = GetEngine();
	auto portal = engine_server->PEntityOfEntIndex(portal_index);
	if (!portal || portal->IsFree() || strcmp(portal->GetClassName(), "prop_portal") != 0) {
		auto& player_origin = clientDLL.GetPlayerEyePos();
		auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

		new_player_origin = player_origin;
		new_player_angles = player_angles;

		return false;
	}
	
	*portal_edict = portal;
	return true;
}

void calculate_ag_position(Vector & new_player_origin, QAngle & new_player_angles)
{
	edict_t * enter_portal = NULL;
	edict_t * exit_portal = NULL;
	auto engine_server = GetEngine();

	if (get_portal_index(&enter_portal, new_player_origin, new_player_angles))
	{
		int exit_portal_ehandle = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_LINKED_OFFSET);
		int exit_portal_index = exit_portal_ehandle & INDEX_MASK;

		exit_portal = engine_server->PEntityOfEntIndex(exit_portal_index);
		if (exit_portal && !exit_portal->IsFree() && !strcmp(exit_portal->GetClassName(), "prop_portal"))
		{
			calculate_offset_portal(enter_portal, exit_portal, new_player_origin, new_player_angles);
		}	
	}
		
}

void calculate_sg_position(Vector & new_player_origin, QAngle & new_player_angles)
{
	edict_t * portal;
	if(get_portal_index(&portal, new_player_origin, new_player_angles))
		calculate_offset_player(portal, new_player_origin, new_player_angles);
}

bool is_infront_of_portal(edict_t* saveglitch_portal, const Vector& player_origin)
{
	auto& portal_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + PORTAL_ORIGIN_OFFSET);
	auto& portal_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + PORTAL_ANGLE_OFFSET);

	Vector delta = player_origin - portal_origin;
	Vector normal;
	AngleVectors(portal_angles, &normal);
	float dot = DotProduct(normal, delta);

	return dot >= 0;
}

void calculate_offset_portal(edict_t* enter_portal, edict_t* exit_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	// Here we make sure that the eye position and the eye angles match up.
	auto& enter_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_ORIGIN_OFFSET);
	auto& enter_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_ANGLE_OFFSET);
	auto& exit_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(exit_portal->GetUnknown()) + PORTAL_ORIGIN_OFFSET);
	auto& exit_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(exit_portal->GetUnknown()) + PORTAL_ANGLE_OFFSET);
	auto& matrix = *reinterpret_cast<VMatrix*>(reinterpret_cast<uintptr_t>(exit_portal->GetUnknown()) + 1072);

	Vector exitForward, exitRight, exitUp;
	Vector enterForward, enterRight, enterUp;
	AngleVectors(enter_angles, &enterForward, &enterRight, &enterUp);
	AngleVectors(exit_angles, &exitForward, &exitRight, &exitUp);

	auto delta = enter_origin - exit_origin;

	Vector exit_portal_coords;
	exit_portal_coords.x = delta.Dot(exitForward);
	exit_portal_coords.y = delta.Dot(exitRight);
	exit_portal_coords.z = delta.Dot(exitUp);

	Vector transition(0, 0, 0);
	transition -= exit_portal_coords.x * enterForward;
	transition -= exit_portal_coords.y * enterRight;
	transition += exit_portal_coords.z * enterUp;

	auto new_eye_origin = enter_origin + transition;
	new_player_origin = new_eye_origin;
	new_player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);
}

void calculate_offset_player(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	// Here we make sure that the eye position and the eye angles match up.
	auto& player_origin = clientDLL.GetPlayerEyePos();
	auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

	if (is_infront_of_portal(saveglitch_portal, player_origin))
	{
		new_player_origin = player_origin;
		new_player_angles = player_angles;
	}
	else
	{
		auto& matrix = *reinterpret_cast<VMatrix*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + 1072);

		auto eye_origin = player_origin;
		auto new_eye_origin = matrix * eye_origin;
		new_player_origin = new_eye_origin;

		new_player_angles = TransformAnglesToWorldSpace(player_angles, matrix.As3x4());
		new_player_angles.x = AngleNormalizePositive(new_player_angles.x);
		new_player_angles.y = AngleNormalizePositive(new_player_angles.y);
		new_player_angles.z = AngleNormalizePositive(new_player_angles.z);
	}

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
