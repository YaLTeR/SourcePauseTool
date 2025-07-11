#include <stdafx.hpp>

#include "fcps_event.hpp"

#ifdef SPT_FCPS_ENABLED

#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"
#include "spt/flatbuffers_schemas/gen/fcps_generated.h"
#include "spt/flatbuffers_schemas/fb_pack_defs.hpp"

#include <memory_resource>

#undef MIN
#undef MAX

fb::fb_off FcpsEvent::Serialize(flatbuffers::FlatBufferBuilder& fbb) const
{
	std::pmr::monotonic_buffer_resource mbr{};

	// build entities

	std::pmr::vector<flatbuffers::Offset<fb::fcps::FcpsEntInfo>> fbEnts(&mbr);

	for (const FcpsEntInfo& ent : detail.ents)
	{
		std::pmr::vector<flatbuffers::Offset<fb::fcps::FcpsEntMesh>> fbEntMeshes(&mbr);

		for (const FcpsEntMesh& entMesh : ent.meshes)
		{
			auto fbPhysMesh = entMesh.physMesh.Serialize(fbb, false);

			fb::fcps::FcpsEntMeshBuilder entMeshBuilder{fbb};

			entMeshBuilder.add_ball_radius(entMesh.ballRadius);
			fb::math::Transform fbTrans{fb_pack::PackVector(entMesh.pos), fb_pack::PackQAngle(entMesh.ang)};
			entMeshBuilder.add_transform(&fbTrans);
			entMeshBuilder.add_phys_mesh(fbPhysMesh);

			fbEntMeshes.push_back(entMeshBuilder.Finish());
		}

		auto fbEntMeshesOff = fbb.CreateVector(fbEntMeshes);
		auto fbNetName = fbb.CreateSharedString(ent.networkClassName);
		auto fbName = fbb.CreateSharedString(ent.name);

		fb::fcps::FcpsEntInfoBuilder entBuilder{fbb};

		auto fbHandle = fb_pack::PackCBaseHandle(ent.handle);
		entBuilder.add_handle(&fbHandle);
		entBuilder.add_net_class_name(fbNetName);
		entBuilder.add_name(fbName);
		fb::math::Transform fbTrans{fb_pack::PackVector(ent.pos), fb_pack::PackQAngle(ent.ang)};
		entBuilder.add_transform(&fbTrans);
		fb::math::Aabb fbObb{fb_pack::PackVector(ent.obbMins), fb_pack::PackVector(ent.obbMaxs)};
		entBuilder.add_obb(&fbObb);
		entBuilder.add_meshes(fbEntMeshesOff);

		fbEnts.push_back(entBuilder.Finish());
	}

	auto fbEntsOff = fbb.CreateVector(fbEnts);

	// build ent traces

	auto fbEntTraceResults = fbb.CreateVectorOfNativeStructs(detail.entTraces, fb_pack::PackFcpsTraceResult);

	// build nudge iterations

	std::pmr::vector<flatbuffers::Offset<fb::fcps::FcpsNudgeIteration>> fbNudges(&mbr);

	for (const FcpsNudgeIteration& nudgeIt : detail.nudges)
	{
		auto fbCornerTraceResults =
		    fbb.CreateVectorOfNativeStructs(nudgeIt.testTraces, fb_pack::PackFcpsTraceResult);
		auto fbCornerBits = fbb.CreateVector(nudgeIt.testTraceCornerBits);
		auto fbWeights = fbb.CreateVector(nudgeIt.weights.data(), nudgeIt.weights.size());

		fb::fcps::FcpsNudgeIterationBuilder nudgeBuilder{fbb};

		nudgeBuilder.add_corner_oob_bits(nudgeIt.cornerOobBits);
		auto fbTestExtents = fb_pack::PackVector(nudgeIt.testExtents);
		nudgeBuilder.add_test_extents(&fbTestExtents);
		nudgeBuilder.add_test_traces(fbCornerTraceResults);
		nudgeBuilder.add_test_trace_corner_bits(fbCornerBits);
		nudgeBuilder.add_corner_weights(fbWeights);
		auto fbNudgeVec = fb_pack::PackVector(nudgeIt.nudgeVec);
		nudgeBuilder.add_nudge_vec(&fbNudgeVec);

		fbNudges.push_back(nudgeBuilder.Finish());
	}

	auto fbNudgesOff = fbb.CreateVector(fbNudges);

	// build event

	auto fbMapName = fbb.CreateSharedString(gameInfo.mapName);
	auto fbPlayerName = fbb.CreateSharedString(gameInfo.playerName);

	fb::fcps::FcpsEventBuilder eventBuilder{fbb};

	// params struct

	auto fbHandle = fb_pack::PackCBaseHandle(params.entHandle);
	eventBuilder.add_handle(&fbHandle);
	auto fbIndecisivePush = fb_pack::PackVector(params.vIndecisivePush);
	eventBuilder.add_indecisive_push(&fbIndecisivePush);
	eventBuilder.add_f_mask(params.fMask);
	auto fbCaller = static_cast<fb::fcps::FcpsCaller>(params.caller);
	Assert(flatbuffers::IsInRange(fbCaller, fb::fcps::FcpsCaller::MIN, fb::fcps::FcpsCaller::MAX));
	eventBuilder.add_func_caller(fbCaller);

	// outcome struct

	auto fbResult = static_cast<fb::fcps::FcpsCallResult>(outcome.result);
	Assert(flatbuffers::IsInRange(fbResult, fb::fcps::FcpsCallResult::MIN, fb::fcps::FcpsCallResult::MAX));
	eventBuilder.add_call_result(fbResult);
	auto fbFrom = fb_pack::PackVector(outcome.fromPos);
	eventBuilder.add_from_origin(&fbFrom);
	auto fbTo = fb_pack::PackVector(outcome.toPos);
	eventBuilder.add_to_origin(&fbTo);

	// gameinfo struct

	eventBuilder.add_game_version(gameInfo.gameVersion);
	eventBuilder.add_host_tick(gameInfo.hostTick);
	eventBuilder.add_server_tick(gameInfo.serverTick);
	eventBuilder.add_map_name(fbMapName);
	eventBuilder.add_player_name(fbPlayerName);

	// detail struct

	eventBuilder.add_entities(fbEntsOff);
	eventBuilder.add_run_on_ent(detail.runOnEnt);
	eventBuilder.add_collision_group(detail.collisionGroup);
	auto fbAabbExtents = fb_pack::PackVector(detail.aabbExtents);
	eventBuilder.add_aabb_extents(&fbAabbExtents);
	auto fbEntCenterToOriginDiff = fb_pack::PackVector(detail.entCenterToOriginDiff);
	eventBuilder.add_ent_center_to_origin_diff(&fbEntCenterToOriginDiff);
	auto fbGrowSize = fb_pack::PackVector(detail.growSize);
	eventBuilder.add_grow_size(&fbGrowSize);
	auto fbRayStartMins = fb_pack::PackVector(detail.rayStartMins);
	eventBuilder.add_ray_start_mins(&fbRayStartMins);
	auto fbRayStartMaxs = fb_pack::PackVector(detail.rayStartMaxs);
	eventBuilder.add_ray_start_maxs(&fbRayStartMaxs);
	eventBuilder.add_ent_traces(fbEntTraceResults);
	eventBuilder.add_nudge_iterations(fbNudgesOff);

	return eventBuilder.Finish().o;
}

