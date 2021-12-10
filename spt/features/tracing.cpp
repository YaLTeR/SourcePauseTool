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
#include "..\strafe\strafestuff.hpp"

ConVar y_spt_hud_oob("y_spt_hud_oob", "0", FCVAR_CHEAT, "Is the player OoB?");
Tracing spt_tracing;

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

void Tracing::InitHooks()
{
	FIND_PATTERN(client, UTIL_TraceRay);
	FIND_PATTERN(server, CGameMovement__TracePlayerBBox);
	FIND_PATTERN(server, CPortalGameMovement__TracePlayerBBox);
	FIND_PATTERN(server, TracePlayerBBoxForGround);
	FIND_PATTERN(server, TracePlayerBBoxForGround2);
	FIND_PATTERN(server, CGameMovement__TracePlayerBBox);
	FIND_PATTERN(server, CGameMovement__TracePlayerBBox);
	FIND_PATTERN(server, CGameMovement__TracePlayerBBox);
	HOOK_FUNCTION(server, CGameMovement__GetPlayerMaxs);
	HOOK_FUNCTION(server, CGameMovement__GetPlayerMins);
#ifdef SSDK2007
	if (utils::DoesGameLookLikePortal())
	{
		AddOffsetHook("server", 0x442090, "FirePortal", reinterpret_cast<void**>(&ORIG_FirePortal));
		AddOffsetHook("server", 0x1B92F0, "SnapEyeAngles", reinterpret_cast<void**>(&ORIG_SnapEyeAngles));
		AddOffsetHook("server", 0xCCE90, "GetActiveWeapon", reinterpret_cast<void**>(&ORIG_GetActiveWeapon));
		AddOffsetHook("server",
		              0x441730,
		              "TraceFirePortal",
		              reinterpret_cast<void**>(&ORIG_TraceFirePortal),
		              HOOKED_TraceFirePortal);
		FIND_PATTERN(engine, CEngineTrace__PointOutsideWorld);
	}
#endif
}

void Tracing::UnloadFeature() {}

float __fastcall Tracing::HOOKED_TraceFirePortal(void* thisptr,
                                                 int edx,
                                                 bool bPortal2,
                                                 const Vector& vTraceStart,
                                                 const Vector& vDirection,
                                                 trace_t& tr,
                                                 Vector& vFinalPosition,
                                                 QAngle& qFinalAngles,
                                                 int iPlacedBy,
                                                 bool bTest)
{
	const auto rv = spt_tracing.ORIG_TraceFirePortal(
	    thisptr, edx, bPortal2, vTraceStart, vDirection, tr, vFinalPosition, qFinalAngles, iPlacedBy, bTest);

	spt_tracing.lastPortalTrace = tr;

	return rv;
}

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
void setang_exact(const QAngle& angles)
{
	auto player = utils::GetServerPlayer();
	auto teleport = reinterpret_cast<void(__fastcall*)(void*, int, const Vector*, const QAngle*, const Vector*)>(
	    (*reinterpret_cast<uintptr_t**>(player))[105]);

	teleport(player, 0, nullptr, &angles, nullptr);
	spt_tracing.ORIG_SnapEyeAngles(player, 0, angles);
}

// Trace as if we were firing a Portal with the given viewangles, return the squared distance to the resulting point and the normal.
double trace_fire_portal(QAngle angles, Vector& normal)
{
	setang_exact(angles);

	spt_tracing.ORIG_FirePortal(spt_tracing.ORIG_GetActiveWeapon(utils::GetServerPlayer()),
	                            0,
	                            false,
	                            nullptr,
	                            true);

	normal = spt_tracing.lastPortalTrace.plane.normal;
	return (spt_tracing.lastPortalTrace.endpos - spt_tracing.lastPortalTrace.startpos).LengthSqr();
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
		    []() {
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