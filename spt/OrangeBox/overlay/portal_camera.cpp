#include "stdafx.h"

#ifdef SSDK2007
#include "portal_camera.hpp"
#include "..\modules.hpp"
#include "..\spt-serverplugin.hpp"
#include "cdll_int.h"
#include "engine\iserverplugin.h"
#include "eiface.h"
#include "tier2\tier2.h"
#include "edict.h"
#include "..\cvars.hpp"
#include "mathlib\vmatrix.h"
#include "overlay-renderer.hpp"
#include "..\..\utils\ent_utils.hpp"
#include "client_class.h"

const int PORTAL_ORIGIN_OFFSET = 1180;
const int PORTAL_ANGLE_OFFSET = 1192;
const int PORTAL_LINKED_OFFSET = 1068;
const int PORTAL_TRANSITION_OFFSET = 1072;
const int PLAYER_ANGLES_OFFSET = 2568;
const int PORTAL_IS_ORANGE_OFFSET = 1137;
const int INDEX_MASK = MAX_EDICTS - 1;

bool invalidPortal(edict_t* portal)
{
	return !portal || strcmp(portal->GetClassName(), "prop_portal") != 0;
}

bool getPortal(edict_t** portal_edict, Vector& new_player_origin, QAngle& new_player_angles)
{
	if (!serverActive())
		return false;

	int portal_index = getPortal(_y_spt_overlay_portal.GetString(), false);
	auto engine_server = GetEngine();
	edict_t* portal = nullptr;

	if (portal_index != -1)
	{
		portal = engine_server->PEntityOfEntIndex(portal_index);
	}

	if (portal_index == -1 || invalidPortal(portal)) {
		auto& player_origin = clientDLL.GetPlayerEyePos();
		auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + PLAYER_ANGLES_OFFSET);

		new_player_origin = player_origin;
		new_player_angles = player_angles;

		return false;
	}
	
	*portal_edict = portal;
	return true;
}

void calculateAGPosition(Vector& new_player_origin, QAngle& new_player_angles)
{
	edict_t * enter_portal = NULL;
	edict_t * exit_portal = NULL;
	auto engine_server = GetEngine();

	if (getPortal(&enter_portal, new_player_origin, new_player_angles))
	{
		int exit_portal_ehandle = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_LINKED_OFFSET);
		int exit_portal_index = exit_portal_ehandle & INDEX_MASK;

		exit_portal = engine_server->PEntityOfEntIndex(exit_portal_index);
		if (!invalidPortal(exit_portal))
		{
			calculateOffsetPortal(enter_portal, exit_portal, new_player_origin, new_player_angles);
		}	
	}	
}

void calculateSGPosition(Vector& new_player_origin, QAngle& new_player_angles)
{
	edict_t * portal;
	if (getPortal(&portal, new_player_origin, new_player_angles))
		calculateOffsetPlayer(portal, new_player_origin, new_player_angles);
}

bool isInfrontOfPortal(edict_t* saveglitch_portal, const Vector& player_origin)
{
	auto& portal_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + PORTAL_ORIGIN_OFFSET);
	auto& portal_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + PORTAL_ANGLE_OFFSET);

	Vector delta = player_origin - portal_origin;
	Vector normal;
	AngleVectors(portal_angles, &normal);
	float dot = DotProduct(normal, delta);

	return dot >= 0;
}

std::wstring calculateWillAGSG(Vector& new_player_origin, QAngle& new_player_angles)
{
	edict_t* enter_portal;
	if (!getPortal(&enter_portal, new_player_origin, new_player_angles))
		return L"no portal selected";

	Vector enter_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_ORIGIN_OFFSET);
	auto& enter_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_ANGLE_OFFSET);
	Vector enterForward;
	AngleVectors(enter_angles, &enterForward);

	if (enterForward.z >= -0.7071f)
		return L"no, bad entry portal";

	Vector pos;
	calculateAGPosition(pos, QAngle());

	if (enterForward.Dot(pos - enter_origin) >= 0)
		return L"no, tp position in front";
	else
		return L"yes";
}

void calculateOffsetPortal(edict_t* enter_portal, edict_t* exit_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	auto& enter_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_ORIGIN_OFFSET);
	auto& enter_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(enter_portal->GetUnknown()) + PORTAL_ANGLE_OFFSET);
	auto& exit_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(exit_portal->GetUnknown()) + PORTAL_ORIGIN_OFFSET);
	auto& exit_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(exit_portal->GetUnknown()) + PORTAL_ANGLE_OFFSET);

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
	new_player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + PLAYER_ANGLES_OFFSET);
}

void calculateOffsetPlayer(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	auto& player_origin = clientDLL.GetPlayerEyePos();
	auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + PLAYER_ANGLES_OFFSET);

	if (isInfrontOfPortal(saveglitch_portal, player_origin))
	{
		new_player_origin = player_origin;
		new_player_angles = player_angles;
	}
	else
	{
		auto& matrix = *reinterpret_cast<VMatrix*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + PORTAL_TRANSITION_OFFSET);

		auto eye_origin = player_origin;
		auto new_eye_origin = matrix * eye_origin;
		new_player_origin = new_eye_origin;

		new_player_angles = TransformAnglesToWorldSpace(player_angles, matrix.As3x4());
		new_player_angles.x = AngleNormalizePositive(new_player_angles.x);
		new_player_angles.y = AngleNormalizePositive(new_player_angles.y);
		new_player_angles.z = AngleNormalizePositive(new_player_angles.z);
	}
}

int getPortal(const char* arg, bool verbose)
{
	int portal_index = atoi(arg);
	auto engine_server = GetEngine();
	bool want_blue = !strcmp(arg, "blue");
	bool want_orange = !strcmp(arg, "orange");
	bool want_auto = !strcmp(arg, "auto");

	if (want_auto)
		return serverDLL.GetEnviromentPortalHandle() & INDEX_MASK;

	if (want_blue || want_orange || want_auto) {
		std::vector<int> indices;

		for (int i = 0; i < MAX_EDICTS; ++i) {
			auto ent = engine_server->PEntityOfEntIndex(i);

			if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal")) {
				auto is_orange_portal = *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + PORTAL_IS_ORANGE_OFFSET);

				if (is_orange_portal && want_orange)
					indices.push_back(i);
				else if (!is_orange_portal && want_blue)
					indices.push_back(i);
			}
		}

		if (indices.size() > 1) {
			if(verbose)
				Msg("There are multiple %s portals, please use the index:\n", arg);

			for (auto i : indices) {
				auto ent = engine_server->PEntityOfEntIndex(i);
				auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + clientDLL.offServerAbsOrigin);

				if (verbose)
					Msg("%d located at %.8f %.8f %.8f\n", i, origin.x, origin.y, origin.z);
			}
		}
		else if (indices.size() == 0) {
			if (verbose)
				Msg("There are no %s portals.\n", arg);
		}
		else {
			portal_index = indices[0];
		}
	}

	return portal_index;
}
#endif