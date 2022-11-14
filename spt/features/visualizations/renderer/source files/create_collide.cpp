#include "stdafx.h"

#include "..\mesh_builder.hpp"
#include "spt\features\ent_props.hpp"

namespace patterns
{
	PATTERNS(CPhysicsCollision__CreateDebugMesh,
	         "5135",
	         "83 EC 10 8B 4C 24 14 8B 01 8B 40 08 55 56 57 33 ED 8D 54 24 10 52",
	         "1910503",
	         "55 8B EC 83 EC 14 8B 4D 08 8B 01 8B 40 08 53 56 57 33 DB 8D 55 EC",
	         "7462488",
	         "55 8B EC 83 EC 18 8B 4D ?? 8D 55 ??");
	PATTERNS(CPhysicsObject__GetPosition,
	         "5135",
	         "8B 49 08 81 EC 80 00 00 00 8D 04 24 50 E8 ?? ?? ?? ?? 8B 84 24 84 00 00 00 85 C0",
	         "1910503",
	         "55 8B EC 8B 49 08 81 EC 80 00 00 00 8D 45 80 50 E8 ?? ?? ?? ?? 8B 45 08 85 C0",
	         "7462488",
	         "55 8B EC 8B 49 ?? 8D 45 ?? 81 EC 80 00 00 00 50 E8 ?? ?? ?? ?? 8B 45 ??");
} // namespace patterns

void CreateCollideFeature::InitHooks()
{
	FIND_PATTERN(vphysics, CPhysicsCollision__CreateDebugMesh);
	FIND_PATTERN(vphysics, CPhysicsObject__GetPosition);
}

bool CreateCollideFeature::Works()
{
	return ORIG_CPhysicsCollision__CreateDebugMesh && ORIG_CPhysicsObject__GetPosition;
}

std::unique_ptr<Vector> CreateCollideFeature::CreateCollideMesh(const CPhysCollide* pCollide, int& outNumTris)
{
	if (!pCollide || !spt_collideToMesh.ORIG_CPhysicsCollision__CreateDebugMesh)
	{
		outNumTris = 0;
		return nullptr;
	}
	Vector* outVerts;
	outNumTris = spt_collideToMesh.ORIG_CPhysicsCollision__CreateDebugMesh(nullptr, 0, pCollide, &outVerts) / 3;
	return std::unique_ptr<Vector>(outVerts);
}

std::unique_ptr<Vector> CreateCollideFeature::CreateCPhysObjMesh(const CPhysicsObject* pPhysObj,
                                                                 int& outNumTris,
                                                                 matrix3x4_t& outMat)
{
	if (!pPhysObj || !Works())
	{
		outNumTris = 0;
		SetIdentityMatrix(outMat);
		return nullptr;
	}
	Vector pos;
	QAngle ang;
	spt_collideToMesh.ORIG_CPhysicsObject__GetPosition((void*)pPhysObj, 0, &pos, &ang);
	AngleMatrix(ang, pos, outMat);
	return CreateCollideMesh(*((CPhysCollide**)pPhysObj + 3), outNumTris);
}

std::unique_ptr<Vector> CreateCollideFeature::CreateEntMesh(const CBaseEntity* pEnt,
                                                            int& outNumTris,
                                                            matrix3x4_t& outMat)
{
	int off = spt_entutils.GetFieldOffset("CBaseEntity", "m_CollisionGroup", true);
	if (!pEnt || !Works() || off == utils::INVALID_DATAMAP_OFFSET)
	{
		outNumTris = 0;
		SetIdentityMatrix(outMat);
		return nullptr;
	}
	off = (off + 4) / 4;
	return CreateCPhysObjMesh(*((CPhysicsObject**)pEnt + off), outNumTris, outMat);
}
