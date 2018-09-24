#include "stdafx.h"

#ifdef SSDK2007

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
const float ASPECT_RATIO = 16.0f / 9.0f;
const int VIEW_CLEAR = 1;
const int VIEWMODEL_MASK = ~RENDERVIEW_DRAWVIEWMODEL;

bool OverlayRenderer::shouldRenderOverlay()
{
	return _y_spt_overlay.GetBool() && !engineDLL.Demo_IsPlayingBack();
}

bool OverlayRenderer::shouldFlipScreens()
{
	return _y_spt_overlay_flip.GetBool();
}

void OverlayRenderer::modifyBigScreenFlags(int & clearFlags, int & drawFlags)
{
	if (shouldFlipScreens())
	{
		drawFlags &= VIEWMODEL_MASK;
	}
}

void OverlayRenderer::modifySmallScreenFlags(int & clearFlags, int & drawFlags)
{
	drawFlags = 0;
	clearFlags |= VIEW_CLEAR;
}

void OverlayRenderer::modifyView(void * view, bool overlay)
{
	static CViewSetup backupView;
	CViewSetup * casted = (CViewSetup *)view;

	if (overlay == shouldFlipScreens())
	{
		if (overlay)
		{
			casted->origin = backupView.origin;
			casted->angles = backupView.angles;
			casted->fov = getFOV();
		}

		return;
	}
	else
	{
		backupView = *casted;
		CameraInformation data;

		switch (_y_spt_overlay_type.GetInt())
		{
		case 0:
			data = sgOverlay();
			break;
		case 1:
			data = agOverlay();
			break;
		default:
			data = rearViewMirrorOverlay();
			break;
		}

		backupView.origin = casted->origin;
		backupView.angles = casted->angles;

		casted->origin = Vector(data.x, data.y, data.z);
		casted->angles = QAngle(data.pitch, data.yaw, 0);
		casted->x = 0;
		casted->y = 0;

		if (!shouldFlipScreens())
		{
			int width = _y_spt_overlay_width.GetFloat();
			int height = static_cast<int>(width / ASPECT_RATIO);
			casted->width = width;
			casted->height = height;
		
		}

		casted->fov = getFOV();
		casted->m_flAspectRatio = ASPECT_RATIO;
	}

}

Rect_t OverlayRenderer::getRect()
{
	int width = _y_spt_overlay_width.GetFloat();
	int height = static_cast<int>(width / ASPECT_RATIO);

	Rect_t rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;

	return rect;
}

float OverlayRenderer::getFOV()
{
	return _y_spt_overlay_fov.GetFloat() * 1.18f; // hack: fix to be accurate later
}


#endif // ! OE