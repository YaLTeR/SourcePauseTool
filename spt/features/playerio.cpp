#include "stdafx.h"
#include "..\cvars.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\strafe\strafestuff.hpp"
#include "aim.hpp"
#include "game_detection.hpp"
#include "math.hpp"
#include "playerio.hpp"
#include "ent_utils.hpp"
#include "interfaces.hpp"

#ifdef SSDK2007
#include "mathlib\vmatrix.h"
#endif

#undef max
#undef min

PlayerIOFeature spt_playerio;
static void* cinput_thisptr = nullptr;

bool PlayerIOFeature::ShouldLoadFeature()
{
	return interfaces::engine != nullptr;
}

void PlayerIOFeature::InitHooks()
{
	HOOK_FUNCTION(client, CreateMove);
	HOOK_FUNCTION(client, GetButtonBits);
	FIND_PATTERN(client, CalcAbsoluteVelocity);
	FIND_PATTERN(client, GetGroundEntity);
	FIND_PATTERN(client, MiddleOfCAM_Think);
	FIND_PATTERN(server, PlayerRunCommand);
}

void PlayerIOFeature::LoadFeature()
{
	playerioAddressesWereFound = PlayerIOAddressesFound();
	if (utils::DoesGameLookLikeHLS())
	{
		sizeofCUserCmd = 64; // Is missing a CUtlVector
	}
	else
	{
		sizeofCUserCmd = 84;
	}
}

void PlayerIOFeature::UnloadFeature() {}

