#include "stdafx.h"

#ifndef OE
#include "..\sptlib-wrapper.hpp"
#include "ent_utils.hpp"
#include "math.hpp"
#include "..\features\shadow.hpp"
#include "..\features\playerio.hpp"
#include "overlays.hpp"
#include "portal_camera.hpp"

CameraInformation rearViewMirrorOverlay()
{
	CameraInformation info = noTransformOverlay();
	info.yaw = utils::NormalizeDeg(info.yaw + 180);
	return info;
}

CameraInformation havokViewMirrorOverlay()
{
	CameraInformation info;
	Vector havokpos;
	spt_player_shadow.GetPlayerHavokPos(&havokpos, nullptr);

	constexpr float duckOffset = 28;
	constexpr float standingOffset = 64;

	auto offset = spt_playerio.m_vecViewOffset.GetValue();
	auto angles = utils::GetPlayerEyeAngles();

	info.x = havokpos.x;
	info.y = havokpos.y;
	info.z = havokpos.z + offset.z;

	info.pitch = angles[PITCH];
	info.yaw = utils::NormalizeDeg(angles[YAW]);
	return info;
}

CameraInformation sgOverlay()
{
	CameraInformation info;
	Vector pos;
	QAngle va;

	calculateSGPosition(pos, va);
	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = va[PITCH];
	info.yaw = utils::NormalizeDeg(va[YAW]);

	return info;
}

CameraInformation agOverlay()
{
	CameraInformation info;
	Vector pos;
	QAngle va;

	calculateAGPosition(pos, va);
	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = va[PITCH];
	info.yaw = utils::NormalizeDeg(va[YAW]);

	return info;
}

CameraInformation noTransformOverlay()
{
	CameraInformation info;

	auto pos = utils::GetPlayerEyePosition();
	auto angles = utils::GetPlayerEyeAngles();
	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = angles[PITCH];
	info.yaw = utils::NormalizeDeg(angles[YAW]);
	return info;
}

#endif
