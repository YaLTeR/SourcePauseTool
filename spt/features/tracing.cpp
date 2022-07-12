#include "stdafx.h"
#include "tracing.hpp"
#include "ent_utils.hpp"
#include "game_detection.hpp"
#include "generic.hpp"
#include "hud.hpp"
#include "math.hpp"
#include "interfaces.hpp"
#include "convar.h"
#include "string_utils.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\strafe\strafestuff.hpp"

ConVar y_spt_hud_oob("y_spt_hud_oob", "0", FCVAR_CHEAT, "Is the player OoB?");

Tracing spt_tracing;

namespace patterns
{
	PATTERNS(
	    UTIL_TraceRay,
	    "5135",
	    "8B 44 24 10 8B 4C 24 0C 83 EC 10 56 6A 00 50 51 8D 4C 24 10 E8 ?? ?? ?? ?? 8B 74 24 28 8B 0D ?? ?? ?? ?? 8B 11 8B 52 10",
	    "3420",
	    "8B 44 24 10 8B 4C 24 0C 83 EC 0C 56 50 51 8D 4C 24 0C E8 ?? ?? ?? ?? 8B 74 24 24 8B 0D ?? ?? ?? ?? 8B 11 8B 52 10");
	PATTERNS(CGameMovement__TracePlayerBBox, "5135", "55 8B EC 83 E4 F0 83 EC 5C 56 8B F1 8B 06 8B 50 24");
	PATTERNS(CPortalGameMovement__TracePlayerBBox,
	         "5135",
	         "55 8B EC 83 E4 F0 81 EC C4 00 00 00 53 56 8B F1 8B 46 08 83 C0 04 8B 00");
	PATTERNS(TracePlayerBBoxForGround,
	         "5135",
	         "55 8B EC 83 E4 F0 81 EC 84 00 00 00 53 56 8B 75 24 8B 46 0C D9 46 2C 8B 4E 10");
	PATTERNS(TracePlayerBBoxForGround2,
	         "5135",
	         "55 8B EC 83 E4 F0 8B 4D 18 8B 01 8B 50 08 81 EC 84 00 00 00 53 56 57 FF D2");
	PATTERNS(
	    CGameMovement__GetPlayerMins,
	    "4104",
	    "8B 41 ?? 8B 88 ?? ?? ?? ?? C1 E9 03 F6 C1 01 8B 0D ?? ?? ?? ?? 8B 11 74 09 8B 42 ?? FF D0 83 C0 48 C3");
	PATTERNS(
	    CGameMovement__GetPlayerMaxs,
	    "4104",
	    "8B 41 ?? 8B 88 ?? ?? ?? ?? C1 E9 03 F6 C1 01 8B 0D ?? ?? ?? ?? 8B 11 74 09 8B 42 ?? FF D0 83 C0 54 C3");
	PATTERNS(
	    CEngineTrace__PointOutsideWorld,
	    "5135",
	    "8B 44 24 04 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? C1 E0 04 83 C4 04 66 83 7C 08 04 FF 0F 94 C0 C2 04 00");

} // namespace patterns

void Tracing::InitHooks()
{
	FIND_PATTERN(client, UTIL_TraceRay);
	FIND_PATTERN(server, CGameMovement__TracePlayerBBox);
	FIND_PATTERN(server, CPortalGameMovement__TracePlayerBBox);
	FIND_PATTERN(server, TracePlayerBBoxForGround);
	FIND_PATTERN(server, TracePlayerBBoxForGround2);
	HOOK_FUNCTION(server, CGameMovement__GetPlayerMaxs);
	HOOK_FUNCTION(server, CGameMovement__GetPlayerMins);
#ifdef SSDK2007
	if (utils::DoesGameLookLikePortal())
	{
		AddOffsetHook("server", 0xCCE90, "GetActiveWeapon", reinterpret_cast<void**>(&ORIG_GetActiveWeapon));
		AddOffsetHook("server", 0x441730, "TraceFirePortal", reinterpret_cast<void**>(&ORIG_TraceFirePortal));
		FIND_PATTERN(engine, CEngineTrace__PointOutsideWorld);
	}
#endif
}

