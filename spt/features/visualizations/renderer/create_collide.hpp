#pragma once

#include <memory>

#ifdef OE
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

#include "spt\feature.hpp"

class CPhysCollide;
class IPhysicsObject;
class CBaseEntity;

// a simple interface to get the vertices of physics models
class CreateCollideFeature : public FeatureWrapper<CreateCollideFeature>
{
public:
	// theses verts don't contain pos/rot info; they'll be at the origin
	std::unique_ptr<Vector> CreateCollideMesh(const CPhysCollide* pCollide, int& outNumTris);

	// you'll need to transform these verts by applying a matrix you can create with pPhysObj->GetPosition();
	// if this returns a sphere (pPhysObj->GetSphereRadius() > 0) then this return an empty mesh
	std::unique_ptr<Vector> CreatePhysObjMesh(const IPhysicsObject* pPhysObj, int& outNumTris);

	// you'll need to transform the verts using either pEnt's matrix or the matrix from the phys obj;
	// only creates a mesh using GetPhysObj(), not GetPhysObjList()
	std::unique_ptr<Vector> CreateEntMesh(const CBaseEntity* pEnt, int& outNumTris);

	// can be used after InitHooks()
	IPhysicsObject* GetPhysObj(const CBaseEntity* pEnt);

	// calls VPhysicsGetObjectList(), used when pEnt has multiple vphys objects; see note about spheres above
	int GetPhysObjList(const CBaseEntity* pEnt, IPhysicsObject** pList, int maxElems);

protected:
	virtual void InitHooks();
	virtual void PreHook();

private:
	uint8_t* ORIG_CBaseEntity__VPhysicsIsFlesh = nullptr;
	int getVPhysObjListVirtualOff = -1;
};

inline CreateCollideFeature spt_collideToMesh;
