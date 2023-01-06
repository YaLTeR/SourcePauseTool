#include "stdafx.hpp"

#include "..\mesh_defs_private.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "interfaces.hpp"

/**************************************** MESH COMPONENT DATA ****************************************/

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

bool MeshComponentData::Empty() const
{
	return verts.size() == 0;
}

void MeshComponentData::Clear()
{
	verts.clear();
	indices.clear();
}

/**************************************** MESH DATA ****************************************/

bool MeshData::Empty() const
{
	return faceData.Empty() && lineData.Empty();
}

void MeshData::Clear()
{
	faceData.Clear();
	lineData.Clear();
}

#endif
