/*
* Purpose: private shared stuff between the mesh builder and renderer. Helps keep the public header(s) relatively
* clean and can be used by any other internal rendering files. You should never have to include this file as a
* user unless you want to do something quirky like adding custom verts/indices.
* 
* Here's a basic overview of the system:
* 
* We want a system that allows the user to create a mesh made of lines, faces, and different colors. At the end of
* the day we need to call CMatRenderContextPtr->GetDynamicMesh() & CMatRenderContextPtr->CraeteStaticMesh(). The
* latter can be used (almost) as-is, but the former needs to be filled every single time it's rendered.
* 
*  ┌────────────────────┐                                       ┌───────────────────────┐
*  │CreateCollideFeature│                                       │       MeshData        │
*  └────────┬───────────┘                                       │┌─────────────────────┐│
*           │                                                   ││MeshComponentData(2x)││
*           │can be used for                                    ││    ┌──────────┐     ││
*           ▼                                                   ││    │vector of │     ││
*  ┌───────────────────┐ edits state of ┌───────────────────┐   ││    │VertexData│     ││
*  │MeshBuilderDelegate├───────────────►│MeshBuilderInternal├──►││    └──────────┘     ││
*  └───────────────────┘                └───────────────────┘   └┴┬────────────────────┴┘
*   (stateless) ▲                                                 │
*               │gives user a delegate                            │converted to a MeshUnit
*               │                                                 │- immediately for statics
*    ┌──────────┴───┐                                             │- during rendering for dynamics
*    │MeshBuilderPro│                                             ▼
*    └──────────────┘                                        ┌──────────────────┐
*     (stateless)                                            │     MeshUnit     │
*                                                            │┌────────────────┐│
*  ┌───────────────────┐          returned to user           ││IMeshWrapper(2x)││
*  │Static/Dynamic Mesh│◄────────────────────────────────────┤│    ┌──────┐    ││
*  └────────┬──────────┘                                     ││    │IMesh*│    ││
*           │                                                ││    └──────┘    ││
*           │user gives to delegate                          └┴────────────────┴┘
*           ▼
* ┌────────────────────┐        ┌────────────────────────────┐    ┌───────────────────────┐
* │MeshRendererDelegate├───────►│std::vector<MeshUnitWrapper>├───►│DrawOpaqueRenderables()│
* └────────────────────┘ queues └───────────────────────────┬┘    └───────────────────────┘
*  (stateless) ▲                                            │ rendered
*              │gives user a delegate                       │   ┌────────────────────────────┐
*              │                                            └──►│DrawTranslucentRenderables()│
*  ┌───────────┴───────┐ signals pre-frame ┌───────────┐        └────────────────────────────┘
*  │MeshRendererFeature│◄──────────────────┤spt_overlay│
*  └───────────────────┘                   └───────────┘
* 
* A brief description of some of the main classes:
* 
* MeshBuilderInternal - holds temporary MeshData buffers for static/dynamic meshes
* MeshComponentData - holds the verts & indices needed for mesh creation
* IMeshWrapper - a wrapper of the game's mesh representation, used to store or make IMesh* objects
* MeshUnit - a fully cooked and seasoned mesh ready for rendering (or has enough data to create one)
* MeshRendererFeature - handles loading/unloading of the whole system, rendering, and render callbacks
* MeshUnitWrapper - wraps static/dynamic meshes, also handles rendering and render callbacks
* 
* The purpose of having stateless classes is to keep public headers clean (state is managed by internal classes).
* The purpose of using std::function for mesh building/rendering is to enforce proper initialization and cleanup.
* The purpose of using delegates is for shorter variable names.
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

#define VPROF_BUDGETGROUP_MESHBUILDER _T("Mesh_Builder")
#define VPROF_BUDGETGROUP_MESH_RENDERER _T("Mesh_Renderer")

#include <vector>

inline int g_meshRenderFrameNum = 0;
inline bool g_inMeshRenderSignal = false;

// a single vertex - may have position, color, uv coordinates, normal, etc
struct VertexData
{
	Vector pos;
	color32 col;
};

// list of verts + indices
struct MeshComponentData
{
	std::vector<VertexData> verts;
	std::vector<unsigned short> indices;

	MeshComponentData() = default;
	MeshComponentData(MeshComponentData&) = delete;
	MeshComponentData(MeshComponentData&&) = default;

	bool Empty() const;
	void Clear();

	void Reserve(size_t numExtraVerts, size_t numExtraIndices);
	IMesh* CreateIMesh(IMaterial* material, const CreateMeshParams& params, bool faces, bool dynamic) const;
};

// two components - faces/lines
struct MeshData
{
	MeshComponentData faceData, lineData;

	MeshData() = default;
	MeshData(MeshData&) = delete;
	MeshData(MeshData&&) = default;

	bool Empty() const;
	void Clear();
};

typedef int DynamicComponentToken;

// two IMeshWrappers - faces/lines
struct MeshUnit
{
	// either an IMesh*, or contains enough data to create one
	struct IMeshWrapper
	{
		union
		{
			IMesh* iMesh;
			DynamicComponentToken dynamicToken;
		};
		const bool opaque, faces, dynamic, zTest;

		IMeshWrapper(const MeshUnit& myWrapper, int mDataIdx, bool opaque, bool faces, bool dynamic);
		IMeshWrapper(IMeshWrapper&) = delete;
		IMeshWrapper(IMeshWrapper&&);
		~IMeshWrapper();
		bool Empty() const;
		IMesh* GetIMesh(const MeshUnit& outer, IMaterial* material) const;
	};

	const MeshPositionInfo posInfo;
	const CreateMeshParams params; // MUST BE BEFORE MESH COMPONENTS FOR CORRECT INITIALIZATION ORDER
	IMeshWrapper faceComponent, lineComponent;

	MeshUnit(int mDataIdx,
	         const MeshPositionInfo& posInfo,
	         const CreateMeshParams& params,
	         bool facesOpaque,
	         bool linesOpaque,
	         bool dynamic);
	MeshUnit(MeshUnit&) = delete;
	MeshUnit(MeshUnit&&) = default;
	~MeshUnit() = default;

	bool Empty();
};

struct MeshBuilderInternal
{
	MeshData* curMeshData = nullptr;
	MeshData reusedStaticMeshData;
	std::vector<MeshData> dynamicMeshData;
	size_t numDynamicMeshes = 0;

	std::vector<MeshUnit> dynamicMeshUnits;

	void ClearOldBuffers();
	void BeginNewMesh(bool dynamic);
	MeshUnit* FinalizeCurMesh(const CreateMeshParams& params);
	const MeshComponentData* GetMeshData(DynamicComponentToken token, bool faces) const;
	const MeshUnit& GetMeshUnit(DynamicMesh dynMesh) const;
};

inline MeshBuilderInternal g_meshBuilderInternal;

struct MeshBuilderMatMgr
{
	IMaterial *matOpaque = nullptr, *matAlpha = nullptr, *matAlphaNoZ = nullptr;
	bool materialsInitialized = false;

	void Load();
	void Unload();
	IMaterial* GetMaterial(bool opaque, bool zTest, color32 colorMod);
};

inline MeshBuilderMatMgr g_meshMaterialMgr;

#endif
