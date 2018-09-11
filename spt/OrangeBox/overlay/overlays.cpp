#include "stdafx.h"
#include "overlays.hpp"
#include "..\modules\ClientDLL.hpp"
#include "..\modules.hpp"

CameraInformation dummyOverlay()
{
	auto pos = clientDLL.GetPlayerEyePos();
	CameraInformation info;
	info.x = pos.x;
	info.y = pos.y;
	info.z = pos.z;
	info.pitch = 0;
	info.yaw = 0;
	return info;
}
