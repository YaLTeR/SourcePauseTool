#include "stdafx.hpp"

#include "hud.hpp"

#ifdef SPT_HUD_ENABLED

#include "..\feature.hpp"
#include "convar.hpp"
#include "interfaces.hpp"
#include "playerio.hpp"
#include "signals.hpp"
#include "game_detection.hpp"

ConVar y_spt_hud_edgebug("y_spt_hud_edgebug",
                         "0",
                         0,
                         "Draws text in the middle of the screen when you get an edgeboog");
ConVar y_spt_hud_edgebug_sec("y_spt_hud_edgebug_sec", "1", 0, "Duration of the boog text in seconds");

// Boog detection
class BoogFeature : public FeatureWrapper<BoogFeature>
{
public:
	void BoogTick();
	bool ShouldDrawBoog();
	void DrawBoog();
	int ticksLeftToDrawBoog = 0;
	bool previousTickFalling = false;
	vgui::HFont boogFont = 0;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static BoogFeature spt_boog;

void BoogFeature::BoogTick()
{
	if (ticksLeftToDrawBoog > 0)
	{
		ticksLeftToDrawBoog -= 1;
	}

	bool ground = spt_playerio.m_fFlags.GetValue() & FL_ONGROUND;
	Vector vel = spt_playerio.m_vecAbsVelocity.GetValue();
	const float boog_vel = -4.5f;

	if (previousTickFalling)
	{
		if (!ground && vel.z == boog_vel)
		{
			ticksLeftToDrawBoog = (y_spt_hud_edgebug_sec.GetFloat()) / 0.015f;
		}
	}

	previousTickFalling = vel.z < boog_vel && !ground;
}

bool BoogFeature::ShouldDrawBoog()
{
	bool rval =
	    ticksLeftToDrawBoog > 0 && y_spt_hud_edgebug.GetBool() && interfaces::surface && spt_hud_feat.renderView;

	if (rval)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void BoogFeature::DrawBoog()
{
	auto surface = interfaces::surface;
	auto renderView = spt_hud_feat.renderView;

	if (boogFont == 0)
	{
		if (!surface)
		{
			return;
		}

		boogFont = surface->CreateFont();

		if (boogFont == 0)
		{
			return;
		}
		if (utils::GetBuildNumber() <= 3420)
		{
			typedef bool(__thiscall * SetFontGlyphSetOld_func)(void* thisptr,
			                                                   vgui::HFont font,
			                                                   const char* windowsFontName,
			                                                   int tall,
			                                                   int weight,
			                                                   int blur,
			                                                   int scanlines,
			                                                   int flags);
			const int vtidx_SetFontGlyphSet = 65;
			((*(SetFontGlyphSetOld_func**)(surface))[vtidx_SetFontGlyphSet])(surface,
			                                                                 boogFont,
			                                                                 "Trebuchet MS",
			                                                                 96,
			                                                                 0,
			                                                                 0,
			                                                                 0,
			                                                                 0x010);
		}
		else
		{
			surface->SetFontGlyphSet(boogFont, "Trebuchet MS", 96, 0, 0, 0, 0x010);
		}
	}

	surface->DrawSetTextFont(boogFont);
	surface->DrawSetTextColor(255, 255, 255, 255);
	surface->DrawSetTexture(0);
	int tall = 0, len = 0;
	surface->GetTextSize(boogFont, L"boog", len, tall);

	int x = renderView->width / 2 - len / 2;
	int y = renderView->height / 2 + 100;

	if (tall + y > renderView->height)
	{
		y = renderView->height - tall;
	}

	surface->DrawSetTextPos(x, y);
	surface->DrawPrintText(L"boog", wcslen(L"boog"));
}

bool BoogFeature::ShouldLoadFeature()
{
	return spt_hud_feat.ShouldLoadFeature() && spt_playerio.ShouldLoadFeature();
}

void BoogFeature::InitHooks() {}

static void SV_FrameWrapper(bool finalTick)
{
	spt_boog.BoogTick();
}

void BoogFeature::LoadFeature()
{
	bool boogHooked = false;

	// Prefer SV_Frame, also works in demos
	if (SV_FrameSignal.Works)
	{
		boogHooked = true;
		SV_FrameSignal.Connect(SV_FrameWrapper);
	}
	// Fall back on Tick signal, only works outside of demos
	else if (TickSignal.Works)
	{
		boogHooked = true;
		TickSignal.Connect(this, &BoogFeature::BoogTick);
	}

	if (boogHooked)
	{
		bool result = spt_hud_feat.AddHudDefaultGroup(HudCallback(std::bind(&BoogFeature::DrawBoog, this),
		                                                          std::bind(&BoogFeature::ShouldDrawBoog, this),
		                                                          false));
		if (result)
		{
			InitConcommandBase(y_spt_hud_edgebug);
			InitConcommandBase(y_spt_hud_edgebug_sec);
		}
	}
}

void BoogFeature::UnloadFeature() {}

#endif