void PlayerIOFeature::PreHook()
{
	offM_vecAbsVelocity = 0;
	offM_afPhysicsFlags = 0;
	offM_moveCollide = 0;
	offM_moveType = 0;
	offM_collisionGroup = 0;
	offM_vecPunchAngle = 0;
	offM_vecPunchAngleVel = 0;

	offM_pCommands = 0;
	offForwardmove = 0;
	offSidemove = 0;

	offMaxspeed = 0;
	offFlags = 0;
	offAbsVelocity = 0;
	offDucking = 0;
	offDuckJumpTime = 0;
	offServerSurfaceFriction = 0;
	offServerPreviouslyPredictedOrigin = 0;
	offServerAbsOrigin = 0;
	ORIG_GetLocalPlayer = nullptr;

	if (ORIG_PlayerRunCommand)
	{
		int ptnNumber = GetPatternIndex((void**)&ORIG_PlayerRunCommand);
		switch (ptnNumber)
		{
		case 0:
			offM_vecAbsVelocity = 476; // 5135 pattern has some other offsets added
			offM_afPhysicsFlags = 2724;
			offM_moveCollide = 307;
			offM_moveType = 306;
			offM_collisionGroup = 420;
			offM_vecPunchAngle = 2192;
			offM_vecPunchAngleVel = 2200;
			break;
		case 1:
			offM_vecAbsVelocity = 476;
			break;

		case 2:
			offM_vecAbsVelocity = 532;
			break;

		case 3:
			offM_vecAbsVelocity = 532;
			break;

		case 4:
			offM_vecAbsVelocity = 532;
			break;

		case 5:
			offM_vecAbsVelocity = 476;
			break;

		case 6:
			offM_vecAbsVelocity = 532;
			break;

		case 7:
			offM_vecAbsVelocity = 532;
			break;

		case 8:
			offM_vecAbsVelocity = 592;
			break;

		case 9:
			offM_vecAbsVelocity = 556;
			break;

		case 10:
			offM_vecAbsVelocity = 364;
			break;

		case 12:
			offM_vecAbsVelocity = 476;
			break;
		}
	}
	else
	{
		DevWarning("[server dll] Could not find PlayerRunCommand!\n");
		Warning("_y_spt_getvel has no effect.\n");
	}

	if (ORIG_CreateMove)
	{
		int index = GetPatternIndex((void**)&ORIG_CreateMove);

		switch (index)
		{
		case 0:
			offM_pCommands = 180;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 1:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 2:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 3:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 4:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;

		case 5:
			offM_pCommands = 196;
			offForwardmove = 24;
			offSidemove = 28;
			break;
		default:
			offM_pCommands = 0;
			offForwardmove = 0;
			offSidemove = 0;
			break;
		}
	}

	if (ORIG_GetGroundEntity)
	{
		int index = GetPatternIndex((void**)&ORIG_GetGroundEntity);

		switch (index)
		{
		case 0:
			offMaxspeed = 4136;
			offFlags = 736;
			offAbsVelocity = 248;
			offDucking = 3545;
			offDuckJumpTime = 3552;
			offServerSurfaceFriction = 3812;
			offServerPreviouslyPredictedOrigin = 3692;
			offServerAbsOrigin = 580;
			break;

		case 1:
			offMaxspeed = 4076;
			offFlags = 732;
			offAbsVelocity = 244;
			offDucking = 3489;
			offDuckJumpTime = 3496;
			offServerSurfaceFriction = 3752;
			offServerPreviouslyPredictedOrigin = 3628;
			offServerAbsOrigin = 580;
			break;

		case 2:
			offMaxspeed = 4312;
			offFlags = 844;
			offAbsVelocity = 300;
			offDucking = 3713;
			offDuckJumpTime = 3720;
			offServerSurfaceFriction = 3872;
			offServerPreviouslyPredictedOrigin = 3752;
			offServerAbsOrigin = 636;
			break;

		case 3:
			offMaxspeed = 4320;
			offFlags = 844;
			offAbsVelocity = 300;
			offDucking = 3721;
			offDuckJumpTime = 3728;
			offServerSurfaceFriction = 3872;
			offServerPreviouslyPredictedOrigin = 3752;
			offServerAbsOrigin = 636;
			break;
		default:
			offMaxspeed = 0;
			offFlags = 0;
			offAbsVelocity = 0;
			offDucking = 0;
			offDuckJumpTime = 0;
			offServerSurfaceFriction = 0;
			offServerPreviouslyPredictedOrigin = 0;
			offServerAbsOrigin = 0;
			Warning("GetGroundEntity did not contain matching if statement for pattern!\n");
			break;
		}
	}

	if (ORIG_MiddleOfCAM_Think)
	{
		int index = GetPatternIndex((void**)&ORIG_MiddleOfCAM_Think);

		switch (index)
		{
		case 0:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 29) + ORIG_MiddleOfCAM_Think + 33);
			break;

		case 1:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 30) + ORIG_MiddleOfCAM_Think + 34);
			break;

		case 2:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 21) + ORIG_MiddleOfCAM_Think + 25);
			break;

		case 3:
			ORIG_GetLocalPlayer = (_GetLocalPlayer)(
			    *reinterpret_cast<uintptr_t*>(ORIG_MiddleOfCAM_Think + 23) + ORIG_MiddleOfCAM_Think + 27);
			break;
		default:
			ORIG_GetLocalPlayer = nullptr;
			break;
		}

		if (ORIG_GetLocalPlayer)
			DevMsg("[client.dll] Found GetLocalPlayer at %p.\n", ORIG_GetLocalPlayer);
	}
}

Strafe::MovementVars PlayerIOFeature::GetMovementVars()
{
	auto vars = Strafe::MovementVars();

	if (!playerioAddressesWereFound || cinput_thisptr == nullptr)
	{
		return vars;
	}

	auto player = ORIG_GetLocalPlayer();
	auto maxspeed = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offMaxspeed);

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

	auto server_player = utils::GetServerPlayer();

	auto previouslyPredictedOrigin =
	    reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(server_player) + offServerPreviouslyPredictedOrigin);
	auto absOrigin = reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(server_player) + offServerAbsOrigin);
	bool gameCodeMovedPlayer = (*previouslyPredictedOrigin != *absOrigin);

	vars.EntFriction =
	    *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(server_player) + offServerSurfaceFriction);

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
	if ((*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(player) + offDucking)) && GetFlagsDucking())
	{
		vars.CantJump = true;
	}

	auto djt = (*reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offDuckJumpTime));
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
	if (!ORIG_GetLocalPlayer)
		return false;
	return (*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ORIG_GetLocalPlayer()) + offFlags)) & FL_DUCKING;
}

int PlayerIOFeature::GetPlayerFlags()
{
	if (!ORIG_GetLocalPlayer)
		return 0;
	return (*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ORIG_GetLocalPlayer()) + offFlags));
}

