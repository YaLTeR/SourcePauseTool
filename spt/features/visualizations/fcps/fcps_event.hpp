#pragma once

#include "fcps_config.hpp"

#ifdef SPT_FCPS_ENABLED

#include "spt/utils/mesh_utils.hpp"
#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"

#include "cmodel.h"

#include <string>
#include <format>

struct FcpsEntMesh
{
	float ballRadius;
	Vector pos; // pos/ang are in world space
	QAngle ang;
	utils::MbCompactMesh physMesh; // points are in local space
};

struct FcpsEntInfo
{
	CBaseHandle handle;
	std::string networkClassName, name;
	Vector pos; // origin
	QAngle ang;
	Vector obbMins, obbMaxs; // relative to origin
	std::vector<FcpsEntMesh> meshes;
};

using fcps_ent_idx = uint32_t;

constexpr fcps_ent_idx FCPS_INVALID_ENT_IDX = std::numeric_limits<fcps_ent_idx>::max();
constexpr size_t FCPS_MAX_NUM_ENTS = FCPS_INVALID_ENT_IDX - 1;

// compact version of Ray_t
struct FcpsCompactRay
{
	Vector start;
	Vector delta;
	Vector extents;

	FcpsCompactRay() = default;
	FcpsCompactRay(Vector start, Vector delta, Vector extents) : start{start}, delta{delta}, extents{extents} {}
	FcpsCompactRay(const Ray_t& ray) : start{ray.m_Start}, delta{ray.m_Delta}, extents{ray.m_Extents} {}
};

struct FcpsTraceResult
{
	FcpsCompactRay ray;
	CBaseTrace trace;
	fcps_ent_idx hitEnt;
};

/*
* If the test ray in FCPS fails, it determines that the entity is still stuck. From there, this
* struct stores what FCPS does:
* - determine which corners are OOB
* - for those that aren't, shoots a ray to every every corner
* - builds a weighted average on each corner based on the length of the resulting traces
* - nudges the entity towards a weighted sum of the corners or uses the indecisive push
* 
* The encoding used for corners here is the same as in FCPS:
* - corner 0 is -z-y-x
* - corner 1 is -z-y+x
* - corner 2 is -z+y-x etc
* 
* The ent center can be gotten through entTraces[i].ray.start and you can add entCenterToOriginDiff
* to get the origin.
*/
struct FcpsNudgeIteration
{
	// one bit for each corner - is it OOB based on PointInsideWorld
	uint8_t cornerOobBits;
	// AABB extents on this iteration relative to the center
	Vector testExtents;
	// only the corners that are inbounds get test rays
	std::vector<FcpsTraceResult> testTraces;
	/*
	* A corresponding list for the test traces. Each entry is 6 bits:
	* - lower 3 bits are the source corner
	* - bits 3-6 are the destination corner
	*/
	std::vector<uint8_t> testTraceCornerBits;
	std::array<float, 8> weights;
	Vector nudgeVec;
};

// return pointer to elem if idx is within bounds
#define FCPS_OPT_ELEM_PTR(vec, idx) ((idx) >= 0 && (size_t)(idx) < (vec).size() ? &(vec)[idx] : nullptr)

/*
* An in-memory representation of an FCPS event. If spt_fcps_override is 0, only 'params' and
* 'outcome' will be filled in, and everything else will be invalid. Use HasDetail() to determine if
* the other stuff is valid.
*/
struct FcpsEvent
{
	struct
	{
		CBaseHandle entHandle;
		Vector vIndecisivePush;
		unsigned int fMask;
		FcpsCaller caller = FcpsCaller::Unknown;
	} params;

	struct
	{
		FcpsCallResult result;
		// entity origin
		Vector fromPos, toPos;
		QAngle ang;
	} outcome;

	struct GameInfo
	{
		int gameVersion;
		int hostTick;
		int serverTick;
		std::string mapName;
		std::string playerName;

		void Populate();
	} gameInfo;

	struct
	{
		std::vector<FcpsEntInfo> ents;
		/*
		* This should be 0. Important note: when we record the meshes for this entity, they are
		* recorded at the start of FCPS. If you continue to draw this entity after that point, you
		* must translate those meshes manually (since this is the only entity that "moves").
		*/
		fcps_ent_idx runOnEnt; // should be 0
		int collisionGroup;
		Vector aabbExtents; // relative to center
		Vector entCenterToOriginDiff;
		Vector growSize, rayStartMins, rayStartMaxs;
		std::vector<FcpsTraceResult> entTraces;
		std::vector<FcpsNudgeIteration> nudges;
	} detail;

	bool imported = false; // was this event imported from a file?

	std::string GetShortDescription() const
	{
		const FcpsEntInfo* pEntInfo = FCPS_OPT_ELEM_PTR(detail.ents, detail.runOnEnt);
		const char* entName = "";
		if (pEntInfo)
			entName = pEntInfo->name.empty() ? pEntInfo->networkClassName.c_str() : pEntInfo->name.c_str();

		return std::format(
		    "result: {} ({} iterations) called from {}, map '{}', server tick {} on entity '{}' (index {})",
		    FcpsCallResultToStr[(int)outcome.result],
		    detail.nudges.size(),
		    FcpsCallerToStr[(int)params.caller],
		    gameInfo.mapName,
		    gameInfo.serverTick,
		    entName,
		    params.entHandle.GetEntryIndex());
	}

	fb::fb_off Serialize(flatbuffers::FlatBufferBuilder& fbb) const;
	void Deserialize(const fb::fcps::FcpsEvent& fbEvent, ser::StatusTracker& stat);
};

#endif
