#include "stdafx.hpp"
#include "..\cvars.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\strafe\strafestuff.hpp"
#include "aim.hpp"
#include "hud.hpp"
#include "game_detection.hpp"
#include "math.hpp"
#include "playerio.hpp"
#include "ent_utils.hpp"
#include "interfaces.hpp"
#include "signals.hpp"
#include "tas.hpp"
#include "property_getter.hpp"
#include "spt\utils\portal_utils.hpp"
#include "spt\utils\command.hpp"
#include "..\strafe\strafestuff.hpp"

#ifdef SSDK2007
#include "mathlib\vmatrix.h"
#endif

#undef max
#undef min

PlayerIOFeature spt_playerio;
static void* cinput_thisptr = nullptr;

ConVar y_spt_hud_accel("y_spt_hud_accel", "0", FCVAR_CHEAT, "Turns on the acceleration hud.\n");
ConVar y_spt_hud_ag_sg_tester("y_spt_hud_ag_sg_tester",
                              "0",
                              FCVAR_CHEAT,
                              "Tests if angle glitch will save glitch you.\n");
ConVar y_spt_hud_flags("y_spt_hud_flags", "0", FCVAR_CHEAT, "Turns on the flags hud.\n");
ConVar y_spt_hud_moveflags("y_spt_hud_moveflags", "0", FCVAR_CHEAT, "Turns on the move type hud.\n");
ConVar y_spt_hud_movecollideflags("y_spt_hud_movecollideflags", "0", FCVAR_CHEAT, "Turns on the move collide hud.\n");
ConVar y_spt_hud_collisionflags("y_spt_hud_collisionflags", "0", FCVAR_CHEAT, "Turns on the collision group hud.\n");
ConVar y_spt_hud_vars("y_spt_hud_vars", "0", FCVAR_CHEAT, "Turns on the movement vars HUD.\n");
ConVar y_spt_hud_velocity("y_spt_hud_velocity", "0", FCVAR_CHEAT, "Turns on the velocity hud.\n");
ConVar y_spt_hud_velocity_angles("y_spt_hud_velocity_angles", "0", FCVAR_CHEAT, "Display velocity Euler angles.");

