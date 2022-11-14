#include "stdafx.h"

#include "..\mesh_defs_private.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "vstdlib\IKeyValuesSystem.h"
#include "interfaces.hpp"

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

	materialsInitialized = matOpaque && matAlpha && matAlphaNoZ;
	if (!materialsInitialized)
		matOpaque = matAlpha = matAlphaNoZ = nullptr;
}

void MeshBuilderMatMgr::Unload()
{
	materialsInitialized = false;
	if (matOpaque)
		matOpaque->DecrementReferenceCount();
	if (matAlpha)
		matAlpha->DecrementReferenceCount();
	if (matAlphaNoZ)
		matAlphaNoZ->DecrementReferenceCount();
	matOpaque = matAlpha = matAlphaNoZ = nullptr;
}

IMaterial* MeshBuilderMatMgr::GetMaterial(bool opaque, bool zTest, color32 colorMod)
{
	if (!materialsInitialized || colorMod.a == 0)
		return nullptr;
	IMaterial* mat = zTest ? (opaque && colorMod.a ? matOpaque : matAlpha) : matAlphaNoZ;
	Assert(mat);
	mat->ColorModulate(colorMod.r / 255.f, colorMod.g / 255.f, colorMod.b / 255.f);
	mat->AlphaModulate(colorMod.a / 255.f);
	return mat;
}

#endif
