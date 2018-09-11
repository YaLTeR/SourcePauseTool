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
	void pushOverlay(void * CVRenderView, void * pRenderTarget);
	void pop();
	void newFrame() { shouldRender = true; }
private:
	bool shouldRender;
	bool overlayOn;
	_CameraCallback currentCallback;
	void * CVRenderView;
};

extern OverlayRenderer g_OverlayRenderer;