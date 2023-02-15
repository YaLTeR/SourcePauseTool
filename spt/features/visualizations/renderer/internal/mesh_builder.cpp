#include "stdafx.hpp"

#include <algorithm>

#include "..\mesh_builder.hpp"
#include "mesh_builder_internal.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "interfaces.hpp"
#include "spt\utils\game_detection.hpp"

#include "internal_defs.hpp"

// TODO use this to implement auto-spilling?
void GetMaxMeshSize(size_t& maxVerts, size_t& maxIndices, bool dynamic)
{
	maxVerts = 32768;
	maxIndices = 32768;
	if (dynamic)
		return;

	if (utils::DoesGameLookLikePortal())
	{
		if (utils::GetBuildNumber() >= 5135)
		{
			maxVerts = 65536;
			maxIndices = 999'999; // unknown upper limit
		}
	}
	// else unkown, assume a safe lower limit
}

/**************************************** MESH VERT DATA ****************************************/

MeshVertData::MeshVertData(MeshVertData&& other)
    : verts(std::move(other.verts)), indices(std::move(other.indices)), type(other.type), material(other.material)
{
	other.material = nullptr;
}

MeshVertData::MeshVertData(std::vector<VertexData>& vertDataVec,
                           std::vector<VertIndex>& vertIndexVec,
                           MeshPrimitiveType type,
                           MaterialRef material)
    : verts(vertDataVec), indices(vertIndexVec), type(type), material(material)
{
}

MeshVertData& MeshVertData::operator=(MeshVertData&& other)
{
	if (this != &other)
	{
		verts = std::move(other.verts);
		indices = std::move(other.indices);
		type = other.type;
		material = other.material;
		other.material = nullptr;
	}
	return *this;
}

bool MeshVertData::Empty() const
{
	return verts.size() == 0 || indices.size() == 0;
}

/**************************************** MESH UNITS ****************************************/

DynamicMeshUnit::DynamicMeshUnit(VectorSlice<MeshVertData>& vDataSlice, const MeshPositionInfo& posInfo)
    : vDataSlice(std::move(vDataSlice)), posInfo(posInfo)
{
}

DynamicMeshUnit::DynamicMeshUnit(DynamicMeshUnit&& other)
    : vDataSlice(std::move(other.vDataSlice)), posInfo(other.posInfo)
{
}

StaticMeshUnit::StaticMeshUnit(size_t nMeshes, const MeshPositionInfo& posInfo)
    : meshesArr(new IMeshWrapper[nMeshes]), nMeshes(nMeshes), posInfo(posInfo)
{
}

StaticMeshUnit::~StaticMeshUnit()
{
	for (size_t i = 0; i < nMeshes; i++)
		CMatRenderContextPtr(interfaces::materialSystem)->DestroyStaticMesh(meshesArr[i].iMesh);
	delete[] meshesArr;
}

/**************************************** MESH BUILDER INTERNAL ****************************************/

MeshVertData& MeshBuilderInternal::GetComponentInCurrentMesh(MeshPrimitiveType type, MaterialRef material)
{
	// look through all components that we already have in the current mesh and see if any match the type/material
	auto vDataIt = std::find_if(curMeshVertData.begin(),
	                            curMeshVertData.end(),
	                            [=](const MeshVertData& vd) { return vd.type == type && vd.material == material; });

	if (vDataIt != curMeshVertData.end())
		return *vDataIt;

	/*
	* We didn't find a matching component, so we need to create a new one. If this is the Nth component, we'll
	* find the Nth shared list (for verts & indices) to initialize the component from. If there aren't N shared
	* lists, then we'll just add a new one to the end of the linked lists.
	*/

	auto vertIt = sharedLists.verts.begin();
	auto idxIt = sharedLists.indices.begin();

	if (vertIt == sharedLists.verts.end())
	{
		Assert(curMeshVertData.size() == 0);
		Assert(idxIt == sharedLists.indices.end());
		return curMeshVertData.emplace_back(sharedLists.verts.emplace_front(),
		                                    sharedLists.indices.emplace_front(),
		                                    type,
		                                    material);
	}

	for (size_t i = 0; i < curMeshVertData.size(); i++)
	{
		auto prevVertIt = vertIt++;
		auto prevIdxIt = idxIt++;
		if (vertIt == sharedLists.verts.end())
		{
			Assert(idxIt == sharedLists.indices.end());
			vertIt = sharedLists.verts.emplace_after(prevVertIt);
			idxIt = sharedLists.indices.emplace_after(prevIdxIt);
		}
	}
	return curMeshVertData.emplace_back(*vertIt, *idxIt, type, material);
}

