#include "stdafx.hpp"

#include "..\create_collide.hpp"
#include "spt\features\ent_props.hpp"
#include "spt\utils\interfaces.hpp"
#include "spt\utils\game_detection.hpp"

#include "thirdparty\x86.h"

namespace patterns
{
	// this functions is kind of a pain to find - it should be the virtual one right after VPhysicsGetObjectList()
	// ironically we're using it to call VPhysicsGetObjectList()
	PATTERNS(CBaseEntity__VPhysicsIsFlesh,
	         "5135-portal1",
	         "B8 00 10 00 00 E8 ?? ?? ?? ?? 8B 01",
	         "7197370-portal1",
	         "55 8B EC B8 00 10 00 00 E8 ?? ?? ?? ?? 8B 01");
} // namespace patterns

void CreateCollideFeature::InitHooks()
{
	FIND_PATTERN(server, CBaseEntity__VPhysicsIsFlesh);
}

void CreateCollideFeature::PreHook()
{
	if (ORIG_CBaseEntity__VPhysicsIsFlesh)
	{
		// we wish to get the virtual offset of VPhysicsGetObjectList()
		uint8_t* _func = ORIG_CBaseEntity__VPhysicsIsFlesh;
		int _len = 0;

		for (uint8_t* addr = _func; _len != -1 && addr - _func < 64; _len = x86_len(addr), addr += _len)
		{
			// look for the first 'MOV reg,[reg+...]' or 'CALL [reg+...]' (which does not use ModRM)
			if ((addr[0] == X86_MOVRMW && (addr[1] & 0b11000111) == X86_MODRM(2, 0, 0))
			    || (addr[0] == X86_MISCMW && (addr[1] & 0b11111000) == 0b10010000))
			{
				getVPhysObjListVirtualOff = *(int*)(addr + 2) / sizeof(void*);
				break;
			}
		}
	}
}

std::unique_ptr<Vector> CreateCollideFeature::CreateCollideMesh(const CPhysCollide* pCollide, int& outNumTris)
{
	if (!pCollide)
	{
		outNumTris = 0;
		return nullptr;
	}
	Vector* outVerts;
	outNumTris = interfaces::physicsCollision->CreateDebugMesh(pCollide, &outVerts) / 3;
	return std::unique_ptr<Vector>(outVerts);
}

std::unique_ptr<Vector> CreateCollideFeature::CreatePhysObjMesh(const IPhysicsObject* pPhysObj, int& outNumTris)
{
	if (!pPhysObj)
	{
		outNumTris = 0;
		return nullptr;
	}
	return CreateCollideMesh(pPhysObj->GetCollide(), outNumTris);
}

std::unique_ptr<Vector> CreateCollideFeature::CreateEntMesh(const CBaseEntity* pEnt, int& outNumTris)
{
	IPhysicsObject* pPhysObj = GetPhysObj(pEnt);
	if (!pPhysObj)
	{
		outNumTris = 0;
		return nullptr;
	}
	return CreatePhysObjMesh(pPhysObj, outNumTris);
}

IPhysicsObject* CreateCollideFeature::GetPhysObj(const CBaseEntity* pEnt)
{
	static int cachedPhysObjOff = utils::INVALID_DATAMAP_OFFSET;

	if (cachedPhysObjOff == utils::INVALID_DATAMAP_OFFSET)
	{
		cachedPhysObjOff = spt_entprops.GetFieldOffset("CBaseEntity", "m_CollisionGroup", true);
		if (cachedPhysObjOff != utils::INVALID_DATAMAP_OFFSET)
			cachedPhysObjOff += sizeof(int);
		else
			return nullptr;
	}
	return *reinterpret_cast<IPhysicsObject**>(reinterpret_cast<uint32_t>(pEnt) + cachedPhysObjOff);
}

int CreateCollideFeature::GetPhysObjList(const CBaseEntity* pEnt, IPhysicsObject** pList, int maxElems)
{
	if (!pEnt || !pList || getVPhysObjListVirtualOff <= 0)
		return 0;

	static utils::CachedField<string_t, "CBaseEntity", "m_iClassname", true, false> classNameField{};

	if (!classNameField.Exists())
		return 0;

	// all class names that inherit from CPropVehicle
	static const char* vehicleNames[] = {
	    "prop_vehicle",
	    "prop_vehicle_driveable",
	    "prop_vehicle_jeep",
	    "prop_vehicle_airboat",
	    "prop_vehicle_apc",
	    "prop_vehicle_jetski",
	    "vehicle_viewcontroller",
	};

	const char* className = classNameField.GetPtr(pEnt)->ToCStr();

	bool isVehicle = false;
	if (className)
	{
		isVehicle = std::any_of(vehicleNames,
		                        vehicleNames + ARRAYSIZE(vehicleNames),
		                        [className](auto vName) { return !strcmp(className, vName); });
	}

	// actually m_VehiclePhysics.m_pOuter
	static utils::CachedField<CBaseHandle, "CPropVehicle", "m_VehiclePhysics.m_controls.throttle", true, false, -8>
	    vehicleOuterField{};

	if (isVehicle)
	{
		if (!vehicleOuterField.Exists())
			return 0;
		/*
		* If a vehicle has not spawned yet, the m_pOuter field in its CFourWheelVehiclePhysics object might not be
		* initialized. This is a problem since CFourWheelVehiclePhysics::VPhysicsGetObjectList() uses that field,
		* and it can cause crashes. Unconditionally setting that field to the vehicle handle *should* be okay.
		*/
		*vehicleOuterField.GetPtr(pEnt) = ((IServerEntity*)pEnt)->GetRefEHandle();
	}

	typedef int(__thiscall * _func)(const CBaseEntity*, IPhysicsObject**, int);
	return ((_func**)pEnt)[0][getVPhysObjListVirtualOff](pEnt, pList, maxElems);
}
