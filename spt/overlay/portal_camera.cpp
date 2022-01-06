#include "stdafx.h"

#ifndef OE
#include "..\spt-serverplugin.hpp"

#include "ent_utils.hpp"
#include "property_getter.hpp"
#include "..\cvars.hpp"
#include "cdll_int.h"
#include "client_class.h"
#include "edict.h"
#include "eiface.h"
#include "engine\iserverplugin.h"
#include "mathlib\vmatrix.h"
#include "overlay-renderer.hpp"
#include "portal_camera.hpp"
#include "tier2\tier2.h"
#include "interfaces.hpp"
#include "..\features\ent_props.hpp"

const int INDEX_MASK = MAX_EDICTS - 1;

IClientEntity* getPortal(const char* arg, bool verbose);

// From /game/shared/portal/prop_portal_shared.cpp
void UpdatePortalTransformationMatrix(const matrix3x4_t& localToWorld,
                                      const matrix3x4_t& remoteToWorld,
                                      VMatrix& matrix)
{
	VMatrix matPortal1ToWorldInv, matPortal2ToWorld, matRotation;

	//inverse of this
	MatrixInverseTR(localToWorld, matPortal1ToWorldInv);

	//180 degree rotation about up
	matRotation.Identity();
	matRotation.m[0][0] = -1.0f;
	matRotation.m[1][1] = -1.0f;

	//final
	matPortal2ToWorld = remoteToWorld;
	matrix = matPortal2ToWorld * matRotation * matPortal1ToWorldInv;
}

void UpdatePortalTransformationMatrix(IClientEntity* localPortal, IClientEntity* remotePortal, VMatrix& matrix)
{
	Vector localForward, localRight, localUp;
	Vector remoteForward, remoteRight, remoteUp;
	AngleVectors(utils::GetPortalAngles(localPortal), &localForward, &localRight, &localUp);
	AngleVectors(utils::GetPortalAngles(remotePortal), &remoteForward, &remoteRight, &remoteUp);

	matrix3x4_t localToWorld(localForward, -localRight, localUp, utils::GetPortalPosition(localPortal));
	matrix3x4_t remoteToWorld(remoteForward, -remoteRight, remoteUp, utils::GetPortalPosition(remotePortal));
	UpdatePortalTransformationMatrix(localToWorld, remoteToWorld, matrix);
}

bool invalidPortal(IClientEntity* portal)
{
	return !portal || strcmp(portal->GetClientClass()->GetName(), "CProp_Portal") != 0;
}

IClientEntity* GetEnvironmentPortal()
{
	int handle = utils::GetProperty<int>(0, "m_hPortalEnvironment");
	int index = (handle & INDEX_MASK) - 1;

	return utils::GetClientEntity(index);
}

IClientEntity* GetLinkedPortal(IClientEntity* portal)
{
	return utils::FindLinkedPortal(portal);
}

bool getPortal(IClientEntity** portal_edict, Vector& new_player_origin, QAngle& new_player_angles)
{
	IClientEntity* portal = getPortal(_y_spt_overlay_portal.GetString(), false);

	if (!portal)
	{
		auto& player_origin = utils::GetPlayerEyePosition();
		auto& player_angles = utils::GetPlayerEyeAngles();

		new_player_origin = player_origin;
		new_player_angles = player_angles;

		return false;
	}

	*portal_edict = portal;
	return true;
}

void calculateAGPosition(Vector& new_player_origin, QAngle& new_player_angles)
{
	IClientEntity* enter_portal = NULL;
	IClientEntity* exit_portal = NULL;

	if (getPortal(&enter_portal, new_player_origin, new_player_angles))
	{
		exit_portal = GetLinkedPortal(enter_portal);
		if (exit_portal)
		{
			calculateAGOffsetPortal(enter_portal, exit_portal, new_player_origin, new_player_angles);
		}
	}
}

void calculateSGPosition(Vector& new_player_origin, QAngle& new_player_angles)
{
	IClientEntity* portal;
	if (getPortal(&portal, new_player_origin, new_player_angles))
		calculateOffsetPlayer(portal, new_player_origin, new_player_angles);
}