template<typename FbFieldPtr>
inline FbFieldPtr FcpsCheckIfFieldPresent(FbFieldPtr fbField, ser::StatusTracker& stat, const char* fieldName)
{
	if (!fbField) [[unlikely]]
		stat.Err(std::format("missing field {}", fieldName));
	return fbField;
}

template<typename NativeField, typename FbFieldPtr>
inline void FcpsUnpackIfPresent(NativeField& nativeField,
                                FbFieldPtr fbField,
                                ser::StatusTracker& stat,
                                const char* fieldName)
{
	if (FcpsCheckIfFieldPresent(fbField, stat, fieldName)) [[likely]]
		nativeField = fb_pack::Unpack(*fbField);
}

template<typename EnumType, typename FbEnumType>
inline void FcpsUnpackEnum(EnumType& nativeEnum, FbEnumType fbVal, ser::StatusTracker& stat, const char* fieldName)
{
	if (flatbuffers::IsInRange(fbVal, FbEnumType::MIN, FbEnumType::MAX)) [[likely]]
		nativeEnum = static_cast<EnumType>(fbVal);
	else
		stat.Err(std::format("invalid enum value in field {}", fieldName));
}

template<typename T>
using fb_vec_to_table = const flatbuffers::Vector<flatbuffers::Offset<T>>*;