namespace patterns
{
	PATTERNS(
	    GetButtonBits,
	    "5135",
	    "55 56 8B E9 B1 03 33 F6 84 0D ?? ?? ?? ?? 57 74 05 BE 00 00 02 00 8B 15 ?? ?? ?? ?? F7 C2 00 00 02 00 B8 FD FF FF",
	    "5339",
	    "55 8B EC 56 33 F6 F6 05 ?? ?? ?? ?? 03 74 05 BE 00 00 02 00 8B 15 ?? ?? ?? ?? B8 FD FF FF FF F7 C2 00 00 02 00 74",
	    "2257546",
	    "55 8B EC 51 8B 15 ?? ?? ?? ?? B8 00 00 02 00 53 56 33 F6 8B D9 F6 C2 03 B9 FD FF FF FF 57 0F 45 F0 BF FC FF FF FF",
	    "2707",
	    "51 A0 ?? ?? ?? ?? B2 03 84 C2 C7 44 24 00 00 00 00 00 74 08 C7 44 24 00 00 00 02 00 8B 0D ?? ?? ?? ?? F7 C1 00 00 02 00",
	    "4044-episodic",
	    "56 B0 03 33 F6 84 05 ?? ?? ?? ?? 74 05 BE ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? F7 C2 ?? ?? ?? ?? B9",
	    "6879",
	    "55 8B EC 83 EC 0C 56 8B F1 8B 0D ?? ?? ?? ?? 8B 01 8B 90 ?? ?? ?? ?? 57 89 75 F8 FF D2 8B F8 C7 45",
	    "missinginfo1_4_7",
	    "55 8B EC 83 EC 08 89 4D F8 C7 45 ?? ?? ?? ?? ?? 83 7D 08 00 0F 95 C0 50 68 ?? ?? ?? ?? 8B 0D",
	    "dmomm",
	    "51 53 56 8B 35 ?? ?? ?? ?? 57 8B 7C 24 ?? 85 FF 0F 95 C3");
	PATTERNS(
	    CreateMove,
	    "5135",
	    "83 EC 14 53 D9 EE 55 56 57 8B F9 8B 4C 24 28 B8 B7 60 0B B6 F7 E9 03 D1 C1 FA 06 8B C2 C1 E8 1F 03 D0 6B D2 5A 8B C1 2B C2 8B F0 6B C0 58 03 87",
	    "4104",
	    "83 EC 14 53 D9 EE 55 56 57 8B F9 8B 4C 24 28 B8 B7 60 0B B6 F7 E9 03 D1 C1 FA 06 8B C2 C1 E8 1F 03 C2 6B C0 5A 8B F1 2B F0 6B F6 54 03 B7 C4 00",
	    "1910503",
	    "55 8B EC 83 EC 50 53 8B D9 8B 4D 08 B8 ?? ?? ?? ?? F7 E9 03 D1 C1 FA 06 8B C2 C1 E8 1F 03 D0 0F 57 C0",
	    "2257546",
	    "55 8B EC 83 EC 54 53 56 8B 75 08 B8 ?? ?? ?? ?? F7 EE 57 03 D6 8B F9 C1 FA 06 8B CE 8B C2 C1 E8 1F",
	    "2257546-hl1",
	    "55 8B EC 83 EC 50 53 8B 5D 08 B8 ?? ?? ?? ?? F7 EB 56 03 D3 C1 FA 06 8B C2 C1 E8 1F 03 C2 8B D3",
	    "BMS-Retail",
	    "55 8B EC 83 EC 54 53 56 8B 75 08 B8 ?? ?? ?? ?? F7 EE 57 03 D6 8B F9 C1 FA 06 8B CE 8B C2 C1 E8 1F 03 C2 6B C0 5A",
	    "hl1movement",
	    "55 8B EC 83 EC 18 53 56 57 8B 7D ?? B8 ?? ?? ?? ?? F7 EF 8B F1 8B CF 03 D7 C1 FA 06 8B C2 C1 E8 1F 03 C2 6B C0 5A",
	    "4044",
	    "8B 44 24 ?? 83 EC 1C 53 55 56 57",
	    "dmomm",
	    "83 EC 10 53 8B 5C 24 ?? 55 56 8B F1");
	PATTERNS(
	    GetGroundEntity,
	    "5135",
	    "8B 81 EC 01 00 00 83 F8 FF 74 20 8B 15 ?? ?? ?? ?? 8B C8 81 E1 FF 0F 00 00 C1 E1 04 8D 4C",
	    "4104",
	    "8B 81 E8 01 00 00 83 F8 FF 74 20 8B 15 ?? ?? ?? ?? 8B C8 81 E1 FF 0F 00 00 C1 E1 04 8D 4C 11 04",
	    "1910503",
	    "8B 81 50 02 00 00 83 F8 FF 74 1F 8B 15 ?? ?? ?? ?? 8B C8 81 E1 ?? ?? ?? ?? 03 C9 8D 4C CA 04 C1 E8 0C",
	    "2257546",
	    "8B 91 50 02 00 00 83 FA FF 74 1D A1 ?? ?? ?? ?? 8B CA 81 E1 ?? ?? ?? ?? C1 EA 0C 03 C9 39 54 C8 08",
	    "hl1movement",
	    "8B 91 EC 01 00 00 83 FA FF 74 1D A1 ?? ?? ?? ?? 8B CA 81 E1 ?? ?? ?? ?? C1 EA 0C 03 C9 39 54 C8 08");
} // namespace patterns

void PlayerIOFeature::InitHooks()
{
	HOOK_FUNCTION(client, CreateMove);
	HOOK_FUNCTION(client, GetButtonBits);
	FIND_PATTERN(client, GetGroundEntity);
}

bool PlayerIOFeature::ShouldLoadFeature()
{
	return interfaces::engine != nullptr && spt_entprops.ShouldLoadFeature();
}

void PlayerIOFeature::UnloadFeature()
{
	fetchedPlayerFields = false;
	cinput_thisptr = nullptr;
}

void PlayerIOFeature::PreHook()
{
	if (ORIG_CreateMove)
	{
		int index = GetPatternIndex((void**)&ORIG_CreateMove);
		CreateMoveSignal.Works = true;

		// New steampipe hl2/portal have a different offset
		if (!utils::DoesGameLookLikeHLS() && utils::GetBuildNumber() >= 7196940)
		{
			offM_pCommands = 224;
		}
		else if (index == 0) // 5135
		{
			offM_pCommands = 180;
		}
		else if (index == 7 || index == 8 || utils::DoesGameLookLikeBMS()) // OE & BMS
		{
			offM_pCommands = 244;
		}
		else
		{
			offM_pCommands = 196;
		}

		offForwardmove = 24;
		offSidemove = 28;
	}

	GetPlayerFields();
}

