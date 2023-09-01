#include "stdafx.hpp"
#include "..\cvars.hpp"
#include "aim.hpp"
#include "generic.hpp"
#include "ent_utils.hpp"
#include "math.hpp"
#include "playerio.hpp"
#include <deque>

#undef min
#undef max

ConVar _y_spt_anglesetspeed(
    "_y_spt_anglesetspeed",
    "360",
    FCVAR_TAS_RESET,
    "Determines how fast the view angle can move per tick while doing spt_setyaw/spt_setpitch.\n");
ConVar _y_spt_pitchspeed("_y_spt_pitchspeed", "0", FCVAR_TAS_RESET);
ConVar _y_spt_yawspeed("_y_spt_yawspeed", "0", FCVAR_TAS_RESET);
ConVar spt_angchange_queuing_behavior("spt_angchanging_queuing_behavior", 
                                    "0", 
                                    FCVAR_TAS_RESET, 
                                    "Determines how SPT processes the angle change command queue.\n"
                                    "0: do changes one by one.\n"
                                    "1: stop ongoing changes immediately, then begin new ones.\n"
                                    "2: snap to resultant angles of old change, then begin new ones.\n");
ConVar spt_angchange_limitpitch("spt_angchange_limitpitch", 
                                "1",
                                FCVAR_TAS_RESET,
                                "Limits pitch changes to stay within cl_pitchup and cl_pitchdown values.");

ConVar tas_anglespeed("tas_anglespeed",
                      "5",
                      FCVAR_CHEAT,
                      "Determines the speed of angle changes when using spt_tas_aim or when TAS strafing\n");

AimFeature spt_aim;

void AimFeature::HandleAiming(float* va, bool& yawChanged, const Strafe::StrafeInput& input)
{
    if (viewState.state == aim::ViewState::AimState::POSITION
        || viewState.state == aim::ViewState::AimState::ENTITY)
    {
        Vector currentPos = utils::GetPlayerEyePosition();
        Vector targetPos;
        if (viewState.state == aim::ViewState::AimState::POSITION)
        {
            targetPos = viewState.targetPos;
        }
        else
        {
            IClientEntity* ent = utils::GetClientEntity(viewState.targetID);
            if (!ent)
            {
                spt_aim.viewState.state = aim::ViewState::AimState::NO_AIM;
                return;
            }
            Vector offsets = spt_aim.viewState.targetPos;
            Vector forward, right, up;
            AngleVectors(ent->GetAbsAngles(), &forward, &right, &up);
            targetPos = ent->GetAbsOrigin() + offsets.x * forward - offsets.y * right + offsets.z * up;
        }
        VectorAngles(targetPos - currentPos, viewState.target);
        if (viewState.target.x > 90.0f)
        {
            viewState.target.x -= 360.0f;
        }
        viewState.target.x = clamp(viewState.target.x, -89, 89);
        viewState.target.y = utils::NormalizeDeg(viewState.target.y);
        viewState.target.z = 0.0f;
    }

    float oldYaw = va[YAW];

    // Use spt_tas_aim stuff for spt_tas_strafe_version >= 4
    if (input.Version >= 4 && input.Version <= 5)
    {
        spt_aim.viewState.UpdateView(va[PITCH], va[YAW], input);
    }

    double pitchSpeed = atof(_y_spt_pitchspeed.GetString()), yawSpeed = atof(_y_spt_yawspeed.GetString());

    if (pitchSpeed != 0.0f)
    {
        va[PITCH] += pitchSpeed;
    }
    if (yawSpeed != 0.0f)
    {
        va[YAW] += yawSpeed;
    }

    while (!angChanges.empty())
    {
        while (angChanges.size() > 1)
        {
            switch (spt_angchange_queuing_behavior.GetInt())
            {
            case 1:  
                angChanges.pop_front();
                continue;
            case 2:
            {
                angchange_command_t current = angChanges.front();
                int component = current.component;
                if (component == YAW) yawChanged = true;

                switch (current.type)
                {
                case SET:
                    va[component] = current.value;
                    break;
                case TURN:
                    va[component] += current.value;
                    break;
                }
                if (component == PITCH && spt_angchange_limitpitch.GetBool())
                    va[component] = BoundPitch(va[component]);

                angChanges.pop_front();
                continue;
            }
            default:
                goto normal;
            }
        }

        normal:

        auto current = angChanges.begin();
        auto component = current->component;
        if (component == YAW) yawChanged = true;

        bool left = true;
        switch (current->type)
        {
        case SET:
            left = DoAngleChange(va[component], current->value);
            break;
        case TURN:
            current->value = DoAngleTurn(va[component], current->value);
            left = std::abs(current->value) > 0;
            break;
        }
        if (component == PITCH && spt_angchange_limitpitch.GetBool() && !IsPitchWithinLimits(va[component]))
        {
            va[component] = BoundPitch(va[component]);
            left = false;
        }
        
        if (!left) angChanges.pop_front();

        break;
    }


    // Fix the case where anglespeed is 0
    if (input.Version >= 6)
    {
        yawChanged = va[YAW] != oldYaw;
    }

    // We only want to do this in case yaw didn't change, bug fix from earlier that resulted in a fight to the death
    // between spt_setyaw and spt_tas_strafe_vectorial
    if (input.Version >= 6 && !yawChanged)
    {
        spt_aim.viewState.UpdateView(va[PITCH], va[YAW], input);
    }
}

