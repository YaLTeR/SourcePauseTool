#include "stdafx.hpp"

#include <algorithm>

#include "..\mesh_builder.hpp"
#include "mesh_builder_internal.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "interfaces.hpp"
#include "spt\utils\game_detection.hpp"

#include "internal_defs.hpp"
#include "mesh_renderer_internal.hpp"

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

IMeshWrapper MeshBuilderInternal::Fuser::CreateIMeshFromSpan(std::span<const MeshComponent> span,
                                                             size_t totalVerts,
                                                             size_t totalIndices)
{
	if (totalVerts == 0 || totalIndices == 0)
		return IMeshWrapper{};

	SPT_VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);

#ifdef DEBUG
	Assert(totalVerts <= maxVerts);
	Assert(totalIndices <= maxIndices);
	Assert(span.front().vertData);
	Assert(totalVerts <= totalIndices);
	for (auto& mc : span.subspan(1))
	{
		Assert(mc.vertData);
		Assert(span.front().vertData->type == mc.vertData->type);
		Assert(span.front().vertData->material == mc.vertData->material);
	}
#endif

	MaterialRef material = span.front().vertData->material;

	if (!material)
		return IMeshWrapper{};

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

	switch (span.front().vertData->type)
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

	for (auto& mc : span)
	{
		for (const VertexData& vert : mc.vertData->verts)
		{
			*(Vector*)((uintptr_t)desc.m_pPosition + vertIdx * desc.m_VertexSize_Position) = vert.pos;
			unsigned char* pColor = desc.m_pColor + vertIdx * desc.m_VertexSize_Color;
			pColor[0] = vert.col.b;
			pColor[1] = vert.col.g;
			pColor[2] = vert.col.r;
			pColor[3] = vert.col.a;
			vertIdx++;
		}
		for (VertIndex vIdx : mc.vertData->indices)
			desc.m_pIndices[idxIdx++] = vIdx + desc.m_nFirstVertex + idxOffset;
		idxOffset = vertIdx;
	}
	AssertEquals(vertIdx, totalVerts);
	AssertEquals(idxIdx, totalIndices);

	iMesh->UnlockMesh(totalVerts, totalIndices, desc);

	return {iMesh, material};
}

void MeshBuilderInternal::Fuser::BeginIMeshCreation(std::span<const MeshComponent> span, bool _dynamic)
{
	curSpan = span;
	dynamic = _dynamic;
	GetMaxMeshSize(maxVerts, maxIndices, _dynamic);
}

IMeshWrapper MeshBuilderInternal::Fuser::GetNextIMeshWrapper()
{
	size_t totalNumVerts, totalNumIndices;
	for (;;)
	{
		if (curSpan.empty())
			return IMeshWrapper{}; // done with iteration

		totalNumVerts = curSpan.front().vertData->verts.size();
		totalNumIndices = curSpan.front().vertData->indices.size();

		if (totalNumIndices > maxIndices || totalNumVerts > maxVerts)
		{
			AssertMsg(0, "too many verts/indices");
			Warning("spt: mesh has too many verts or indices, this would cause a game error\n");
			curSpan = curSpan.subspan(1); // skip this component
			continue;
		}
		else
		{
			break;
		}
	}
	size_t numComponents = 1;
	for (; numComponents < curSpan.size(); numComponents++)
	{
		size_t curNumVerts = curSpan[numComponents].vertData->verts.size();
		size_t curNumIndices = curSpan[numComponents].vertData->indices.size();
		Assert(curNumIndices >= curNumVerts);

		if (totalNumIndices + curNumIndices > maxIndices || totalNumVerts + curNumVerts > maxVerts)
		{
			break;
		}
		totalNumVerts += curNumVerts;
		totalNumIndices += curNumIndices;
	}
	lastFusedSpan = curSpan.first(numComponents);
	curSpan = curSpan.subspan(numComponents);
	return CreateIMeshFromSpan(lastFusedSpan, totalNumVerts, totalNumIndices);
}

void MeshBuilderInternal::FrameCleanup()
{
	while (!dynamicMeshUnits.empty())
		dynamicMeshUnits.pop();
#ifdef DEBUG
	for (auto& vertArray : sharedLists.simple.verts)
		Assert(vertArray.size() == 0);
	for (auto& idxArray : sharedLists.simple.indices)
		Assert(idxArray.size() == 0);
#endif
}

