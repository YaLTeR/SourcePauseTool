#pragma once
#include "..\feature.hpp"

#if defined(SSDK2013) || defined(SSDK2007)
#define SPT_TRACE_PORTAL_ENABLED
#endif

#include "ent_props.hpp"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif
#include "cmodel.h"

class CBaseCombatWeapon;
class IGameMovement;
class ITraceFilter;
#include "iserverunknown.h"
#include "engine\IEngineTrace.h"

// Since we're hooking engine-side tracing functions, when this includes dispcoll_common.h we need it to use the
// CUtlVector<T, CHunkMemory<T>> type to be accurate. This allows us to use the CDispCollTree class from the SDK.
#pragma push_macro("ENGINE_DLL")
#define ENGINE_DLL
#include "SDK\cmodel_private.h"
#pragma pop_macro("ENGINE_DLL")

// for TraceLineWithWorldInfoServer()
struct WorldHitInfo
{
	const CCollisionBSPData* bspData;

	const cbrush_t* brush;
	ICollideable* staticProp;
	CDispCollTree* dispTree;

	void Clear()
	{
		bspData = nullptr;
		brush = nullptr;
		staticProp = nullptr;
		dispTree = nullptr;
	}
};

template<bool server>
struct TraceFilterIgnorePlayer : public CTraceFilter
{
	virtual bool ShouldHitEntity(IHandleEntity* pEntity, int contentsMask) override
	{
		return pEntity != spt_entprops.GetPlayer(server);
	}
};

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

	// Traces a line, returns more detailed info if the world was hit. hitInfo.bspData is only guaranteed to be
	// set if a brush or displacement was hit. If hooks aren't found this may return incorrect results.
	WorldHitInfo TraceLineWithWorldInfoServer(const Ray_t& ray,
	                                          unsigned int fMask,
	                                          ITraceFilter* filter,
	                                          trace_t& tr);

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
	ITraceFilter* GetPortalTraceFilter();
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

	// for TraceLineWithWorldInfoServer()
	struct
	{
		float bestFrac;
		WorldHitInfo hitInfo;
	} curTraceInfo;

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

	DECL_HOOK_FASTCALL(void,
	                   CM_ClipBoxToBrush_1,
	                   TraceInfo_t* __restrict pTraceInfo,
	                   const cbrush_t* __restrict brush);

	DECL_HOOK_FASTCALL(void,
	                   CM_TraceToDispTree_1,
	                   TraceInfo_t* pTraceInfo,
	                   CDispCollTree* pDispTree,
	                   float startFrac,
	                   float endFrac);
};

extern Tracing spt_tracing;
