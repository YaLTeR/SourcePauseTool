#include "stdafx.hpp"

#include "overlay.hpp"

#include "vguimatsurface\imatsystemsurface.h"
#include "convar.hpp"

#include "interfaces.hpp"

#include "spt\utils\portal_utils.hpp"
#include "spt\utils\signals.hpp"
#include "spt\utils\math.hpp"
#include "spt\utils\game_detection.hpp"

#include "playerio.hpp"
#include "hud.hpp"
#include "shadow.hpp"

#ifndef SPT_HUD_TEXTONLY

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
ConVar _y_spt_overlay_no_roll("_y_spt_overlay_no_roll", "1", FCVAR_CHEAT, "Set the roll of overlay camera roll to 0.");

#endif

Overlay spt_overlay;

namespace patterns
{
	PATTERNS(CViewRender__RenderView,
	         "3420",
	         "55 8B EC 83 E4 F8 81 EC 24 01 00 00",
	         "1910503",
	         "55 8B EC 81 EC 10 02 00 00 53 56 8B F1",
	         "7462488",
	         "55 8B EC 81 EC FC 01 00 00 53 56 57",
	         "BMS-Retail-Xen",
	         "55 8B EC 81 EC DC 02 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 8B 5D 08 56 8B F1");
	PATTERNS(CViewRender__RenderView_4044, "4044", "81 EC 98 00 00 00 53 55 56 57 6A 00 6A 00");
} // namespace patterns

void Overlay::InitHooks()
{
#ifndef OE
	HOOK_FUNCTION(client, CViewRender__RenderView);
#else
	HOOK_FUNCTION(client, CViewRender__RenderView_4044);
#endif
}

void Overlay::PreHook()
{
	if (!ORIG_CViewRender__RenderView)
		return;

#ifdef BMS
	QueueOverlayRenderView_Offset = 26;
#elif OE
	QueueOverlayRenderView_Offset = -1;
#else
	if (utils::GetBuildNumber() >= 5135)
		QueueOverlayRenderView_Offset = 26;
	else
		QueueOverlayRenderView_Offset = 24;
#endif

	if (QueueOverlayRenderView_Offset != -1)
		RenderViewPre_Signal.Works = true; // we use this as a "loading successful" flag
}

void Overlay::LoadFeature()
{
	if (!RenderViewPre_Signal.Works)
		return;

#ifndef SPT_HUD_TEXTONLY

	InitConcommandBase(_y_spt_overlay);
	InitConcommandBase(_y_spt_overlay_type);
	InitConcommandBase(_y_spt_overlay_portal);
	InitConcommandBase(_y_spt_overlay_width);
	InitConcommandBase(_y_spt_overlay_fov);
	InitConcommandBase(_y_spt_overlay_swap);
	InitConcommandBase(_y_spt_overlay_no_roll);

#ifdef SPT_HUD_ENABLED
	bool result = spt_hud_feat.AddHudDefaultGroup(HudCallback(
	    std::bind(&Overlay::DrawCrosshair, this), []() { return true; }, true));

	if (result)
	{
		InitConcommandBase(_y_spt_overlay_crosshair_size);
		InitConcommandBase(_y_spt_overlay_crosshair_thickness);
		InitConcommandBase(_y_spt_overlay_crosshair_color);
	}
#endif

#endif
}

IMPL_HOOK_THISCALL(Overlay,
                   void,
                   CViewRender__RenderView,
                   void*,
                   CViewSetup* cameraView,
                   int nClearFlags,
                   int whatToDraw)
{
#ifdef SPT_HUD_TEXTONLY

	spt_hud_feat.renderView = cameraView;
	spt_overlay.ORIG_CViewRender__RenderView(thisptr, cameraView, nClearFlags, whatToDraw);

#else

	/*
	* If the overlay is enabled, we'll trigger a recursive call to RenderView() via QueueOverlayRenderView(). You
	* could in theory use this to have several overlays, in which case you'd need to keep track of the overlay
	* depth to make everyone happy.
	*/

	static int callDepth = 0;
	callDepth++;

	// this is hardcoded to false for overlays, so we copy its value to be the same as for the main view
	static bool doBloomAndToneMapping;

	auto& ovr = spt_overlay;

	if (callDepth == 1)
	{
		doBloomAndToneMapping = cameraView->m_bDoBloomAndToneMapping;
	}
	else
	{
		ovr.renderingOverlay = true;
		cameraView->m_bDoBloomAndToneMapping = doBloomAndToneMapping;
	}
	RenderViewPre_Signal(thisptr, cameraView);

	if (_y_spt_overlay.GetBool())
	{
		if (!ovr.renderingOverlay)
		{
			// Queue a RenderView() call, this will make a copy of the params that we got called with. This
			// function is in the SDK but has different virtual offsets for 3420/5135 :/.
			ovr.CViewRender__QueueOverlayRenderView(thisptr, *cameraView, nClearFlags, whatToDraw);
		}
		ovr.ModifyView(cameraView);
		ovr.ModifyScreenFlags(nClearFlags, whatToDraw);
	}

	// it just so happens that we don't need to make a full copy of the view and can use pointers instead
	if (ovr.renderingOverlay)
		ovr.overlayView = cameraView;
	else
		ovr.mainView = cameraView;

	ovr.ORIG_CViewRender__RenderView(thisptr, cameraView, nClearFlags, whatToDraw);
	callDepth--;
	if (callDepth == 1)
		ovr.renderingOverlay = false;

#endif
}