Strafe::MovementVars PlayerIOFeature::GetMovementVars()
{
	auto vars = Strafe::MovementVars();

	if (!playerioAddressesWereFound || cinput_thisptr == nullptr)
	{
		return vars;
	}

	auto maxspeed = m_flMaxspeed.GetValue();

	auto pl = GetPlayerData();
	vars.OnGround = Strafe::GetPositionType(pl, pl.Ducking ? Strafe::HullType::DUCKED : Strafe::HullType::NORMAL)
	                == Strafe::PositionType::GROUND;
	bool ground; // for backwards compatibility with old bugs

	if (tas_strafe_version.GetInt() <= 1)
	{
		ground = IsGroundEntitySet();
	}
	else
	{
		ground = vars.OnGround;
	}

	if (tas_force_onground.GetBool())
	{
		ground = true;
		vars.OnGround = true;
	}

	vars.ReduceWishspeed = ground && GetFlagsDucking();
	vars.Accelerate = _sv_accelerate->GetFloat();

	if (tas_force_airaccelerate.GetString()[0] != '\0')
		vars.Airaccelerate = tas_force_airaccelerate.GetFloat();
	else
		vars.Airaccelerate = _sv_airaccelerate->GetFloat();

	vars.EntFriction = 1;
	vars.Frametime = 0.015f;
	vars.Friction = _sv_friction->GetFloat();
	vars.Maxspeed = (maxspeed > 0) ? std::min(maxspeed, _sv_maxspeed->GetFloat()) : _sv_maxspeed->GetFloat();
	vars.Stopspeed = _sv_stopspeed->GetFloat();

	if (tas_force_wishspeed_cap.GetString()[0] != '\0')
		vars.WishspeedCap = tas_force_wishspeed_cap.GetFloat();
	else
		vars.WishspeedCap = 30;

#ifdef OE
	vars.EntFriction = 1.0f;
#else
	auto previouslyPredictedOrigin = m_vecPreviouslyPredictedOrigin.GetValue();
	auto absOrigin = m_vecAbsOrigin.GetValue();
	bool gameCodeMovedPlayer = (previouslyPredictedOrigin != absOrigin);

	vars.EntFriction = m_surfaceFriction.GetValue();

	if (gameCodeMovedPlayer)
	{
		if (tas_reset_surface_friction.GetBool())
		{
			vars.EntFriction = 1.0f;
		}

		if (pl.Velocity.z <= 140.f)
		{
			if (ground)
			{
				// TODO: This should check the real surface friction.
				vars.EntFriction = 1.0f;
			}
			else if (pl.Velocity.z > 0.f)
			{
				vars.EntFriction = 0.25f;
			}
		}
	}
#endif

	vars.EntGravity = 1.0f;
	vars.Maxvelocity = _sv_maxvelocity->GetFloat();
	vars.Gravity = _sv_gravity->GetFloat();
	vars.Stepsize = _sv_stepsize->GetFloat();
	vars.Bounce = _sv_bounce->GetFloat();
	vars.CantJump = false;
	// This will report air on the first frame of unducking and report ground on the last one.
	if (m_bDucking.GetValue() && GetFlagsDucking())
	{
		vars.CantJump = true;
	}

	auto djt = m_flDuckJumpTime.GetValue();
	djt -= vars.Frametime * 1000;
	djt = std::max(0.f, djt);
	float flDuckMilliseconds = std::max(0.0f, 1000.f - djt);
	float flDuckSeconds = flDuckMilliseconds * 0.001f;
	if (flDuckSeconds > 0.2)
	{
		djt = 0;
	}
	if (djt > 0)
	{
		vars.CantJump = true;
	}

	return vars;
}

void __fastcall PlayerIOFeature::HOOKED_CreateMove_Func(void* thisptr,
                                                        int edx,
                                                        int sequence_number,
                                                        float input_sample_frametime,
                                                        bool active)
{
	auto m_pCommands = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(thisptr) + offM_pCommands);
	pCmd = m_pCommands + sizeofCUserCmd * (sequence_number % 90);

	ORIG_CreateMove(thisptr, edx, sequence_number, input_sample_frametime, active);

	CreateMoveSignal(pCmd);

	pCmd = 0;
}

void __fastcall PlayerIOFeature::HOOKED_CreateMove(void* thisptr,
                                                   int edx,
                                                   int sequence_number,
                                                   float input_sample_frametime,
                                                   bool active)
{
	spt_playerio.HOOKED_CreateMove_Func(thisptr, edx, sequence_number, input_sample_frametime, active);
}

int __fastcall PlayerIOFeature::HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState)
{
	int rv = ORIG_GetButtonBits(thisptr, edx, bResetState);

	if (bResetState == 1)
	{
		static int keyPressed = 0;
		keyPressed = (keyPressed ^ spamButtons) & spamButtons;
		rv |= keyPressed;

		if (forceJump)
		{
			rv |= (1 << 1); // IN_JUMP
			forceJump = false;
		}

		if (forceUnduck)
		{
			rv ^= (1 << 2); // IN_DUCK
			forceUnduck = false;
		}
	}

	return rv;
}

int __fastcall PlayerIOFeature::HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState)
{
	return spt_playerio.HOOKED_GetButtonBits_Func(thisptr, edx, bResetState);
}

