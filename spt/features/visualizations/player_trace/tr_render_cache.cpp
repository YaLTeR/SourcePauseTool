#include "stdafx.hpp"

#include "tr_render_cache.hpp"
#include "spt/utils/interfaces.hpp"
#include "spt/utils/map_utils.hpp"
#include "spt/utils/math.hpp"
#include "spt/features/ent_props.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

#define PORTAL_HALF_WIDTH 32.0f
#define PORTAL_HALF_HEIGHT 54.0f

using namespace player_trace;

TrRenderingCache::TrRenderingCache(const TrPlayerTrace& tr) : tr{&tr} {}

void TrRenderingCache::RebuildPlayerHullMeshes()
{
	// qphys

	static constexpr float qphysHullReduction = 0.7f;

	struct
	{
		StaticMesh& mesh;
		TrIdx<TrAbsBox> hullIdx;
	} qPhysData[] = {
	    {meshes.qPhysStand, tr->playerStandBboxIdx},
	    {meshes.qPhysDuck, tr->playerDuckBboxIdx},
	};

	for (auto [mesh, hullIdx] : qPhysData)
	{
		if (mesh.Valid())
			continue;

		mesh = spt_meshBuilder.CreateStaticMesh(
		    [hullIdx](MeshBuilderDelegate& mb)
		    {
			    auto& style = trStyles.playerHull;
			    auto& colors = trColors.playerHull;

			    const Vector& mins = **hullIdx->minsIdx;
			    const Vector& maxs = **hullIdx->maxsIdx;
			    mb.AddBox(vec3_origin, mins, maxs, vec3_angle, colors.qPhys);
			    mb.AddBox(vec3_origin,
			              Vector{mins.x * qphysHullReduction, mins.y * qphysHullReduction, mins.z},
			              Vector{maxs.x * qphysHullReduction, maxs.y * qphysHullReduction, maxs.z},
			              vec3_angle,
			              colors.qPhysReduction);
			    if (style.drawOriginCube)
			    {
				    mb.AddBox(vec3_origin,
				              Vector{-style.originCubeSize},
				              Vector{style.originCubeSize},
				              vec3_angle,
				              colors.qPhysOrigin);
			    }
			    if (style.drawQPhysCenter)
			    {
				    // `ent_bbox player` usually draws a cross at the vphys center, but qphys is prolly more relevant (for portals)
				    mb.AddCross((mins + maxs) * .5f,
				                style.qPhysCenterCrossRadius,
				                colors.qPhys.lineColor);
			    }
			    // add a small almost invisible shell around the qphhys box to try to prevent common z-fighting cases
			    mb.AddBox(vec3_origin,
			              mins - Vector{0.001f},
			              maxs + Vector{0.001f},
			              vec3_angle,
			              ShapeColor{C_WIRE(0, 0, 0, 1)});
		    });
	}

	// vphys

	struct
	{
		StaticMesh& mesh;
		TrIdx<TrAbsBox> hullIdx;
	} vPhysData[] = {
	    {meshes.vPhysStand, tr->playerStandBboxIdx},
	    {meshes.vPhysDuck, tr->playerDuckBboxIdx},
	};

	for (auto [mesh, hullIdx] : vPhysData)
	{
		if (mesh.Valid())
			continue;
		mesh = spt_meshBuilder.CreateStaticMesh(
		    [hullIdx](MeshBuilderDelegate& mb)
		    {
			    const Vector& mins = **hullIdx->minsIdx;
			    const Vector& maxs = **hullIdx->maxsIdx;
			    mb.AddBox(vec3_origin, mins, maxs, vec3_angle, trColors.playerHull.vPhys);
		    });
	}
}

