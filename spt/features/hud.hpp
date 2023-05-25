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
#include "Color.h"

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
	HudCallback();
	HudCallback(std::function<void(std::string)> draw, std::function<bool()> shouldDraw, bool overlay);

	bool drawInOverlay;
	std::function<void(std::string)> draw;
	std::function<bool()> shouldDraw;
};

struct HudUserGroup
{
	HudUserGroup();
	std::string ToString() const;

	float x, y;
	bool shouldDraw;
	vgui::HFont font;
	Color textcolor;
	Color background;
	struct GruopCallback
	{
		std::string name;
		std::string args;
	};
	std::vector<GruopCallback> callbacks;
};

// HUD stuff
class HUDFeature : public FeatureWrapper<HUDFeature>
{
public:
	const CViewSetup* renderView = nullptr;
	std::unordered_map<std::string, vgui::HFont> fonts;

	std::unordered_map<std::string, HudUserGroup> hudUserGroups;
	std::map<std::string, HudCallback> hudCallbacks;

	bool AddHudCallback(std::string key, HudCallback callback);
	bool AddHudDefaultGroup(HudCallback callback);

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
	std::vector<HudCallback> hudDefaultCallbacks;

	int topX = 0;
	int topY = 0;
	int topVertIndex = 0;
	int topFontTall = 0;

	int textWidth = 0;
	int textHeight = 0;
	bool drawText = false;

	Color hudTextColor;
	vgui::HFont font = 0;

	bool loadingSuccessful = false;

	ConVar* cl_showpos = nullptr;
	ConVar* cl_showfps = nullptr;

	DECL_HOOK_THISCALL(void, CEngineVGui__Paint, void*, PaintMode_t mode);
	DECL_MEMBER_THISCALL(void, CMatSystemSurface__StartDrawing, void*);
	DECL_MEMBER_THISCALL(void, CMatSystemSurface__FinishDrawing, void*);

#ifdef BMS
	DECL_MEMBER_THISCALL(void,
	                     ISurface__DrawPrintText,
	                     void*,
	                     const wchar_t* text,
	                     int textLen,
	                     vgui::FontDrawType_t drawType);

	DECL_MEMBER_THISCALL(void,
	                     ISurface__DrawPrintText_BMSLatest,
	                     void*,
	                     const wchar_t* text,
	                     int textLen,
	                     vgui::FontDrawType_t drawType,
	                     char unkBoolVal);

	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTextPos, void*, int x, int y);
	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTextFont, void*, vgui::HFont font);
	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTextColor, void*, int r, int g, int b, int a);
	DECL_MEMBER_THISCALL(void, ISurface__DrawSetTexture, void*, int id);
	DECL_MEMBER_THISCALL(int, ISurface__GetFontTall, void*, vgui::HFont font);
	DECL_MEMBER_THISCALL(void, ISurface__AddCustomFontFile, void*, const char* fontName, const char* fontFileName);

	std::vector<patterns::MatchedPattern> MATCHES_ISurface__DrawSetTextFont;
	std::vector<patterns::MatchedPattern> MATCHES_ISurface__GetFontTall;

	bool isLatest = false;
#endif

	void DrawHUD(bool overlay);
	void DrawDefaultHUD();
	void vDrawTopHudElement(Color color, const wchar* format, va_list args);
};

Color StringToColor(const std::string& color);
std::string ColorToString(Color color);

extern HUDFeature spt_hud_feat;
