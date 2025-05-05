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

void UpdatePortalTransformationMatrix(const utils::PortalInfo* info, VMatrix& matrix)
{
	Vector localForward, localRight, localUp;
	Vector remoteForward, remoteRight, remoteUp;
	AngleVectors(info->ang, &localForward, &localRight, &localUp);
	AngleVectors(info->linkedAng, &remoteForward, &remoteRight, &remoteUp);

	matrix3x4_t localToWorld(localForward, -localRight, localUp, info->pos);
	matrix3x4_t remoteToWorld(remoteForward, -remoteRight, remoteUp, info->linkedPos);
	UpdatePortalTransformationMatrix(localToWorld, remoteToWorld, matrix);
}

void calculateAGPosition(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	if (portal && portal->linkedHandle.IsValid())
		calculateAGOffsetPortal(portal, new_player_origin, new_player_angles);
}

void calculateSGPosition(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	const auto& player_origin = utils::GetPlayerEyePosition();
	const auto& player_angles = utils::GetPlayerEyeAngles();
	transformThroughPortal(portal, player_origin, player_angles, new_player_origin, new_player_angles);
}

std::wstring calculateWillAGSG(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	if (!portal)
		return L"no portal selected";

	Vector enterForward;
	AngleVectors(portal->ang, &enterForward);

	Vector pos;
	QAngle qa;
	calculateAGPosition(portal, pos, qa);

	if (enterForward.Dot(pos - portal->pos) >= 0)
		return L"no, tp position in front";
	else
		return L"yes";
}

void calculateAGOffsetPortal(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	Vector exitForward, exitRight, exitUp;
	Vector enterForward, enterRight, enterUp;
	AngleVectors(portal->ang, &enterForward, &enterRight, &enterUp);
	AngleVectors(portal->linkedAng, &exitForward, &exitRight, &exitUp);

	auto delta = portal->pos - portal->linkedPos;

	Vector exit_portal_coords;
	exit_portal_coords.x = delta.Dot(exitForward);
	exit_portal_coords.y = delta.Dot(exitRight);
	exit_portal_coords.z = delta.Dot(exitUp);

	Vector transition(0.f);
	transition -= exit_portal_coords.x * enterForward;
	transition -= exit_portal_coords.y * enterRight;
	transition += exit_portal_coords.z * enterUp;

	new_player_origin = portal->pos + transition;
	new_player_angles = utils::GetPlayerEyeAngles();
}

void transformThroughPortal(const utils::PortalInfo* portal,
                            const Vector& start_pos,
                            const QAngle start_angles,
                            Vector& transformed_origin,
                            QAngle& transformed_angles)
{
	Vector normal;
	if (portal)
		AngleVectors(portal->ang, &normal);

	if (!portal || normal.Dot(start_pos - portal->pos) >= 0)
	{
		// in front of portal
		transformed_origin = start_pos;
		transformed_angles = start_angles;
		return;
	}

	VMatrix matrix;
	if (portal->linkedHandle.IsValid())
	{
		UpdatePortalTransformationMatrix(portal, matrix);
	}
	else
	{
		// after fizzling the linked handle is invalid, so use the portal's linked matrix if we're on server
		static utils::CachedField<VMatrix, "CProp_Portal", "m_matrixThisToLinked", true> fMatrix;
		auto serverPortalEnt = utils::spt_serverEntList.GetEnt(portal->handle.GetEntryIndex());
		if (serverPortalEnt && fMatrix.Exists())
			matrix = *fMatrix.GetPtr(serverPortalEnt);
	}

	transformed_origin = matrix * start_pos;
	transformed_angles = TransformAnglesToWorldSpace(start_angles, matrix.As3x4());
	transformed_angles.x = AngleNormalizePositive(transformed_angles.x);
	transformed_angles.y = AngleNormalizePositive(transformed_angles.y);
	transformed_angles.z = AngleNormalizePositive(transformed_angles.z);
}