IMPL_HOOK_THISCALL(Overlay, void, CViewRender__RenderView_4044, void*, CViewSetup* cameraView, bool drawViewmodel)
{
	spt_hud_feat.renderView = cameraView;
	spt_overlay.ORIG_CViewRender__RenderView_4044(thisptr, cameraView, drawViewmodel);
}

#ifndef SPT_HUD_TEXTONLY

void Overlay::CViewRender__QueueOverlayRenderView(void* thisptr,
                                                  const CViewSetup& renderView,
                                                  int nClearFlags,
                                                  int whatToDraw)
{
	Assert(QueueOverlayRenderView_Offset != -1);
	typedef void(__thiscall * _QueueOverlayRenderView)(void* thisptr, const CViewSetup&, int, int);
	((_QueueOverlayRenderView**)thisptr)[0][QueueOverlayRenderView_Offset](thisptr,
	                                                                       renderView,
	                                                                       nClearFlags,
	                                                                       whatToDraw);
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

	interfaces::surface->DrawSetColor(r, g, b, a);
	int x = spt_hud_feat.renderView->x + spt_hud_feat.renderView->width / 2;
	int y = spt_hud_feat.renderView->y + spt_hud_feat.renderView->height / 2;
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

void Overlay::ModifyView(CViewSetup* renderView)
{
	if (renderingOverlay)
	{
		// scale this view to be smol

		renderView->x = 0;
		renderView->y = 0;

		// game does some funny fov scaling, fixup so that overlay fov is scaled in the same way as the main view
		const float half_ang = M_PI_F / 360.f;
		float aspect = renderView->m_flAspectRatio;
		renderView->fov = atan(tan(_y_spt_overlay_fov.GetFloat() * half_ang) * .75f * aspect) / half_ang;

		renderView->width = _y_spt_overlay_width.GetFloat();
		renderView->height = static_cast<int>(renderView->width / renderView->m_flAspectRatio);
	}

	if (renderingOverlay != _y_spt_overlay_swap.GetBool())
	{
		// move/rotate this view

		switch (_y_spt_overlay_type.GetInt())
		{
#ifdef SPT_PORTAL_UTILS
		case 0: // saveglitch offset
			if (utils::DoesGameLookLikePortal())
				calculateSGPosition(renderView->origin, renderView->angles);
			break;
		case 1: // angle glitch tp pos
			if (utils::DoesGameLookLikePortal())
				calculateAGPosition(renderView->origin, renderView->angles);
			break;
#endif
		case 2: // rear view cam
			renderView->angles.y += 180;
			break;
		case 3: // havok pos
			spt_player_shadow.GetPlayerHavokPos(&renderView->origin, nullptr);
			renderView->origin += spt_playerio.m_vecViewOffset.GetValue();
			break;
		case 4: // don't transform when behind sg portal
			renderView->origin = utils::GetPlayerEyePosition();
			renderView->angles = utils::GetPlayerEyeAngles();
			break;
		default:
			break;
		}
		// normalize yaw
		renderView->angles.y = utils::NormalizeDeg(renderView->angles.y);

		if (_y_spt_overlay_no_roll.GetBool())
		{
			renderView->angles.z = 0;
		}
	}
}

void Overlay::ModifyScreenFlags(int& clearFlags, int& drawFlags)
{
	if (renderingOverlay)
	{
		drawFlags = RENDERVIEW_UNSPECIFIED;
		clearFlags |= VIEW_CLEAR_COLOR;
	}
	else if (_y_spt_overlay_swap.GetBool())
	{
		drawFlags &= ~RENDERVIEW_DRAWVIEWMODEL;
	}
}

#endif