bool PlayerIOFeature::GetFlagsDucking()
{
	return m_fFlags.GetValue() & FL_DUCKING;
}

Strafe::PlayerData PlayerIOFeature::GetPlayerData()
{
	if (!playerioAddressesWereFound)
		return Strafe::PlayerData();

	Strafe::PlayerData data;
	const int IN_DUCK = 1 << 2;

	data.Ducking = GetFlagsDucking();
	data.DuckPressed = (ORIG_GetButtonBits(cinput_thisptr, 0, 0) & IN_DUCK);
	data.UnduckedOrigin = m_vecAbsOrigin.GetValue();
	data.Velocity = GetPlayerVelocity();
	data.Basevelocity = Vector();

	if (data.Ducking)
	{
		data.UnduckedOrigin.z -= 36;

		if (tas_strafe_use_tracing.GetBool() && Strafe::CanUnduck(data))
			data.Ducking = false;
	}

	return data;
}

Vector PlayerIOFeature::GetPlayerVelocity()
{
	return m_vecAbsVelocity.GetValue();
}

Vector PlayerIOFeature::GetPlayerEyePos()
{
	Vector rval = m_vecAbsOrigin.GetValue();
	constexpr float duckOffset = 28;
	constexpr float standingOffset = 64;

	if (GetFlagsDucking())
	{
		rval.z += duckOffset;
	}
	else
	{
		rval.z += standingOffset;
	}

	return rval;
}

void PlayerIOFeature::GetPlayerFields()
{
	if (fetchedPlayerFields)
		return;

	if (spt_entprops.ShouldLoadFeature())
	{
		m_afPhysicsFlags = spt_entprops.GetPlayerField<int>("m_afPhysicsFlags");
		m_bDucking = spt_entprops.GetPlayerField<bool>("m_Local.m_bDucking");
		m_CollisionGroup = spt_entprops.GetPlayerField<int>("m_CollisionGroup");
		m_fFlags = spt_entprops.GetPlayerField<int>("m_fFlags");
		m_flDuckJumpTime = spt_entprops.GetPlayerField<float>("m_Local.m_flDuckJumpTime");
		m_flMaxspeed = spt_entprops.GetPlayerField<float>("m_flMaxspeed");
		m_hGroundEntity = spt_entprops.GetPlayerField<int>("m_hGroundEntity");
		m_MoveCollide = spt_entprops.GetPlayerField<int>("m_MoveCollide");
		m_MoveType = spt_entprops.GetPlayerField<int>("m_MoveType");
		m_vecAbsOrigin = spt_entprops.GetPlayerField<Vector>("m_vecAbsOrigin");
		m_vecAbsVelocity = spt_entprops.GetPlayerField<Vector>("m_vecAbsVelocity");
		m_vecPreviouslyPredictedOrigin = spt_entprops.GetPlayerField<Vector>("m_vecPreviouslyPredictedOrigin");
		m_vecPunchAngle = spt_entprops.GetPlayerField<QAngle>("m_Local.m_vecPunchAngle");
		m_vecPunchAngleVel = spt_entprops.GetPlayerField<QAngle>("m_Local.m_vecPunchAngleVel");
		m_vecViewOffset = spt_entprops.GetPlayerField<Vector>("m_vecViewOffset");
		offServerAbsOrigin = m_vecAbsOrigin.field.serverOffset;

		int m_bSinglePlayerGameEndingOffset = spt_entprops.GetPlayerOffset("m_bSinglePlayerGameEnding", true);
		if (m_bSinglePlayerGameEndingOffset != utils::INVALID_DATAMAP_OFFSET)
		{
			// There's 2 chars between m_bSinglePlayerGameEnding and m_surfaceFriction and floats are 4 byte aligned
			// Therefore it should be -6 relative to m_bSinglePlayerGameEnding.
			m_surfaceFriction.field.serverOffset =
			    m_bSinglePlayerGameEndingOffset & ~3;  // 4 byte align the offset
			m_surfaceFriction.field.serverOffset -= 4; // Take the previous 4 byte aligned address
		}
	}
	fetchedPlayerFields = true;
}

bool PlayerIOFeature::IsGroundEntitySet()
{
	if (tas_strafe_version.GetInt() <= 4)
	{
		// This is bugged around portals, here for backwards compat
		auto player = spt_entprops.GetPlayer(false);
		if (ORIG_GetGroundEntity == nullptr || !player)
			return false;
		else
		{
			return (ORIG_GetGroundEntity(player, 0) != NULL);
		}
	}
	else
	{
		// Not bugged around portals
		int index = m_hGroundEntity.GetValue() & INDEX_MASK;
		return index != INDEX_MASK;
	}
}

bool PlayerIOFeature::TryJump()
{
	const int IN_JUMP = (1 << 1);
	return ORIG_GetButtonBits(cinput_thisptr, 0, 0) & IN_JUMP;
}

