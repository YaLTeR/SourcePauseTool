#include "stdafx.hpp"

#include "hud.hpp"

#ifdef SPT_HUD_ENABLED

#include <algorithm>
#include "overlay.hpp"
#include "convar.hpp"
#include "game_detection.hpp"
#include "interfaces.hpp"
#include "tier0\basetypes.h"
#include "..\vgui\vgui_utils.hpp"
#include "string_utils.hpp"

ConVar y_spt_hud_left("y_spt_hud_left", "0", FCVAR_CHEAT, "When set to 1, displays SPT HUD on the left.\n");

extern ConVar _y_spt_overlay;

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
		"55 8B EC 64 A1 ?? ?? ?? ?? 6A FF 68 ?? ?? ?? ?? 50 64 89 25 ?? ?? ?? ?? 83 EC 14",
		"BMS-Retail-0.9",
		"55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 83 EC 14 56 57 A1 ?? ?? ?? ?? 33 C5 50 8D 45 F4 64 A3 ?? ?? ?? ?? 8B F9 80 3D ?? ?? ?? ?? 00");
	PATTERNS(
		CMatSystemSurface__FinishDrawing,
		"5135",
		"56 6A 00 E8 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 8B 01 8B ?? ?? ?? ?? ?? 83 C4 04 FF D2 8B F0 85 F6 74 09 8B 06 8B 50 08 8B CE FF D2",
		"7462488",
		"55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 64 89 25 ?? ?? ?? ?? 51 56 6A 00",
		"BMS-Retail-0.9",
		"55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 51 56 A1 ?? ?? ?? ?? 33 C5 50 8D 45 ?? 64 A3 ?? ?? ?? ?? 6A 00");
	PATTERNS(
		CEngineVGui__Paint,
		"5135",
		"83 EC 18 56 6A 04 6A 00",
		"BMS-Retail-0.9",
		"55 8B EC 83 EC 18 53 8B D9 8B 0D ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 88 45 FE 84 C0",
		"7462488",
		"55 8B EC 83 EC 2C 53 8B D9 8B 0D ?? ?? ?? ?? 56",
		"4044",
		"6A FF 68 B8 CA 2C 20");

#ifdef BMS
	PATTERNS(
		ISurface__DrawPrintText, 
		"BMS-Retail-0.9", 
		"55 8B EC 83 EC ?? A1 ?? ?? ?? ?? 33 C5 89 45 ?? 83 7D ?? 00");	
	PATTERNS(
		ISurface__DrawSetTextPos, 
		"BMS-Retail-0.9", 
		"55 8b ec 8b 45 08 89 41 28 8b 45 0c 89 41 2c 5d c2 08 00");
	PATTERNS(
		ISurface__DrawSetTextFont, 
		"BMS-Retail-0.9", 
		"55 8b ec 8b 45 08 89 81 ?? ?? 00 00 5d c2 04 00");
	PATTERNS(
		ISurface__DrawSetTextColor, 
		"BMS-Retail-0.9", 
		"55 8B EC F3 0F 2A 45 ?? 53");
	PATTERNS(
		ISurface__DrawSetTexture, 
		"BMS-Retail-0.9", 
		"55 8B EC 56 8B F1 57 8B 7D ?? 3B BE ?? ?? ?? ??");
	PATTERNS(
		ISurface__AddCustomFontFile, 
		"BMS-Retail-0.9", 
		"55 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 81 EC 48 01 00 00");
	PATTERNS(
		ISurface__GetFontTall, 
		"BMS-Retail-0.9", 
		"55 8B EC FF 75 08 E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? 5D C2 04 00");

#endif

} // namespace patterns

void HUDFeature::InitHooks()
{
#ifndef OE
	FIND_PATTERN(vguimatsurface, CMatSystemSurface__StartDrawing);
	FIND_PATTERN(vguimatsurface, CMatSystemSurface__FinishDrawing);
#endif

#ifdef BMS
	isLatest = utils::DoesGameLookLikeBMSLatest();

	FIND_PATTERN_ALL(vguimatsurface, ISurface__DrawSetTextFont);
	FIND_PATTERN_ALL(vguimatsurface, ISurface__GetFontTall);

	FIND_PATTERN(vguimatsurface, ISurface__DrawPrintText);
	FIND_PATTERN(vguimatsurface, ISurface__DrawSetTextPos);
	FIND_PATTERN(vguimatsurface, ISurface__DrawSetTextColor);
	FIND_PATTERN(vguimatsurface, ISurface__DrawSetTexture);
	FIND_PATTERN(vguimatsurface, ISurface__AddCustomFontFile);
#endif

	HOOK_FUNCTION(engine, CEngineVGui__Paint);
}

#ifdef BMS
#define CALL(func_name, ...) ORIG_ISurface__##func_name(interfaces::surface, __VA_ARGS__); 
#else
#define CALL(func_name, ...) interfaces::surface->##func_name(__VA_ARGS__);
#endif

bool HUDFeature::AddHudCallback(HudCallback callback)
{
	if (!this->loadingSuccessful)
		return false;
	hudCallbacks.push_back(callback);
	callbacksSorted = false;
	return true;
}