void TrRenderingCache::RebuildEyeMeshes(float fov)
{
	int expectedDrawType = clamp(spt_trace_draw_cam_style.GetInt(), 0, TR_PCDT_COUNT - 1);

	if (meshes.camType != expectedDrawType || meshes.eyeMeshFov != fov)
		StaticMesh::DestroyAllV(meshes.eyes, meshes.sgEyes);

	meshes.camType = (TrPlayerCameraDrawType)expectedDrawType;
	meshes.eyeMeshFov = fov;

	auto buildFrustumFunc = [fov](MeshBuilderDelegate& mb, ShapeColor color)
	{
		auto& frustumStyle = trStyles.playerHull.camera.frustum;
		float scaledFov = utils::ScaleFOVByWidthRatio(fov, frustumStyle.aspect * 3.f / 4.f);
		float halfHorizontalAng = DEG2RAD(scaledFov * .5f);
		float halfVerticalAng = atanf(tanf(halfHorizontalAng) / frustumStyle.aspect);

		float hc, hs, vc, vs;
		SinCos(halfHorizontalAng, &hs, &hc);
		SinCos(halfVerticalAng, &vs, &vc);
		VPlane planes[] = {
		    {{-hs, hc, 0}, 0},
		    {{-hs, -hc, 0}, 0},
		    {{-vs, 0, vc}, 0},
		    {{-vs, 0, -vc}, 0},
		    {{1, 0, 0}, frustumStyle.zFar},
		};
		CPolyhedron* poly = GeneratePolyhedronFromPlanes((float*)planes, ARRAYSIZE(planes), 0.001f, true);
		mb.AddCPolyhedron(poly, color);
		if (poly)
			poly->Release();

		color.wd = WD_BOTH;
		Vector p1 = Vector{
		    frustumStyle.zFar,
		    frustumStyle.zFar * hs / hc,
		    frustumStyle.zFar * vs / vc + frustumStyle.hatFloat,
		};
		Vector p2{p1.x, -p1.y, p1.z};
		Vector p3 = (p1 + p2) * .5f + Vector{0, 0, frustumStyle.hatHeight};
		mb.AddTri(p1, p2, p3, color);
	};

	auto buildBoxAndLineFunc = [](MeshBuilderDelegate& mb, ShapeColor color)
	{
		auto& style = trStyles.playerHull.camera.boxAndLine;
		mb.AddBox(vec3_origin, Vector{-style.boxRadius}, Vector{style.boxRadius}, vec3_angle, color);
		mb.AddLine(vec3_origin, Vector{style.lineLength, 0, 0}, color.faceColor);
	};

	auto& colors = trColors.playerHull.camera;

	if (!meshes.eyes.Valid())
	{
		meshes.eyes = spt_meshBuilder.CreateStaticMesh(
		    [&](MeshBuilderDelegate& mb)
		    {
			    if (meshes.camType == TR_PCDT_FRUSTUM)
				    buildFrustumFunc(mb, colors.frustum.eyes);
			    else
				    buildBoxAndLineFunc(mb, colors.boxAndLine.eyes);
		    });
	}

	if (!meshes.sgEyes.Valid())
	{
		meshes.sgEyes = spt_meshBuilder.CreateStaticMesh(
		    [&](MeshBuilderDelegate& mb)
		    {
			    if (meshes.camType == TR_PCDT_FRUSTUM)
				    buildFrustumFunc(mb, colors.frustum.sgEyes);
			    else
				    buildBoxAndLineFunc(mb, colors.boxAndLine.sgEyes);
		    });
	}
}