IMeshWrapper MeshBuilderInternal::_CreateIMeshFromInterval(ConstCompIntrvl intrvl,
                                                           size_t totalVerts,
                                                           size_t totalIndices,
                                                           bool dynamic)
{
	if (totalVerts == 0 || totalIndices == 0)
		return IMeshWrapper{};

#ifdef DEBUG
	size_t maxVerts, maxIndices;
	GetMaxMeshSize(maxVerts, maxIndices, dynamic);
	Assert(totalVerts <= maxVerts);
	Assert(totalIndices <= maxIndices);
	Assert(intrvl.first->vertData);
	Assert(totalVerts <= totalIndices);
	for (auto it = intrvl.first + 1; it < intrvl.second; it++)
	{
		Assert(it->vertData);
		Assert(intrvl.first->vertData->type == it->vertData->type);
		Assert(intrvl.first->vertData->material == it->vertData->material);
	}
#endif

	MaterialRef material = intrvl.first->vertData->material;

	if (!material)
		return IMeshWrapper{};

	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESHBUILDER);

	CMatRenderContextPtr context{interfaces::materialSystem};
	context->Bind(material);

	if (!dynamic && material->GetVertexFormat() == VERTEX_FORMAT_UNKNOWN)
	{
		AssertMsg(0, "We tried so hard, but in the end it doesn't even matter");
		Warning("spt: Static mesh material vertex format is unknown but shouldn't be. Grab a programmer!\n");
		return IMeshWrapper{};
	}

	IMesh* iMesh;
	if (dynamic)
		iMesh = context->GetDynamicMesh(true, nullptr, nullptr, material);
	else
		iMesh = context->CreateStaticMesh(material->GetVertexFormat() & ~VERTEX_FORMAT_COMPRESSED,
		                                  TEXTURE_GROUP_STATIC_VERTEX_BUFFER_WORLD,
		                                  material);

	if (!iMesh)
	{
		AssertMsg(0, "Didn't get an IMesh* object");
		return IMeshWrapper{};
	}

	switch (intrvl.first->vertData->type)
	{
	case MeshPrimitiveType::Lines:
		iMesh->SetPrimitiveType(MATERIAL_LINES);
		Assert(totalIndices % 2 == 0);
		break;
	case MeshPrimitiveType::Triangles:
		iMesh->SetPrimitiveType(MATERIAL_TRIANGLES);
		Assert(totalIndices % 3 == 0);
		break;
	default:
		AssertMsg(0, "Unknown mesh primitive type");
		return IMeshWrapper{};
	}

	// now we can fill the IMesh buffers

	MeshDesc_t desc;
	iMesh->LockMesh(totalVerts, totalIndices, desc);

	size_t vertIdx = 0;
	size_t idxIdx = 0; // ;)
	size_t idxOffset = 0;

	for (auto it = intrvl.first; it < intrvl.second; it++)
	{
		for (const VertexData& vert : it->vertData->verts)
		{
			*(Vector*)((uintptr_t)desc.m_pPosition + vertIdx * desc.m_VertexSize_Position) = vert.pos;
			unsigned char* pColor = desc.m_pColor + vertIdx * desc.m_VertexSize_Color;
			pColor[0] = vert.col.b;
			pColor[1] = vert.col.g;
			pColor[2] = vert.col.r;
			pColor[3] = vert.col.a;
			vertIdx++;
		}
		for (VertIndex vIdx : it->vertData->indices)
			desc.m_pIndices[idxIdx++] = vIdx + desc.m_nFirstVertex + idxOffset;
		idxOffset = vertIdx;
	}
	AssertEquals(vertIdx, totalVerts);
	AssertEquals(idxIdx, totalIndices);

	iMesh->UnlockMesh(totalVerts, totalIndices, desc);

	return {iMesh, material};
}

void MeshBuilderInternal::BeginIMeshCreation(ConstCompIntrvl intrvl, bool dynamic)
{
	creationState.curIntrvl = intrvl;
	creationState.dynamic = dynamic;
	GetMaxMeshSize(creationState.maxVerts, creationState.maxIndices, dynamic);
}

