#include "stdafx.h"

#ifndef OE
#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\ent_utils.hpp"
#include "..\..\utils\math.hpp"
#include "..\modules.hpp"
#include "..\modules\ClientDLL.hpp"
#include "overlays.hpp"
#include "portal_camera.hpp"

CameraInformation rearViewMirrorOverlay()
{
	CameraInformation info;

	auto pos = utils::GetPlayerEyePosition();
	auto angles = utils::GetPlayerEyeAngles();

	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = angles[PITCH];
	info.yaw = utils::NormalizeDeg(angles[YAW] + 180);
	return info;
}

CameraInformation havokViewMirrorOverlay()
{
	CameraInformation info;
	Vector add(0, 0, 64);
	Vector* pos = &(vphysicsDLL.PlayerHavokPos + add);

	auto ducked = clientDLL.GetFlagsDucking();
	auto angles = utils::GetPlayerEyeAngles();

	info.x = pos->x;
	info.y = pos->y;

	if (ducked)
		info.z = pos->z - 36;
	else
		info.z = pos->z;

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
#endif