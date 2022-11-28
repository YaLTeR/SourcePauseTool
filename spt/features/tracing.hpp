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

typedef void(__cdecl* _UTIL_TraceRay)(const Ray_t& ray,
                                      unsigned int mask,
                                      const IHandleEntity* ignore,
                                      int collisionGroup,
                                      trace_t* ptr);
typedef void(__cdecl* _TracePlayerBBoxForGround)(const Vector& start,
                                                 const Vector& end,
                                                 const Vector& mins,
                                                 const Vector& maxs,
                                                 IHandleEntity* player,
                                                 unsigned int fMask,
                                                 int collisionGroup,
                                                 trace_t& pm);
typedef void(__cdecl* _TracePlayerBBoxForGround2)(const Vector& start,
                                                  const Vector& end,
                                                  const Vector& mins,
                                                  const Vector& maxs,
                                                  IHandleEntity* player,
                                                  unsigned int fMask,
                                                  int collisionGroup,
                                                  trace_t& pm);
typedef void(__fastcall* _CGameMovement__TracePlayerBBox)(void* thisptr,
                                                          int edx,
                                                          const Vector& start,
                                                          const Vector& end,
                                                          unsigned int fMask,
                                                          int collisionGroup,
                                                          trace_t& pm);
typedef void(__fastcall* _CPortalGameMovement__TracePlayerBBox)(void* thisptr,
                                                                int edx,
                                                                const Vector& start,
                                                                const Vector& end,
                                                                unsigned int fMask,
                                                                int collisionGroup,
                                                                trace_t& pm);
typedef void(__fastcall* _SnapEyeAngles)(void* thisptr, int edx, const QAngle& viewAngles);
typedef float(__fastcall* _FirePortal)(void* thisptr, int edx, bool bPortal2, Vector* pVector, bool bTest);
typedef float(__fastcall* _TraceFirePortal)(void* thisptr,
                                            int edx,
                                            bool bPortal2,
                                            const Vector& vTraceStart,
                                            const Vector& vDirection,
                                            trace_t& tr,
                                            Vector& vFinalPosition,
                                            QAngle& qFinalAngles,
                                            int iPlacedBy,
                                            bool bTest);
typedef void*(__fastcall* _GetActiveWeapon)(void* thisptr);
typedef const Vector&(__fastcall* _CGameMovement__GetPlayerMaxs)(void* thisptr, int edx);
typedef const Vector&(__fastcall* _CGameMovement__GetPlayerMins)(void* thisptr, int edx);

// Tracing related functionality
class Tracing : public FeatureWrapper<Tracing>
{
public:
	_UTIL_TraceRay ORIG_UTIL_TraceRay = nullptr;
	_TracePlayerBBoxForGround ORIG_TracePlayerBBoxForGround = nullptr;
	_TracePlayerBBoxForGround2 ORIG_TracePlayerBBoxForGround2 = nullptr;
	_GetActiveWeapon ORIG_GetActiveWeapon = nullptr;
	_TraceFirePortal ORIG_TraceFirePortal = nullptr;

	bool TraceClientRay(const Ray_t& ray,
	                    unsigned int mask,
	                    const IHandleEntity* ignore,
	                    int collisionGroup,
	                    trace_t* ptr);
	bool CanTracePlayerBBox();
	void TracePlayerBBox(const Vector& start,
	                     const Vector& end,
	                     const Vector& mins,
	                     const Vector& maxs,
	                     unsigned int fMask,
	                     int collisionGroup,
	                     trace_t& pm);

#ifdef SPT_TRACE_PORTAL_ENABLED
	void* GetActiveWeapon();
	float TraceFirePortal(trace_t& tr, const Vector& startPos, const Vector& vDirection);

	// Transform through portal if in portal enviroment
	float TraceTransformFirePortal(trace_t& tr, const Vector& startPos, const QAngle& startAngles);
	float TraceTransformFirePortal(trace_t& tr,
	                               const Vector& startPos,
	                               const QAngle& startAngles,
	                               Vector& finalPos,
	                               QAngle& finalAngles,
	                               bool isPortal2);
#endif

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	static const Vector& __fastcall HOOKED_CGameMovement__GetPlayerMaxs(void* thisptr, int edx);
	static const Vector& __fastcall HOOKED_CGameMovement__GetPlayerMins(void* thisptr, int edx);

	bool overrideMinMax = false;
	Vector _mins;
	Vector _maxs;

	_CGameMovement__TracePlayerBBox ORIG_CGameMovement__TracePlayerBBox = nullptr;
	_CPortalGameMovement__TracePlayerBBox ORIG_CPortalGameMovement__TracePlayerBBox = nullptr;
	_CGameMovement__GetPlayerMins ORIG_CGameMovement__GetPlayerMins = nullptr;
	_CGameMovement__GetPlayerMaxs ORIG_CGameMovement__GetPlayerMaxs = nullptr;
};

extern Tracing spt_tracing;
