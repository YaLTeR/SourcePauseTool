#include "stdafx.h"

#include "hud.hpp"

#ifdef SPT_HUD_ENABLED

#include <algorithm>
#include "overlay.hpp"
#include "convar.hpp"
#include "game_detection.hpp"
#include "interfaces.hpp"
#include "tier0\basetypes.h"
#include "overlay.hpp"
#include "..\vgui\vgui_utils.hpp"
#include "..\cvars.hpp"
#include "string_utils.hpp"

ConVar y_spt_hud_left("y_spt_hud_left", "0", FCVAR_CHEAT, "When set to 1, displays SPT HUD on the left.\n");

HUDFeature spt_hud;

const std::string FONT_DefaultFixedOutline = "DefaultFixedOutline";
const std::string FONT_Trebuchet20 = "Trebuchet20";
const std::string FONT_Trebuchet24 = "Trebuchet24";
const std::string FONT_HudNumbers = "HudNumbers";

namespace patterns
{
	PATTERNS(
	    CMatSystemSurface__StartDrawing,
	    "5135",
	    "55 8B EC 83 E4 C0 83 EC 38 80 ?? ?? ?? ?? ?? ?? 56 57 8B F9 75 57 8B ?? ?? ?? ?? ?? C6 ?? ?? ?? ?? ?? ?? FF ?? 8B 10 6A 00 8B C8 8B 42 20",
	    "7462488",
	    "55 8B EC 64 A1 ?? ?? ?? ?? 6A FF 68 ?? ?? ?? ?? 50 64 89 25 ?? ?? ?? ?? 83 EC 14");
	PATTERNS(
	    CMatSystemSurface__FinishDrawing,
	    "5135",
	    "56 6A 00 E8 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 8B 01 8B ?? ?? ?? ?? ?? 83 C4 04 FF D2 8B F0 85 F6 74 09 8B 06 8B 50 08 8B CE FF D2",
	    "7462488",
	    "55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 64 89 25 ?? ?? ?? ?? 51 56 6A 00");
	PATTERNS(CEngineVGui__Paint,
	         "5135",
	         "83 EC 18 56 6A 04 6A 00",
	         "7462488",
	         "55 8B EC 83 EC 2C 53 8B D9 8B 0D ?? ?? ?? ?? 56");
} // namespace patterns

void HUDFeature::InitHooks()
{
	FIND_PATTERN(vguimatsurface, CMatSystemSurface__StartDrawing);
	FIND_PATTERN(vguimatsurface, CMatSystemSurface__FinishDrawing);
	HOOK_FUNCTION(engine, CEngineVGui__Paint);
}

bool HUDFeature::AddHudCallback(HudCallback callback)
{
	if (!this->loadingSuccessful)
		return false;
	hudCallbacks.push_back(callback);
	callbacksSorted = false;
	return true;
}

void HUDFeature::DrawTopHudElement(const wchar* format, ...)
{
	vgui::HFont font;

	if (!GetFont(FONT_DefaultFixedOutline, font))
		return;

	va_list args;
	va_start(args, format);
	const wchar* text = FormatTempString(format, args);
	va_end(args);

	interfaces::surface->DrawSetTextFont(font);
	interfaces::surface->DrawSetTextColor(255, 255, 255, 255);
	interfaces::surface->DrawSetTexture(0);

	interfaces::surface->DrawSetTextPos(topX, 2 + (topFontTall + 2) * topVertIndex);
	interfaces::surface->DrawPrintText(text, wcslen(text));
	++topVertIndex;
}

bool HUDFeature::ShouldLoadFeature()
{
	return !utils::DoesGameLookLikeBMSMod();
}

bool HUDFeature::GetFont(const std::string& fontName, vgui::HFont& fontOut)
{
	if (!loadingSuccessful)
	{
		return false;
	}

	if (fonts.find(fontName) != fonts.end())
	{
		fontOut = fonts[fontName];
		return true;
	}
	else
	{
		auto scheme = vgui::GetScheme();

		if (scheme)
		{
			fontOut = scheme->GetFont(fontName.c_str());
			fonts[fontName] = fontOut;
			return true;
		}
		else
		{
			return false;
		}
	}
}