bool PlayerIOFeature::PlayerIOAddressesFound()
{
	GetPlayerFields();

	return
#ifndef OE
	    m_vecPreviouslyPredictedOrigin.Found() &&
#endif
	    m_vecAbsOrigin.Found() && m_flMaxspeed.Found() && m_fFlags.Found() && m_bDucking.Found()
	    && m_vecAbsOrigin.Found() && m_flMaxspeed.Found() && m_fFlags.Found() && m_bDucking.Found()
	    && m_flDuckJumpTime.Found() && m_hGroundEntity.Found() && ORIG_CreateMove && ORIG_GetButtonBits
	    && _sv_airaccelerate && _sv_accelerate && _sv_friction && _sv_maxspeed && _sv_stopspeed
	    && interfaces::engine_server != nullptr;
}

void PlayerIOFeature::GetMoveInput(float& forwardmove, float& sidemove)
{
	if (pCmd)
	{
		forwardmove = *reinterpret_cast<float*>(pCmd + offForwardmove);
		sidemove = *reinterpret_cast<float*>(pCmd + offSidemove);
	}
	else
	{
		forwardmove = sidemove = 0;
	}
}

void PlayerIOFeature::SetTASInput(float* va, const Strafe::ProcessedFrame& _out)
{
	Strafe::ProcessedFrame out = _out;
	// This bool is set if strafing should occur
	if (out.Processed)
	{
#define CHECK_FINITE(value) \
	if (!std::isfinite(value)) \
	{ \
		Warning("Strafe input " #value " was %f, reverting to 0.\n", value); \
		value = 0; \
	}

		CHECK_FINITE(out.ForwardSpeed);
		CHECK_FINITE(out.SideSpeed);
		CHECK_FINITE(out.Yaw);

		if (out.Jump && tas_strafe_jumptype.GetInt() > 0)
		{
			spt_aim.SetJump();
		}

		// Apply jump and unduck regardless of whether we are strafing
		forceUnduck = out.ForceUnduck;
		forceJump = out.Jump;

		*reinterpret_cast<float*>(pCmd + offForwardmove) = out.ForwardSpeed;
		*reinterpret_cast<float*>(pCmd + offSidemove) = out.SideSpeed;
		va[YAW] = static_cast<float>(out.Yaw);
	}
}

double PlayerIOFeature::GetDuckJumpTime()
{
	return m_flDuckJumpTime.GetValue();
}

void PlayerIOFeature::Set_cinput_thisptr(void* thisptr)
{
	cinput_thisptr = thisptr;
}

void PlayerIOFeature::OnTick()
{
	previousVelocity = currentVelocity;
	currentVelocity = GetPlayerVelocity();
}

static const std::map<std::string, int> buttonCodeMap = {
    {"attack", 1 << 0},
    {"jump", 1 << 1},
    {"duck", 1 << 2},
    {"forward", 1 << 3},
    {"back", 1 << 4},
    {"use", 1 << 5},
    {"left", 1 << 7},
    {"right", 1 << 8},
    {"moveleft", 1 << 9},
    {"moveright", 1 << 10},
    {"attack2", 1 << 11},
    {"reload", 1 << 13},
    {"speed", 1 << 17},
    {"walk", 1 << 18},
    {"zoom", 1 << 19},
};

CON_COMMAND_DOWN(
    y_spt_spam,
    "Enables key spam.\n"
    "Usage: +y_spt_spam <key>\n"
    "keys: attack, jump, duck, forward, back, use, left, right, moveleft, moveright, attack2, reload, speed, walk, zoom")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: +y_spt_spam <key>\n"
		    "keys: attack, jump, duck, forward, back, use, left, right, moveleft, moveright, attack2, reload, speed, walk, zoom\n");
		return;
	}

	auto buttonCodeIter = buttonCodeMap.find(args.Arg(1));
	if (buttonCodeIter != buttonCodeMap.end())
		spt_playerio.EnableSpam(buttonCodeIter->second);
	else
		Msg("Cannot find key <%s>\n", args.Arg(1));
}

CON_COMMAND_UP(y_spt_spam, "Disables key spam.")
{
	if (args.ArgC() < 2)
	{
		// clear all spam keys
		spt_playerio.DisableSpam(0xffffffff);
		return;
	}

	auto buttonCodeIter = buttonCodeMap.find(args.Arg(1));
	if (buttonCodeIter != buttonCodeMap.end())
		spt_playerio.DisableSpam(buttonCodeIter->second);
}

CON_COMMAND_DOWN(y_spt_duckspam, "Enables the duckspam. (Outdated, use +y_spt_spam instead)")
{
	spt_playerio.EnableSpam(1 << 2); // IN_DUCK
}

