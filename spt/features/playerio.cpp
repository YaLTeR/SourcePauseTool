#include "stdafx.h"
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
#include "..\overlay\portal_camera.hpp"
#include "ihud.hpp"
#include "tas.hpp"
#include "property_getter.hpp"
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

bool PlayerIOFeature::ShouldLoadFeature()
{
	return interfaces::engine != nullptr && spt_entutils.ShouldLoadFeature();
}

void PlayerIOFeature::InitHooks()
{
	HOOK_FUNCTION(client, CreateMove);
	HOOK_FUNCTION(client, GetButtonBits);
	FIND_PATTERN(client, GetGroundEntity);
}

void PlayerIOFeature::UnloadFeature() {}

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
		else if (index == 0)
		{
			offM_pCommands = 180;
		}
		else if (utils::DoesGameLookLikeBMS())
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
		static bool duckPressed = false;

		if (duckspam)
		{
			if (duckPressed)
				duckPressed = false;
			else
			{
				duckPressed = true;
				rv |= (1 << 2); // IN_DUCK
			}
		}
		else
			duckPressed = false;

		if (forceJump)
		{
			forceJump = false;
			rv |= (1 << 1); // IN_JUMP
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
	static bool fetched = false;
	if (fetched)
		return;

	if (spt_entutils.ShouldLoadFeature())
	{
		m_afPhysicsFlags = spt_entutils.GetPlayerField<int>("m_afPhysicsFlags");
		m_bDucking = spt_entutils.GetPlayerField<bool>("m_Local.m_bDucking");
		m_CollisionGroup = spt_entutils.GetPlayerField<int>("m_CollisionGroup");
		m_fFlags = spt_entutils.GetPlayerField<int>("m_fFlags");
		m_flDuckJumpTime = spt_entutils.GetPlayerField<float>("m_Local.m_flDuckJumpTime");
		m_flMaxspeed = spt_entutils.GetPlayerField<float>("m_flMaxspeed");
		m_hGroundEntity = spt_entutils.GetPlayerField<int>("m_hGroundEntity");
		m_MoveCollide = spt_entutils.GetPlayerField<int>("m_MoveCollide");
		m_MoveType = spt_entutils.GetPlayerField<int>("m_MoveType");
		m_vecAbsOrigin = spt_entutils.GetPlayerField<Vector>("m_vecAbsOrigin");
		m_vecAbsVelocity = spt_entutils.GetPlayerField<Vector>("m_vecAbsVelocity");
		m_vecPreviouslyPredictedOrigin = spt_entutils.GetPlayerField<Vector>("m_vecPreviouslyPredictedOrigin");
		m_vecPunchAngle = spt_entutils.GetPlayerField<QAngle>("m_Local.m_vecPunchAngle");
		m_vecPunchAngleVel = spt_entutils.GetPlayerField<QAngle>("m_Local.m_vecPunchAngleVel");
		m_vecViewOffset = spt_entutils.GetPlayerField<Vector>("m_vecViewOffset");
		offServerAbsOrigin = m_vecAbsOrigin.field.serverOffset;

		int m_bSinglePlayerGameEndingOffset = spt_entutils.GetPlayerOffset("m_bSinglePlayerGameEnding", true);
		if (m_bSinglePlayerGameEndingOffset != utils::INVALID_DATAMAP_OFFSET)
		{
			// There's 2 chars between m_bSinglePlayerGameEnding and m_surfaceFriction and floats are 4 byte aligned
			// Therefore it should be -6 relative to m_bSinglePlayerGameEnding.
			m_surfaceFriction.field.serverOffset =
			    m_bSinglePlayerGameEndingOffset & ~3;  // 4 byte align the offset
			m_surfaceFriction.field.serverOffset -= 4; // Take the previous 4 byte aligned address
		}
	}
	fetched = true;
}

