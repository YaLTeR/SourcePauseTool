/*
* Purpose: private shared stuff between the mesh builder and renderer. Helps keep the public header(s) relatively
* clean and can be used by any other internal rendering files. You should never have to include this file as a
* user unless you want to do something quirky like adding custom verts/indices.
*/

#pragma once

#include "mesh_defs_public.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "mathlib\vector.h"
#include "materialsystem\imaterial.h"
#include "materialsystem\imesh.h"

#define VPROF_LEVEL 1
#ifndef SSDK2007
#define RAD_TELEMETRY_DISABLED
#endif
#include "vprof.h"

#include <vector>

inline int g_meshRenderFrameNum = 0;
inline bool g_inMeshRenderSignal = false;

// a single vertex - may have position, color, uv coordinates, normal, etc
struct VertexData
{
	Vector pos;
	color32 col;
};

// list of verts + indices into that list
struct MeshComponentData
{
	std::vector<VertexData> verts;
	std::vector<unsigned short> indices;

	MeshComponentData() = default;
	MeshComponentData(MeshComponentData&) = delete;
	MeshComponentData(MeshComponentData&&) = default;

	inline bool Empty() const
	{
		return verts.size() == 0;
	};

	inline void Clear()
	{
		verts.clear();
		indices.clear();
	}

	void Reserve(size_t numExtraVerts, size_t numExtraIndices);
	IMesh* CreateIMesh(IMaterial* material, const CreateMeshParams& params, bool faces, bool dynamic) const;
};

// verts & indices for faces & lines
struct MeshData
{
	MeshComponentData faceData, lineData;

	MeshData() = default;
	MeshData(MeshData&) = delete;
	MeshData(MeshData&&) = default;

	inline bool Empty() const
	{
		return faceData.Empty() && lineData.Empty();
	}

	inline void Clear()
	{
		faceData.Clear();
		lineData.Clear();
	}
};

typedef int DynamicComponentToken;

// face & line component
struct MeshWrapper
{
	// either an IMesh*, or contains enough data to create one
	struct MeshWrapperComponent
	{
		union
		{
			IMesh* iMesh;
			DynamicComponentToken dynamicToken;
		};
		const bool opaque, faces, dynamic, zTest;

		MeshWrapperComponent(const MeshWrapper& myWrapper, int mDataIdx, bool opaque, bool faces, bool dynamic);
		MeshWrapperComponent(MeshWrapperComponent&) = delete;
		MeshWrapperComponent(MeshWrapperComponent&&);
		~MeshWrapperComponent();
		bool Empty() const;
		IMesh* GetIMesh(const MeshWrapper& outer, IMaterial* material) const;
	};

	const MeshPositionInfo posInfo;
	const CreateMeshParams params; // MUST BE BEFORE MESH COMPONENTS FOR CORRECT INITIALIZATION ORDER
	MeshWrapperComponent faceComponent, lineComponent;

	MeshWrapper(int mDataIdx,
	            const MeshPositionInfo& posInfo,
	            const CreateMeshParams& params,
	            bool facesOpaque,
	            bool linesOpaque,
	            bool dynamic);
	MeshWrapper(MeshWrapper&) = delete;
	MeshWrapper(MeshWrapper&&) = default;
	~MeshWrapper() = default;

	bool Empty();
};

// the public mesh builder doesn't have any state, it's stored in this internal version instead
struct MeshBuilderInternal
{
	MeshData* curMeshData = nullptr;
	MeshData reusedStaticMeshData;
	std::vector<MeshData> dynamicMeshData;
	size_t numDynamicMeshes = 0;

	std::vector<MeshWrapper> dynamicMeshWrappers;

	void ClearOldBuffers();
	void BeginNewMesh(bool dynamic);
	MeshWrapper* FinalizeCurMesh(const CreateMeshParams& params);

	inline const MeshComponentData* GetMeshData(DynamicComponentToken token, bool faces) const
	{
		if (token < 0)
			return nullptr;
		auto& mData = dynamicMeshData[token];
		return faces ? &mData.faceData : &mData.lineData;
	}

	inline const MeshWrapper& GetMeshWrapper(DynamicMesh dynMesh) const
	{
		return dynamicMeshWrappers[dynMesh.dynamicMeshIdx];
	}
};

inline MeshBuilderInternal g_meshBuilder;

struct MeshBuilderMatMgr
{
	IMaterial *matOpaque = nullptr, *matAlpha = nullptr, *matAlphaNoZ = nullptr;
	bool materialsInitialized = false;

	void Load();
	void Unload();
	IMaterial* GetMaterial(bool opaque, bool zTest, color32 colorMod);
};

inline MeshBuilderMatMgr g_meshBuilderMaterials;

#endif
