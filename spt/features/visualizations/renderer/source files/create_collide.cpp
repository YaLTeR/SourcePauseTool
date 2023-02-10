#include "stdafx.hpp"

#include "..\mesh_builder.hpp"
#include "spt\features\ent_props.hpp"
#include "vphysics_interface.h"

namespace patterns
{
	PATTERNS(CPhysicsCollision__CreateDebugMesh,
	         "5135",
	         "83 EC 10 8B 4C 24 14 8B 01 8B 40 08 55 56 57 33 ED 8D 54 24 10 52",
	         "1910503",
	         "55 8B EC 83 EC 14 8B 4D 08 8B 01 8B 40 08 53 56 57 33 DB 8D 55 EC",
	         "7462488",
	         "55 8B EC 83 EC 18 8B 4D ?? 8D 55 ??");
} // namespace patterns

void CreateCollideFeature::InitHooks()
{
	FIND_PATTERN(vphysics, CPhysicsCollision__CreateDebugMesh);
}

void CreateCollideFeature::UnloadFeature()
{
	cachedOffset = false;
}

bool CreateCollideFeature::Works()
{
	return ORIG_CPhysicsCollision__CreateDebugMesh;
}

std::unique_ptr<Vector> CreateCollideFeature::CreateCollideMesh(const CPhysCollide* pCollide, int& outNumTris)
{
	if (!pCollide || !spt_collideToMesh.ORIG_CPhysicsCollision__CreateDebugMesh)
	{
		outNumTris = 0;
		return nullptr;
	}
	Vector* outVerts;
	outNumTris = spt_collideToMesh.ORIG_CPhysicsCollision__CreateDebugMesh(nullptr, pCollide, &outVerts) / 3;
	return std::unique_ptr<Vector>(outVerts);
}

std::unique_ptr<Vector> CreateCollideFeature::CreatePhysObjMesh(const IPhysicsObject* pPhysObj, int& outNumTris)
{
	if (!pPhysObj || !Works())
	{
		outNumTris = 0;
		return nullptr;
	}
	return CreateCollideMesh(pPhysObj->GetCollide(), outNumTris);
}

std::unique_ptr<Vector> CreateCollideFeature::CreateEntMesh(const CBaseEntity* pEnt, int& outNumTris)
{
	IPhysicsObject* pPhysObj = GetPhysObj(pEnt);
	if (!pPhysObj || !Works())
	{
		outNumTris = 0;
		return nullptr;
	}
	return CreatePhysObjMesh(pPhysObj, outNumTris);
}

IPhysicsObject* CreateCollideFeature::GetPhysObj(const CBaseEntity* pEnt)
{
	if (!cachedOffset)
	{
		cached_phys_obj_off = spt_entprops.GetFieldOffset("CBaseEntity", "m_CollisionGroup", true);
		if (cached_phys_obj_off != utils::INVALID_DATAMAP_OFFSET)
			cached_phys_obj_off = (cached_phys_obj_off + 4) / 4;
		cachedOffset = true;
	}
	if (cached_phys_obj_off == utils::INVALID_DATAMAP_OFFSET)
		return nullptr;
	return *((IPhysicsObject**)pEnt + cached_phys_obj_off);
}