const utils::PortalInfo* getPortal(const char* arg, int getPortalFlags)
{
	if (!arg || !utils::DoesGameLookLikePortal())
		return nullptr;

	if (!stricmp(arg, "overlay"))
	{
		arg = _y_spt_overlay_portal.GetString();
		// copy same portal as used for overlay, meaning that 'auto' & 'env' are allowed
		getPortalFlags |= GPF_ALLOW_AUTO | GPF_ALLOW_PLAYER_ENV;
	}

	enum PortalSelector
	{
		PS_ORANGE,
		PS_BLUE,
		PS_AUTO,
		_PS_FIRST_ONLY_OPEN,
		PS_ORANGE_ONLY_OPEN = _PS_FIRST_ONLY_OPEN,
		PS_BLUE_ONLY_OPEN,
		PS_AUTO_ONLY_OPEN,

		_PS_N_STATEFUL_SELECTORS,

		PS_ENV,
		PS_INDEX,
	};

	static std::array<utils::PortalInfo, _PS_N_STATEFUL_SELECTORS> lastUsedLookup{};

	PortalSelector selector;
	if (!stricmp(arg, "blue"))
		selector = (getPortalFlags & GPF_ONLY_OPEN_PORTALS) ? PS_BLUE_ONLY_OPEN : PS_BLUE;
	else if (!stricmp(arg, "orange"))
		selector = (getPortalFlags & GPF_ONLY_OPEN_PORTALS) ? PS_ORANGE_ONLY_OPEN : PS_ORANGE;
	else if ((getPortalFlags & GPF_ALLOW_AUTO) && !stricmp(arg, "auto"))
		selector = (getPortalFlags & GPF_ONLY_OPEN_PORTALS) ? PS_AUTO_ONLY_OPEN : PS_AUTO;
	else if (!stricmp(arg, "env"))
		selector = PS_ENV;
	else
		selector = PS_INDEX;

	bool wantBlue = true;
	bool wantOrange = true;

	// check if a portal matches the selector string & flags
	const auto portalIsMatch = [&wantBlue, &wantOrange, getPortalFlags](const utils::PortalInfo* p)
	{
		return p && (!(getPortalFlags & GPF_ONLY_OPEN_PORTALS) || p->isOpen)
		       && ((wantOrange && p->isOrange) || (wantBlue && !p->isOrange));
	};

	// index selector is easy
	if (selector == PS_INDEX)
	{
		char* end;
		int idx = (int)strtol(arg, &end, 10);
		if (end == arg)
			return nullptr;
		const utils::PortalInfo* p = utils::GetPortalAtIndex(idx);
		return portalIsMatch(p) ? p : nullptr;
	}

	// 'env' selector is easy
	if (selector == PS_ENV)
	{
		const utils::PortalInfo* p = utils::GetEnvironmentPortal();
		return portalIsMatch(p) ? p : nullptr;
	}

	// looking for blue/orange/auto

	wantBlue = selector != PS_ORANGE && selector != PS_ORANGE_ONLY_OPEN;
	wantOrange = selector != PS_BLUE && selector != PS_BLUE_ONLY_OPEN;

	utils::PortalInfo& lastUsed = lastUsedLookup[selector];

	// overwrite with player's environment, a bit weird but nice for most features
	{
		const utils::PortalInfo* p = utils::GetEnvironmentPortal();
		if (portalIsMatch(p))
		{
			lastUsed = *p;
			return p;
		}
	}

	// use same index if possible
	if (lastUsed.handle.IsValid())
	{
		const utils::PortalInfo* p = utils::GetPortalAtIndex(lastUsed.handle.GetEntryIndex());
		if (portalIsMatch(p))
		{
			lastUsed = *p;
			return p;
		}
	}

	/*
	* The player's environment and the previously used index do not match this selector. Look
	* through all portals and find one which matches. Prioritize:
	* - portals which have the same pos/ang/color as the previously used portal since the index
	*   of portals may change across saveloads
	* - activated portals
	* - open portals
	*/
	const utils::PortalInfo* bestMatch = nullptr;
	for (auto& p : utils::GetPortalList())
	{
		if (!portalIsMatch(&p))
			continue;
		if (!bestMatch || (!bestMatch->isActivated && p.isActivated) || (!bestMatch->isOpen && p.isOpen))
			bestMatch = &p;

		if (lastUsed.handle.IsValid() && lastUsed.isOrange == p.isOrange
		    && VectorsAreEqual(lastUsed.pos, p.pos, 1) && QAnglesAreEqual(lastUsed.ang, p.ang, 1))
		{
			bestMatch = &p;
			break;
		}
	}

	if (bestMatch)
	{
		lastUsed = *bestMatch;
		return bestMatch;
	}
	else
	{
		lastUsed.Invalidate();
		return nullptr;
	}
}

#endif
