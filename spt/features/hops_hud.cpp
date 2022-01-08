#include "stdafx.h"
#if defined(SSDK2007)
#include "..\feature.hpp"
#include <algorithm>
#include "convar.hpp"
#include "hud.hpp"
#include "signals.hpp"
#include "playerio.hpp"
#include "property_getter.hpp"

#undef min
#undef max

ConVar y_spt_hud_hops("y_spt_hud_hops", "0", FCVAR_CHEAT, "When set to 1, displays the hop practice HUD.");
ConVar y_spt_hud_hops_x("y_spt_hud_hops_x", "-85", FCVAR_CHEAT, "Hops HUD x offset");
ConVar y_spt_hud_hops_y("y_spt_hud_hops_y", "100", FCVAR_CHEAT, "Hops HUD y offset");

// Hops HUD
class HopsHud : public FeatureWrapper<HopsHud>
{
public:
	void OnGround(bool onground);
	void Jump();
	void CalculateAbhVel();
	void DrawHopHud();

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	int sinceLanded = 0;
	bool velNotCalced = true;
	int lastHop = 0;
	int displayHop = 0;
	float percentage = 0;
	float maxVel = 0;
	float loss = 0;
	vgui::HFont hopsFont = 0;
};

static HopsHud spt_hops_hud;

bool HopsHud::ShouldLoadFeature()
{
	return spt_hud.ShouldLoadFeature();
}

void HopsHud::InitHooks() {}

void HopsHud::LoadFeature()
{
	OngroundSignal.Connect(this, &HopsHud::OnGround);
	JumpSignal.Connect(this, &HopsHud::Jump);

	bool result = AddHudCallback("hops", std::bind(&HopsHud::DrawHopHud, this), y_spt_hud_hops);

	if (result)
	{
		InitConcommandBase(y_spt_hud_hops_x);
		InitConcommandBase(y_spt_hud_hops_y);
	}
}

void HopsHud::UnloadFeature() {}

void HopsHud::DrawHopHud()
{
	if (hopsFont == 0)
	{
		if (!spt_hud.scheme)
			return;
		hopsFont = spt_hud.scheme->GetFont("Trebuchet24", false);
	}

	auto surface = spt_hud.surface;
	auto screen = spt_hud.screen;

	surface->DrawSetTextFont(hopsFont);
	surface->DrawSetTextColor(255, 255, 255, 255);
	surface->DrawSetTexture(0);
	int fontTall = surface->GetFontTall(hopsFont);

	const int MARGIN = 2;
	const int BUFFER_SIZE = 256;
	wchar_t buffer[BUFFER_SIZE];

	swprintf_s(buffer, BUFFER_SIZE, L"Timing: %d", displayHop);
	surface->DrawSetTextPos(screen->width / 2 + y_spt_hud_hops_x.GetFloat(),
	                        screen->height / 2 + y_spt_hud_hops_y.GetFloat());
	surface->DrawPrintText(buffer, wcslen(buffer));

	swprintf_s(buffer, BUFFER_SIZE, L"Speed loss: %.3f", loss);
	surface->DrawSetTextPos(screen->width / 2 + y_spt_hud_hops_x.GetFloat(),
	                        screen->height / 2 + y_spt_hud_hops_y.GetFloat() + (fontTall + MARGIN));
	surface->DrawPrintText(buffer, wcslen(buffer));

	swprintf_s(buffer, BUFFER_SIZE, L"Percentage: %.3f", percentage);
	surface->DrawSetTextPos(screen->width / 2 + y_spt_hud_hops_x.GetFloat(),
	                        screen->height / 2 + y_spt_hud_hops_y.GetFloat() + (fontTall + MARGIN) * 2);
	surface->DrawPrintText(buffer, wcslen(buffer));
}

void HopsHud::Jump()
{
	if (!y_spt_hud_hops.GetBool())
		return;

	if (sinceLanded == 0)
		CalculateAbhVel();

	velNotCalced = true;
	lastHop = sinceLanded;
}

void HopsHud::OnGround(bool onground)
{
	if (!y_spt_hud_hops.GetBool())
		return;

	if (!onground)
	{
		sinceLanded = 0;

		if (velNotCalced)
		{
			velNotCalced = false;

			// Don't count very delayed hops
			if (lastHop > 15)
				return;

			auto vel = spt_playerio.GetPlayerVelocity().Length2D();
			loss = maxVel - vel;
			percentage = (vel / maxVel) * 100;
			displayHop = lastHop - 1;
			displayHop = std::max(0, displayHop);
		}
	}
	else
	{
		if (sinceLanded == 0)
		{
			CalculateAbhVel();
		}
		++sinceLanded;
	}
}

void HopsHud::CalculateAbhVel()
{
	auto vel = spt_playerio.GetPlayerVelocity().Length2D();
	auto ducked = spt_playerio.m_fFlags.GetValue() & FL_DUCKING;
	auto sprinting = utils::GetProperty<bool>(0, "m_fIsSprinting");
	auto vars = spt_playerio.GetMovementVars();

	float modifier;

	if (ducked)
		modifier = 0.1;
	else if (sprinting)
		modifier = 0.5;
	else
		modifier = 1;

	float jspeed = vars.Maxspeed + (vars.Maxspeed * modifier);

	maxVel = vel + (vel - jspeed);
	maxVel = std::max(maxVel, jspeed);
}
#endif