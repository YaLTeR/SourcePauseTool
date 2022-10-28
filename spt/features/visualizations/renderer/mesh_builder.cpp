#include "stdafx.h"

#include "mesh_builder.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "vstdlib\IKeyValuesSystem.h"
#include "interfaces.hpp"

#include "mesh_defs_private.hpp"

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
* A MeshWrapperComponent is either an IMesh* or contains enough information to create one. In the case of dynamic
* meshes, it's an index into the MeshData mesh buffers stored here plus some additional flags. IMesh* objects are
* made out of either lines or triangles, which brings us to the second level of abstraction.
* 
* An MeshWrapper is two MeshWrapperComponent - one for lines and one for triangles.
* 
* For both types of meshes we create a vector of MeshData + indices from the user's mesh creation func. For
* statics, we can immediately discard/reuse those buffers after the IMesh* is created, but we need to keep them
* around for dynamics until rendering happens later in the frame. So we have reusedStaticMeshData which are reused
* and dynamicMeshData which are not, but are instead cleared just before the next MeshRenderSignal.
* 
* When the user requests a mesh to be created, an MeshWrapper object is created, but where does it live? For
* statics, it lives on the heap and the user has a shared pointer to it (so that it doesn't get deleted when we
* try to render it). For dynamics, the MeshWrapper lives in the dynamicMeshWrappers buffer and the user gets back
* an index to it. Of course, the user can't actually do anything with it, but the mesh renderer can get the
* associated MeshWrapper object with said index.
*/

/**************************************** MESH WRAPPER ****************************************/

MeshWrapper::MeshWrapperComponent::MeshWrapperComponent(const MeshWrapper& myWrapper,
                                                        int mDataIdx,
                                                        bool opaque,
                                                        bool faces,
                                                        bool dynamic)
    : opaque(opaque)
    , dynamic(dynamic)
    , faces(faces)
    , zTest(myWrapper.params.zTestFlags & (faces ? ZTEST_FACES : ZTEST_LINES))
{
	if (dynamic)
	{
		this->dynamicToken = {mDataIdx};
	}
	else
	{
		// don't care about setting the exact material until rendering happens
		auto& mData = g_meshBuilder.reusedStaticMeshData;
		const auto& mDataComp = faces ? mData.faceData : mData.lineData;
		iMesh = mDataComp.CreateIMesh(g_meshBuilderMaterials.matOpaque, myWrapper.params, faces, false);
	}
}

MeshWrapper::MeshWrapperComponent::MeshWrapperComponent(MeshWrapperComponent&& mc)
    : opaque(mc.opaque), dynamic(mc.dynamic), faces(mc.faces), zTest(mc.zTest)
{
	if (dynamic)
	{
		dynamicToken = mc.dynamicToken;
	}
	else
	{
		iMesh = mc.iMesh;
		mc.iMesh = nullptr;
	}
}

MeshWrapper::MeshWrapperComponent::~MeshWrapperComponent()
{
	if (!dynamic && iMesh)
	{
		CMatRenderContextPtr(interfaces::materialSystem)->DestroyStaticMesh(iMesh);
		iMesh = nullptr;
	}
}

bool MeshWrapper::MeshWrapperComponent::Empty() const
{
	return dynamic ? !g_meshBuilder.GetMeshData(dynamicToken, faces) : !iMesh;
}

IMesh* MeshWrapper::MeshWrapperComponent::GetIMesh(const MeshWrapper& outer, IMaterial* material) const
{
	if (Empty())
		return nullptr;
	if (dynamic)
		return g_meshBuilder.GetMeshData(dynamicToken, faces)->CreateIMesh(material, outer.params, faces, true);
	else
		return iMesh;
}

MeshWrapper::MeshWrapper(int mDataIdx,
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

IMesh* MeshComponentData::CreateIMesh(IMaterial* material,
                                      const CreateMeshParams& params,
                                      bool faces,
                                      bool dynamic) const
{
	Assert(verts.size() <= indices.size());

	if (verts.size() == 0 || !material)
		return nullptr;

	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESHBUILDER);

	CMatRenderContextPtr context{interfaces::materialSystem};
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

void MeshBuilderInternal::ClearOldBuffers()
{
	// keep only as many dynamic buffers as there were dynamic meshes last frame
	dynamicMeshData.resize(numDynamicMeshes);
	numDynamicMeshes = 0;
	dynamicMeshWrappers.clear();
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

MeshWrapper* MeshBuilderInternal::FinalizeCurMesh(const CreateMeshParams& params)
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
		return new MeshWrapper(-1, pos, params, opaque[0], opaque[1], false);
	}
	else
	{
		// dynamic mesh, we keep it in the mesh builder and delete at the start of every frame
		dynamicMeshWrappers.emplace_back(dynamicMeshWrappers.size(), pos, params, opaque[0], opaque[1], true);
		return nullptr; // can't point to vector element because reallocations will invalidate the pointer
	}
}

StaticMesh MeshBuilderPro::CreateStaticMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params)
{
	g_meshBuilder.BeginNewMesh(false);
	static MeshBuilderPro mb;
	createFunc(mb);
	return std::shared_ptr<MeshWrapper>(g_meshBuilder.FinalizeCurMesh(params));
}

DynamicMesh MeshBuilderPro::CreateDynamicMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params)
{
	AssertMsg(g_inMeshRenderSignal, "spt: Must create dynamic meshes in MeshRenderSignal!");
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESHBUILDER);
	g_meshBuilder.BeginNewMesh(true);
	static MeshBuilderPro mb;
	createFunc(mb);
	g_meshBuilder.FinalizeCurMesh(params);
	return {g_meshBuilder.dynamicMeshWrappers.size() - 1, g_meshRenderFrameNum};
}

#endif