void TrRenderingCache::RebuildPlayerPathMeshes()
{
	meshes.playerPath.dynamicMeshes.clear();

	if (!StaticMesh::AllValid(meshes.playerPath.staticMeshes)
	    || meshes.playerPathGeneratedWithCones != spt_trace_draw_path_cones.GetBool())
	{
		meshes.playerPath.staticMeshes.clear();
	}

	if (meshes.playerPath.staticMeshes.empty())
		meshes.playerPath.staticMeshesBuiltUpToTick = 0;

	if (meshes.playerPath.staticMeshesBuiltUpToTick + 1 >= tr->numRecordedTicks)
		return;

	meshes.playerPathGeneratedWithCones = spt_trace_draw_path_cones.GetBool();

	/*
	* TODO: right now, it's possible to overdraw if e.g. the line gets drawn but the cone doesn't -
	* the line will get added in the next mesh as well. To properly fix this we need support for
	* transactions directly in the MeshBuilderDelegate. This is useful for other meshes as well.
	*/

	/*
	* The whole player path gets built as one mesh (if possible). As we iterate, the landmark
	* transforms are added directly to the player path. This means that all of the coordinates in
	* the mesh(es) are relative to the first map. If we're loaded in another map, we have to
	* transform the mesh(es).
	*/

	tr_tick firstTickToDraw = meshes.playerPath.staticMeshesBuiltUpToTick;

	auto pdIdx = tr->GetAtTick<TrPlayerData>(firstTickToDraw);
	auto segmentIdx = tr->GetAtTick<TrSegmentStart>(firstTickToDraw);
	auto mapTransitionIdx = tr->GetAtTick<TrMapTransition>(firstTickToDraw);

	Vector lastImplicitVel{1.f, 0.f, 0.f};
	Vector prevImplicitVel = lastImplicitVel;
	Vector landmarkOff =
	    mapTransitionIdx.IsValid() ? **mapTransitionIdx->toMapIdx->landmarkDeltaToFirstMapIdx : vec3_origin;
	int ticksWithoutCone = 0;
	TrSegmentReason deferredSegmentReason = TR_SR_NONE;

	if (!pdIdx.IsValid())
		return;

	auto createFunc = [&](MeshBuilderDelegate& mb, tr_tick tick)
	{
		auto incrementIdxToCurTick = [tick]<typename T>(TrIdx<T>& idx)
		{
			if (!(idx + 1).IsValid() || tick < (idx + 1)->tick)
				return false;
			++idx;
			return true;
		};

		auto& pathStyle = trStyles.playerPath;
		auto& pathColors = trColors.playerPath;

		Vector p1 = **pdIdx->qPosIdx + landmarkOff;

		incrementIdxToCurTick(pdIdx);
		if (incrementIdxToCurTick(mapTransitionIdx))
			landmarkOff = **mapTransitionIdx->toMapIdx->landmarkDeltaToFirstMapIdx;

		Vector p2 = **pdIdx->qPosIdx + landmarkOff;

		bool drawCones = spt_trace_draw_path_cones.GetBool();
		bool drawPath = true;
		TrSegmentReason segmentReason = TR_SR_NONE;

		if (p1 == p2)
		{
			drawPath = false;
			drawCones = false;
		}
		else
		{
			prevImplicitVel = lastImplicitVel;
			lastImplicitVel = p2 - p1;
		}

		bool ptsValid = p1.IsValid() && p2.IsValid();
		if (!ptsValid)
		{
			drawPath = false;
			drawCones = false;
			segmentReason = TR_SR_NONE;
		}

		if (incrementIdxToCurTick(segmentIdx))
		{
			drawPath = false;
			segmentReason = segmentIdx->reason;
		}
		else if (ptsValid && p2.DistToSqr(p1) > trStyles.playerPath.maxDistBeforeImplicitBreakSqr)
		{
			drawPath = false;
			segmentReason = TR_SR_IMPLICIT;
		}
		else
		{
			segmentReason = TR_SR_NONE;
		}

		LineColor pathCol = pathColors.grounded;
		if (drawPath)
		{
			if (!mb.AddLine(p1, p2, pathCol)) [[unlikely]]
				return false;
		}

		if (drawCones && segmentReason == TR_SR_NONE && ticksWithoutCone++ > pathStyle.cones.tickInterval)
		{
			ticksWithoutCone = 0;
			Vector implVelNorm = lastImplicitVel;
			VectorNormalize(implVelNorm);
			QAngle ang;
			VectorAngles(implVelNorm, ang);
			ShapeColor coneCol{
			    pathCol.lineColor,
			    pathCol.lineColor,
			    pathCol.zTestLines,
			    pathCol.zTestLines,
			};
			coneCol.faceColor.a *= pathColors.coneOpacity;
			if (!mb.AddCone(p2 - implVelNorm * pathStyle.cones.length,
			                ang,
			                pathStyle.cones.length,
			                pathStyle.cones.radius,
			                pathStyle.cones.nCirclePoints,
			                false,
			                coneCol))
			{
				return false;
			}
		}

		if (pathStyle.segmentEndpoints.enabled
		    && (segmentReason != TR_SR_NONE || deferredSegmentReason != TR_SR_NONE))
		{
			bool firstCircle = segmentReason != TR_SR_NONE;

			TrSegmentReason curReason = firstCircle ? segmentReason : deferredSegmentReason;
			Vector vel = firstCircle ? prevImplicitVel : lastImplicitVel;

			if (firstCircle)
				deferredSegmentReason = segmentReason;
			else
				deferredSegmentReason = TR_SR_NONE;

			QAngle ang;
			VectorAngles(vel, ang);
			ShapeColor endpointColor = pathColors.segmentEndPoints[curReason];
			endpointColor.faceColor.a *= pathColors.segmentEndpointOpacity;
			if (!mb.AddCircle(p1,
			                  ang,
			                  pathStyle.segmentEndpoints.radius,
			                  pathStyle.segmentEndpoints.nCirclePoints,
			                  endpointColor))
			{
				return false;
			}
		}

		tick++;
		return true;
	};

	tr_tick maxTicksForDynamic = tr->IsRecording() ? trStyles.playerPath.maxTicksToRenderAsDynamicMesh : 0;

	if (tr->numRecordedTicks - meshes.playerPath.staticMeshesBuiltUpToTick > maxTicksForDynamic)
	{
		spt_meshBuilder.CreateMultipleMeshes<StaticMesh>(std::back_inserter(meshes.playerPath.staticMeshes),
		                                                 meshes.playerPath.staticMeshesBuiltUpToTick,
		                                                 tr->numRecordedTicks,
		                                                 createFunc);

		meshes.playerPath.staticMeshesBuiltUpToTick = tr->numRecordedTicks - 1;
	}
	else
	{
		spt_meshBuilder.CreateMultipleMeshes<DynamicMesh>(std::back_inserter(meshes.playerPath.dynamicMeshes),
		                                                  meshes.playerPath.staticMeshesBuiltUpToTick,
		                                                  tr->numRecordedTicks,
		                                                  createFunc);
	}
}

