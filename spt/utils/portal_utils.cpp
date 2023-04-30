#include "stdafx.hpp"

#include "portal_utils.hpp"

#ifdef SPT_PORTAL_UTILS

#include "..\spt-serverplugin.hpp"

#include "ent_utils.hpp"
#include "..\features\property_getter.hpp"
#include "cdll_int.h"
#include "client_class.h"
#include "edict.h"
#include "eiface.h"
#include "engine\iserverplugin.h"
#include "mathlib\vmatrix.h"
#include "tier2\tier2.h"
#include "interfaces.hpp"
#include "..\features\ent_props.hpp"
#include "game_detection.hpp"

extern ConVar _y_spt_overlay_portal;

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
	if (!utils::DoesGameLookLikePortal())
		return nullptr;
	int handle = spt_propertyGetter.GetProperty<int>(0, "m_hPortalEnvironment");
	if (handle == 0)
		return nullptr;
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
		new_player_origin = utils::GetPlayerEyePosition();
		new_player_angles = utils::GetPlayerEyeAngles();

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
	const auto& portal_origin = utils::GetPortalPosition(saveglitch_portal);
	const auto& portal_angles = utils::GetPortalAngles(saveglitch_portal);

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
	const auto& enter_angles = utils::GetPortalAngles(enter_portal);
	Vector enterForward;
	AngleVectors(enter_angles, &enterForward);

	if (enterForward.z >= -0.7071f)
		return L"no, bad entry portal";

	Vector pos;
	QAngle qa;
	calculateAGPosition(pos, qa);

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
	const auto& enter_origin = utils::GetPortalPosition(enter_portal);
	const auto& enter_angles = utils::GetPortalAngles(enter_portal);
	const auto& exit_origin = utils::GetPortalPosition(exit_portal);
	const auto& exit_angles = utils::GetPortalAngles(exit_portal);

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

void transformThroughPortal(IClientEntity* saveglitch_portal,
                            const Vector& start_pos,
                            const QAngle start_angles,
                            Vector& transformed_origin,
                            QAngle& transformed_angles)
{
	VMatrix matrix;
	matrix.Identity();

	if (isInfrontOfPortal(saveglitch_portal, start_pos))
	{
		transformed_origin = start_pos;
		transformed_angles = start_angles;
	}
	else
	{
		edict_t* e = interfaces::engine_server->PEntityOfEntIndex(saveglitch_portal->entindex());
		static int offset = spt_entprops.GetFieldOffset("CProp_Portal", "m_matrixThisToLinked", true);
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

	auto eye_origin = start_pos;
	auto new_eye_origin = matrix * eye_origin;
	transformed_origin = new_eye_origin;

	transformed_angles = TransformAnglesToWorldSpace(start_angles, matrix.As3x4());
	transformed_angles.x = AngleNormalizePositive(transformed_angles.x);
	transformed_angles.y = AngleNormalizePositive(transformed_angles.y);
	transformed_angles.z = AngleNormalizePositive(transformed_angles.z);
}

void calculateOffsetPlayer(IClientEntity* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	const auto& player_origin = utils::GetPlayerEyePosition();
	const auto& player_angles = utils::GetPlayerEyeAngles();
	transformThroughPortal(saveglitch_portal, player_origin, player_angles, new_player_origin, new_player_angles);
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
				const auto& origin = utils::GetPortalPosition(ent);

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
	else
	{
		portal = utils::GetClientEntity(portal_index);
	}

	return portal;
}

#endif
