#include "stdafx.hpp"

#include "shadow.hpp"
#include "..\utils\game_detection.hpp"
#include "hud.hpp"
#include "ent_utils.hpp"
#include "ent_props.hpp"
#include "playerio.hpp"
#include "visualizations/imgui/imgui_interface.hpp"

#include "vphysics_interface.h"

ConVar y_spt_hud_shadow_info("y_spt_hud_shadow_info",
                             "0",
                             FCVAR_CHEAT,
                             "Displays player shadow position and angles.\n");

CON_COMMAND_F(y_spt_set_shadow_roll, "Sets the player's physics shadow roll in degrees.\n", FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
	{
		Warning("Must provide a roll in degrees.\n");
		return;
	}
	spt_player_shadow.SetPlayerHavokRoll(atof(args.Arg(1)));
}

ShadowPosition spt_player_shadow;

namespace patterns
{
	PATTERNS(
	    GetShadowPosition,
	    "5135",
	    "81 EC ?? ?? ?? ?? 8B 41 ?? 8B 40 ?? 8B 40 18 8B 90 20 01 00 00 8B 80 24 01 00 00 89 14 24",
	    "5377866",
	    "55 8B EC 81 EC ?? ?? ?? ?? 8B 41 08 8B 40 08 8B 50",
	    "BMS-Retail",
	    "55 8B EC 81 EC ?? ?? ?? ?? A1 ?? ?? ?? ?? 33 C5 89 45 FC 8B 41 ?? 56 8B 75 ?? 57 8B 40 ?? 8B 7D ?? 8B 50",
	    "estranged",
	    "55 8B EC 81 EC ?? ?? ?? ?? 8B 41 08 8B 40 08 8B 40 18 8B 90 20 01 00 00 8B 80 24 01 00 00 89 55 F8");
	PATTERNS(beam_object_to_new_position,
	         "5135",
	         "81 EC D8 00 00 00 53 8B D9 F7 43 78 00 0C 00 00 8B 83 ?? ?? ?? ?? 55",
	         "BMS-Retail",
	         "55 8B EC 81 EC E4 00 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 ?? 8B 55 ??");
} // namespace patterns

void ShadowPosition::InitHooks()
{
	HOOK_FUNCTION(vphysics, GetShadowPosition);
	FIND_PATTERN(vphysics, beam_object_to_new_position);
	PlayerHavokPos.Init();
	PlayerHavokAngles.Init();
}

int __fastcall ShadowPosition::HOOKED_GetShadowPosition(void* thisptr, int _, Vector* worldPosition, QAngle* angles)
{
	auto& feat = spt_player_shadow;
	// yoink
	int numTicksSinceUpdate = feat.ORIG_GetShadowPosition(thisptr, &feat.PlayerHavokPos, &feat.PlayerHavokAngles);
	if (worldPosition)
		*worldPosition = feat.PlayerHavokPos;
	if (angles)
		*angles = feat.PlayerHavokAngles;
	return numTicksSinceUpdate;
}

bool ShadowPosition::ShouldLoadFeature()
{
	return true;
}

void ShadowPosition::LoadFeature()
{
	if (ORIG_GetShadowPosition)
	{
#ifdef SPT_HUD_ENABLED
		bool hudEnabled = AddHudCallback(
		    "shadow_info",
		    [this](std::string)
		    {
			    const Vector& pos = PlayerHavokPos;
			    const QAngle& ang = PlayerHavokAngles;
			    spt_hud_feat.DrawTopHudElement(L"shadow pos (xyz): %.3f %.3f %.3f", pos.x, pos.y, pos.z);
			    spt_hud_feat.DrawTopHudElement(L"shadow ang (pyr): %.3f %.3f %.3f", ang.x, ang.y, ang.z);
		    },
		    y_spt_hud_shadow_info);

		if (hudEnabled)
			SptImGui::RegisterHudCvarCheckbox(y_spt_hud_shadow_info);
#endif
	}

	if (ORIG_beam_object_to_new_position)
	{
		InitCommand(y_spt_set_shadow_roll);

		SptImGuiGroup::Cheats_Misc_PlayerShadow.RegisterUserCallback(
		    []()
		    {
			    static double df = 0.f;
			    SptImGui::InputDouble("shadow roll in degrees", &df);
			    ImGui::SameLine();
			    if (ImGui::Button("set"))
				    spt_player_shadow.SetPlayerHavokRoll((float)df);
			    ImGui::SameLine();
			    SptImGui::CmdHelpMarkerWithName(y_spt_set_shadow_roll_command);
			    SptImGui::CvarCheckbox(y_spt_hud_shadow_info, "##checkbox_hud");
		    });
	}
}

void ShadowPosition::GetPlayerHavokPos(Vector* worldPosition, QAngle* angles)
{
	if (worldPosition)
		*worldPosition = spt_player_shadow.PlayerHavokPos;
	if (angles)
		*angles = spt_player_shadow.PlayerHavokAngles;
}

void ShadowPosition::SetPlayerHavokRoll(float roll)
{
	SetPlayerHavokPos(spt_playerio.m_vecAbsOrigin.GetValue(), QAngle{0, 0, roll});
}

void ShadowPosition::SetPlayerHavokPos(const Vector& worldPosition, const QAngle& angles)
{
	if (!ORIG_beam_object_to_new_position)
		return;
	auto shadow = GetPlayerController();
	if (!shadow)
		return;
	// actually offset 12 in CPlayerController, see the note for GetPlayerController()
	IPhysicsObject* cPhysObject = ((IPhysicsObject**)shadow)[2];
	if (!cPhysObject)
		return;
	void* pivp = ((void**)cPhysObject)[2]; // IVP_Real_Object
	if (!pivp)
		return;
	uint flags = ((uint*)pivp)[30];
	// this is about the same logic the game uses when teleporting the shadow
	IvpPoint pos(worldPosition, true);
	IvpQuat quat(angles);
	if ((flags & 0x300) == 0)
	{
		// collision disabled, just teleport
		ORIG_beam_object_to_new_position(pivp, &quat, &pos, true);
	}
	else
	{
		cPhysObject->EnableCollisions(false);
		ORIG_beam_object_to_new_position(pivp, &quat, &pos, true);
		cPhysObject->EnableCollisions(true);
	}
}

IPhysicsPlayerController* ShadowPosition::GetPlayerController()
{
	constexpr int fOff = -3 * (int)sizeof(void*);
	static utils::CachedField<IPhysicsPlayerController*, "CBasePlayer", "m_oldOrigin", true, fOff> fCont;
	auto ppController = fCont.GetPtrPlayer();
	return ppController ? *ppController : nullptr;
}
