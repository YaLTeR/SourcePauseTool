#include "stdafx.h"

#include "..\mesh_builder.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "interfaces.hpp"

#include "..\mesh_defs_private.hpp"

/**************************************** MESH BUILDER INTERNAL ****************************************/

void MeshBuilderInternal::ClearOldBuffers()
{
	// keep only as many dynamic buffers as there were dynamic meshes last frame
	dynamicMeshData.resize(numDynamicMeshes);
	numDynamicMeshes = 0;
	dynamicMeshUnits.clear();
}

void MeshBuilderInternal::BeginNewMesh(bool dynamic)
{
	if (dynamic)
	{
		// We have to keep the buffers around for dynamics to create the IMesh* objects later. To prevent completely
		// reallocating the verts array for every dynamic mesh, we keep some old buffers around from the previous frame.
		if (numDynamicMeshes >= dynamicMeshData.size())
			dynamicMeshData.emplace_back();
		curMeshData = &dynamicMeshData[numDynamicMeshes++];
	}
	else
	{
		// static meshes immediately turn into IMesh* objects, so we can just reuse the same buffers
		curMeshData = &reusedStaticMeshData;
	}
	curMeshData->Clear();
}

MeshUnit* MeshBuilderInternal::FinalizeCurMesh(const CreateMeshParams& params)
{
	const MeshComponentData* meshData[] = {&curMeshData->faceData, &curMeshData->lineData};

	// figure out if the meshes are opaque
	bool opaque[2] = {true, true};
	for (int i = 0; i < 2; i++)
	{
		if ((i == 0 && (params.zTestFlags & ZTEST_FACES)) || (i == 1 && (params.zTestFlags & ZTEST_LINES)))
		{
			for (auto& vertData : meshData[i]->verts)
			{
				if (vertData.col.a < 255)
				{
					opaque[i] = false;
					break;
				}
			}
		}
		else
		{
			opaque[i] = false; // treat as translucent since that's when it'll have to be rendered
		}
	}

	// calculate the mesh "position" - iterate over all verts for lines/faces and get the AABB
	Vector mins;
	Vector maxs;
	if (curMeshData->Empty())
	{
		mins = maxs = vec3_origin;
	}
	else
	{
		mins = Vector(FLT_MAX);
		maxs = -mins;
		for (int i = 0; i < 2; i++)
		{
			for (auto& vertData : meshData[i]->verts)
			{
				VectorMin(vertData.pos, mins, mins);
				VectorMax(vertData.pos, maxs, maxs);
			}
		}
	}
	Vector center = (mins + maxs) / 2;
	const MeshPositionInfo pos{mins, maxs};

	if (curMeshData == &reusedStaticMeshData)
	{
		// static mesh, yeet into the heap and return it
		return new MeshUnit(-1, pos, params, opaque[0], opaque[1], false);
	}
	else
	{
		// dynamic mesh, we keep it in the mesh builder and delete at the start of every frame
		dynamicMeshUnits.emplace_back(dynamicMeshUnits.size(), pos, params, opaque[0], opaque[1], true);
		return nullptr; // can't point to vector element because reallocations will invalidate the pointer
	}
}

const MeshComponentData* MeshBuilderInternal::GetMeshData(DynamicComponentToken token, bool faces) const
{
	if (token < 0)
		return nullptr;
	auto& mData = dynamicMeshData[token];
	return faces ? &mData.faceData : &mData.lineData;
}

const MeshUnit& MeshBuilderInternal::GetMeshUnit(DynamicMesh dynMesh) const
{
	return dynamicMeshUnits[dynMesh.dynamicMeshIdx];
}

/**************************************** MESH BUILDER PRO ****************************************/

StaticMesh MeshBuilderPro::CreateStaticMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params)
{
	g_meshBuilderInternal.BeginNewMesh(false);
	MeshBuilderDelegate builderDelegate = {};
	createFunc(builderDelegate);
	return StaticMesh{g_meshBuilderInternal.FinalizeCurMesh(params)};
}

DynamicMesh MeshBuilderPro::CreateDynamicMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params)
{
	AssertMsg(g_inMeshRenderSignal, "spt: Must create dynamic meshes in MeshRenderSignal!");
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESHBUILDER);
	g_meshBuilderInternal.BeginNewMesh(true);
	MeshBuilderDelegate builderDelegate = {};
	createFunc(builderDelegate);
	g_meshBuilderInternal.FinalizeCurMesh(params);
	return {g_meshBuilderInternal.dynamicMeshUnits.size() - 1, g_meshRenderFrameNum};
}

#endif
