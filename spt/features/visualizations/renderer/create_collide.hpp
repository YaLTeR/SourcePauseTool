#pragma once

#include <memory>

#ifdef OE
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

#include "spt\feature.hpp"

namespace utils
{
	extern const int INVALID_DATAMAP_OFFSET;
}

class CPhysCollide;
class IPhysicsObject;
class CBaseEntity;

// a simple interface to get the vertices of physics models
class CreateCollideFeature
{
public:
	// theses verts don't contain pos/rot info; they'll be at the origin
	std::unique_ptr<Vector> CreateCollideMesh(const CPhysCollide* pCollide, int& outNumTris);

	// you'll need to transform these verts by applying a matrix you can create with pPhysObj->GetPosition()
	std::unique_ptr<Vector> CreatePhysObjMesh(const IPhysicsObject* pPhysObj, int& outNumTris);

	// you'll need to transform the verts like described above
	std::unique_ptr<Vector> CreateEntMesh(const CBaseEntity* pEnt, int& outNumTris);

	// can be used after InitHooks()
	IPhysicsObject* GetPhysObj(const CBaseEntity* pEnt);

private:
	int cached_phys_obj_off = utils::INVALID_DATAMAP_OFFSET;
};

inline CreateCollideFeature spt_collideToMesh;
