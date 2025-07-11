#pragma once

#include "../renderer/mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED
#define SPT_FCPS_ENABLED

#include <inttypes.h>

extern ConVar spt_fcps_record;
extern ConVar spt_fcps_max_queue_size;
extern ConVar spt_fcps_draw_detail;
extern ConVar spt_fcps_animation_hz;

struct FcpsEvent;

/*
* Due to feature dependency shenanigans, you cannot reliably check fcpsFinishedSignal.Works.
* Until we get a feature dependency system, call this function from LoadFeature() or later to check
* if you connect yourself to the signal.
*/
bool FcpsFinishedSignalWorks();

/*
* Called after every time FCPS is triggered. Note that if spt_fcps_record is disabled, the event
* will not have a full recording of the event. DO NOT CHECK the .Works boolean. Instead, use
* FcpsFinishedSignalWorks() from LoadFeature() or later.
*/
inline Gallant::Signal1<const FcpsEvent&> fcpsFinishedSignal;

enum class FcpsCaller : uint8_t
{
	Unknown,
	// happens when the player is in a portal environment and a bunch of other conditions
	// vIndecisivePush: newPosition - GetAbsOrigin()
	VPhysicsShadowUpdate,
	// inlined in CPortalSimulator::ReleaseAllEntityOwnership
	// called when the portal moves/closes?
	// vIndecisivePush: portal normal
	RemoveEntityFromPortalHole,
	// called when in portal hole and stuck on something?
	// vIndecisivePush: portal normal if in portal environment, <0,0,0> otherwise
	CheckStuck,
	// called when the m_bHeldObjectOnOppositeSideOfPortal flag switches from 1 to 0, only called on entities the player is holding
	// vIndecisivePush: portal normal of the portal to which the entity just teleported to?
	TeleportTouchingEntity,
	// not sure when this is called
	// vIndecisivePush: portal normal
	PortalSimulator__MoveTo,
	// called when running command debug_fixmyposition
	// vIndecisivePush: <0,0,0>
	Debug_FixMyPosition,

	COUNT,
};

inline std::array<const char*, (int)FcpsCaller::COUNT> FcpsCallerToStr{
    "UNKNOWN_CALLER",
    "CPortal_Player::VPhysicsShadowUpdate",
    "CPortalSimulator::RemoveEntityFromPortalHole",
    "CPortalGameMovement::CheckStuck",
    "CProp_Portal::TeleportTouchingEntity",
    "CPortalSimulator::MoveTo",
    "CC_Debug_FixMyPosition",
};

enum FcpsCallResult : uint8_t
{
	// FcpsImpl is not supported for this game version - never serialized or seen in fcpsFinishedSignal
	FCPS_IMPLEMENTATION_INCOMPLETE,

	FCPS_SUCESS,                  // FCPS would have returned true
	FCPS_FAIL,                    // FCPS would have returned false
	FCPS_NOT_RUN_CVAR_DISABLED,   // FCPS was not run because sv_use_find_closest_passable_space was false
	FCPS_NOT_RUN_HAS_MOVE_PARENT, // FCPS was not run because the entity has a move parent

	FCPS_CR_COUNT,
};

inline std::array<const char*, FCPS_CR_COUNT> FcpsCallResultToStr{
    "NOT SUPPORTED",
    "SUCCESS",
    "FAIL",
    "NOT RUN (cvar disabled)",
    "NOT RUN (entity has move parent)",
};

inline ConVar* sv_use_find_closest_passable_space = nullptr;

/*
* Replicates the game's FCPS code and populates all the event fields. The caller return address is
* the game function which called FCPS - it can be null. Assumes that the event lists are empty.
*/
void FcpsImpl(FcpsEvent& event,
              IServerEntity* pEntity,
              const Vector& vIndecisivePush,
              unsigned int fMask,
              void* callerReturnAddress);

using fcps_event_id = int64_t;
#define FCPS_PR_ID PRId64
#define FCPS_SC_ID SCNd64

// inclusive [from, to], only strictly positive ranges are valid
using fcps_event_range = std::pair<fcps_event_id, fcps_event_id>;
constexpr fcps_event_range FCPS_INVALID_EVENT_RANGE = {-1, -1};

inline bool FcpsIdInRange(fcps_event_id id, fcps_event_range r)
{
	if (r.first > r.second || r.first <= 0 || r.second <= 0)
		return false;
	return id >= r.first && id <= r.second;
}

struct
{
	ShapeColor curPos{C_OUTLINE(200, 200, 0, 50), false, true};
	ShapeColor mainEntPhysMeshes{C_OUTLINE(80, 80, 150, 30)};
	ShapeColor success{C_OUTLINE(0, 255, 0, 40), false, true};
	ShapeColor fail{C_OUTLINE(255, 0, 0, 40), false, true};

	ShapeColor entRaySweep{C_OUTLINE(100, 100, 100, 10), false, true};
	ShapeColor entRayHit = success; // {C_OUTLINE(100, 100, 100, 10), false, true};
	ShapeColor entRayTarget{C_OUTLINE(0, 200, 200, 20), false, true};

	struct
	{
		ShapeColor endpoints{C_WIRE(255, 255, 255, 255)};
		ShapeColor sweepSuccess{C_WIRE(0, 255, 0, 255)};
		ShapeColor sweepFail{C_WIRE(255, 0, 0, 255)};
		ShapeColor assymetricOverlap{C_WIRE(50, 50, 255, 255)};

		ShapeColor invisible{C_WIRE(0, 0, 0, 0)};
	} entTest;

	ShapeColor traceHitEntPhysMeshes{C_OUTLINE(255, 0, 255, 50)};
	ShapeColor traceHitEntObb{C_OUTLINE(20, 20, 70, 20)};

} inline fcps_colors;

#endif
