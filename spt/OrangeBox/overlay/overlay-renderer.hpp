#pragma once
#include "tier0\basetypes.h"


struct CameraInformation
{
	float x, y, z;
	float pitch, yaw;
};

typedef CameraInformation(*_CameraCallback) ();

class ITexture;

class OverlayRenderer
{
public:
	OverlayRenderer() {}
	bool shouldRenderOverlay();
	void modifyView(void * view, int & clearFlags, int &drawflags);
	Rect_t getRect();
};

extern OverlayRenderer g_OverlayRenderer;