Strafe::PlayerData PlayerIOFeature::GetPlayerData()
{
	if (!playerioAddressesWereFound)
		return Strafe::PlayerData();

	Strafe::PlayerData data;
	const int IN_DUCK = 1 << 2;

	data.Ducking = GetFlagsDucking();
	data.DuckPressed = (ORIG_GetButtonBits(cinput_thisptr, 0, 0) & IN_DUCK);
	data.UnduckedOrigin =
	    *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(utils::GetServerPlayer()) + offServerAbsOrigin);
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
	if (!ORIG_GetLocalPlayer)
		return Vector();
	auto player = ORIG_GetLocalPlayer();
	ORIG_CalcAbsoluteVelocity(player, 0);
	float* vel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offAbsVelocity);

	return Vector(vel[0], vel[1], vel[2]);
}

Vector PlayerIOFeature::GetPlayerEyePos()
{
	Vector rval =
	    *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(utils::GetServerPlayer()) + offServerAbsOrigin);
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

bool PlayerIOFeature::IsGroundEntitySet()
{
	if (ORIG_GetGroundEntity == nullptr || ORIG_GetLocalPlayer == nullptr)
		return false;

	auto player = ORIG_GetLocalPlayer();
	return (ORIG_GetGroundEntity(player, 0) != NULL); // TODO: This should really be a proper check.
}

bool PlayerIOFeature::TryJump()
{
	const int IN_JUMP = (1 << 1);
	return ORIG_GetButtonBits(cinput_thisptr, 0, 0) & IN_JUMP;
}

bool PlayerIOFeature::PlayerIOAddressesFound()
{
	return ORIG_GetGroundEntity && ORIG_CreateMove && ORIG_GetButtonBits && ORIG_GetLocalPlayer
	       && ORIG_CalcAbsoluteVelocity && ORIG_MiddleOfCAM_Think && _sv_airaccelerate && _sv_accelerate
	       && _sv_friction && _sv_maxspeed && _sv_stopspeed && interfaces::engine_server != nullptr;
}

void PlayerIOFeature::SetTASInput(float* va, const Strafe::ProcessedFrame& out)
{
	// This bool is set if strafing should occur
	if (out.Processed)
	{
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
	if (!ORIG_GetLocalPlayer)
		return 0;

	auto player = ORIG_GetLocalPlayer();
	return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offDuckJumpTime);
}

int PlayerIOFeature::GetPlayerPhysicsFlags() const
{
	auto player = utils::GetServerPlayer();
	if (!player || offM_afPhysicsFlags == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)utils::GetServerPlayer() + offM_afPhysicsFlags));
}

int PlayerIOFeature::GetPlayerMoveType() const
{
	auto player = utils::GetServerPlayer();
	if (!player || offM_moveType == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)utils::GetServerPlayer() + offM_moveType)) & 0xF;
}

int PlayerIOFeature::GetPlayerMoveCollide() const
{
	auto player = utils::GetServerPlayer();
	if (!player || offM_moveCollide == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)utils::GetServerPlayer() + offM_moveCollide)) & 0x7;
}

int PlayerIOFeature::GetPlayerCollisionGroup() const
{
	auto player = utils::GetServerPlayer();
	if (!player || offM_collisionGroup == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)utils::GetServerPlayer() + offM_collisionGroup));
}

void PlayerIOFeature::Set_cinput_thisptr(void* thisptr)
{
	cinput_thisptr = thisptr;
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

#if SSDK2007
// TODO: remove fixed offsets.

CON_COMMAND(y_spt_find_portals, "Yes")
{
	if (spt_playerio.offServerAbsOrigin == 0)
		return;

	for (int i = 0; i < MAX_EDICTS; ++i)
	{
		auto ent = interfaces::engine_server->PEntityOfEntIndex(i);

		if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal"))
		{
			auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown())
			                                          + spt_playerio.offServerAbsOrigin);

			Msg("SPT: There's a portal with index %d at %.8f %.8f %.8f.\n",
			    i,
			    origin.x,
			    origin.y,
			    origin.z);
		}
	}
}

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

	if (spt_playerio.offServerAbsOrigin == 0)
	{
		Warning("Could not find the required offset in the client DLL.\n");
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
