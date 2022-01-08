#pragma once

#if defined(SSDK2007)

#include <functional>
#include <vector>
#include "..\feature.hpp"
#include "vgui\VGUI.h"
#include "vgui\IScheme.h"
#include "VGuiMatSurface\IMatSystemSurface.h"

typedef void(__fastcall* _StartDrawing)(void* thisptr, int edx);
typedef void(__fastcall* _FinishDrawing)(void* thisptr, int edx);
typedef void(__fastcall* _VGui_Paint)(void* thisptr, int edx, int mode);

enum class RenderTime
{
	overlay,
	paint
};

struct HudCallback
{
	HudCallback(std::string key, std::function<void()> draw, std::function<bool()> shouldDraw, bool overlay);

	RenderTime renderTime;
	std::string sortKey;
	std::function<void()> draw;
	std::function<bool()> shouldDraw;
};

// HUD stuff
class HUDFeature : public FeatureWrapper<HUDFeature>
{
public:
	bool AddHudCallback(HudCallback callback);
	void DrawTopHudElement(const wchar* format, ...);
	virtual bool ShouldLoadFeature() override;

	IMatSystemSurface* surface = nullptr;
	vgui::IScheme* scheme = nullptr;
	vgui::HFont font = 0;
	vrect_t* screen = nullptr;

protected:
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void PreHook() override;
	virtual void UnloadFeature() override;

private:
	void DrawHUD();

	static void __fastcall HOOKED_VGui_Paint(void* thisptr, int edx, int mode);

	bool callbacksSorted = true;
	std::vector<HudCallback> hudCallbacks;

	_VGui_Paint ORIG_VGui_Paint = nullptr;
	_StartDrawing ORIG_StartDrawing = nullptr;
	_FinishDrawing ORIG_FinishDrawing = nullptr;

	int topX = 0;
	int topVertIndex = 0;

	ConVar* cl_showpos = nullptr;
	ConVar* cl_showfps = nullptr;
	int topFontTall = 0;
	bool loadingSuccessful = false;
};

extern HUDFeature spt_hud;

#endif