bool AimFeature::IsPitchWithinLimits(float pitch)
{
    if (cl_pitchup == nullptr && cl_pitchdown == nullptr) return true;

    float upperLimit = cl_pitchup == nullptr ? 180 : cl_pitchup->GetFloat();
    float lowerLimit = cl_pitchdown == nullptr ? 180 : cl_pitchdown->GetFloat();
    return pitch <= lowerLimit && pitch >= -upperLimit;
}

float AimFeature::BoundPitch(float pitch)
{
    if (cl_pitchup == nullptr && cl_pitchdown == nullptr) return pitch;

    float upperLimit = cl_pitchup == nullptr ? 180 : cl_pitchup->GetFloat();
    float lowerLimit = cl_pitchdown == nullptr ? 180 : cl_pitchdown->GetFloat();
    if (pitch > lowerLimit)
    {
        pitch = lowerLimit;
    }
    if (pitch < -upperLimit)
    {
        pitch = -upperLimit;
    }
    return pitch;
}

bool AimFeature::DoAngleChange(float& angle, float target)
{
    float normalizedDiff = utils::NormalizeDeg(target - angle);
    if (std::abs(normalizedDiff) > _y_spt_anglesetspeed.GetFloat())
    {
        angle += std::copysign(_y_spt_anglesetspeed.GetFloat(), normalizedDiff);
        return true;
    }
    else
    {
        angle = target;
        return false;
    }
}
float AimFeature::DoAngleTurn(float& angle, float left)
{
    float add = MIN(std::abs(left), _y_spt_anglesetspeed.GetFloat());
    add = std::copysign(add, left);
    angle += add;
    return left - add;
}

void AimFeature::AddChange(angchange_command_t change)
{
    angChanges.push_back(change);
}

void AimFeature::SetPitch(float pitch)
{
    angchange_command_t set;
    set.value = pitch;
    set.component = PITCH;
    set.type = SET;
    AddChange(set);

}
void AimFeature::TurnPitch(float add)
{
    angchange_command_t turn;
    turn.value = add;
    turn.component = PITCH;
    turn.type = TURN;
    AddChange(turn);
}

void AimFeature::SetYaw(float yaw)
{
    angchange_command_t set;
    set.value = yaw;
    set.component = YAW;
    set.type = SET;
    AddChange(set);
}
void AimFeature::TurnYaw(float add)
{
    angchange_command_t turn;
    turn.value = add;
    turn.component = YAW;
    turn.type = TURN;
    AddChange(turn);
}
void AimFeature::ResetPitchYawCommands()
{
    angChanges.clear();
}

void AimFeature::SetJump()
{
    viewState.SetJump();
}

CON_COMMAND(tas_aim_reset, "Resets spt_tas_aim state")
{
    spt_aim.viewState.state = aim::ViewState::AimState::NO_AIM;
    spt_aim.viewState.ticksLeft = 0;
    spt_aim.viewState.timedChange = false;
    spt_aim.viewState.jumpedLastTick = false;
}

