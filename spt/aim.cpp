#include "stdafx.h"

#include "aim.hpp"

#include "convar.h"
#include "OrangeBox/modules.hpp"
#include "utils/math.hpp"
#include "utils/ent_utils.hpp"
#include "OrangeBox/cvars.hpp"
#include "spt/sptlib-wrapper.hpp"
#include "tier1/checksum_md5.h"

ConVar tas_anglespeed("tas_anglespeed",
                      "5",
                      FCVAR_CHEAT,
                      "Determines the speed of angle changes when using tas_aim or when TAS strafing\n");

namespace aim
{
	float angleChange(float value,
	                  float prevValue,
	                  float target,
	                  float anglespeed,
	                  bool timedChange,
	                  int ticksLeft,
	                  bool yaw,
	                  bool jumpedLastTick)
	{
		float diff;

		if (yaw && jumpedLastTick)
			value = prevValue;

		if (yaw)
		{
			diff = utils::NormalizeDeg(target - value);
		}
		else
		{
			diff = target - value;
		}

		if (!timedChange)
		{
			if (std::abs(diff) < anglespeed)
			{
				return target;
			}
			else
			{
				if (yaw)
					return utils::NormalizeDeg(value + std::copysignf(anglespeed, diff));
				else
					return value + std::copysignf(anglespeed, diff);
			}
		}
		else if (ticksLeft == 1)
		{
			return target;
		}
		else
		{
			if (yaw)
			{
				return utils::NormalizeDeg(value + diff / ticksLeft);
			}
			else
			{
				return value + diff / ticksLeft;
			}
		}
	}

	struct ViewState
	{
		QAngle current;
		QAngle target;

		int ticksLeft;
		bool set;
		bool timedChange;
		bool jumpedLastTick;

		ViewState()
		{
			ticksLeft = 0;
			set = false;
			timedChange = false;
			jumpedLastTick = false;
		}

		float CalculateNewYaw(float newYaw)
		{
			if (!set && tas_strafe.GetBool() && tas_strafe_vectorial.GetBool()
			    && tas_strafe_version.GetInt() >= 4)
			{
				float targetYaw = utils::NormalizeDeg(tas_strafe_yaw.GetFloat()
				                                      + tas_strafe_vectorial_offset.GetFloat());
				current[YAW] = angleChange(newYaw,
				                           current[YAW],
				                           targetYaw,
				                           tas_anglespeed.GetFloat(),
				                           false,
				                           0,
				                           true,
				                           jumpedLastTick);
			}
			else if (set)
			{
				current[YAW] = angleChange(newYaw,
				                           current[YAW],
				                           target[YAW],
				                           tas_anglespeed.GetFloat(),
				                           timedChange,
				                           ticksLeft,
				                           true,
				                           jumpedLastTick);
			}
			else
			{
				current[YAW] = newYaw;
			}

			return current[YAW];
		}

		float CalculateNewPitch(float newPitch)
		{
			if (set)
				current[PITCH] = angleChange(newPitch,
				                             current[PITCH],
				                             target[PITCH],
				                             tas_anglespeed.GetFloat(),
				                             timedChange,
				                             ticksLeft,
				                             false,
				                             jumpedLastTick);
			else
				current[PITCH] = newPitch;
			return current[PITCH];
		}
	};

	static ViewState g_viewState;

	void UpdateView(float& pitch, float& yaw)
	{
		pitch = g_viewState.CalculateNewPitch(pitch);
		yaw = g_viewState.CalculateNewYaw(yaw);

		if (g_viewState.timedChange && g_viewState.ticksLeft > 0)
		{
			--g_viewState.ticksLeft;
			if (g_viewState.ticksLeft == 0)
			{
				g_viewState.timedChange = false;
			}
		}

		g_viewState.jumpedLastTick = false;
	}

	void SetJump()
	{
		g_viewState.jumpedLastTick = true;
	}

	CON_COMMAND(tas_aim_reset, "Resets tas_aim state")
	{
		g_viewState.set = false;
		g_viewState.ticksLeft = 0;
		g_viewState.timedChange = false;
		g_viewState.jumpedLastTick = false;
	}

	CON_COMMAND(tas_aim, "Aims at a point.")
	{
#ifdef OE
		ArgsWrapper args;
#endif

		if (args.ArgC() < 3)
		{
			Msg("Usage: tas_aim <pitch> <yaw> [ticks] [cone]\nWeapon cones(in degrees):\n\t- AR2: 3\n\t- Pistol & SMG: 5\n");
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

		g_viewState.set = true;
		g_viewState.target = aimAngle;

		if (frames == -1)
		{
			g_viewState.timedChange = false;
		}
		else
		{
			g_viewState.timedChange = true;
			g_viewState.ticksLeft = std::max(1, frames);
		}
	}

} // namespace aim