#include "stdafx.h"

#include "mesh_builder.hpp"

#ifdef SPT_MESH_BUILDER_ENABLED

#include "spt\feature.hpp"

#include "vstdlib\IKeyValuesSystem.h"

#define VPROF_LEVEL 1
#ifndef SSDK2007
#define RAD_TELEMETRY_DISABLED
#endif
#include "vprof.h"

#define VPROF_BUDGETGROUP_MESHBUILDER _T("Mesh_Builder")

// We need KeyValues to make materials; they in turn need KeyValuesSystem(), but we don't link against vstdlib.
// Here's a little hack to make that work - find the KeyValuesSystem function by searching for its symbol.
#pragma warning(push)
#pragma warning(disable : 4273)
IKeyValuesSystem* KeyValuesSystem()
{
	static IKeyValuesSystem* ptr = nullptr;

	if (ptr != nullptr)
		return ptr;

	void* moduleHandle;
	if (MemUtils::GetModuleInfo(L"vstdlib.dll", &moduleHandle, nullptr, nullptr))
	{
		typedef IKeyValuesSystem*(__cdecl * _KVS)();
		ptr = ((_KVS)MemUtils::GetSymbolAddress(moduleHandle, "KeyValuesSystem"))();
	}
	return ptr;
}
#pragma warning(pop)

/*
* There are a few levels of abstraction to understand how this whole system works. The game's mesh representation
* is an IMesh*. We can use this directly for static meshes, but for dynamics we need to recreate the IMesh* object
* every time we render because there's really just one dynamic mesh. This gives us our first level of abstraction.
* 
* A MeshComponent is either an IMesh* or contains enough information to create one. In the case of dynamic meshes,
* a MeshComponent is an index into the VertexData mesh buffers stored here plus some additional flags. IMesh*
* objects are made out of either lines or triangles, which brings us to the second level of abstraction.
* 
* An OverlayMesh is two MeshComponents - one for lines and one for triangles.
* 
* For both types of meshes we create a vector of VertexData + indices from the user's mesh creation func. For
* statics, we can immediately discard/reuse those buffers after the IMesh* is created, but we need to keep them
* around for dynamics until rendering happens later in the frame. So we have staticMDataBufs which are reused and
* dynamicMDataBufs which are not, but are instead cleared just before the next overlay signal.
* 
* When the user requests a mesh to be created, an OverlayMesh object is created, but where does it live? For
* statics, it lives on the heap and the user has a shared pointer to it (so that it doesn't get deleted when we
* try to render it). For dynamics, the OverlayMesh lives in the dynamicOvrMeshes buffer and the user gets back
* an index to it. Of course, the user can't actually do anything with it, but the overlay renderer can get the
* associated OverlayMesh object with said index.
*/

/**************************************** OVERLAY MESH ****************************************/

OverlayMesh::MeshComponent::MeshComponent(const OverlayMesh& outerOvrMesh,
                                          int mDataIdx,
                                          bool opaque,
                                          bool faces,
                                          bool dynamic)
    : opaque(opaque)
    , dynamic(dynamic)
    , faces(faces)
    , zTest(outerOvrMesh.params.zTestFlags & (faces ? ZTEST_FACES : ZTEST_LINES))
{
	auto& mb = MeshBuilderPro::singleton;
	if (dynamic)
	{
		this->mDataIdx = mDataIdx;
	}
	else
	{
		// don't care about setting the exact material until rendering happens
		const auto& mDataBuf = faces ? mb.staticMDataFaceBuf : mb.staticMDataLineBuf;
		iMesh = mDataBuf.CreateIMesh(mb.matOpaque, outerOvrMesh.params, faces, false);
	}
}

OverlayMesh::MeshComponent::MeshComponent(MeshComponent&& mc)
    : opaque(mc.opaque), dynamic(mc.dynamic), faces(mc.faces), zTest(mc.zTest)
{
	if (dynamic)
	{
		mDataIdx = mc.mDataIdx;
	}
	else
	{
		iMesh = mc.iMesh;
		mc.iMesh = nullptr;
	}
}

OverlayMesh::MeshComponent::~MeshComponent()
{
	if (!dynamic && iMesh)
	{
		CMatRenderContextPtr(interfaces::materialSystem)->DestroyStaticMesh(iMesh);
		iMesh = nullptr;
	}
}

