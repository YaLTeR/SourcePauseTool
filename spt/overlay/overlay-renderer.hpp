#pragma once
#ifndef OE
#include "tier0\basetypes.h"
#include "view_shared.h"

struct CameraInformation
{
	float x, y, z;
	float pitch, yaw;
};

typedef CameraInformation (*_CameraCallback)();

class ITexture;

class OverlayRendererInternal
{
public:
	OverlayRendererInternal() {}
	bool shouldRenderOverlay();
	bool shouldFlipScreens();
	void modifyBigScreenFlags(int& clearFlags, int& drawFlags);
	void modifySmallScreenFlags(int& clearFlags, int& drawFlags);
	void modifyView(CViewSetup* view, bool overlay);
	Rect_t getRect();

private:
	float getFOV();
};

extern OverlayRendererInternal g_OverlayRenderer;
#endif