bool isInfrontOfPortal(IClientEntity* saveglitch_portal, const Vector& player_origin)
{
	auto& portal_origin = utils::GetPortalPosition(saveglitch_portal);
	auto& portal_angles = utils::GetPortalAngles(saveglitch_portal);

	Vector delta = player_origin - portal_origin;
	Vector normal;
	AngleVectors(portal_angles, &normal);
	float dot = DotProduct(normal, delta);

	return dot >= 0;
}

std::wstring calculateWillAGSG(Vector& new_player_origin, QAngle& new_player_angles)
{
	IClientEntity* enter_portal;
	if (!getPortal(&enter_portal, new_player_origin, new_player_angles))
		return L"no portal selected";

	Vector enter_origin = utils::GetPortalPosition(enter_portal);
	auto& enter_angles = utils::GetPortalAngles(enter_portal);
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

void calculateAGOffsetPortal(IClientEntity* enter_portal,
                             IClientEntity* exit_portal,
                             Vector& new_player_origin,
                             QAngle& new_player_angles)
{
	auto& enter_origin = utils::GetPortalPosition(enter_portal);
	auto& enter_angles = utils::GetPortalAngles(enter_portal);
	auto& exit_origin = utils::GetPortalPosition(exit_portal);
	auto& exit_angles = utils::GetPortalAngles(exit_portal);

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
	new_player_angles = utils::GetPlayerEyeAngles();
}

void calculateOffsetPlayer(IClientEntity* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	auto& player_origin = utils::GetPlayerEyePosition();
	auto& player_angles = utils::GetPlayerEyeAngles();
	VMatrix matrix;
	matrix.Identity();

	if (isInfrontOfPortal(saveglitch_portal, player_origin))
	{
		new_player_origin = player_origin;
		new_player_angles = player_angles;
	}
	else
	{
		edict_t* e = interfaces::engine_server->PEntityOfEntIndex(saveglitch_portal->entindex());
		static int offset = spt_entutils.GetFieldOffset("CProp_Portal", "m_matrixThisToLinked", true);
		if (e && offset != -1)
		{
			uintptr_t serverEnt = reinterpret_cast<uintptr_t>(e->GetUnknown());
			matrix = *reinterpret_cast<VMatrix*>(serverEnt + offset);
		}
		else
		{
			IClientEntity* linkedPortal = GetLinkedPortal(saveglitch_portal);

			if (linkedPortal)
			{
				UpdatePortalTransformationMatrix(saveglitch_portal, linkedPortal, matrix);
			}
		}
	}

	auto eye_origin = player_origin;
	auto new_eye_origin = matrix * eye_origin;
	new_player_origin = new_eye_origin;

	new_player_angles = TransformAnglesToWorldSpace(player_angles, matrix.As3x4());
	new_player_angles.x = AngleNormalizePositive(new_player_angles.x);
	new_player_angles.y = AngleNormalizePositive(new_player_angles.y);
	new_player_angles.z = AngleNormalizePositive(new_player_angles.z);
}

IClientEntity* getPortal(const char* arg, bool verbose)
{
	int portal_index = atoi(arg);
	IClientEntity* portal = nullptr;
	bool want_blue = !strcmp(arg, "blue");
	bool want_orange = !strcmp(arg, "orange");
	bool want_auto = !strcmp(arg, "auto");

	if (want_auto)
	{
		return GetEnvironmentPortal();
	}

	if (want_blue || want_orange || want_auto)
	{
		std::vector<int> indices;

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			IClientEntity* ent = utils::GetClientEntity(i);

			if (!invalidPortal(ent))
			{
				const char* modelName = utils::GetModelName(ent);
				bool is_orange_portal = strstr(modelName, "portal2");

				if (is_orange_portal && want_orange)
				{
					indices.push_back(i);
				}
				else if (!is_orange_portal && want_blue)
				{
					indices.push_back(i);
				}
			}
		}

		if (indices.size() > 1)
		{
			if (verbose)
				Msg("There are multiple %s portals, please use the index:\n", arg);

			for (auto i : indices)
			{
				auto ent = utils::GetClientEntity(i);
				auto& origin = utils::GetPortalPosition(ent);

				if (verbose)
					Msg("%d located at %.8f %.8f %.8f\n", i, origin.x, origin.y, origin.z);
			}
		}
		else if (indices.size() == 0)
		{
			if (verbose)
				Msg("There are no %s portals.\n", arg);
		}
		else
		{
			portal_index = indices[0];
			portal = utils::GetClientEntity(portal_index);
		}
	}

	return portal;
}

#endif