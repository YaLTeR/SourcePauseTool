#pragma once

#if !defined(OE) && !defined(BMS)
#define SPT_OVERLAY_ENABLED
#endif

#include "..\feature.hpp"

class C_BasePlayer;
enum SkyboxVisibility_t;
#include "networkvar.h"

#ifndef OE
#include "viewrender.h"
#else
#include "view_shared.h"
#endif

#include "spt\utils\ent_list.hpp"

// Overlay hook stuff, could combine with overlay renderer as well
class Overlay : public FeatureWrapper<Overlay>
{
public:
	bool renderingOverlay = false;
	// these point to the call stack, only valid while we're in RenderView()
	CViewSetup* mainView = nullptr;
	CViewSetup* overlayView = nullptr;

	DECL_HOOK_THISCALL(void,
	                   CViewRender__RenderView,
	                   void*,
	                   CViewSetup* cameraView,
	                   int nClearFlags,
	                   int whatToDraw);

	DECL_HOOK_THISCALL(void,
	                   CViewRender__RenderView_4044,
	                   void*,
	                   CViewSetup* cameraView,
	                   bool drawViewmodel);

	const utils::PortalInfo* GetOverlayPortal();

protected:
	virtual void InitHooks() override;

	virtual void PreHook() override;

	virtual void LoadFeature() override;

private:
	int QueueOverlayRenderView_Offset = -1;

	void CViewRender__QueueOverlayRenderView(void* thisptr,
	                                         const CViewSetup& renderView,
	                                         int nClearFlags,
	                                         int whatToDraw);
	void DrawCrosshair();
	void ModifyView(CViewSetup* renderView);
	void ModifyScreenFlags(int& clearFlags, int& drawFlags);
};

extern Overlay spt_overlay;
