#include "stdafx.h"

#include "convar.hpp"
#include "interfaces.hpp"
#include "spt\utils\signals.hpp"

ConVar _y_spt_overlay("_y_spt_overlay",
                      "0",
                      FCVAR_CHEAT,
                      "Enables the overlay camera in the top left of the screen.\n");
ConVar _y_spt_overlay_type("_y_spt_overlay_type",
                           "0",
                           FCVAR_CHEAT,
                           "Overlay type:\n"
                           "  0 = save glitch body\n"
                           "  1 = angle glitch simulation\n"
                           "  2 = rear view mirror\n"
                           "  3 = havok view mirror\n"
                           "  4 = no camera transform (even when behind SG portal)\n");
ConVar _y_spt_overlay_portal(
    "_y_spt_overlay_portal",
    "auto",
    FCVAR_CHEAT,
    "Chooses the portal for the overlay camera. Valid options are blue/orange/portal index. For the SG camera this is the portal you save glitch on, for angle glitch simulation this is the portal you enter.\n");
ConVar _y_spt_overlay_width("_y_spt_overlay_width",
                            "480",
                            FCVAR_CHEAT,
                            "Determines the width of the overlay. Height is determined automatically from width.\n",
                            true,
                            20.0f,
                            true,
                            3840.0f);
ConVar _y_spt_overlay_fov("_y_spt_overlay_fov",
                          "90",
                          FCVAR_CHEAT,
                          "Determines the FOV of the overlay.\n",
                          true,
                          5.0f,
                          true,
                          140.0f);
ConVar _y_spt_overlay_swap("_y_spt_overlay_swap", "0", FCVAR_CHEAT, "Swap alternate view and main screen?\n");
ConVar _y_spt_overlay_crosshair_size("_y_spt_overlay_crosshair_size", "10", FCVAR_CHEAT, "Overlay crosshair size.");
ConVar _y_spt_overlay_crosshair_thickness("_y_spt_overlay_crosshair_thickness",
                                          "1",
                                          FCVAR_CHEAT,
                                          "Overlay crosshair thickness.");
ConVar _y_spt_overlay_crosshair_color("_y_spt_overlay_crosshair_color",
                                      "0 255 0 255",
                                      FCVAR_CHEAT,
                                      "Overlay crosshair RGBA color.");

#if defined(SSDK2007)
#include "overlay.hpp"
#include "hud.hpp"
#include "..\overlay\overlay-renderer.hpp"

Overlay spt_overlay;

bool Overlay::ShouldLoadFeature()
{
	return true;
}

namespace patterns
{
	PATTERNS(
	    CViewRender__RenderView,
	    "5135",
	    "55 8B EC 83 E4 F8 81 EC 24 01 00 00 53 56 57 8B F9 8D 8F 20 01 00 00 89 7C 24 24 E8",
	    "3420",
	    "55 8B EC 83 E4 F8 81 EC 24 01 00 00 53 8B 5D 08 56 57 8B F9 8D 8F 94 00 00 00 53 89 7C 24 28 89 4C 24 34 E8");
	PATTERNS(
	    CViewRender__Render,
	    "5135",
	    "81 EC 98 00 00 00 53 56 57 6A 04 6A 00 68 ?? ?? ?? ?? 6A 00 8B F1 8B ?? ?? ?? ?? ?? 68 ?? ?? ?? ?? FF ?? ?? ?? ?? ?? 8B BC 24 A8 00 00 00 8B 4F 04");
	PATTERNS(GetScreenAspect, "5135", "83 EC 0C A1 ?? ?? ?? ?? F3");
} // namespace patterns

void Overlay::InitHooks()
{
	HOOK_FUNCTION(client, CViewRender__Render);
	HOOK_FUNCTION(client, CViewRender__RenderView);
	FIND_PATTERN(engine, GetScreenAspect);
}

void Overlay::PreHook()
{
	if (ORIG_CViewRender__Render)
		RenderSignal.Works = true;
}