void TrRenderingCache::RebuildPortalMeshes()
{
	if (StaticMesh::AllValidV(meshes.openBluePortal,
	                          meshes.openOrangePortal,
	                          meshes.closedBluePortal,
	                          meshes.closedOrangePortal))
	{
		return;
	}

	auto buildFunc = [this](bool open, bool orange)
	{
		return spt_meshBuilder.CreateStaticMesh(
		    [open, orange](MeshBuilderDelegate& mb)
		    {
			    auto& portalStyle = trStyles.portals;
			    ShapeColor col{C_OUTLINE(orange ? trColors.portals.orange : trColors.portals.blue)};
			    if (!open)
				    col.faceColor.a /= 4;
			    Vector boxMaxs{
			        portalStyle.twicePortalThickness,
			        PORTAL_HALF_WIDTH,
			        PORTAL_HALF_HEIGHT,
			    };
			    mb.AddBox(vec3_origin, -boxMaxs, boxMaxs, vec3_angle, col);

			    col.wd = WD_BOTH;
			    Vector p1{boxMaxs.x, PORTAL_HALF_WIDTH, PORTAL_HALF_HEIGHT + portalStyle.hatFloat};
			    Vector p2{p1.x, -p1.y, p1.z};
			    Vector p3 = (p1 + p2) * .5f + Vector{0, 0, portalStyle.hatHeight};
			    mb.AddTri(p1, p2, p3, col);

			    col.wd = WD_CW;
			    Vector arrowPos{boxMaxs.x, 0, 0};
			    mb.AddArrow3D(arrowPos, arrowPos + Vector{1, 0, 0}, portalStyle.arrowParams, col);
		    });
	};

	meshes.openBluePortal = buildFunc(true, false);
	meshes.openOrangePortal = buildFunc(true, true);
	meshes.closedBluePortal = buildFunc(false, false);
	meshes.closedOrangePortal = buildFunc(false, true);
}

void TrRenderingCache::RebuildPhysMeshes()
{
	auto& physMeshes = meshes.ents.physObjs;

	for (auto& [_, tracked] : physMeshes)
		tracked.isActive = false;

	for (auto& [entIdx, entTransIdx] : entSnapshot.entMap)
	{
		const TrEnt& ent = **entIdx;

		const ShapeColor& shapeCol = strcmp(*entIdx->classNameIdx, "portalsimulator_collisionentity")
		                                 ? trColors.entities.physMesh
		                                 : trColors.entities.physMeshPortalCollisionEnt;

		for (auto physIdx : *ent.physSp)
		{
			auto [it, new_elem] = physMeshes.try_emplace(physIdx->meshIdx);
			it->second.isActive = true;
			if (it->second.mesh.Valid())
				continue;

			const TrPhysMesh& physMesh = **it->first;

			if (physMesh.ballRadius > 0)
			{
				it->second.mesh = spt_meshBuilder.CreateStaticMesh(
				    [&](MeshBuilderDelegate& mb)
				    {
					    mb.AddSphere(vec3_origin,
					                 physMesh.ballRadius,
					                 trStyles.entities.nBallMeshSubdivisions,
					                 shapeCol);
				    });
			}
			else
			{
				// TODO optimize?
				it->second.mesh = spt_meshBuilder.CreateStaticMesh(
				    [&](MeshBuilderDelegate& mb)
				    {
					    static std::vector<Vector> pts;
					    pts.clear();
					    for (auto vertIdx : *physMesh.vertIdxSp)
						    pts.push_back(**vertIdx);
					    mb.AddTris(pts.data(), physMesh.vertIdxSp.n / 3, shapeCol);
				    });
			}
		}
	}

	std::erase_if(physMeshes, [](const auto& entry) { return !entry.second.isActive; });
}

void TrRenderingCache::VerifySnapshotState() const
{
	if (!entSnapshot.initialized)
		return;
	Assert(entSnapshot.snapshotIdx.IsValid());
	if (entSnapshot.snapshotIdx.IsValid() && entSnapshot.snapshotDeltaIdx.IsValid())
	{
		Assert(entSnapshot.snapshotIdx->snapDeltaIdx <= entSnapshot.snapshotDeltaIdx);
		Assert(entSnapshot.snapshotIdx->tick <= entSnapshot.snapshotDeltaIdx->tick);
	}
}

