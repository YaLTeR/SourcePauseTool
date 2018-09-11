#pragma once


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
	OverlayRenderer() : overlayOn(false), currentCallback(nullptr) {}
	void setupOverlay(_CameraCallback callback);
	void disableOverlay();
	void renderOverlay(void * CVRenderView, void * pRenderTarget);
private:
	bool overlayOn;
	_CameraCallback currentCallback;
};

extern OverlayRenderer g_OverlayRenderer;