CON_COMMAND_UP(y_spt_duckspam, "Disables the duckspam.")
{
	spt_playerio.DisableSpam(1 << 2); // IN_DUCK
}

CON_COMMAND(_y_spt_getvel, "Gets the last velocity of the player.")
{
	const Vector vel = spt_playerio.GetPlayerVelocity();

	Warning("Velocity (x, y, z): %f %f %f\n", vel.x, vel.y, vel.z);
	Warning("Velocity (xy): %f\n", vel.Length2D());
}

#ifdef SPT_PORTAL_UTILS
CON_COMMAND(y_spt_find_portals, "Prints info for all portals")
{
	bool found = false;
	for (int i = 0; i < MAX_EDICTS; ++i)
	{
		auto ent = utils::GetClientEntity(i);
		if (!invalidPortal(ent))
		{
			found = true;
			auto color = spt_propertyGetter.GetProperty<bool>(i, "m_bIsPortal2") ? "orange" : "blue";
			int remoteIdx = spt_propertyGetter.GetProperty<int>(i, "m_hLinkedPortal");
			bool activated = spt_propertyGetter.GetProperty<bool>(i, "m_bActivated");
			bool closed = (remoteIdx & INDEX_MASK) == INDEX_MASK;
			auto openStr = closed ? (activated ? "a closed" : "an invisible") : "an open";
			const auto& origin = utils::GetPortalPosition(ent);

			Msg("SPT: There's %s %s portal with index %d at %.8f %.8f %.8f.\n",
			    openStr,
			    color,
			    i,
			    origin.x,
			    origin.y,
			    origin.z);
		}
	}
	if (!found)
		Msg("SPT: No portals!\n");
}
#endif

CON_COMMAND(tas_print_movement_vars, "Prints movement vars.")
{
	auto vars = spt_playerio.GetMovementVars();

	Msg("Accelerate %f\n", vars.Accelerate);
	Msg("AirAccelerate %f\n", vars.Airaccelerate);
	Msg("Bounce %f\n", vars.Bounce);
	Msg("EntFriction %f\n", vars.EntFriction);
	Msg("EntGrav %f\n", vars.EntGravity);
	Msg("Frametime %f\n", vars.Frametime);
	Msg("Friction %f\n", vars.Friction);
	Msg("Grav %f\n", vars.Gravity);
	Msg("Maxspeed %f\n", vars.Maxspeed);
	Msg("Maxvelocity %f\n", vars.Maxvelocity);
	Msg("OnGround %d\n", vars.OnGround);
	Msg("ReduceWishspeed %d\n", vars.ReduceWishspeed);
	Msg("Stepsize %f\n", vars.Stepsize);
	Msg("Stopspeed %f\n", vars.Stopspeed);
	Msg("WS cap %f\n", vars.WishspeedCap);
}

CON_COMMAND(_y_spt_getangles, "Gets the view angles of the player.")
{
	if (!interfaces::engine)
		return;

	QAngle va;
	interfaces::engine->GetViewAngles(va);

	Warning("View Angle (x): %f\n", va.x);
	Warning("View Angle (y): %f\n", va.y);
	Warning("View Angle (z): %f\n", va.z);
	Warning("View Angle (x, y, z): %f %f %f\n", va.x, va.y, va.z);
}

const wchar* FLAGS[] = {L"FL_ONGROUND",
                        L"FL_DUCKING",
                        L"FL_WATERJUMP",
                        L"FL_ONTRAIN",
                        L"FL_INRAIN",
                        L"FL_FROZEN",
                        L"FL_ATCONTROLS",
                        L"FL_CLIENT",
                        L"FL_FAKECLIENT",
                        L"FL_INWATER",
                        L"FL_FLY",
                        L"FL_SWIM",
                        L"FL_CONVEYOR",
                        L"FL_NPC",
                        L"FL_GODMODE",
                        L"FL_NOTARGET",
                        L"FL_AIMTARGET",
                        L"FL_PARTIALGROUND",
                        L"FL_STATICPROP",
                        L"FL_GRAPHED",
                        L"FL_GRENADE",
                        L"FL_STEPMOVEMENT",
                        L"FL_DONTTOUCH",
                        L"FL_BASEVELOCITY",
                        L"FL_WORLDBRUSH",
                        L"FL_OBJECT",
                        L"FL_KILLME",
                        L"FL_ONFIRE",
                        L"FL_DISSOLVING",
                        L"FL_TRANSRAGDOLL",
                        L"FL_UNBLOCKABLE_BY_PLAYER"};