void TrRenderingCache::UpdateEntSnapshot(tr_tick toTick)
{
	if (entSnapshot.initialized && toTick == entSnapshot.tick)
		return;

	VerifySnapshotState();

	TrIdx<TrEntSnapshot> snapIdxLow = tr->GetAtTick<TrEntSnapshot>(toTick);
	auto snapIdxHigh = snapIdxLow + 1;

	if (!snapIdxLow.IsValid())
		return; // no baselines :/

	TrIdx<TrEntSnapshotDelta> snapDeltaIdx = tr->GetAtTick<TrEntSnapshotDelta>(toTick);
	if (!snapDeltaIdx.IsValid())
	{
		// no deltas, rely on snapshots only
		if (entSnapshot.snapshotIdx == snapIdxLow && entSnapshot.initialized)
			entSnapshot.tick = toTick;
		else
			JumpToEntSnapshot(snapIdxLow);
		return;
	}

#define TR_DEBUG_UPDATE_SNAPSHOT 0

	// small optimization
	if (entSnapshot.initialized && snapDeltaIdx == entSnapshot.snapshotDeltaIdx)
	{
		entSnapshot.tick = toTick;
#if TR_DEBUG_UPDATE_SNAPSHOT
		DevMsg("SPT: [%s] no deltas applied\n", __FUNCTION__);
#endif
		return;
	}

	// calculate most efficient approach to the desired delta

	enum SnapshotDeltaApproach
	{
		SDA_JUMP_TO_SNAPSHOT_THEN_INCREMENT,
		SDA_JUMP_TO_SNAPSHOT_THEN_DECREMENT,
		SDA_INCREMENT,
		SDA_DECREMENT,

		SDA_COUNT,
	};

	constexpr uint32_t JUMP_TO_SNAPSHOT_COST = 3;
	constexpr uint32_t SNAPSHOT_DELTA_COST = 1;

	SnapshotDeltaApproach approach = SDA_JUMP_TO_SNAPSHOT_THEN_INCREMENT;
	size_t cost = JUMP_TO_SNAPSHOT_COST + SNAPSHOT_DELTA_COST * (snapDeltaIdx - snapIdxLow->snapDeltaIdx);
	size_t nDeltas = snapDeltaIdx - snapIdxLow->snapDeltaIdx;

	if (snapIdxHigh.IsValid())
	{
		size_t newCost =
		    JUMP_TO_SNAPSHOT_COST + SNAPSHOT_DELTA_COST * (snapIdxHigh->snapDeltaIdx - snapDeltaIdx);
		if (newCost < cost)
		{
			approach = SDA_JUMP_TO_SNAPSHOT_THEN_DECREMENT;
			cost = newCost;
			nDeltas = snapIdxHigh->snapDeltaIdx - snapDeltaIdx;
		}
	}

	if (entSnapshot.initialized)
	{
		if (entSnapshot.tick <= toTick)
		{
			size_t newCost = SNAPSHOT_DELTA_COST * (snapDeltaIdx - entSnapshot.snapshotDeltaIdx);
			if (newCost < cost)
			{
				approach = SDA_INCREMENT;
				cost = newCost;
				nDeltas = snapDeltaIdx - entSnapshot.snapshotDeltaIdx;
			}
		}
		else
		{
			size_t newCost = SNAPSHOT_DELTA_COST * (entSnapshot.snapshotDeltaIdx - snapDeltaIdx);
			if (newCost < cost)
			{
				approach = SDA_DECREMENT;
				cost = newCost;
				nDeltas = entSnapshot.snapshotDeltaIdx - snapDeltaIdx;
			}
		}
	}

#if TR_DEBUG_UPDATE_SNAPSHOT
	static std::array<const char*, SDA_COUNT> approachStrs = {
	    "JUMP THEN INCREMENT",
	    "JUMP THEN DECREMENT",
	    "INCREMENT",
	    "DECREMENT",
	};
	DevMsg("SPT: [%s] using approach %s with %u deltas\n", __FUNCTION__, approachStrs[approach], nDeltas);
#endif

	switch (approach)
	{
	case SDA_JUMP_TO_SNAPSHOT_THEN_INCREMENT:
		JumpToEntSnapshot(snapIdxLow);
		[[fallthrough]];
	case SDA_INCREMENT:
		ForwardIterateSnapshotDeltas(toTick);
		break;
	case SDA_JUMP_TO_SNAPSHOT_THEN_DECREMENT:
		JumpToEntSnapshot(snapIdxHigh);
		[[fallthrough]];
	case SDA_DECREMENT:
		BackwardIterateSnapshotDeltas(toTick);
		break;
	default:
		Assert(0);
	}
}

void TrRenderingCache::JumpToEntSnapshot(TrIdx<TrEntSnapshot> snapIdx)
{
	entSnapshot.entMap.clear();
	for (auto& create : *snapIdx->createSp)
		entSnapshot.entMap[create.entIdx] = {create.transIdx};

	meshes.ents.anyStale = true;
	entSnapshot.snapshotIdx = snapIdx;
	entSnapshot.snapshotDeltaIdx = snapIdx->snapDeltaIdx;
	entSnapshot.tick = snapIdx->tick;
	entSnapshot.initialized = true;
}

