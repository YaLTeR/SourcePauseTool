#include "stdafx.h"

#include "..\mesh_defs_private.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "interfaces.hpp"

/**************************************** IMESH WRAPPER ****************************************/

MeshUnit::IMeshWrapper::IMeshWrapper(const MeshUnit& myWrapper, int mDataIdx, bool opaque, bool faces, bool dynamic)
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
		auto& mData = g_meshBuilderInternal.reusedStaticMeshData;
		const auto& mDataComp = faces ? mData.faceData : mData.lineData;
		iMesh = mDataComp.CreateIMesh(g_meshMaterialMgr.matOpaque, myWrapper.params, faces, false);
	}
}

MeshUnit::IMeshWrapper::IMeshWrapper(IMeshWrapper&& mc)
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

MeshUnit::IMeshWrapper::~IMeshWrapper()
{
	if (!dynamic && iMesh)
	{
		CMatRenderContextPtr(interfaces::materialSystem)->DestroyStaticMesh(iMesh);
		iMesh = nullptr;
	}
}

bool MeshUnit::IMeshWrapper::Empty() const
{
	return dynamic ? !g_meshBuilderInternal.GetMeshData(dynamicToken, faces) : !iMesh;
}

IMesh* MeshUnit::IMeshWrapper::GetIMesh(const MeshUnit& outer, IMaterial* material) const
{
	if (Empty())
		return nullptr;
	if (dynamic)
	{
		auto mData = g_meshBuilderInternal.GetMeshData(dynamicToken, faces);
		return mData->CreateIMesh(material, outer.params, faces, true);
	}
	else
	{
		return iMesh;
	}
}

/**************************************** MESH UNIT ****************************************/

MeshUnit::MeshUnit(int mDataIdx,
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

bool MeshUnit::Empty()
{
	return faceComponent.Empty() && lineComponent.Empty();
}

#endif