const wchar* MOVETYPE_FLAGS[] = {L"MOVETYPE_NONE",
                                 L"MOVETYPE_ISOMETRIC",
                                 L"MOVETYPE_WALK",
                                 L"MOVETYPE_STEP",
                                 L"MOVETYPE_FLY",
                                 L"MOVETYPE_FLYGRAVITY",
                                 L"MOVETYPE_VPHYSICS",
                                 L"MOVETYPE_PUSH",
                                 L"MOVETYPE_NOCLIP",
                                 L"MOVETYPE_LADDER",
                                 L"MOVETYPE_OBSERVER",
                                 L"MOVETYPE_CUSTOM"};

const wchar* MOVECOLLIDE_FLAGS[] = {L"MOVECOLLIDE_DEFAULT",
                                    L"MOVECOLLIDE_FLY_BOUNCE",
                                    L"MOVECOLLIDE_FLY_CUSTOM",
                                    L"MOVECOLLIDE_FLY_SLIDE"};

const wchar* COLLISION_GROUPS[] = {L"COLLISION_GROUP_NONE",
                                   L"COLLISION_GROUP_DEBRIS",
                                   L"COLLISION_GROUP_DEBRIS_TRIGGER",
                                   L"COLLISION_GROUP_INTERACTIVE_DEBRIS",
                                   L"COLLISION_GROUP_INTERACTIVE",
                                   L"COLLISION_GROUP_PLAYER",
                                   L"COLLISION_GROUP_BREAKABLE_GLASS,",
                                   L"COLLISION_GROUP_VEHICLE",
                                   L"COLLISION_GROUP_PLAYER_MOVEMENT",
                                   L"COLLISION_GROUP_NPC",
                                   L"COLLISION_GROUP_IN_VEHICLE",
                                   L"COLLISION_GROUP_WEAPON",
                                   L"COLLISION_GROUP_VEHICLE_CLIP",
                                   L"COLLISION_GROUP_PROJECTILE",
                                   L"COLLISION_GROUP_DOOR_BLOCKER",
                                   L"COLLISION_GROUP_PASSABLE_DOOR",
                                   L"COLLISION_GROUP_DISSOLVING",
                                   L"COLLISION_GROUP_PUSHAWAY",
                                   L"COLLISION_GROUP_NPC_ACTOR",
                                   L"COLLISION_GROUP_NPC_SCRIPTED",
                                   L"LAST_SHARED_COLLISION_GROUP"};

#define DRAW_FLAGS(name, namesArray, flags, mutuallyExclusive) \
	{ \
		DrawFlagsHud(mutuallyExclusive, \
		             name, \
		             vertIndex, \
		             x, \
		             namesArray, \
		             ARRAYSIZE(namesArray), \
		             surface, \
		             buffer, \
		             BUFFER_SIZE, \
		             flags, \
		             fontTall); \
	}

#ifdef SPT_HUD_ENABLED
void DrawFlagsHud(bool mutuallyExclusiveFlags, const wchar* hudName, const wchar** nameArray, uint count, uint flags)
{
	uint mask = (1 << count) - 1;
	for (uint u = 0; u < count; ++u)
	{
		if (nameArray[u])
		{
			if (mutuallyExclusiveFlags && (flags & mask) == u)
			{
				spt_hud.DrawTopHudElement(L"%s: %s", hudName, nameArray[u]);
			}
			else if (!mutuallyExclusiveFlags)
			{
				spt_hud.DrawTopHudElement(L"%s: %d", nameArray[u], (flags & (1 << u)) != 0);
			}
		}
	}
}
#endif