void TrRenderingCache::ForwardIterateSnapshotDeltas(tr_tick toTick)
{
	VerifySnapshotState();
	TrIdx<TrEntSnapshot> snapIdxLow = entSnapshot.snapshotIdx;

	while ((entSnapshot.snapshotDeltaIdx + 1).IsValid() && entSnapshot.snapshotDeltaIdx->tick < toTick)
	{
		auto& delta = **++entSnapshot.snapshotDeltaIdx;

		for (auto& create : *delta.createSp)
			entSnapshot.entMap.emplace(create.entIdx, create.transIdx);

		for (auto& transDelta : *delta.deltaSp)
		{
			auto it = entSnapshot.entMap.find(transDelta.entIdx);
			if (it == entSnapshot.entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delta non-existing ent");
				continue;
			}
			it->second = transDelta.toTransIdx;
		}

		for (auto& del : *delta.deleteSp)
		{
			auto it = entSnapshot.entMap.find(del.entIdx);
			if (it == entSnapshot.entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delete non-existing ent");
				continue;
			}
			entSnapshot.entMap.erase(it);
			meshes.ents.anyStale = true;
		}

		entSnapshot.tick = delta.tick;

		if ((snapIdxLow + 1).IsValid() && (snapIdxLow + 1)->tick <= delta.tick)
			entSnapshot.snapshotIdx = ++snapIdxLow;
	}
	entSnapshot.tick = toTick;
}

void TrRenderingCache::BackwardIterateSnapshotDeltas(tr_tick toTick)
{
	VerifySnapshotState();
	TrIdx<TrEntSnapshot> snapIdxLow = entSnapshot.snapshotIdx;

	while ((entSnapshot.snapshotDeltaIdx - 1).IsValid() && entSnapshot.snapshotDeltaIdx->tick > toTick)
	{
		auto& delta = **entSnapshot.snapshotDeltaIdx--;

		for (auto& create : *delta.createSp)
		{
			auto it = entSnapshot.entMap.find(create.entIdx);
			if (it == entSnapshot.entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delete non-existing ent");
				continue;
			}
			entSnapshot.entMap.erase(it);
			meshes.ents.anyStale = true;
		}

		for (auto& transDelta : *delta.deltaSp)
		{
			auto it = entSnapshot.entMap.find(transDelta.entIdx);
			if (it == entSnapshot.entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delta non-existing ent");
				continue;
			}
			it->second = transDelta.fromTransIdx;
		}

		for (auto& del : *delta.deleteSp)
			entSnapshot.entMap.emplace(del.entIdx, del.oldTransIdx);

		entSnapshot.tick = delta.tick;

		if ((snapIdxLow - 1).IsValid() && snapIdxLow->tick >= delta.tick)
			entSnapshot.snapshotIdx = --snapIdxLow;
	}
	entSnapshot.tick = toTick;
}
void TrRenderingCache::RenderPlayerPath(MeshRendererDelegate& mr, const Vector& landmarkDeltaToFirstMap)
{
	RebuildPlayerPathMeshes();

	RenderCallback cb = [landmarkDeltaToFirstMap](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
	{ PositionMatrix(landmarkDeltaToFirstMap, infoOut.mat); };

	for (auto& m : meshes.playerPath.staticMeshes)
		mr.DrawMesh(m, cb);
	for (auto& m : meshes.playerPath.dynamicMeshes)
		mr.DrawMesh(m, cb);
}

void TrRenderingCache::RenderPlayerHull(MeshRendererDelegate& mr,
                                        const Vector& landmarkDeltaToMapAtTick,
                                        tr_tick atTick)
{
	auto pdIdx = tr->GetAtTick<TrPlayerData>(atTick);
	if (!pdIdx.IsValid())
		return;

	RebuildPlayerHullMeshes();
	RebuildEyeMeshes(pdIdx->fov == 0 ? 90.f : pdIdx->fov);

	const TrPlayerData& pd = **pdIdx;
	TrTransform qPhysTransform{pd.qPosIdx, {}};

	struct
	{
		StaticMesh& mesh;
		const TrTransform& trans;
		bool bDraw = true;
	} hulls[] = {
	    {(pd.m_fFlags & FL_DUCKING) ? meshes.qPhysDuck : meshes.qPhysStand, qPhysTransform},
	    {meshes.eyes, **pd.transEyesIdx},
	    {meshes.sgEyes, **pd.transSgEyesIdx, pd.transEyesIdx != pd.transSgEyesIdx},
	    {(pd.m_fFlags & FL_DUCKING) ? meshes.vPhysDuck : meshes.vPhysStand, **pd.transVPhysIdx},
	};

	for (auto& [mesh, trans, bDraw] : hulls)
	{
		if (!bDraw || !mesh.Valid())
			continue;
		Vector pos = **trans.posIdx + landmarkDeltaToMapAtTick;
		QAngle ang = trans.angIdx.IsValid() ? **trans.angIdx : vec3_angle;
		if (!pos.IsValid() || !ang.IsValid())
			continue;
		mr.DrawMesh(mesh,
		            [pos, ang](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
		            { AngleMatrix(ang, pos, infoOut.mat); });
	}

	if (spt_trace_draw_contact_points.GetBool())
	{
		for (auto contactPtIdx : *pd.contactPtsSp)
		{
			mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
			    [contactPtIdx, &pd, &landmarkDeltaToMapAtTick](MeshBuilderDelegate& mb)
			    {
				    Vector maxs{1.f};
				    const Vector& pos = **contactPtIdx->posIdx + landmarkDeltaToMapAtTick;
				    mb.AddBox(pos, -maxs, maxs, vec3_angle, trColors.playerHull.contactPt);
				    // DebugDrawContactPoints does (pt - norm * len), not sure why it's not (pt + norm * len)
				    mb.AddLine(pos,
				               pos - **contactPtIdx->normIdx * trStyles.playerHull.contactNormalLength,
				               trColors.playerHull.contactPt.lineColor);
			    }));
		}
	}
}

