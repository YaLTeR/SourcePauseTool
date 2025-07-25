#include "stdafx.hpp"

#include "hud.hpp"
#include "visualizations/imgui/imgui_interface.hpp"
#include "spt/utils/ent_list.hpp"
#include "create_collide.hpp"

#include "vphysics_interface.h"

class ShadowPositionFeature : public FeatureWrapper<ShadowPositionFeature>
{
protected:
	void LoadFeature() override;

public:
	static bool SetShadowRoll(float roll, const char** errMsg);
	static void HudCallback();
	static void ImGuiCallback();

} inline spt_shadow_feat;

ConVar y_spt_hud_shadow_info("y_spt_hud_shadow_info",
                             "0",
                             FCVAR_DONTRECORD,
                             "Displays player shadow position and angles.");

CON_COMMAND_F(y_spt_set_shadow_roll, "Sets the player's physics shadow roll in degrees.", FCVAR_DONTRECORD)
{
	if (args.ArgC() < 2)
	{
		Warning("Must provide a roll in degrees.\n");
		return;
	}
	const char* errMsg = nullptr;
	if (!ShadowPositionFeature::SetShadowRoll(atof(args.Arg(1)), &errMsg))
		Warning("%s\n", errMsg);
}

void ShadowPositionFeature::LoadFeature()
{
#ifdef SPT_HUD_ENABLED
	bool hudEnabled = AddHudCallback("shadow_info", [](std::string) { HudCallback(); }, y_spt_hud_shadow_info);

	if (hudEnabled)
		SptImGui::RegisterHudCvarCheckbox(y_spt_hud_shadow_info);
#endif

	InitCommand(y_spt_set_shadow_roll);

	SptImGuiGroup::Cheats_Misc_PlayerShadow.RegisterUserCallback(ImGuiCallback);
}

bool ShadowPositionFeature::SetShadowRoll(float roll, const char** errMsg)
{
	IPhysicsObject* physObj = spt_collideToMesh.GetPhysObj(utils::spt_serverEntList.GetPlayer());
	if (!physObj)
	{
		*errMsg = "Player's shadow object does not exist";
		return false;
	}
	Vector pos;
	QAngle ang;
	physObj->GetPosition(&pos, &ang);
	ang.z = roll;
	physObj->SetPosition(pos, ang, true);
	return true;
}

void ShadowPositionFeature::HudCallback()
{
	IPhysicsObject* physObj = spt_collideToMesh.GetPhysObj(utils::spt_serverEntList.GetPlayer());
	if (physObj)
	{
		Vector pos;
		QAngle ang;
		physObj->GetPosition(&pos, &ang);
		spt_hud_feat.DrawTopHudElement(L"shadow pos (xyz): %.3f %.3f %.3f", pos.x, pos.y, pos.z);
		spt_hud_feat.DrawTopHudElement(L"shadow ang (pyr): %.3f %.3f %.3f", ang.x, ang.y, ang.z);
	}
	else
	{
		spt_hud_feat.DrawTopHudElement(L"shadow pos (xyz): Does not exist");
		spt_hud_feat.DrawTopHudElement(L"shadow ang (pyr): Does not exist");
	}
}

void ShadowPositionFeature::ImGuiCallback()
{
	static SptImGui::TimedToolTip errTip;
	static double df = 0.f;

	SptImGui::InputDouble("shadow roll in degrees", &df);
	ImGui::SameLine();
	if (ImGui::Button("set"))
	{
		if (ShadowPositionFeature::SetShadowRoll((float)df, &errTip.text))
			errTip.StopShowing();
		else
			errTip.StartShowing();
	}
	ImGui::SameLine();
	SptImGui::CmdHelpMarkerWithName(y_spt_set_shadow_roll_command);

	errTip.Show(SPT_IMGUI_WARN_COLOR_YELLOW, 3);

	SptImGui::CvarCheckbox(y_spt_hud_shadow_info, "##checkbox_hud");
}