const DynamicMeshUnit& MeshBuilderInternal::GetDynamicMeshFromToken(DynamicMeshToken token) const
{
	return dynamicMeshUnits._Get_container()[token.dynamicMeshIdx];
}

void MeshBuilderInternal::TmpMesh::Create(const MeshCreateFunc& createFunc, bool dynamic)
{
	// check that we don't have any existing data
	Assert(!std::count_if(components.cbegin(),
	                      components.cend(),
	                      [](const MeshVertData& vd) { return vd.verts.vec || vd.indices.vec || vd.material; }));

	// initialize the simple components
	components.resize(MAX_SIMPLE_COMPONENTS);
	for (size_t i = 0; i < (size_t)MeshPrimitiveType::Count; i++)
	{
		for (size_t j = 0; j < (size_t)MeshMaterialSimple::Count; j++)
		{
			size_t k = SIMPLE_COMPONENT_INDEX(i, j);
			components[k].verts.assign_to_end(g_meshBuilderInternal.sharedLists.simple.verts[k]);
			components[k].indices.assign_to_end(g_meshBuilderInternal.sharedLists.simple.indices[k]);
			components[k].type = (MeshPrimitiveType)i;
			components[k].material = g_meshMaterialMgr.GetMaterial((MeshMaterialSimple)j);
		}
	}

	// used by the delegate to check if the temp mesh is too big
	GetMaxMeshSize(maxVerts, maxIndices, dynamic);

	// let the user fill the tmp mesh buffers
	MeshBuilderDelegate builderDelegate{};
	createFunc(builderDelegate);
}

MeshPositionInfo MeshBuilderInternal::TmpMesh::CalcPosInfo()
{
	MeshPositionInfo pi{Vector{INFINITY}, Vector{-INFINITY}};
	for (MeshVertData& vData : components)
	{
		for (VertexData& vert : vData.verts)
		{
			VectorMin(vert.pos, pi.mins, pi.mins);
			VectorMax(vert.pos, pi.maxs, pi.maxs);
		}
	}
	// this isn't strictly necessary, but infinities and NaNs might mess with the system so best to avoid them
	for (int i = 0; i < 3; i++)
		AssertMsg(pi.mins[i] > -1e30 && pi.maxs[i] < 1e30, "mesh likely contains weird point(s)");

	return pi;
}

/**************************************** MESH BUILDER PRO ****************************************/

StaticMesh MeshBuilderPro::CreateStaticMesh(const MeshCreateFunc& createFunc)
{
	SPT_VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);
	auto& tmpMesh = g_meshBuilderInternal.tmpMesh;
	tmpMesh.Create(createFunc, false);

	size_t numPopulated = std::count_if(tmpMesh.components.cbegin(),
	                                    tmpMesh.components.cend(),
	                                    [](const MeshVertData& vd) { return !vd.Empty(); });

	StaticMeshUnit* mu = new StaticMeshUnit{numPopulated, tmpMesh.CalcPosInfo()};

	for (size_t i = tmpMesh.components.size(), muIdx = 0; i-- > 0;)
	{
		auto& vd = tmpMesh.components[i];
		if (!vd.Empty())
		{
			MeshComponent mc{.vertData = &vd};
			g_meshBuilderInternal.fuser.BeginIMeshCreation({&mc, 1}, false);
			mu->meshesArr[muIdx++] = g_meshBuilderInternal.fuser.GetNextIMeshWrapper();
		}
		tmpMesh.components.pop_back();
	}
	return StaticMesh{mu};
}

DynamicMesh MeshBuilderPro::CreateDynamicMesh(const MeshCreateFunc& createFunc)
{
	SPT_VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);
	auto& tmpMesh = g_meshBuilderInternal.tmpMesh;
	tmpMesh.Create(createFunc, true);

	const MeshPositionInfo posInfo = tmpMesh.CalcPosInfo();
	VectorSlice<MeshVertData> dynamicSlice{g_meshBuilderInternal.sharedLists.dynamicMeshData};
	for (size_t i = tmpMesh.components.size(); i-- > 0;)
	{
		if (!tmpMesh.components[i].Empty())
			dynamicSlice.emplace_back(std::move(tmpMesh.components[i]));
		tmpMesh.components.pop_back();
	}
	g_meshBuilderInternal.dynamicMeshUnits.emplace(dynamicSlice, posInfo);
	return {g_meshBuilderInternal.dynamicMeshUnits.size() - 1, g_meshRendererInternal.frameNum};
}

#endif