void PlayerIOFeature::LoadFeature()
{
	if (utils::DoesGameLookLikeHLS())
	{
		sizeofCUserCmd = 64; // Is missing a CUtlVector
	}
	else if (utils::DoesGameLookLikeDMoMM())
	{
		sizeofCUserCmd = 188;
	}
	else
	{
		sizeofCUserCmd = 84;
	}

	playerioAddressesWereFound = PlayerIOAddressesFound();

	if (interfaces::engine)
	{
		InitCommand(_y_spt_getangles);
	}

	if (playerioAddressesWereFound)
	{
		InitCommand(tas_print_movement_vars);

#ifdef SPT_HUD_ENABLED
		AddHudCallback(
		    "accelerate",
		    [this]()
		    {
			    auto vars = GetMovementVars();
			    spt_hud.DrawTopHudElement(L"accelerate: %.3f", vars.Accelerate);
			    spt_hud.DrawTopHudElement(L"airaccelerate: %.3f", vars.Airaccelerate);
			    spt_hud.DrawTopHudElement(L"ent friction: %.3f", vars.EntFriction);
			    spt_hud.DrawTopHudElement(L"frametime: %.3f", vars.Frametime);
			    spt_hud.DrawTopHudElement(L"friction %.3f", vars.Friction);
			    spt_hud.DrawTopHudElement(L"maxspeed: %.3f", vars.Maxspeed);
			    spt_hud.DrawTopHudElement(L"stopspeed: %.3f", vars.Stopspeed);
			    spt_hud.DrawTopHudElement(L"wishspeed cap: %.3f", vars.WishspeedCap);
			    spt_hud.DrawTopHudElement(L"onground: %d", (int)vars.OnGround);
		    },
		    y_spt_hud_vars);
#endif
	}

	if (m_vecAbsVelocity.Found())
	{
		InitCommand(_y_spt_getvel);
#ifdef SPT_HUD_ENABLED
		if (TickSignal.Works)
		{
			TickSignal.Connect(this, &PlayerIOFeature::OnTick);

			AddHudCallback(
			    "accel(xyz)",
			    [this]()
			    {
				    Vector accel = currentVelocity - previousVelocity;
				    spt_hud.DrawTopHudElement(L"accel(xyz): %.3f %.3f %.3f", accel.x, accel.y, accel.z);
				    spt_hud.DrawTopHudElement(L"accel(xy): %.3f", accel.Length2D());
			    },
			    y_spt_hud_accel);
		}

		AddHudCallback(
		    "vel(xyz)",
		    [this]()
		    {
			    Vector currentVel = GetPlayerVelocity();
			    spt_hud.DrawTopHudElement(L"vel(xyz): %.3f %.3f %.3f",
			                              currentVel.x,
			                              currentVel.y,
			                              currentVel.z);
			    spt_hud.DrawTopHudElement(L"vel(xy): %.3f", currentVel.Length2D());
		    },
		    y_spt_hud_velocity);

		AddHudCallback(
		    "vel(p/y/r)",
		    [this]()
		    {
			    Vector currentVel = GetPlayerVelocity();
			    QAngle angles;
			    VectorAngles(currentVel, Vector(0, 0, 1), angles);
			    spt_hud.DrawTopHudElement(L"vel(p/y/r): %.3f %.3f %.3f", angles.x, angles.y, angles.z);
		    },
		    y_spt_hud_velocity_angles);

#ifdef SPT_PORTAL_UTILS
		if (utils::DoesGameLookLikePortal())
		{
			AddHudCallback(
			    "ag sg",
			    [this]()
			    {
				    Vector v = spt_playerio.GetPlayerEyePos();
				    QAngle q;
				    std::wstring result = calculateWillAGSG(v, q);
				    spt_hud.DrawTopHudElement(L"ag sg: %s", result.c_str());
			    },
			    y_spt_hud_ag_sg_tester);
		}
#endif
#endif
	}

	if (ORIG_GetButtonBits)
	{
		InitConcommandBase(y_spt_spam_down_command);
		InitConcommandBase(y_spt_spam_up_command);
		InitConcommandBase(y_spt_duckspam_down_command);
		InitConcommandBase(y_spt_duckspam_up_command);
	}

#ifdef SPT_HUD_ENABLED
#ifdef SPT_PORTAL_UTILS
	if (utils::DoesGameLookLikePortal() && interfaces::engine_server
	    && offServerAbsOrigin != utils::INVALID_DATAMAP_OFFSET)
	{
		InitCommand(y_spt_find_portals);
	}
#endif

	if (m_fFlags.Found())
	{
		AddHudCallback(
		    "fl_",
		    [this]()
		    {
			    int flags = spt_playerio.m_fFlags.GetValue();
			    DrawFlagsHud(false, NULL, FLAGS, ARRAYSIZE(FLAGS), flags);
		    },
		    y_spt_hud_flags);
	}

	if (m_MoveType.Found())
	{
		AddHudCallback(
		    "moveflags",
		    [this]()
		    {
			    int flags = spt_playerio.m_MoveType.GetValue();
			    DrawFlagsHud(true, L"Move type", MOVETYPE_FLAGS, ARRAYSIZE(MOVETYPE_FLAGS), flags);
		    },
		    y_spt_hud_moveflags);
	}

	if (m_CollisionGroup.Found())
	{
		AddHudCallback(
		    "collisionflags",
		    [this]()
		    {
			    int flags = spt_playerio.m_CollisionGroup.GetValue();
			    DrawFlagsHud(true,
			                 L"Collision group",
			                 COLLISION_GROUPS,
			                 ARRAYSIZE(COLLISION_GROUPS),
			                 flags);
		    },
		    y_spt_hud_collisionflags);
	}

	if (m_MoveCollide.Found())
	{
		AddHudCallback(
		    "movecollide",
		    [this]()
		    {
			    int flags = spt_playerio.m_MoveCollide.GetValue();
			    DrawFlagsHud(true, L"Move collide", MOVECOLLIDE_FLAGS, ARRAYSIZE(MOVECOLLIDE_FLAGS), flags);
		    },
		    y_spt_hud_movecollideflags);
	}
#endif
}
