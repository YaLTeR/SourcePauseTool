#pragma once

#define SPT_HUD_ENABLED

#ifdef SPT_HUD_ENABLED

#include <functional>
#include <vector>
#include <unordered_map>
#include "..\feature.hpp"
#include "vgui\VGUI.h"
#include "vgui\IScheme.h"
#include "custom_interfaces\surface.hpp"
#include "ienginevgui.h"
#include "Color.h"

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
	struct Screen
	{
		Screen() {}
		Screen(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}

		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};
	Screen screen;

	std::unordered_map<std::string, vgui::HFont> fonts;

	std::unordered_map<std::string, HudUserGroup> hudUserGroups;
	std::map<std::string, HudCallback> hudCallbacks;

	bool AddHudCallback(std::string key, HudCallback callback);
	bool AddHudDefaultGroup(HudCallback callback);

	void DrawTopHudElement(const wchar* format, ...);
	void DrawColorTopHudElement(Color color, const wchar* format, ...);
	virtual bool ShouldLoadFeature() override;
	bool GetFont(const std::string& fontName, vgui::HFont& fontOut);

	// call from LoadFeature or later
	bool LoadingSuccessful() const
	{
		return loadingSuccessful;
	}

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

	void DrawHUD(bool overlay);
	void DrawDefaultHUD();
	void vDrawTopHudElement(Color color, const wchar* format, va_list args);
};

Color StringToColor(const std::string& color);
std::string ColorToString(Color color);

extern HUDFeature spt_hud_feat;

#endif