bool OverlayMesh::MeshComponent::Empty() const
{
	if (dynamic)
	{
		auto& pair = MeshBuilderPro::singleton.dynamicMDataBufs[mDataIdx];
		auto& meshData = faces ? pair.first : pair.second;
		return meshData.verts.size() == 0;
	}
	else
	{
		return !iMesh;
	}
}

IMesh* OverlayMesh::MeshComponent::GetIMesh(const OverlayMesh& outerOvrMesh, IMaterial* material) const
{
	if (Empty())
		return nullptr;
	if (dynamic)
	{
		auto& dynamicMData = MeshBuilderPro::singleton.dynamicMDataBufs[mDataIdx];
		auto& meshData = faces ? dynamicMData.first : dynamicMData.second;
		return meshData.CreateIMesh(material, outerOvrMesh.params, faces, true);
	}
	else
	{
		return iMesh;
	}
}

OverlayMesh::OverlayMesh(int mDataIdx,
                         const MeshPositionInfo& posInfo,
                         const CreateMeshParams& params,
                         bool facesOpaque,
                         bool linesOpaque,
                         bool dynamic)
    : faceComponent(*this, mDataIdx, facesOpaque, true, dynamic)
    , lineComponent(*this, mDataIdx, linesOpaque, false, dynamic)
    , posInfo(posInfo)
    , params(params)
{
}

/**************************************** MESH DATA ****************************************/

void MeshBuilderPro::MeshData::Clear()
{
	verts.clear();
	indices.clear();
}

IMesh* MeshBuilderPro::MeshData::CreateIMesh(IMaterial* material,
                                             const CreateMeshParams& params,
                                             bool faces,
                                             bool dynamic) const
{
	Assert(verts.size() <= indices.size());

	if (verts.size() == 0 || !material)
		return nullptr;

	VPROF_BUDGET("MeshBuilderPro::MeshData::CreateIMesh", VPROF_BUDGETGROUP_MESHBUILDER);

	CMatRenderContextPtr context(interfaces::materialSystem);
	context->Bind(material);

	if (!dynamic && material->GetVertexFormat() == VERTEX_FORMAT_UNKNOWN)
	{
		AssertMsg(0, "We tried so hard, but in the end it doesn't even matter");
		Warning("spt: Static mesh material vertex format is unknown but shouldn't be. Grab a programmer!\n");
		return nullptr;
	}

	IMesh* iMesh = dynamic ? context->GetDynamicMesh(true, nullptr, nullptr, material)
	                       : context->CreateStaticMesh(material->GetVertexFormat() & ~VERTEX_FORMAT_COMPRESSED,
	                                                   TEXTURE_GROUP_STATIC_VERTEX_BUFFER_WORLD,
	                                                   material);
	Assert(iMesh);
	iMesh->SetPrimitiveType(faces ? MATERIAL_TRIANGLES : MATERIAL_LINES);

	// init and populate the mesh data

	const int numVerts = (int)verts.size();
	const int numIndices = (int)indices.size() * (params.cullType == CullType::ShowBoth && faces ? 2 : 1);
	Assert(faces ? ((numIndices % 3) == 0) : ((numIndices % 2) == 0));

	MeshDesc_t desc;
	iMesh->LockMesh(numVerts, numIndices, desc);

	for (size_t i = 0; i < verts.size(); i++)
	{
		*(Vector*)((uintptr_t)desc.m_pPosition + i * desc.m_VertexSize_Position) = verts[i].pos;
		unsigned char* pColor = desc.m_pColor + i * desc.m_VertexSize_Color;
		pColor[0] = verts[i].col.b;
		pColor[1] = verts[i].col.g;
		pColor[2] = verts[i].col.r;
		pColor[3] = verts[i].col.a;
	}

	int descIdx = 0;
	switch (params.cullType)
	{
	case CullType::Default:
	case CullType::ShowBoth:
		// add indices in the regular order
		for (size_t i = 0; i < indices.size(); i++)
		{
			Assert(indices[i] < verts.size());
			desc.m_pIndices[descIdx++] = indices[i] + desc.m_nFirstVertex;
		}
		if (params.cullType == CullType::Default || !faces)
			break; // fall through if both
	case CullType::Reverse:
		// add indices backwards
		for (int i = (int)indices.size() - 1; i >= 0; i--)
		{
			Assert(indices[i] < verts.size());
			desc.m_pIndices[descIdx++] = indices[i] + desc.m_nFirstVertex;
		}
		break;
	}
	Assert(descIdx == numIndices);

	iMesh->UnlockMesh(numVerts, numIndices, desc);
	return iMesh;
}

