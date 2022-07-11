#include "stdafx.h"
#if defined(SSDK2007)
#include <algorithm>
#include "hud.hpp"
#include "convar.hpp"
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

namespace patterns
{
	namespace engine
	{
		PATTERNS(VGui_Paint, "5135", "E8 ?? ?? ?? ?? 8B 10 8B 52 34 8B C8 FF E2 CC CC");
	}

	namespace vguimatsurface
	{
		PATTERNS(
		    StartDrawing,
		    "5135",
		    "55 8B EC 83 E4 C0 83 EC 38 80 ?? ?? ?? ?? ?? ?? 56 57 8B F9 75 57 8B ?? ?? ?? ?? ?? C6 ?? ?? ?? ?? ?? ?? FF ?? 8B 10 6A 00 8B C8 8B 42 20");
		PATTERNS(
		    FinishDrawing,
		    "5135",
		    "56 6A 00 E8 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 8B 01 8B ?? ?? ?? ?? ?? 83 C4 04 FF D2 8B F0 85 F6 74 09 8B 06 8B 50 08 8B CE FF D2");
	} // namespace vguimatsurface
} // namespace patterns

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
	{
		return;
	}

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
	return true;
}

bool HUDFeature::GetFont(const std::string& fontName, vgui::HFont& fontOut)
{
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

void HUDFeature::InitHooks()
{
	FIND_PATTERN(vguimatsurface, StartDrawing);
	FIND_PATTERN(vguimatsurface, FinishDrawing);
	HOOK_FUNCTION(engine, VGui_Paint);
}

void HUDFeature::LoadFeature()
{
	if (loadingSuccessful)
	{
		InitConcommandBase(y_spt_hud_left);
		cl_showpos = interfaces::g_pCVar->FindVar("cl_showpos");
		cl_showfps = interfaces::g_pCVar->FindVar("cl_showfps");
	}
}

void HUDFeature::PreHook()
{
	loadingSuccessful = ORIG_FinishDrawing && ORIG_StartDrawing && ORIG_VGui_Paint;
}

void HUDFeature::UnloadFeature() {}

void HUDFeature::DrawHUD()
{
	vgui::HFont font;

	if (!interfaces::surface || !GetFont(FONT_DefaultFixedOutline, font))
		return;

	ORIG_StartDrawing(interfaces::surface, 0);

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
			topX = screen->width - 300 + 2;
			if (cl_showpos && cl_showpos->GetBool())
				topVertIndex += 3;
			if (cl_showfps && cl_showfps->GetBool())
				++topVertIndex;
		}

		for (auto& callback : hudCallbacks)
		{
			if (spt_overlay.renderingOverlay && callback.renderTime == RenderTime::overlay
			    && callback.shouldDraw())
			{
				callback.draw();
			}
			else if (!spt_overlay.renderingOverlay && callback.renderTime == RenderTime::paint
			         && callback.shouldDraw())
			{
				callback.draw();
			}
		}
	}
	catch (const std::exception& e)
	{
		Msg("Error drawing HUD: %s\n", e.what());
	}

	ORIG_FinishDrawing(interfaces::surface, 0);
}

void __fastcall HUDFeature::HOOKED_VGui_Paint(void* thisptr, int edx, int mode)
{
#ifndef OE
	if (spt_hud.loadingSuccessful && (mode == 2 || spt_overlay.renderingOverlay))
	{
		spt_hud.screen = (vrect_t*)spt_overlay.screenRect;
		spt_hud.DrawHUD();
	}

#endif

	spt_hud.ORIG_VGui_Paint(thisptr, edx, mode);
}

HudCallback::HudCallback(std::string key, std::function<void()> draw, std::function<bool()> shouldDraw, bool overlay)
{
	this->sortKey = key;
	this->draw = draw;
	this->shouldDraw = shouldDraw;
	this->renderTime = overlay ? RenderTime::overlay : RenderTime::paint;
}

#endif