CON_COMMAND(tas_aim, "Aims at an angle")
{
    if (args.ArgC() < 3)
    {
        Msg("Usage: spt_tas_aim <pitch> <yaw> [ticks] [cone]\nWeapon cones(in degrees):\n\t- AR2: 3\n\t- Pistol & SMG: 5\n");
        return;
    }

    float pitch = clamp(std::atof(args.Arg(1)), -89, 89);
    float yaw = utils::NormalizeDeg(std::atof(args.Arg(2)));
    int frames = -1;
    int cone = -1;

    if (args.ArgC() >= 4)
        frames = std::atoi(args.Arg(3));

    if (args.ArgC() >= 5)
        cone = std::atoi(args.Arg(4));

    QAngle angle(pitch, yaw, 0);
    QAngle aimAngle = angle;

    if (cone >= 0)
    {
        if (!utils::playerEntityAvailable())
        {
            Warning(
                "Trying to apply nospread while map not loaded in! Wait until map is loaded before issuing spt_tas_aim with spread cone set.\n");
            return;
        }

        Vector vecSpread;
        if (!aim::GetCone(cone, vecSpread))
        {
            Warning("Couldn't find cone: %s\n", args.Arg(4));
            return;
        }

        // Even the first approximation seems to be relatively accurate and it seems to converge after 2nd iteration
        for (int i = 0; i < 2; ++i)
            aim::GetAimAngleIterative(angle, aimAngle, frames, vecSpread);

        QAngle punchAngle, punchAnglevel;

        if (utils::GetPunchAngleInformation(punchAngle, punchAnglevel))
        {
            QAngle futurePunchAngle = aim::DecayPunchAngle(punchAngle, punchAnglevel, frames);
            aimAngle -= futurePunchAngle;
            aimAngle[PITCH] = clamp(aimAngle[PITCH], -89, 89);
        }
    }

    spt_aim.viewState.state = aim::ViewState::AimState::ANGLES;
    spt_aim.viewState.target = aimAngle;

    if (frames == -1)
    {
        spt_aim.viewState.timedChange = false;
    }
    else
    {
        spt_aim.viewState.timedChange = true;
        spt_aim.viewState.ticksLeft = std::max(1, frames);
    }
}

CON_COMMAND(tas_aim_pos, "Aims at a position")
{
    int argc = args.ArgC();
    if (argc != 4 && argc != 5)
    {
        Msg("Usage: spt_tas_aim_pos <x> <y> <z> [ticks]\n");
        return;
    }

    int frames = -1;
    if (argc == 5)
        frames = std::atoi(args.Arg(4));

    spt_aim.viewState.state = aim::ViewState::AimState::POSITION;
    spt_aim.viewState.targetPos = Vector(std::atof(args.Arg(1)), std::atof(args.Arg(2)), std::atof(args.Arg(3)));

    if (frames == -1)
    {
        spt_aim.viewState.timedChange = false;
    }
    else
    {
        spt_aim.viewState.timedChange = true;
        spt_aim.viewState.ticksLeft = std::max(1, frames);
    }
}

CON_COMMAND(tas_aim_ent, "Aim at the absolute origin of a entity with offsets (optional)")
{
    int argc = args.ArgC();
    if (argc != 2 && argc != 3 && argc != 5 && argc != 6)
    {
        Msg("Usage: spt_tas_aim_ent <id> [x y z] [ticks]\n");
        return;
    }

    int frames = -1;
    if (argc == 3 || argc == 6)
    {
        frames = std::atoi(args.Arg(argc - 1));
    }
    if (argc >= 5)
        spt_aim.viewState.targetPos =
            Vector(std::atof(args.Arg(2)), std::atof(args.Arg(3)), std::atof(args.Arg(4)));
    else
        spt_aim.viewState.targetPos = Vector(0.0f, 0.0f, 0.0f);

    spt_aim.viewState.state = aim::ViewState::AimState::ENTITY;
    spt_aim.viewState.targetID = std::atoi(args.Arg(1));

    if (frames == -1)
    {
        spt_aim.viewState.timedChange = false;
    }
    else
    {
        spt_aim.viewState.timedChange = true;
        spt_aim.viewState.ticksLeft = std::max(1, frames);
    }
}