template<typename T>
using fb_vec_to_struct = const flatbuffers::Vector<const T*>*;

static void FcpsUnpackEntMeshes(std::vector<FcpsEntMesh>& meshes,
                                fb_vec_to_table<fb::fcps::FcpsEntMesh> fbMeshes,
                                ser::StatusTracker& stat)
{
	meshes.reserve(fbMeshes->size());
	for (const fb::fcps::FcpsEntMesh* fbEntMesh : *fbMeshes)
	{
		FcpsEntMesh& entMesh = meshes.emplace_back();
		entMesh.ballRadius = fbEntMesh->ball_radius();
		if (auto fbMeshTrans = FcpsCheckIfFieldPresent(fbEntMesh->transform(), stat, "ent mesh transform"))
		{
			entMesh.pos = fb_pack::UnpackVector(fbMeshTrans->pos());
			entMesh.ang = fb_pack::UnpackQAngle(fbMeshTrans->ang());
		}
		if (auto fbPhysMesh = FcpsCheckIfFieldPresent(fbEntMesh->phys_mesh(), stat, "ent phys mesh"))
			entMesh.physMesh.Deserialize(*fbPhysMesh, stat);
	}
}

static void FcpsUnpackEntities(std::vector<FcpsEntInfo>& ents,
                               fb_vec_to_table<fb::fcps::FcpsEntInfo> fbEnts,
                               ser::StatusTracker& stat)
{
	ents.reserve(fbEnts->size());
	for (const fb::fcps::FcpsEntInfo* fbEnt : *fbEnts)
	{
		FcpsEntInfo& ent = ents.emplace_back();
		FcpsUnpackIfPresent(ent.handle, fbEnt->handle(), stat, "ent handle");
		ent.networkClassName = flatbuffers::GetString(fbEnt->net_class_name());
		ent.name = flatbuffers::GetString(fbEnt->name());
		if (auto fbEntTrans = FcpsCheckIfFieldPresent(fbEnt->transform(), stat, "ent transform"))
		{
			ent.pos = fb_pack::UnpackVector(fbEntTrans->pos());
			ent.ang = fb_pack::UnpackQAngle(fbEntTrans->ang());
		}
		if (auto fbEntObb = FcpsCheckIfFieldPresent(fbEnt->obb(), stat, "ent OBB"))
		{
			ent.obbMins = fb_pack::UnpackVector(fbEntObb->mins());
			ent.obbMaxs = fb_pack::UnpackVector(fbEntObb->maxs());
		}

		auto fbMeshes = fbEnt->meshes();
		if (fbMeshes && !fbMeshes->empty())
			FcpsUnpackEntMeshes(ent.meshes, fbMeshes, stat);
	}
}

static void FcpsUnpackEntTraces(std::vector<FcpsTraceResult>& entTraces,
                                fb_vec_to_struct<fb::fcps::FcpsTraceResult> fbEntTraces,
                                ser::StatusTracker& stat)
{
	entTraces.reserve(fbEntTraces->size());
	for (const fb::fcps::FcpsTraceResult* fbTrace : *fbEntTraces)
		entTraces.push_back(fb_pack::UnpackFcpsTraceResult(*fbTrace));
}

static void FcpsUnpackNudgeIterations(std::vector<FcpsNudgeIteration>& nudgeIts,
                                      fb_vec_to_table<fb::fcps::FcpsNudgeIteration> fbNudgeIts,
                                      ser::StatusTracker& stat)
{
	nudgeIts.reserve(fbNudgeIts->size());
	for (const fb::fcps::FcpsNudgeIteration* fbNudge : *fbNudgeIts)
	{
		FcpsNudgeIteration& nudge = nudgeIts.emplace_back();
		nudge.cornerOobBits = fbNudge->corner_oob_bits();
		FcpsUnpackIfPresent(nudge.testExtents, fbNudge->test_extents(), stat, "nudge test_extents");
		auto fbTestTraces = fbNudge->test_traces();
		if (fbTestTraces && !fbTestTraces->empty())
		{
			nudge.testTraces.reserve(fbTestTraces->size());
			for (const fb::fcps::FcpsTraceResult* fbTrace : *fbTestTraces)
				nudge.testTraces.push_back(fb_pack::UnpackFcpsTraceResult(*fbTrace));
		}
		auto fbCornerBits = fbNudge->test_trace_corner_bits();
		if (fbCornerBits && !fbCornerBits->empty())
			nudge.testTraceCornerBits.assign(fbCornerBits->cbegin(), fbCornerBits->cend());

		if (nudge.testTraces.size() != nudge.testTraceCornerBits.size() || nudge.testTraces.size() > 56)
			stat.Err("unexpected number of corner traces");

		auto fbWeights = fbNudge->corner_weights();
		if (!fbWeights || fbWeights->size() != 8)
			stat.Err("unexpected number of corner weights");
		else
			std::copy(fbWeights->cbegin(), fbWeights->cend(), nudge.weights.begin());
		FcpsUnpackIfPresent(nudge.nudgeVec, fbNudge->nudge_vec(), stat, "nudge nudge_vec");
	}
}