void Overlay::LoadFeature()
{
	if (ORIG_CViewRender__RenderView != nullptr && ORIG_CViewRender__Render != nullptr)
	{
		InitConcommandBase(_y_spt_overlay);
		InitConcommandBase(_y_spt_overlay_type);
		InitConcommandBase(_y_spt_overlay_portal);
		InitConcommandBase(_y_spt_overlay_width);
		InitConcommandBase(_y_spt_overlay_fov);
		InitConcommandBase(_y_spt_overlay_swap);

		bool result = spt_hud.AddHudCallback(HudCallback(
		    "overlay", std::bind(&Overlay::DrawCrosshair, this), []() { return true; }, true));

		if (result)
		{
			InitConcommandBase(_y_spt_overlay_crosshair_size);
			InitConcommandBase(_y_spt_overlay_crosshair_thickness);
			InitConcommandBase(_y_spt_overlay_crosshair_color);
		}
	}
}

void Overlay::UnloadFeature() {}

float Overlay::GetScreenAspectRatio()
{
	// The VEngineClient013 interface isn't compatible between 3420 and 5135,
	// so we hook this function instead of using the SDK
	// TODO: implement a custom interface to be used with the IVEngineClientWrapper and/or move to spt_generic if more features need this
	if (spt_overlay.ORIG_GetScreenAspect)
		return spt_overlay.ORIG_GetScreenAspect();
	return 16.0f / 9.0f; // assume 16:9 as a default
}

void Overlay::DrawCrosshair()
{
	static std::string color = "";
	static int r = 0, g = 0, b = 0, a = 0;

	if (strcmp(color.c_str(), _y_spt_overlay_crosshair_color.GetString()) != 0)
	{
		color = _y_spt_overlay_crosshair_color.GetString();
		sscanf(color.c_str(), "%d %d %d %d", &r, &g, &b, &a);
	}

	vrect_t* screen = (vrect_t*)screenRect;
	interfaces::surface->DrawSetColor(r, g, b, a);
	int x = screen->x + screen->width / 2;
	int y = screen->y + screen->height / 2;
	int width = _y_spt_overlay_crosshair_size.GetInt();
	int thickness = _y_spt_overlay_crosshair_thickness.GetInt();

	if (thickness > width)
		std::swap(thickness, width);

	interfaces::surface->DrawFilledRect(x - thickness / 2, y - width / 2, x + thickness / 2 + 1, y + width / 2 + 1);
	interfaces::surface->DrawFilledRect(x - width / 2, y - thickness / 2, x - thickness / 2, y + thickness / 2 + 1);
	interfaces::surface->DrawFilledRect(x + thickness / 2 + 1,
	                                    y - thickness / 2,
	                                    x + width / 2 + 1,
	                                    y + thickness / 2 + 1);
}

void __fastcall Overlay::HOOKED_CViewRender__RenderView(void* thisptr,
                                                        int edx,
                                                        void* cameraView,
                                                        int nClearFlags,
                                                        int whatToDraw)
{
	if (spt_overlay.ORIG_CViewRender__Render == nullptr)
	{
		spt_overlay.ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
	}
	else
	{
		if (g_OverlayRenderer.shouldRenderOverlay())
		{
			g_OverlayRenderer.modifyView(static_cast<CViewSetup*>(cameraView),
			                             spt_overlay.renderingOverlay);
			if (spt_overlay.renderingOverlay)
			{
				g_OverlayRenderer.modifySmallScreenFlags(nClearFlags, whatToDraw);
			}
			else
			{
				g_OverlayRenderer.modifyBigScreenFlags(nClearFlags, whatToDraw);
			}
		}

		spt_overlay.ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
	}
}

void __fastcall Overlay::HOOKED_CViewRender__Render(void* thisptr, int edx, vrect_t* rect)
{
	RenderSignal(thisptr, rect);
	if (spt_overlay.ORIG_CViewRender__RenderView == nullptr)
	{
		spt_overlay.ORIG_CViewRender__Render(thisptr, edx, rect);
	}
	else
	{
		spt_overlay.renderingOverlay = false;
		spt_overlay.screenRect = rect;
		if (!g_OverlayRenderer.shouldRenderOverlay())
		{
			spt_overlay.ORIG_CViewRender__Render(thisptr, edx, rect);
		}
		else
		{
			spt_overlay.ORIG_CViewRender__Render(thisptr, edx, rect);

			spt_overlay.renderingOverlay = true;
			Rect_t rec = g_OverlayRenderer.getRect();
			spt_overlay.screenRect = &rec;
			spt_overlay.ORIG_CViewRender__Render(thisptr, edx, &rec);
			spt_overlay.renderingOverlay = false;
		}
	}
}

#endif
