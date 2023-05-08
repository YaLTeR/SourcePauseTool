#pragma once

#include "internal_defs.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "..\mesh_builder.hpp"
#include "materialsystem\itexture.h"

struct MaterialRefMgr
{
	inline void AddRef(IMaterial* mat) const
	{
		if (mat)
			mat->IncrementReferenceCount();
	}

	// TODO - materials that have been used don't get deleted here, not an issue now but will be one with text
	inline void Release(IMaterial*& mat) const
	{
		if (mat)
		{
			mat->DecrementReferenceCount();
			mat->DeleteIfUnreferenced();
			mat = nullptr;
		}
	}
};

using MaterialRef = AutoRefPtr<IMaterial*, MaterialRefMgr>;

/*struct TextureRefMgr
{
	inline void AddRef(ITexture* tex) const
	{
		if (tex)
			tex->IncrementReferenceCount();
	}

	inline void Release(ITexture*& tex) const
	{
		if (tex)
			tex->DecrementReferenceCount(); // TODO delete if unreferenced
		tex = nullptr;
	}
};

using TextureRef = AutoRefPtr<ITexture*, TextureRefMgr>;*/

enum class MeshMaterialSimple : unsigned char
{
	Opaque,
	Alpha,
	AlphaNoZ,
	Count
};


struct MeshBuilderMatMgr
{
	MaterialRef matOpaque, matAlpha, matAlphaNoZ;

	void Load();
	void Unload();
	MaterialRef GetMaterial(MeshMaterialSimple materialType);
};

inline MeshBuilderMatMgr g_meshMaterialMgr;

#endif