CON_COMMAND(_y_spt_setpitch, "Sets the pitch")
{
    if (args.ArgC() != 2)
    {
        Msg("Usage: spt_setpitch <pitch>\n");
        return;
    }

    spt_aim.SetPitch(atof(args.Arg(1)));
}

CON_COMMAND(spt_turnpitch, "Turns a number of degrees of pitch (up and down).")
{
    if (args.ArgC() != 2)
    {
        Msg("Usage: spt_turnpitch <degrees>\n");
        return;
    }

    spt_aim.TurnPitch(atof(args.Arg(1)));
}

CON_COMMAND(_y_spt_setyaw, "Sets the yaw")
{
    if (args.ArgC() != 2)
    {
        Msg("Usage: spt_setyaw <yaw>\n");
        return;
    }

    spt_aim.SetYaw(atof(args.Arg(1)));
}

CON_COMMAND(spt_turnyaw, "Turns a number of degrees of yaw (left and right).")
{
    if (args.ArgC() != 2)
    {
        Msg("Usage: spt_turnyaw <degrees>\n");
        return;
    }

    spt_aim.TurnYaw(atof(args.Arg(1)));
}

CON_COMMAND(_y_spt_resetpitchyaw, "Resets pitch/yaw commands.")
{
    spt_aim.ResetPitchYawCommands();
}


CON_COMMAND(_y_spt_setangles, "Sets the angles")
{
    if (args.ArgC() != 3)
    {
        Msg("Usage: spt_setangles <pitch> <yaw>\n");
        return;
    }

    spt_aim.SetPitch(atof(args.Arg(1)));
    spt_aim.SetYaw(atof(args.Arg(2)));
}

CON_COMMAND(spt_turnangles, "Turns a number of degrees of pitch (down and up), and yaw (left and right).")
{
    if (args.ArgC() != 3)
    {
        Msg("Usage: spt_turnangles <degrees of pitch> <degrees of yaw>\n");
        return;
    }

    spt_aim.TurnPitch(atof(args.Arg(1)));
    spt_aim.TurnYaw(atof(args.Arg(2)));
}

CON_COMMAND(_y_spt_setangle,
            "Sets the yaw/pitch angle required to look at the given position from player's current position.")
{
    Vector target;

    if (args.ArgC() > 3)
    {
        target.x = atof(args.Arg(1));
        target.y = atof(args.Arg(2));
        target.z = atof(args.Arg(3));

        Vector player_origin = spt_playerio.GetPlayerEyePos();
        Vector diff = (target - player_origin);
        QAngle angles;
        VectorAngles(diff, angles);
        spt_aim.SetPitch(angles[PITCH]);
        spt_aim.SetYaw(angles[YAW]);
    }
}

void AimFeature::LoadFeature()
{
    if (spt_generic.ORIG_ControllerMove && spt_playerio.ORIG_CreateMove)
    {
        InitCommand(tas_aim_reset);
        InitCommand(tas_aim);
        InitCommand(tas_aim_pos);
        InitCommand(tas_aim_ent);
        InitCommand(_y_spt_setyaw);
        InitCommand(spt_turnyaw);
        InitCommand(_y_spt_setpitch);
        InitCommand(spt_turnpitch);
        InitCommand(_y_spt_resetpitchyaw);
        InitCommand(_y_spt_setangles);
        InitCommand(spt_turnangles);
        InitCommand(_y_spt_setangle);

        InitConcommandBase(_y_spt_anglesetspeed);
        InitConcommandBase(_y_spt_pitchspeed);
        InitConcommandBase(_y_spt_yawspeed);
        InitConcommandBase(tas_anglespeed);
        InitConcommandBase(spt_angchange_queuing_behavior);

        cl_pitchup = g_pCVar->FindVar("cl_pitchup");
        cl_pitchdown = g_pCVar->FindVar("cl_pitchdown");
        if (cl_pitchdown != nullptr && cl_pitchup != nullptr)
        {
            InitConcommandBase(spt_angchange_limitpitch);
        }
    }
}