bool PlayerIOFeature::IsGroundEntitySet()
{
	if (tas_strafe_version.GetInt() <= 4)
	{
		// This is bugged around portals, here for backwards compat
		auto player = spt_entutils.GetPlayer(false);
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
		const int INDEX_MASK = MAX_EDICTS - 1;
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

	return m_vecAbsVelocity.Found() && m_vecAbsOrigin.Found() && m_flMaxspeed.Found() && m_fFlags.Found()
	       && m_vecPreviouslyPredictedOrigin.Found() && m_bDucking.Found() && m_flDuckJumpTime.Found()
	       && m_surfaceFriction.Found() && m_hGroundEntity.Found() && ORIG_CreateMove && ORIG_GetButtonBits
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

#if defined(OE)
static void DuckspamDown()
#else
static void DuckspamDown(const CCommand& args)
#endif
{
	spt_playerio.EnableDuckspam();
}
static ConCommand DuckspamDown_Command("+y_spt_duckspam", DuckspamDown, "Enables the duckspam.");

#if defined(OE)
static void DuckspamUp()
#else
static void DuckspamUp(const CCommand& args)
#endif
{
	spt_playerio.DisableDuckspam();
}
static ConCommand DuckspamUp_Command("-y_spt_duckspam", DuckspamUp, "Disables the duckspam.");

CON_COMMAND(_y_spt_getvel, "Gets the last velocity of the player.")
{
	const Vector vel = spt_playerio.GetPlayerVelocity();

	Warning("Velocity (x, y, z): %f %f %f\n", vel.x, vel.y, vel.z);
	Warning("Velocity (xy): %f\n", vel.Length2D());
}

#if defined(SSDK2007) || defined(SSDK2013)
CON_COMMAND(y_spt_find_portals, "Prints info for all portals")
{
	for (int i = 0; i < MAX_EDICTS; ++i)
	{
		auto ent = utils::GetClientEntity(i);
		if (!invalidPortal(ent))
		{
			auto color = utils::GetProperty<bool>(i, "m_bIsPortal2") ? "orange" : "blue";
			int remoteIdx = utils::GetProperty<int>(i, "m_hLinkedPortal");
			bool activated = utils::GetProperty<bool>(i, "m_bActivated");
			bool closed = (remoteIdx & INDEX_MASK) == INDEX_MASK;
			auto openStr = closed ? (activated ? "a closed" : "an invisible") : "an open";
			auto& origin = utils::GetPortalPosition(ent);

			Msg("SPT: There's %s %s portal with index %d at %.8f %.8f %.8f.\n",
			    openStr,
			    color,
			    i,
			    origin.x,
			    origin.y,
			    origin.z);
		}
	}
}
#endif

#if SSDK2007
// TODO: remove fixed offsets.

void calculate_offset_player_pos(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	// Here we make sure that the eye position and the eye angles match up.
	const Vector view_offset(0, 0, 64);
	auto& player_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(utils::GetServerPlayer())
	                                                 + spt_playerio.offServerAbsOrigin);
	auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(utils::GetServerPlayer()) + 2568);

	auto& matrix = *reinterpret_cast<VMatrix*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + 1072);

	auto eye_origin = player_origin + view_offset;
	auto new_eye_origin = matrix * eye_origin;
	new_player_origin = new_eye_origin - view_offset;

	new_player_angles = TransformAnglesToWorldSpace(player_angles, matrix.As3x4());
	new_player_angles.x = AngleNormalizePositive(new_player_angles.x);
	new_player_angles.y = AngleNormalizePositive(new_player_angles.y);
	new_player_angles.z = AngleNormalizePositive(new_player_angles.z);
}

CON_COMMAND(
    y_spt_calc_relative_position,
    "y_spt_calc_relative_position <index of the save glitch portal | \"blue\" | \"orange\"> [1 if you want to teleport there instead of just printing]")
{
	if (args.ArgC() != 2 && args.ArgC() != 3)
	{
		Msg("Usage: y_spt_calc_relative_position <index of the save glitch portal | \"blue\" | \"orange\"> [1 if you want to teleport there instead of just printing]\n");
		return;
	}

	int portal_index = atoi(args.Arg(1));

	if (!strcmp(args.Arg(1), "blue") || !strcmp(args.Arg(1), "orange"))
	{
		std::vector<int> indices;

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			auto ent = interfaces::engine_server->PEntityOfEntIndex(i);

			if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal"))
			{
				auto is_orange_portal =
				    *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + 1137);

				if (is_orange_portal == (args.Arg(1)[0] == 'o'))
					indices.push_back(i);
			}
		}

		if (indices.size() > 1)
		{
			Msg("There are multiple %s portals, please use the index:\n", args.Arg(1));

			for (auto i : indices)
			{
				auto ent = interfaces::engine_server->PEntityOfEntIndex(i);
				auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown())
				                                          + spt_playerio.offServerAbsOrigin);

				Msg("%d located at %.8f %.8f %.8f\n", i, origin.x, origin.y, origin.z);
			}

			return;
		}
		else if (indices.size() == 0)
		{
			Msg("There are no %s portals.\n", args.Arg(1));
			return;
		}
		else
		{
			portal_index = indices[0];
		}
	}

	auto portal = interfaces::engine_server->PEntityOfEntIndex(portal_index);
	if (!portal || portal->IsFree() || strcmp(portal->GetClassName(), "prop_portal") != 0)
	{
		Warning("The portal index is invalid.\n");
		return;
	}

	Vector new_player_origin;
	QAngle new_player_angles;
	calculate_offset_player_pos(portal, new_player_origin, new_player_angles);

	if (args.ArgC() == 2)
	{
		Msg("setpos %.8f %.8f %.8f;setang %.8f %.8f %.8f\n",
		    new_player_origin.x,
		    new_player_origin.y,
		    new_player_origin.z,
		    new_player_angles.x,
		    new_player_angles.y,
		    new_player_angles.z);
	}
	else
	{
		char buf[256];
		snprintf(buf,
		         ARRAYSIZE(buf),
		         "setpos %.8f %.8f %.8f;setang %.8f %.8f %.8f\n",
		         new_player_origin.x,
		         new_player_origin.y,
		         new_player_origin.z,
		         new_player_angles.x,
		         new_player_angles.y,
		         new_player_angles.z);

		interfaces::engine->ClientCmd(buf);
	}
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
                                    L"MOVECOLLIDE_COUNT"};

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