bool Tracing::TraceClientRay(const Ray_t& ray,
                             unsigned int mask,
                             const IHandleEntity* ignore,
                             int collisionGroup,
                             trace_t* ptr)
{
	if (!ORIG_UTIL_TraceRay)
		return false;
	ORIG_UTIL_TraceRay(ray, mask, ignore, collisionGroup, ptr);
	return true;
}

bool Tracing::CanTracePlayerBBox()
{
	if (utils::DoesGameLookLikePortal())
	{
		return interfaces::gm != nullptr && ORIG_TracePlayerBBoxForGround2
		       && ORIG_CGameMovement__TracePlayerBBox && ORIG_CGameMovement__GetPlayerMaxs
		       && ORIG_CGameMovement__GetPlayerMins;
	}
	else
	{
		return interfaces::gm != nullptr && ORIG_TracePlayerBBoxForGround && ORIG_CGameMovement__TracePlayerBBox
		       && ORIG_CGameMovement__GetPlayerMaxs && ORIG_CGameMovement__GetPlayerMins;
	}
}

void Tracing::TracePlayerBBox(const Vector& start,
                              const Vector& end,
                              const Vector& mins,
                              const Vector& maxs,
                              unsigned int fMask,
                              int collisionGroup,
                              trace_t& pm)
{
	overrideMinMax = true;
	_mins = mins;
	_maxs = maxs;

	if (utils::DoesGameLookLikePortal())
		ORIG_CPortalGameMovement__TracePlayerBBox(interfaces::gm, 0, start, end, fMask, collisionGroup, pm);
	else
		ORIG_CGameMovement__TracePlayerBBox(interfaces::gm, 0, start, end, fMask, collisionGroup, pm);
	overrideMinMax = false;
}

float Tracing::TraceFirePortal(trace_t& tr, const Vector& startPos, const Vector& vDirection)
{
	auto weapon = ORIG_GetActiveWeapon(utils::GetServerPlayer());

	if (!weapon)
	{
		tr.fraction = 1.0f;
		return 0;
	}

	const int PORTAL_PLACED_BY_PLAYER = 2;
	Vector vFinalPosition;
	QAngle qFinalAngles;

	return ORIG_TraceFirePortal(
	    weapon, 0, false, startPos, vDirection, tr, vFinalPosition, qFinalAngles, PORTAL_PLACED_BY_PLAYER, true);
}

bool Tracing::ShouldLoadFeature()
{
#if defined(SSDK2007) || defined(SSDK2013)
	return true;
#else
	return false;
#endif
}

void Tracing::UnloadFeature() {}

const Vector& __fastcall Tracing::HOOKED_CGameMovement__GetPlayerMaxs(void* thisptr, int edx)
{
	if (spt_tracing.overrideMinMax)
	{
		return spt_tracing._maxs;
	}
	else
		return spt_tracing.ORIG_CGameMovement__GetPlayerMaxs(thisptr, edx);
}

const Vector& __fastcall Tracing::HOOKED_CGameMovement__GetPlayerMins(void* thisptr, int edx)
{
	if (spt_tracing.overrideMinMax)
	{
		return spt_tracing._mins;
	}
	else
		return spt_tracing.ORIG_CGameMovement__GetPlayerMins(thisptr, edx);
}

#ifdef SSDK2007
double trace_fire_portal(QAngle angles, Vector& normal)
{
	Vector fwd;
	AngleVectors(angles, &fwd);
	trace_t tr;
	spt_tracing.TraceFirePortal(tr, spt_generic.GetCameraOrigin(), fwd);

	if (tr.DidHit())
	{
		normal = tr.plane.normal;
	}

	return (tr.endpos - tr.startpos).LengthSqr();
}

static QAngle firstAngle;
static bool firstInvocation = true;