void TrRenderingCache::RenderPortals(MeshRendererDelegate& mr, const Vector& landmarkDeltaToMapAtTick, tr_tick atTick)
{
	RebuildPortalMeshes();
	auto snapIdx = tr->GetAtTick<TrPortalSnapshot>(atTick);
	if (!snapIdx.IsValid())
		return;

	for (auto idx : *snapIdx->portalsSp)
	{
		const TrPortal& portal = **idx;
		if (!portal.isActivated)
			continue; // not rendering these for now
		const StaticMesh& mesh = portal.isOpen
		                             ? (portal.isOrange ? meshes.openOrangePortal : meshes.openBluePortal)
		                             : (portal.isOrange ? meshes.closedOrangePortal : meshes.closedBluePortal);
		if (!mesh.Valid())
			continue;
		Vector pos = **portal.transIdx->posIdx + landmarkDeltaToMapAtTick;
		QAngle ang = **portal.transIdx->angIdx;
		mr.DrawMesh(mesh,
		            [pos, ang](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
		            { AngleMatrix(ang, pos, infoOut.mat); });
	}
}

void TrRenderingCache::RenderEntities(MeshRendererDelegate& mr, const Vector& landmarkDeltaToMapAtTick, tr_tick atTick)
{
	UpdateEntSnapshot(atTick);
	RebuildPhysMeshes();

	if (trStyles.entities.drawEntCollectRadius)
	{
		auto traceStateIdx = tr->GetAtTick<TrTraceState>(atTick);
		auto plDataIdx = tr->GetAtTick<TrPlayerData>(atTick);
		if (plDataIdx.IsValid() && plDataIdx->qPosIdx->IsValid() && traceStateIdx.IsValid())
		{
			mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
			    [&](MeshBuilderDelegate& mb)
			    {
				    mb.AddBox(**plDataIdx->qPosIdx + landmarkDeltaToMapAtTick,
				              **traceStateIdx->entCollectBboxAroundPlayerIdx->minsIdx,
				              **traceStateIdx->entCollectBboxAroundPlayerIdx->maxsIdx,
				              vec3_angle,
				              trColors.entities.collectAABB);
			    }));
		}
	}

	const char* allowedTriggers[] = {
	    "trigger_once",
	    "trigger_transition",
	    "trigger_changelevel",
	    "trigger_portal_cleanser",
	    "trigger_hurt",
	};
	uint32_t triggerFlags = FSOLID_NOT_SOLID | FSOLID_TRIGGER;

	// draw each OBB as a dynamic mesh, the mesh system will fuse them anyways
	for (auto [entIdx, transIdx] : entSnapshot.entMap)
	{
		if (entIdx->m_nSolidType == SOLID_NONE)
			continue;
		const char* className = *entIdx->classNameIdx;
		bool isTrigger = false;
		if ((entIdx->m_usSolidFlags & triggerFlags) == triggerFlags)
		{
			isTrigger = std::any_of(allowedTriggers,
			                        allowedTriggers + ARRAYSIZE(allowedTriggers),
			                        [className](const char* allowedTriggerName)
			                        { return !strcmp(className, allowedTriggerName); });
			if (!isTrigger)
				continue; // don't draw other trigger types
		}
		const TrAbsBox& obb = **transIdx->obbIdx;
		const TrTransform& trans = **transIdx->obbTransIdx;
		if (**obb.minsIdx == vec3_origin && **obb.maxsIdx == vec3_origin && **trans.posIdx == vec3_origin)
			continue; // don't draw ZERO OBBs
		mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
		    [=](MeshBuilderDelegate& mb)
		    {
			    Vector origin = **trans.posIdx + landmarkDeltaToMapAtTick;
			    // interfaces::debugOverlay->AddTextOverlay(origin, 0, "%s", className);
			    ShapeColor color = isTrigger ? trColors.entities.obbTrigger : trColors.entities.obb;
			    mb.AddBox(origin, **obb.minsIdx, **obb.maxsIdx, **trans.angIdx, color);
			    if (trStyles.entities.drawObbCenter && !isTrigger && **trans.posIdx != vec3_origin)
			    {
				    // a position at exactly the origin *probably* means it's not relevant
				    mb.AddCross(origin,
				                trStyles.entities.obbCenterCrossRadius,
				                trColors.entities.obb.lineColor);
			    }
		    }));
	}

	for (auto [entIdx, transIdx] : entSnapshot.entMap)
	{
		if (!spt_trace_draw_portal_collision_entities.GetBool()
		    && !strcmp(*entIdx->classNameIdx, "portalsimulator_collisionentity"))
		{
			continue;
		}

		auto physMeshTransIdxSp = *transIdx->physTransSp;
		auto physIdxsp = *entIdx->physSp;
		Assert(physIdxsp.size() == physMeshTransIdxSp.size());
		for (size_t i = 0; i < physIdxsp.size(); i++)
		{
			auto physMeshIt = meshes.ents.physObjs.find(physIdxsp[i]->meshIdx);
			if (physMeshIt != meshes.ents.physObjs.cend() && physMeshIt->second.mesh.Valid())
			{
				Vector pos = **physMeshTransIdxSp[i]->posIdx + landmarkDeltaToMapAtTick;
				QAngle ang = **physMeshTransIdxSp[i]->angIdx;
				mr.DrawMesh(physMeshIt->second.mesh,
				            [pos, ang](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
				            {
					            AngleMatrix(ang, pos, infoOut.mat);
					            // since the scale factor is small it should be fine that the OBB doesn't have the same fix
					            RenderCallbackZFightFix(infoIn, infoOut);
				            });
			}
			else
			{
				Assert(0);
			}
		}
	}
}