void FcpsEvent::Deserialize(const fb::fcps::FcpsEvent& fbEvent, ser::StatusTracker& stat)
{
	*this = FcpsEvent{}; // clear
	imported = true;

	// params struct

	FcpsUnpackIfPresent(params.entHandle, fbEvent.handle(), stat, "handle");
	FcpsUnpackIfPresent(params.vIndecisivePush, fbEvent.indecisive_push(), stat, "indecisive_push");
	params.fMask = fbEvent.f_mask();
	FcpsUnpackEnum(params.caller, fbEvent.func_caller(), stat, "func_caller");

	// outcome struct

	FcpsUnpackEnum(outcome.result, fbEvent.call_result(), stat, "call_result");
	FcpsUnpackIfPresent(outcome.fromPos, fbEvent.from_origin(), stat, "from_origin");
	FcpsUnpackIfPresent(outcome.toPos, fbEvent.to_origin(), stat, "to_origin");

	// gameinfo struct

	gameInfo.gameVersion = fbEvent.game_version();
	gameInfo.hostTick = fbEvent.host_tick();
	gameInfo.serverTick = fbEvent.server_tick();
	gameInfo.mapName = flatbuffers::GetString(fbEvent.map_name());
	gameInfo.playerName = flatbuffers::GetString(fbEvent.player_name());

	// detail struct

	auto fbEnts = fbEvent.entities();
	if (fbEnts && !fbEnts->empty())
		FcpsUnpackEntities(detail.ents, fbEnts, stat);
	else
		stat.Err("missing or empty entities field");

	detail.runOnEnt = fbEvent.run_on_ent();
	if (!FCPS_OPT_ELEM_PTR(detail.ents, detail.runOnEnt))
		stat.Err("run_on_ent index out of bounds");
	detail.collisionGroup = fbEvent.collision_group();
	FcpsUnpackIfPresent(detail.aabbExtents, fbEvent.aabb_extents(), stat, "aabb_extents");
	FcpsUnpackIfPresent(detail.entCenterToOriginDiff,
	                    fbEvent.ent_center_to_origin_diff(),
	                    stat,
	                    "ent_center_to_origin_diff");
	FcpsUnpackIfPresent(detail.growSize, fbEvent.grow_size(), stat, "grow_size");
	FcpsUnpackIfPresent(detail.rayStartMins, fbEvent.ray_start_mins(), stat, "ray_start_mins");
	FcpsUnpackIfPresent(detail.rayStartMaxs, fbEvent.ray_start_maxs(), stat, "ray_start_maxs");

	auto fbEntTraces = fbEvent.ent_traces();
	if (fbEntTraces && !fbEntTraces->empty())
		FcpsUnpackEntTraces(detail.entTraces, fbEntTraces, stat);

	auto fbNudges = fbEvent.nudge_iterations();
	if (fbNudges && !fbNudges->empty())
		FcpsUnpackNudgeIterations(detail.nudges, fbNudges, stat);

	size_t nNudges = detail.nudges.size();
	size_t nEntTraces = detail.entTraces.size();

	if ((outcome.result == FCPS_SUCESS && nNudges + 1 != nEntTraces)
	    || (outcome.result == FCPS_FAIL && nNudges != nEntTraces)
	    || (outcome.result != FCPS_SUCESS && outcome.result != FCPS_FAIL && (nNudges > 0 || nEntTraces > 0)))
	{
		stat.Err("unexpected number of ent traces & nudges given the outcome");
	}
}

#endif