CON_COMMAND(
    y_spt_find_seam_shot,
    "y_spt_find_seam_shot [<pitch1> <yaw1> <pitch2> <yaw2> <epsilon>] - tries to find a seam shot on a \"line\" between viewangles (pitch1; yaw1) and (pitch2; yaw2) with binary search. Decreasing epsilon will result in more viewangles checked. A default value is 0.00001. If no arguments are given, first invocation selects the first point, second invocation selects the second point and searches between them.")
{
	QAngle a, b;
	double eps = 0.00001 * 0.00001;

	if (args.ArgC() == 1)
	{
		if (firstInvocation)
		{
			interfaces::engine->GetViewAngles(firstAngle);
			firstInvocation = !firstInvocation;

			Msg("First point set.\n");
			return;
		}
		else
		{
			firstInvocation = !firstInvocation;

			a = firstAngle;
			interfaces::engine->GetViewAngles(b);
		}
	}
	else
	{
		if (args.ArgC() != 5 && args.ArgC() != 6)
		{
			Msg("Usage: y_spt_find_seam_shot <pitch1> <yaw1> <pitch2> <yaw2> <epsilon> - tries to find a seam shot on a \"line\" between viewangles (pitch1; yaw1) and (pitch2; yaw2) with binary search. Decreasing epsilon will result in more viewangles checked. A default value is 0.00001. If no arguments are given, first invocation selects the first point, second invocation selects the second point and searches between them.\n");
			return;
		}

		a = QAngle(atof(args.Arg(1)), atof(args.Arg(2)), 0);
		b = QAngle(atof(args.Arg(3)), atof(args.Arg(4)), 0);
		eps = (args.ArgC() == 5) ? eps : std::pow(atof(args.Arg(5)), 2);
	}

	if (!spt_tracing.ORIG_GetActiveWeapon(utils::GetServerPlayer()))
	{
		Msg("You need to be holding a portal gun.\n");
		return;
	}

	Vector a_normal;
	const auto distance = std::max(trace_fire_portal(a, a_normal), trace_fire_portal(b, Vector()));

	// If our trace had a distance greater than the a or b distance by this amount, treat it as a seam shot.
	constexpr double GOOD_DISTANCE_DIFFERENCE = 50.0 * 50.0;

	QAngle test;
	utils::GetMiddlePoint(a, b, test);
	double difference;

	do
	{
		Vector test_normal;

		if (trace_fire_portal(test, test_normal) - distance > GOOD_DISTANCE_DIFFERENCE)
		{
			Msg("Found a seam shot at setang %.8f %.8f 0\n", test.x, test.y);
			interfaces::engine->SetViewAngles(test);
			return;
		}

		if (test_normal == a_normal)
		{
			a = test;
			utils::GetMiddlePoint(a, b, test);
			difference = (test - a).LengthSqr();
		}
		else
		{
			b = test;
			utils::GetMiddlePoint(a, b, test);
			difference = (test - b).LengthSqr();
		}

		Msg("Difference: %.8f\n", std::sqrt(difference));
	} while (difference > eps);

	Msg("Could not find a seam shot. Best guess: setang %.8f %.8f 0\n", test.x, test.y);
	interfaces::engine->SetViewAngles(test);
}
#endif

void Tracing::LoadFeature()
{
	if (!ORIG_UTIL_TraceRay)
		Warning("tas_strafe_version 1 not available\n");

	if (!CanTracePlayerBBox())
		Warning("tas_strafe_version 2 not available\n");

#ifdef SSDK2007
	if (utils::DoesGameLookLikePortal())
	{
		InitCommand(
		    y_spt_find_seam_shot); // 5135 only but version detection doesnt properly exist, so cba to fix this
	}
#endif

#if defined(SSDK2007)
	if (ORIG_CEngineTrace__PointOutsideWorld)
	{
		AddHudCallback(
		    "oob",
		    []()
		    {
			    Vector v = spt_generic.GetCameraOrigin();
			    trace_t tr;
			    Strafe::Trace(tr, v, v + Vector(1, 1, 1));
			    int oob = spt_tracing.ORIG_CEngineTrace__PointOutsideWorld(nullptr, 0, v) && !tr.startsolid;
			    spt_hud.DrawTopHudElement(L"oob: %d", oob);
		    },
		    y_spt_hud_oob);
	}
#endif
}
