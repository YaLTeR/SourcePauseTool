#include "stdafx.h"

#ifndef OE
#include "overlays.hpp"
#include "..\modules\ClientDLL.hpp"
#include "..\modules.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\math.hpp"
#include "..\..\utils\ent_utils.hpp"
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
	info.yaw = NormalizeDeg(angles[YAW] + 180);
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
	info.yaw = NormalizeDeg(va[YAW]);

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
	info.yaw = NormalizeDeg(va[YAW]);

	return info;
}
#endif