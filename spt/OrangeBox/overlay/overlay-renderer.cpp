#include "stdafx.h"

#include "overlay-renderer.hpp"
#include "ivrenderview.h"
#include "..\..\game\client\iviewrender.h"
#include "view_shared.h"
#include "..\modules\ClientDLL.hpp"
#include "..\modules\EngineDLL.hpp"
#include "..\modules.hpp"
#include <SDK\hl_movedata.h>

const int VIEW_MONITOR = 2;
OverlayRenderer g_OverlayRenderer;

void OverlayRenderer::setupOverlay(_CameraCallback callback)
{
	overlayOn = true;
	currentCallback = callback;
}

void OverlayRenderer::disableOverlay()
{
	overlayOn = false;
}

// Called by the client DLL
void OverlayRenderer::renderOverlay(void * CVRenderView, void * pRenderTarget)
{
	if (overlayOn)
	{
		CViewSetup setup;
		auto data = currentCallback();
		setup.origin = Vector(data.x, data.y, data.z);
		setup.angles = QAngle(data.pitch, data.yaw, 0);

		setup.x = 20;
		setup.y = 20;
		setup.width = 320;
		setup.height = 180;
		setup.fov = 90;
		setup.m_bOrtho = false;
		setup.m_flAspectRatio = 16.0f / 9.0f;


		Frustum frustum;
		engineDLL.CVRenderView__Push3DView_Func(CVRenderView, (void*)&setup, 34, pRenderTarget, (void*)&frustum);
		//clientDLL.ViewDrawScene(false, true, (void*)&setup, VIEW_MONITOR);
		engineDLL.CVRenderView__PopView_Func(CVRenderView, (void*)&frustum);
	}
}