const Vector& TrRenderingCache::GetLandmarkOffsetToFirstMap(const char* fromMap)
{
	if (!fromMap)
		return vec3_origin;

	CacheLandmarkOffsetsToFirstMapFromTraceData();

	if (tr->Get<TrMap>().empty())
		return vec3_origin;

	auto [it, new_elem] = mapToFirstMapLandmarkOffset.try_emplace(fromMap);
	if (!new_elem)
		return it->second;

	/*
	* Iterate from the start of the trace and recompute the transition offset. At every map, check
	* if any of the landmarks match with the map we're loaded in. This is "slow", but I don't
	* expect more than a few transitions in the trace and a few landmarks for each map.
	* 
	* TODO: if the server is not active, we can't find landmarks and the cache will get borked. Fix that
	*/
	auto& landmarksInCurMap = utils::GetLandmarksInLoadedMap();
	TrIdx<TrMap> mapIdx = 0;
	Vector offFromFirst = vec3_origin;
	for (TrIdx<TrMapTransition> transitionIdx = 1u;; transitionIdx++)
	{
		for (auto& [curLandmarkName, curLandmarkOff] : landmarksInCurMap)
			for (auto& trLandmark : *mapIdx->landmarkSp)
				if (curLandmarkName == *trLandmark.nameIdx)
					return it->second = offFromFirst + curLandmarkOff - **trLandmark.posIdx;

		if (!transitionIdx.IsValid())
			break;

		offFromFirst -= tr->GetAdjacentLandmarkDelta(*transitionIdx->fromMapIdx->landmarkSp,
		                                             *transitionIdx->toMapIdx->landmarkSp);
		mapIdx = transitionIdx->toMapIdx;
	}
	return it->second = vec3_origin;
}

void TrRenderingCache::CacheLandmarkOffsetsToFirstMapFromTraceData()
{
	if (!mapToFirstMapLandmarkOffset.empty())
		return;

	if (tr->Get<TrMap>().empty())
		return;

	TrIdx<TrMap> mapIdx = 0;
	Vector offToFirstMap = vec3_origin;
	for (TrIdx<TrMapTransition> transitionIdx = 1u;; transitionIdx++)
	{
		mapToFirstMapLandmarkOffset[*mapIdx->nameIdx] = offToFirstMap;
		if (!transitionIdx.IsValid())
			break;
		offToFirstMap -= tr->GetAdjacentLandmarkDelta(*transitionIdx->fromMapIdx->landmarkSp,
		                                              *transitionIdx->toMapIdx->landmarkSp);
		mapIdx = transitionIdx->toMapIdx;
	}
}

void TrRenderingCache::RenderAll(MeshRendererDelegate& mr, tr_tick atTick)
{
	if (renderedLastTimeOnMap != utils::GetLoadedMap())
	{
		mapToFirstMapLandmarkOffset.clear();
		renderedLastTimeOnMap = utils::GetLoadedMap();
	}

	TrReadContextScope scope{*tr};
	atTick = tr->numRecordedTicks == 0 ? 0 : clamp(atTick, 0, tr->numRecordedTicks - 1);
	Vector landmarkdelta = GetLandmarkOffsetToFirstMap(utils::GetLoadedMap());
	RenderPlayerPath(mr, landmarkdelta);
	TrIdx<TrMap> atMap = tr->GetMapAtTick(atTick);
	if (!atMap.IsValid())
		return;
	landmarkdelta -= GetLandmarkOffsetToFirstMap(*atMap->nameIdx);
	RenderPlayerHull(mr, landmarkdelta, atTick);
	RenderPortals(mr, landmarkdelta, atTick);
	RenderEntities(mr, landmarkdelta, atTick);
}

#endif
