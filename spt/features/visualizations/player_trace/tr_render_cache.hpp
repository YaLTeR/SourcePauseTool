#pragma once

#include <unordered_set>
#include <unordered_map>

#include "tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

namespace player_trace
{
	class TrRenderingCache
	{
	public:
		TrRenderingCache(const TrPlayerTrace& tr);

	private:
		friend struct TrPlayerTrace;
		const TrPlayerTrace* tr;
		std::unordered_map<std::string, Vector> mapToFirstMapLandmarkOffset;

		void RebuildPlayerHullMeshes();
		void RebuildEyeMeshes(float fov);
		void RebuildPlayerPathMeshes();
		void RebuildPortalMeshes();
		void RebuildPhysMeshes();

		void RenderPlayerPath(MeshRendererDelegate& mr, const Vector& landmarkDeltaToFirstMap);
		void RenderPlayerHull(MeshRendererDelegate& mr, const Vector& landmarkDeltaToMapAtTick, tr_tick atTick);
		void RenderPortals(MeshRendererDelegate& mr, const Vector& landmarkDeltaToMapAtTick, tr_tick atTick);
		void RenderEntities(MeshRendererDelegate& mr, const Vector& landmarkDeltaToMapAtTick, tr_tick atTick);

		void VerifySnapshotState() const;
		void UpdateEntSnapshot(tr_tick toTick);
		void JumpToEntSnapshot(TrIdx<TrEntSnapshot> snapIdx);
		void ForwardIterateSnapshotDeltas(tr_tick toTick);
		void BackwardIterateSnapshotDeltas(tr_tick toTick);

		/*
		* The player path coordinates are computed relative to the first map of the trace, but in order
		* to draw correctly in multi-map traces, we have to transform the whole thing.
		* 
		* Say we have the maps which are connected with landmarks: A <-> B <-> C <-> D
		* 
		* Case 1: we have a trace that goes in maps A->B:
		* - When you're in map A, no offset is applied.
		* - When you're in map B, an offset of A-B is applied to the path.
		* - When you're in map C, an offset of B-C is applied to the path (we can look at the landmarks in the current map).
		* - When you're in map D, no offset is applied since the trace does not have enough landmark data.
		* 
		* Case 2: we have a trace that is only in map A:
		* - When you're in map A, no offset is applied.
		* - When you're in map B, an offset of A-B is applied to the path.
		* - When you're in map C or D, no offset is applied since the trace does not have enough landmark data.
		* 
		* This means that different traces may get different map transforms applied depending on how
		* much landmark data they have. This is usually not gonna be a problem because if you're
		* loading the trace in a map two or more transitions away you're not gonna care about how it
		* lines up with the map.
		*/
		const Vector& GetLandmarkOffsetToFirstMap(const char* fromMap);
		void CacheLandmarkOffsetsToFirstMapFromTraceData();

		struct Meshes
		{
			StaticMesh qPhysDuck, qPhysStand;
			StaticMesh vPhysDuck, vPhysStand;
			StaticMesh eyes, sgEyes;
			StaticMesh openBluePortal, openOrangePortal, closedBluePortal, closedOrangePortal;

			TrPlayerCameraDrawType camType;
			float eyeMeshFov;
			bool playerPathGeneratedWithCones;

			struct
			{
				std::vector<StaticMesh> staticMeshes;
				std::vector<DynamicMesh> dynamicMeshes;
				tr_tick staticMeshesBuiltUpToTick = 0;
			} playerPath;

			struct EntityCache
			{
				struct TrackedMesh
				{
					StaticMesh mesh;
					bool isActive = false;
				};

				std::unordered_map<TrIdx<TrPhysMesh>, TrackedMesh> physObjs;
				bool anyStale = false;
			} ents;

		} meshes;

		struct EntSnapshot
		{
			std::unordered_map<TrIdx<TrEnt>, TrIdx<TrEntTransform>> entMap;
			TrIdx<TrEntSnapshot> snapshotIdx = 0;
			TrIdx<TrEntSnapshotDelta> snapshotDeltaIdx = 0;
			tr_tick tick = 0;
			bool initialized = false;
		} entSnapshot;

		std::string renderedLastTimeOnMap;

	public:
		void RenderAll(MeshRendererDelegate& mr, tr_tick atTick);
	};

} // namespace player_trace

#endif
