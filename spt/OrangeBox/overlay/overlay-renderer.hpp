#pragma once
#ifdef SSDK2007
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
	bool shouldFlipScreens();
	void modifyBigScreenFlags(int& clearFlags, int& drawFlags);
	void modifySmallScreenFlags(int& clearFlags, int& drawFlags);
	void modifyView(void* view, bool overlay);
	Rect_t getRect();
private:
	float getFOV();
};

extern OverlayRenderer g_OverlayRenderer;
#endif