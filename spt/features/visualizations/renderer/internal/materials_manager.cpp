#include "stdafx.hpp"

#include "internal_defs.hpp"
#include "materials_manager.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "interfaces.hpp"

/**************************************** MATERIAL MANAGER ****************************************/

void MeshBuilderMatMgr::Load()
{
	KeyValues* kv;

	kv = new KeyValues("unlitgeneric");
	kv->SetInt("$vertexcolor", 1);
	matOpaque = interfaces::materialSystem->CreateMaterial("_spt_UnlitOpaque", kv);

	kv = new KeyValues("unlitgeneric");
	kv->SetInt("$vertexcolor", 1);
	kv->SetInt("$vertexalpha", 1);
	matAlpha = interfaces::materialSystem->CreateMaterial("_spt_UnlitTranslucent", kv);

	kv = new KeyValues("unlitgeneric");
	kv->SetInt("$vertexcolor", 1);
	kv->SetInt("$vertexalpha", 1);
	kv->SetInt("$ignorez", 1);
	matAlphaNoZ = interfaces::materialSystem->CreateMaterial("_spt_UnlitTranslucentNoZ", kv);
}

void MeshBuilderMatMgr::Unload()
{
	std::array<MaterialRef*, 3> mats{&matOpaque, &matAlpha, &matAlphaNoZ};
	for (MaterialRef* mat : mats)
		(*mat).Release();
}

MaterialRef MeshBuilderMatMgr::GetMaterial(MeshMaterialSimple materialType)
{
	switch (materialType)
	{
	case MeshMaterialSimple::Opaque:
		return matOpaque;
	case MeshMaterialSimple::Alpha:
		return matAlpha;
	case MeshMaterialSimple::AlphaNoZ:
		return matAlphaNoZ;
	default:
		Assert(0);
		return nullptr;
	}
}

#endif
