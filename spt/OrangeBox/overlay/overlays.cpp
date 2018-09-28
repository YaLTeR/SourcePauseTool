#include "stdafx.h"

#ifdef SSDK2007
#include "overlays.hpp"
#include "..\modules\ClientDLL.hpp"
#include "..\modules.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include "..\..\strafestuff.hpp"
#include "portal_camera.hpp"

CameraInformation rearViewMirrorOverlay()
{
	auto pos = clientDLL.GetPlayerEyePos();
	float va[3];
	EngineGetViewAngles(va);
	CameraInformation info;
	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = va[PITCH];
	info.yaw = NormalizeDeg(va[YAW] + 180);
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