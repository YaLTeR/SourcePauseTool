#pragma once


#define SPT_HUD_ENABLED

#include <functional>
#include <vector>
#include <unordered_map>
#include "..\feature.hpp"
#include "vgui\VGUI.h"
#include "vgui\IScheme.h"
#include "VGuiMatSurface\IMatSystemSurface.h"
#include "ienginevgui.h"

class C_BasePlayer;
enum SkyboxVisibility_t;

#include "networkvar.h"
#ifndef OE
#include "viewrender.h"
#else
#include "view_shared.h"
#endif

extern const std::string FONT_DefaultFixedOutline;
extern const std::string FONT_Trebuchet20;
extern const std::string FONT_Trebuchet24;
extern const std::string FONT_HudNumbers;

struct HudCallback
{
	HudCallback(std::string key, std::function<void()> draw, std::function<bool()> shouldDraw, bool overlay);

	bool drawInOverlay;
	std::string sortKey;
	std::function<void()> draw;
	std::function<bool()> shouldDraw;
};

// HUD stuff
class HUDFeature : public FeatureWrapper<HUDFeature>
{
public:
	const CViewSetup* renderView = nullptr;
	std::unordered_map<std::string, vgui::HFont> fonts;

	bool AddHudCallback(HudCallback callback);
	void DrawTopHudElement(const wchar* format, ...);
	void DrawColorTopHudElement(Color color, const wchar* format, ...);
	virtual bool ShouldLoadFeature() override;
	bool GetFont(const std::string& fontName, vgui::HFont& fontOut);

protected:
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	bool callbacksSorted = true;
	std::vector<HudCallback> hudCallbacks;

	int topX = 0;
	int topVertIndex = 0;

	int topFontTall = 0;
	bool loadingSuccessful = false;

	ConVar* cl_showpos = nullptr;
	ConVar* cl_showfps = nullptr;

	DECL_HOOK_THISCALL(void, CEngineVGui__Paint, PaintMode_t mode);
	DECL_MEMBER_THISCALL(void, CMatSystemSurface__StartDrawing);
	DECL_MEMBER_THISCALL(void, CMatSystemSurface__FinishDrawing);

#ifdef BMS
	DECL_MEMBER_THISCALL(void, ISurface__DrawPrintText, const wchar_t* text, int textLen, vgui::FontDrawType_t drawType);
	DECL_MEMBER_THISCALL(void, ISurface__DrawPrintText_BMSLatest, const wchar_t* text, int textLen, vgui::FontDrawType_t drawType, char unkBoolVal);
	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTextPos, int x, int y);
	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTextFont, vgui::HFont font);
	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTextColor, int r, int g, int b, int a);
	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTexture, int id);
	DECL_MEMBER_THISCALL(int, ISurface__GetFontTall, vgui::HFont font);
	DECL_MEMBER_THISCALL(void, ISurface__AddCustomFontFile, const char* fontName, const char* fontFileName);

	std::vector<patterns::MatchedPattern> drawSetTextFontMatches;
	std::vector<patterns::MatchedPattern> getFontTallMatches;

	bool isLatest = false;
#endif	

	void DrawHUD(bool overlay);
	void vDrawTopHudElement(Color color, const wchar* format, va_list args);
};

extern HUDFeature spt_hud;