IMeshWrapper MeshBuilderInternal::GetNextIMeshWrapper()
{
	auto& cs = creationState;
	size_t totalNumVerts, totalNumIndices;
	for (;;)
	{
		if (cs.curIntrvl.first == cs.curIntrvl.second)
			return IMeshWrapper{}; // done with iteration

		totalNumVerts = cs.curIntrvl.first->vertData->verts.size();
		totalNumIndices = cs.curIntrvl.first->vertData->indices.size();

		if (totalNumIndices > creationState.maxIndices || totalNumVerts > creationState.maxVerts)
		{
			AssertMsg(0, "too many verts/indices");
			Warning("spt: mesh has too many verts or indices, this would cause a game error\n");
			cs.curIntrvl.first++; // skip this component
			continue;
		}
		else
		{
			break;
		}
	}
	cs.fusedIntrvl = ConstCompIntrvl{cs.curIntrvl.first, cs.curIntrvl.first + 1};
	for (; cs.fusedIntrvl.second < cs.curIntrvl.second; cs.fusedIntrvl.second++)
	{
		size_t curNumVerts = cs.fusedIntrvl.second->vertData->verts.size();
		size_t curNumIndices = cs.fusedIntrvl.second->vertData->indices.size();
		Assert(curNumIndices >= curNumVerts);

		if (totalNumIndices + curNumIndices > creationState.maxIndices
		    || totalNumVerts + curNumVerts > creationState.maxVerts)
		{
			break;
		}
		totalNumVerts += curNumVerts;
		totalNumIndices += curNumIndices;
	}
	cs.curIntrvl.first = cs.fusedIntrvl.second;
	return _CreateIMeshFromInterval(cs.fusedIntrvl, totalNumVerts, totalNumIndices, cs.dynamic);
}

void MeshBuilderInternal::FrameCleanup()
{
	while (!dynamicMeshUnits.empty())
		dynamicMeshUnits.pop();
#ifdef DEBUG
	for (auto& vertArray : sharedLists.verts)
		Assert(vertArray.size() == 0);
	for (auto& idxArray : sharedLists.indices)
		Assert(idxArray.size() == 0);
#endif
}

MeshPositionInfo MeshBuilderInternal::_CalcPosInfoForCurrentMesh()
{
	MeshPositionInfo pi{Vector{INFINITY}, Vector{-INFINITY}};
	for (MeshVertData& vData : curMeshVertData)
	{
		for (VertexData& vert : vData.verts)
		{
			VectorMin(vert.pos, pi.mins, pi.mins);
			VectorMax(vert.pos, pi.maxs, pi.maxs);
		}
	}
	for (int i = 0; i < 3; i++)
	{
		Assert(pi.mins[i] > -1e30);
		Assert(pi.maxs[i] < 1e30);
	}
	return pi;
}

const DynamicMeshUnit& MeshBuilderInternal::GetDynamicMeshFromToken(DynamicMeshToken token) const
{
	return dynamicMeshUnits._Get_container()[token.dynamicMeshIdx];
}

/**************************************** MESH BUILDER PRO ****************************************/

StaticMesh MeshBuilderPro::CreateStaticMesh(const MeshCreateFunc& createFunc)
{
	auto& builder = g_meshBuilderInternal;
	builder.curMeshVertData.assign_to_end(builder.sharedLists.vertData);

	MeshBuilderDelegate builderDelegate{};
	createFunc(builderDelegate);

	builder.curMeshVertData.erase_if([](const MeshVertData& vd) { return vd.Empty(); });

	StaticMeshUnit* mu = new StaticMeshUnit{builder.curMeshVertData.size(), builder._CalcPosInfoForCurrentMesh()};

	for (size_t i = 0; i < builder.curMeshVertData.size(); i++)
	{
		auto& vd = builder.curMeshVertData[i];
		if (!vd.Empty())
		{
			static std::vector<MeshComponent> tmp{1};
			tmp[0] = {nullptr, &vd, IMeshWrapper{}};
			mu->meshesArr[i] = builder._CreateIMeshFromInterval(ConstCompIntrvl{tmp.begin(), tmp.end()},
			                                                    vd.verts.size(),
			                                                    vd.indices.size(),
			                                                    false);
		}
	}
	builder.curMeshVertData.pop();
	return StaticMesh{mu};
}

DynamicMesh MeshBuilderPro::CreateDynamicMesh(const MeshCreateFunc& createFunc)
{
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESHBUILDER);
	auto& builder = g_meshBuilderInternal;
	builder.curMeshVertData.assign_to_end(builder.sharedLists.vertData);

	MeshBuilderDelegate builderDelegate{};
	createFunc(builderDelegate);

	builder.curMeshVertData.erase_if([](const MeshVertData& vd) { return vd.Empty(); });

	MeshPositionInfo posInfo = builder._CalcPosInfoForCurrentMesh();
	builder.dynamicMeshUnits.emplace(builder.curMeshVertData, posInfo);
	return {builder.dynamicMeshUnits.size() - 1, g_meshRenderFrameNum};
}

#endif
