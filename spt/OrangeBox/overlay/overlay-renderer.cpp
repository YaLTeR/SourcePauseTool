#include "stdafx.h"

#include "overlay-renderer.hpp"
#include "ivrenderview.h"
#include "..\..\game\client\iviewrender.h"
#include "view_shared.h"
#include "..\modules\ClientDLL.hpp"
#include "..\modules\EngineDLL.hpp"
#include "..\modules.hpp"
#include <SDK\hl_movedata.h>
#include "..\cvars.hpp"
#include "overlays.hpp"

OverlayRenderer g_OverlayRenderer;

bool OverlayRenderer::shouldRenderOverlay()
{
	return _y_spt_overlay.GetBool();
}

void OverlayRenderer::modifyView(void * view, int & clearFlags, int & drawFlags)
{
	CViewSetup * casted = (CViewSetup *)view;

	CameraInformation data;

	switch (_y_spt_overlay_type.GetInt())
	{
	case 0:
		data = sgOverlay();
		break;
	case 1:
		data = sgOverlay();
		break;
	default:
		data = rearViewMirrorOverlay();
		break;
	}

	casted->origin = Vector(data.x, data.y, data.z);
	casted->angles = QAngle(data.pitch, data.yaw, 0);

	int width = _y_spt_overlay_width.GetFloat();
	int height = static_cast<int>((width / 16.0f) * 9.0f);

	casted->x = 0;
	casted->y = 0;
	casted->width = width;
	casted->height = height;
	casted->fov = _y_spt_overlay_fov.GetFloat() * 1.18f; // hack: fix to be accurate later
	casted->m_flAspectRatio = 16.0f / 9.0f;
	drawFlags = 0;
}

Rect_t OverlayRenderer::getRect()
{
	int width = _y_spt_overlay_width.GetFloat();
	int height = static_cast<int>((width / 16.0f) * 9.0f);

	Rect_t rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;

	return rect;
}
