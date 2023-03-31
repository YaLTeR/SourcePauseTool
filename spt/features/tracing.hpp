#pragma once
#include "..\feature.hpp"

#if defined(SSDK2013) || defined(SSDK2007)
#define SPT_TRACE_PORTAL_ENABLED
#endif

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif
#include "cmodel.h"

class CBaseCombatWeapon;
class IGameMovement;
#include "iserverunknown.h"

// Tracing related functionality
class Tracing : public FeatureWrapper<Tracing>
{
public:
	DECL_MEMBER_CDECL(void,
	                  UTIL_TraceRay,
	                  const Ray_t& ray,
	                  unsigned int mask,
	                  const IHandleEntity* ignore,
	                  int collisionGroup,
	                  trace_t* ptr);

	DECL_MEMBER_CDECL(void,
	                  TracePlayerBBoxForGround,
	                  const Vector& start,
	                  const Vector& end,
	                  const Vector& mins,
	                  const Vector& maxs,
	                  IHandleEntity* player,
	                  unsigned int fMask,
	                  int collisionGroup,
	                  trace_t& pm);

	DECL_MEMBER_CDECL(void,
	                  TracePlayerBBoxForGround2,
	                  const Vector& start,
	                  const Vector& end,
	                  const Vector& mins,
	                  const Vector& maxs,
	                  IHandleEntity* player,
	                  unsigned int fMask,
	                  int collisionGroup,
	                  trace_t& pm);

	DECL_HOOK_THISCALL(CBaseCombatWeapon*, GetActiveWeapon, IServerUnknown*);

	DECL_MEMBER_THISCALL(float,
	                     TraceFirePortal,
	                     CBaseCombatWeapon*,
	                     bool bPortal2,
	                     const Vector& vTraceStart,
	                     const Vector& vDirection,
	                     trace_t& tr,
	                     Vector& vFinalPosition,
	                     QAngle& qFinalAngles,
	                     int iPlacedBy,
	                     bool bTest);

	bool CanTracePlayerBBox();

	void TracePlayerBBox(const Vector& start,
	                     const Vector& end,
	                     const Vector& mins,
	                     const Vector& maxs,
	                     unsigned int fMask,
	                     int collisionGroup,
	                     trace_t& pm);

#ifdef SPT_TRACE_PORTAL_ENABLED
	CBaseCombatWeapon* GetActiveWeapon();
	float TraceFirePortal(trace_t& tr, const Vector& startPos, const Vector& vDirection);

	// Transform through portal if in portal enviroment
	float TraceTransformFirePortal(trace_t& tr, const Vector& startPos, const QAngle& startAngles);
	float TraceTransformFirePortal(trace_t& tr,
	                               const Vector& startPos,
	                               const QAngle& startAngles,
	                               Vector& finalPos,
	                               QAngle& finalAngles,
	                               bool isPortal2);

	// returns the server-side filter instance which can be used for TraceFirePortal-like shenanigans
	class ITraceFilter* GetPortalTraceFilter();
#endif

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	bool overrideMinMax = false;
	Vector _mins;
	Vector _maxs;

	DECL_MEMBER_THISCALL(void,
	                     CGameMovement__TracePlayerBBox,
	                     IGameMovement*,
	                     const Vector& start,
	                     const Vector& end,
	                     unsigned int fMask,
	                     int collisionGroup,
	                     trace_t& pm);

	DECL_MEMBER_THISCALL(void,
	                     CPortalGameMovement__TracePlayerBBox,
	                     IGameMovement*,
	                     const Vector& start,
	                     const Vector& end,
	                     unsigned int fMask,
	                     int collisionGroup,
	                     trace_t& pm);

	DECL_HOOK_THISCALL(const Vector&, CGameMovement__GetPlayerMins, IGameMovement*);
	DECL_HOOK_THISCALL(const Vector&, CGameMovement__GetPlayerMaxs, IGameMovement*);
};

extern Tracing spt_tracing;