void HUDFeature::vDrawTopHudElement(Color color, const wchar* format, va_list args)
{
	vgui::HFont font;

	if (!GetFont(FONT_DefaultFixedOutline, font))
		return;
	const wchar* text = FormatTempString(format, args);

	CALL(DrawSetTextFont, font);
	CALL(DrawSetTextColor, color.r(), color.g(), color.b(), color.a());
	CALL(DrawSetTexture, 0);
	CALL(DrawSetTextPos, topX, 2 + (topFontTall + 2) * topVertIndex);

#ifdef BMS
	if (isLatest)
	{
		CALL(DrawPrintText_BMSLatest, text, wcslen(text), vgui::FONT_DRAW_DEFAULT, 0x0);
	}
	else
#endif
	{
		CALL(DrawPrintText, text, wcslen(text), vgui::FONT_DRAW_DEFAULT);
	}

	++topVertIndex;
}

void HUDFeature::DrawTopHudElement(const wchar* format, ...)
{
	va_list args;
	va_start(args, format);
	vDrawTopHudElement({255, 255, 255, 255}, format, args);
	va_end(args);
}

void HUDFeature::DrawColorTopHudElement(Color color, const wchar* format, ...)
{
	va_list args;
	va_start(args, format);
	vDrawTopHudElement(color, format, args);
	va_end(args);
}

bool HUDFeature::ShouldLoadFeature()
{
	return !utils::DoesGameLookLikeBMSMod() && !utils::DoesGameLookLikeDMoMM();
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
#ifdef OE
	    ORIG_CEngineVGui__Paint && spt_overlay.ORIG_CViewRender__RenderView_4044;
#else
	    ORIG_CMatSystemSurface__FinishDrawing && ORIG_CMatSystemSurface__StartDrawing && ORIG_CEngineVGui__Paint
	    && spt_overlay.ORIG_CViewRender__RenderView;
#endif

	if (!loadingSuccessful)
		return;

#ifdef BMS

	if (!ORIG_ISurface__DrawPrintText || !ORIG_ISurface__DrawSetTextColor || !ORIG_ISurface__DrawSetTextPos
		|| !ORIG_ISurface__DrawSetTexture || !ORIG_ISurface__AddCustomFontFile || MATCHES_ISurface__DrawSetTextFont.empty()
		|| MATCHES_ISurface__GetFontTall.empty())
	{
		loadingSuccessful = false;
		return;
	}

	// soem functions are nearly indistiguishable from others in its class, so we'll make some assumptions about
	// their positions in the vftable:
	//		GetFontTall() below AddCustomFontFile()
	//		DrawSetTextFont() above DrawSetTextColor()
	auto vftableStart = *(uintptr_t*)interfaces::surface;
	for (int i = 0; i < 0x100; i++)
	{
		auto prevPtr = *(uintptr_t*)(vftableStart + (i - 1) * 4);
		auto curPtr = *(uintptr_t*)(vftableStart + i * 4);
		auto nextPtr = *(uintptr_t*)(vftableStart + (i + 1) * 4);

		if (curPtr == (uintptr_t)ORIG_ISurface__AddCustomFontFile)
		{
			for (auto candidate : MATCHES_ISurface__GetFontTall) 
				if (candidate.ptr == nextPtr)
					ORIG_ISurface__GetFontTall = (_ISurface__GetFontTall)(candidate.ptr);
		}

		if (nextPtr == (uintptr_t)ORIG_ISurface__DrawSetTextColor)
		{
			for (auto candidate : MATCHES_ISurface__DrawSetTextFont)
				// overloaded function, observation shows declaration order doesn't match actual order...
				if (candidate.ptr == curPtr || candidate.ptr == prevPtr)
					ORIG_ISurface__DrawSetTextFont = (_ISurface__DrawSetTextFont)(candidate.ptr);
		}
	}

	if (isLatest)
		ORIG_ISurface__DrawPrintText_BMSLatest =
			(_ISurface__DrawPrintText_BMSLatest)ORIG_ISurface__DrawPrintText;

	loadingSuccessful = ORIG_ISurface__GetFontTall && ORIG_ISurface__DrawSetTextFont;
#endif
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

#ifdef BMS
	MATCHES_ISurface__DrawSetTextFont.clear();
	MATCHES_ISurface__GetFontTall.clear();
#endif

}

void HUDFeature::DrawHUD(bool overlay)
{
	vgui::HFont font;

	if (!interfaces::surface || !renderView || !GetFont(FONT_DefaultFixedOutline, font))
		return;

#ifndef OE
	ORIG_CMatSystemSurface__StartDrawing(interfaces::surface);
#endif

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
		topFontTall = CALL(GetFontTall, font);
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

#ifndef OE
	ORIG_CMatSystemSurface__FinishDrawing(interfaces::surface);
#endif
}

IMPL_HOOK_THISCALL(HUDFeature, void, CEngineVGui__Paint, void*, PaintMode_t mode)
{
	if (spt_hud.loadingSuccessful && mode & PAINT_INGAMEPANELS)
	{
#ifdef SPT_OVERLAY_ENABLED
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
#else
		Assert(spt_hud.renderView);
		spt_hud.DrawHUD(false);
#endif
	}
	spt_hud.ORIG_CEngineVGui__Paint(thisptr, mode);
}

HudCallback::HudCallback(std::string key, std::function<void()> draw, std::function<bool()> shouldDraw, bool overlay)
{
	this->sortKey = key;
	this->draw = draw;
	this->shouldDraw = shouldDraw;
	this->drawInOverlay = overlay;
}

#endif