#if defined(SSDK2007)
void DrawFlagsHud(bool mutuallyExclusiveFlags, const wchar* hudName, const wchar** nameArray, int count, int flags)
{
	for (int u = 0; u < count; ++u)
	{
		if (nameArray[u])
		{
			if (mutuallyExclusiveFlags && flags == u)
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

#if defined(SSDK2007)
		AddHudCallback(
		    "accelerate",
		    [this]() {
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
#if defined(SSDK2007)
		if (TickSignal.Works)
		{
			TickSignal.Connect(this, &PlayerIOFeature::OnTick);

			AddHudCallback(
			    "accel(xyz)",
			    [this]() {
				    Vector accel = currentVelocity - previousVelocity;
				    spt_hud.DrawTopHudElement(L"accel(xyz): %.3f %.3f %.3f", accel.x, accel.y, accel.z);
				    spt_hud.DrawTopHudElement(L"accel(xy): %.3f", accel.Length2D());
			    },
			    y_spt_hud_accel);
		}

		AddHudCallback(
		    "vel(xyz)",
		    [this]() {
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
		    [this]() {
			    Vector currentVel = GetPlayerVelocity();
			    QAngle angles;
			    VectorAngles(currentVel, Vector(0, 0, 1), angles);
			    spt_hud.DrawTopHudElement(L"vel(p/y/r): %.3f %.3f %.3f", angles.x, angles.y, angles.z);
		    },
		    y_spt_hud_velocity_angles);

		if (utils::DoesGameLookLikePortal())
		{
			AddHudCallback(
			    "ag sg",
			    [this]() {
				    Vector v = spt_playerio.GetPlayerEyePos();
				    QAngle q;

				    std::wstring result = calculateWillAGSG(v, q);
				    spt_hud.DrawTopHudElement(L"ag sg: %s", result.c_str());
			    },
			    y_spt_hud_ag_sg_tester);
		}
#endif
	}

	if (ORIG_GetButtonBits)
	{
		InitConcommandBase(DuckspamUp_Command);
		InitConcommandBase(DuckspamDown_Command);
	}

#ifdef SSDK2007
	if (interfaces::engine_server && interfaces::engine && utils::DoesGameLookLikePortal()
	    && m_vecAbsOrigin.Found()) // 5135 only
	{
		InitCommand(y_spt_calc_relative_position);
	}
#endif
#if defined(SSDK2007)
	if (utils::DoesGameLookLikePortal() && interfaces::engine_server
	    && offServerAbsOrigin != utils::INVALID_DATAMAP_OFFSET)
	{
		InitCommand(y_spt_find_portals);
	}

	if (m_fFlags.Found())
	{
		AddHudCallback(
		    "fl_",
		    [this]() {
			    int flags = spt_playerio.m_fFlags.GetValue();
			    DrawFlagsHud(false, NULL, FLAGS, ARRAYSIZE(FLAGS), flags);
		    },
		    y_spt_hud_flags);
	}

	if (m_MoveType.Found())
	{
		AddHudCallback(
		    "moveflags",
		    [this]() {
			    int flags = spt_playerio.m_MoveType.GetValue();
			    DrawFlagsHud(true, L"Move type", MOVETYPE_FLAGS, ARRAYSIZE(MOVETYPE_FLAGS), flags);
		    },
		    y_spt_hud_moveflags);
	}

	if (m_CollisionGroup.Found())
	{
		AddHudCallback(
		    "collisionflags",
		    [this]() {
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
		    [this]() {
			    int flags = spt_playerio.m_MoveCollide.GetValue();
			    DrawFlagsHud(true, L"Move collide", MOVECOLLIDE_FLAGS, ARRAYSIZE(MOVECOLLIDE_FLAGS), flags);
		    },
		    y_spt_hud_movecollideflags);
	}
#endif
}
