#include "stdafx.h"

#ifdef SSDK2007
#include "..\spt-serverplugin.hpp"
#include "..\cvars.hpp"
#include "..\features\overlay.hpp"
#include "SDK\hl_movedata.h"
#include "overlay-renderer.hpp"
#include "overlays.hpp"

OverlayRenderer g_OverlayRenderer;
const int VIEW_CLEAR = 1;
const int VIEWMODEL_MASK = ~RENDERVIEW_DRAWVIEWMODEL;

bool OverlayRenderer::shouldRenderOverlay()
{
	return _y_spt_overlay.GetBool();
}

bool OverlayRenderer::shouldFlipScreens()
{
	return _y_spt_overlay_swap.GetBool();
}

void OverlayRenderer::modifyBigScreenFlags(int& clearFlags, int& drawFlags)
{
	if (shouldFlipScreens())
	{
		drawFlags &= VIEWMODEL_MASK;
	}
}

void OverlayRenderer::modifySmallScreenFlags(int& clearFlags, int& drawFlags)
{
	drawFlags = 0;
	clearFlags |= VIEW_CLEAR;
}

void OverlayRenderer::modifyView(CViewSetup* view, bool overlay)
{
	static CViewSetup backupView;

	if (overlay == shouldFlipScreens())
	{
		if (overlay)
		{
			view->origin = backupView.origin;
			view->angles = backupView.angles;
			view->fov = getFOV();
		}

		return;
	}
	else
	{
		backupView = *view;
		CameraInformation data;

		switch (_y_spt_overlay_type.GetInt())
		{
		case 0:
			data = sgOverlay();
			break;
		case 1:
			data = agOverlay();
			break;
		case 2:
			data = rearViewMirrorOverlay();
			break;
		case 3:
			data = havokViewMirrorOverlay();
			break;
		default:
			data = noTransformOverlay();
			break;
		}

		backupView.origin = view->origin;
		backupView.angles = view->angles;

		view->origin = Vector(data.x, data.y, data.z);
		view->angles = QAngle(data.pitch, data.yaw, 0);
		view->x = 0;
		view->y = 0;

		if (!shouldFlipScreens())
		{
			int width = _y_spt_overlay_width.GetFloat();
			int height = static_cast<int>(width / spt_overlay.GetScreenAspectRatio());
			view->width = width;
			view->height = height;
		}

		view->fov = getFOV();
		view->m_flAspectRatio = spt_overlay.GetScreenAspectRatio();
	}
}

Rect_t OverlayRenderer::getRect()
{
	int width = _y_spt_overlay_width.GetFloat();
	int height = static_cast<int>(width / spt_overlay.GetScreenAspectRatio());

	Rect_t rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;

	return rect;
}

float OverlayRenderer::getFOV()
{
	const float ratioRatio = spt_overlay.GetScreenAspectRatio() / (4.0f / 3.0f);
	float fovRad = DEG2RAD(_y_spt_overlay_fov.GetFloat());
	float fov = 2 * RAD2DEG(std::atan(std::tan(fovRad / 2) * ratioRatio));

	return fov;
}
#endif