void HUDFeature::PreHook()
{
	loadingSuccessful =
	    ORIG_CMatSystemSurface__StartDrawing && ORIG_CMatSystemSurface__FinishDrawing && ORIG_CEngineVGui__Paint;
}

void HUDFeature::LoadFeature()
{
	if (!loadingSuccessful)
		return;
	cl_showpos = g_pCVar->FindVar("cl_showpos");
	cl_showfps = g_pCVar->FindVar("cl_showfps");
	InitConcommandBase(y_spt_hud_left);
}

void HUDFeature::UnloadFeature()
{
	cl_showpos = cl_showfps = nullptr;
}

void HUDFeature::DrawHUD(bool overlay)
{
	vgui::HFont font;

	if (!interfaces::surface || !GetFont(FONT_DefaultFixedOutline, font))
		return;

	ORIG_CMatSystemSurface__StartDrawing(interfaces::surface, 0);

	try
	{
		if (!callbacksSorted)
		{
			std::sort(hudCallbacks.begin(),
			          hudCallbacks.end(),
			          [](HudCallback& lhs, HudCallback& rhs) { return lhs.sortKey < rhs.sortKey; });
			callbacksSorted = true;
		}

		// Reset top HUD stuff
		topVertIndex = 0;
		topFontTall = interfaces::surface->GetFontTall(font);
		if (y_spt_hud_left.GetBool())
		{
			topX = 6;
		}
		else
		{
			topX = renderView->width - 300 + 2;
			if (cl_showpos && cl_showpos->GetBool())
				topVertIndex += 3;
			if (cl_showfps && cl_showfps->GetBool())
				++topVertIndex;
		}

		for (auto& callback : hudCallbacks)
		{
#ifdef SPT_OVERLAY_ENABLED
			if (overlay == callback.drawInOverlay && callback.shouldDraw())
				callback.draw();
#else
			if (!callback.drawInOverlay && callback.shouldDraw())
				callback.draw();
#endif
		}
	}
	catch (const std::exception& e)
	{
		Msg("Error drawing HUD: %s\n", e.what());
	}

	ORIG_CMatSystemSurface__FinishDrawing(interfaces::surface, 0);
}

HOOK_THISCALL(void, HUDFeature, CEngineVGui__Paint, PaintMode_t mode)
{
#ifdef SPT_OVERLAY_ENABLED
	if (spt_hud.loadingSuccessful && mode == PAINT_INGAMEPANELS)
	{
		/*
		* Getting the HUD & overlay to not fight turns out to be somewhat tricky. There might be a better way, but
		* the only approach that I've found so far is to draw the HUD for both the main view and overlay at the
		* same time, otherwise the HUD for the main view gets cleared. The old version did this from VGui_Paint(),
		* but the pattern for that isn't unique on steampipe so we use CEngineVGui::Paint() instead. By the time
		* this is called, we're rendering the main view (after the overlay) so spt_overlay has the pointers for
		* both views.
		*/
		Assert(spt_overlay.mainView);
		spt_hud.renderView = spt_overlay.mainView;
		spt_hud.DrawHUD(false);
		if (_y_spt_overlay.GetBool())
		{
			Assert(spt_overlay.overlayView);
			spt_hud.renderView = spt_overlay.overlayView;
			spt_hud.DrawHUD(true);
		}
	}
#endif
	spt_hud.ORIG_CEngineVGui__Paint(thisptr, edx, mode);
}

HudCallback::HudCallback(std::string key, std::function<void()> draw, std::function<bool()> shouldDraw, bool overlay)
{
	this->sortKey = key;
	this->draw = draw;
	this->shouldDraw = shouldDraw;
	this->drawInOverlay = overlay;
}

#endif