/**************************************** MESH BUILDER PRO ****************************************/

MeshBuilderPro MeshBuilderPro::singleton;

IMaterial* MeshBuilderPro::GetMaterial(bool opaque, bool zTest, color32 colorMod)
{
	if (!materialsInitialized || colorMod.a == 0)
		return nullptr;
	IMaterial* mat = zTest ? (opaque && colorMod.a ? matOpaque : matAlpha) : matAlphaNoZ;
	Assert(mat);
	mat->ColorModulate(colorMod.r / 255.f, colorMod.g / 255.f, colorMod.b / 255.f);
	mat->AlphaModulate(colorMod.a / 255.f);
	return mat;
}

void MeshBuilderPro::ClearOldBuffers()
{
	// keep only as many dynamic buffers as there were dynamic meshes last frame
	dynamicMDataBufs.resize(numDynamicMeshes);
	numDynamicMeshes = 0;
	dynamicOvrMeshes.clear();
}

void MeshBuilderPro::BeginNewMesh(bool dynamic)
{
	if (dynamic)
	{
		// We have to keep the buffers around for dynamics to create the IMesh* objects later. To prevent completely
		// reallocating the verts array for every dynamic mesh, we keep some old buffers around from the previous frame.
		if (numDynamicMeshes >= dynamicMDataBufs.size())
			dynamicMDataBufs.emplace_back();
		faceData = &dynamicMDataBufs[numDynamicMeshes].first;
		lineData = &dynamicMDataBufs[numDynamicMeshes].second;
		numDynamicMeshes++;
	}
	else
	{
		// static meshes immediately turn into IMesh* objects, so we can just reuse the same buffers
		faceData = &staticMDataFaceBuf;
		lineData = &staticMDataLineBuf;
	}
	faceData->Clear();
	lineData->Clear();
}

OverlayMesh* MeshBuilderPro::CreateOverlayMesh(const CreateMeshParams& params, bool dynamic)
{
	const MeshData* meshData[] = {faceData, lineData};

	// figure out if the meshes are opaque
	bool opaque[2];
	for (int i = 0; i < 2; i++)
	{
		if ((i == 0 && (params.zTestFlags & ZTEST_FACES)) || (i == 1 && (params.zTestFlags & ZTEST_LINES)))
		{
			opaque[i] = true;
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
	if (faceData->verts.size() == 0 && lineData->verts.size() == 0)
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

	if (dynamic)
	{
		dynamicOvrMeshes.emplace_back(dynamicOvrMeshes.size(), pos, params, opaque[0], opaque[1], true);
		return nullptr; // can't point to vector element because reallocations will invalidate the pointer
	}
	else
	{
		return new OverlayMesh(-1, pos, params, opaque[0], opaque[1], false);
	}
}

OverlayMesh& MeshBuilderPro::GetOvrMesh(const DynamicOverlayMesh& dynamicInfo)
{
	Assert(curFrameNum == dynamicInfo.createdFrame); // should have already checked for this in the renderer
	return dynamicOvrMeshes[dynamicInfo.dynamicOvrIdx];
}

StaticOverlayMesh MeshBuilderPro::CreateStaticMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params)
{
	VPROF_BUDGET("MeshBuilderPro::CreateStaticMesh", VPROF_BUDGETGROUP_MESHBUILDER);
	singleton.BeginNewMesh(false);
	createFunc(singleton);
	return std::shared_ptr<OverlayMesh>(singleton.CreateOverlayMesh(params, false));
}

DynamicOverlayMesh MeshBuilderPro::CreateDynamicMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params)
{
	AssertMsg(singleton.inOverlaySignal, "spt: Must create dynamic meshes in OverlaySignal!");
	VPROF_BUDGET("MeshBuilderPro::CreateDynamicMesh", VPROF_BUDGETGROUP_MESHBUILDER);
	singleton.BeginNewMesh(true);
	createFunc(singleton);
	singleton.CreateOverlayMesh(params, true);
	return {singleton.dynamicOvrMeshes.size() - 1, singleton.curFrameNum};
}

#endif
