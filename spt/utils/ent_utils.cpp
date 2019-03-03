#include "stdafx.h"
#include "ent_utils.hpp"
#include "client_class.h"
#include "..\OrangeBox\overlay\portal_camera.hpp"
#include <map>

static IClientEntityList* entList;
static IVModelInfo* modelInfo;
static IBaseClientDLL* clientInterface;
const int INDEX_MASK = MAX_EDICTS - 1;
const int VIEW_OFFSET_OFFSET = 224;
const int VIEW_ANGLES_OFFSET = 5180;

namespace utils
{
	void SetEntityList(IClientEntityList* list)
	{
		entList = list;
	}

	void SetModelInfo(IVModelInfo* info)
	{
		modelInfo = info;
	}

	void SetClientDLL(IBaseClientDLL* interf)
	{
		clientInterface = interf;
	}

	IClientEntity * GetClientEntity(int index)
	{
		return entList->GetClientEntity(index+1);
	}

	void PrintAllClientEntities()
	{
		int maxIndex = entList->GetHighestEntityIndex();

		for (int i = 0; i <= maxIndex; ++i)
		{
			auto ent = GetClientEntity(i);
			if (ent)
			{
				Msg("%d : %s\n", i, ent->GetClientClass()->m_pNetworkName);
			}
		}

	}

	void PrintAllPortals()
	{
		int maxIndex = entList->GetHighestEntityIndex();

		for (int i = 0; i <= maxIndex; ++i)
		{
			auto ent = GetClientEntity(i);
			if (!invalidPortal(ent))
			{
				auto& pos = ent->GetAbsOrigin();
				auto& angles = ent->GetAbsAngles();
				Msg("%d : %s, position (%.3f, %.3f, %.3f), angles (%.3f, %.3f, %.3f)\n", i, ent->GetClientClass()->m_pNetworkName, pos.x, pos.y, pos.z, angles.x, angles.y, angles.z);
			}
		}
	}

	IClientEntity * GetPlayer()
	{
		return GetClientEntity(0);
	}

	const char * GetModelName(IClientEntity * ent)
	{
		if (ent)
			return modelInfo->GetModelName(ent->GetModel());
		else
			return nullptr;
	}

	ClientClass* GetClass(const char* name)
	{
		auto list = clientInterface->GetAllClasses();

		while (list && strcmp(list->GetName(), name) != 0)
			list = list->m_pNext;

		return list;
	}

	void GetPropValue(RecvProp* prop, void* ptr, char* buffer, size_t size)
	{
		void* value = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + prop->GetOffset());
		Vector v;
		int ehandle;

		switch (prop->m_RecvType)
		{
		case DPT_Int:
			if (strstr(prop->GetName(), "m_h") != NULL)
			{
				ehandle = *reinterpret_cast<int*>(value);
				sprintf_s(buffer, size, "(index %d, handle %d)", ehandle & INDEX_MASK, ehandle & (~INDEX_MASK));
			}
			else
			{
				sprintf_s(buffer, size, "%d", *reinterpret_cast<int*>(value));
			}
			break;
		case DPT_Float:
			sprintf_s(buffer, size, "%f", *reinterpret_cast<float*>(value));
			break;
		case DPT_Vector:
			v = *reinterpret_cast<Vector*>(value);
			sprintf_s(buffer, size, "(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
			break;
		default:
			sprintf_s(buffer, size, "unable to parse");
			break;
		}
	}

	void PrintAllProps(RecvTable* table, void* ptr)
	{
		int numProps = table->m_nProps;
		const size_t BUFFER_SIZE = 80;
		char buffer[BUFFER_SIZE];

		for (int i = 0; i < table->m_nProps; ++i)
		{
			auto prop = table->GetProp(i);
			
			if (strcmp(prop->GetName(), "baseclass") == 0)
			{
				RecvTable* base = prop->GetDataTable();
				
				if (base)
					PrintAllProps(base, ptr);
				else
					Msg("Unable to locate base class!");
			}
			else if (prop->GetOffset() != 0)
			{
				GetPropValue(prop, ptr, buffer, BUFFER_SIZE);
				Msg("offset %d, name %s : value: %s\n", prop->GetOffset(), prop->GetName(), buffer);
			}				
		}
	}

	void PrintAllProps(int index)
	{
		IClientEntity* ent = GetClientEntity(index);

		if (ent)
		{
			Msg("Position (%.3f, %.3f, %.3f)\n", ent->GetAbsOrigin().x, ent->GetAbsOrigin().y, ent->GetAbsOrigin().z);
			Msg("Angles (%.3f, %.3f, %.3f)\n", ent->GetAbsAngles().x, ent->GetAbsAngles().y, ent->GetAbsAngles().z);
			auto table = ent->GetClientClass()->m_pRecvTable;
			int index = 0;
			PrintAllProps(table, ent);
		}
	}

	Vector GetVector(IClientEntity* ent, int offset)
	{
		if (ent)
			return *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent) + VIEW_OFFSET_OFFSET);
		else
			return Vector();
	}

	QAngle GetQAngle(IClientEntity* ent, int offset)
	{
		if (ent)
			return *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(ent) + offset);
		else
			return QAngle();
	}

	Vector GetPortalPosition(IClientEntity * ent)
	{
		return ent->GetAbsOrigin();
	}

	QAngle GetPortalAngles(IClientEntity * ent)
	{
		return ent->GetAbsAngles();
	}

	Vector GetPlayerEyePosition()
	{
		auto ply = GetPlayer();

		if (ply)
		{
			auto offset = GetVector(ply, VIEW_OFFSET_OFFSET);

			return ply->GetAbsOrigin() + offset;
		}
		else
			return Vector();
	}

	QAngle GetPlayerEyeAngles()
	{
		return GetQAngle(GetPlayer(), VIEW_ANGLES_OFFSET);
	}

	int PortalIsOrange(IClientEntity* ent)
	{
		return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ent) + 2528);
	}

	static IClientEntity* prevPortal = nullptr;
	static IClientEntity* prevLinkedPortal = nullptr;

	IClientEntity * FindLinkedPortal(IClientEntity * ent)
	{
		int ehandle = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ent) + 2536);
		int isOrange = PortalIsOrange(ent);
		int index = ehandle & INDEX_MASK;

		// Backup linking thing for fizzled portals(not sure if you can actually properly figure it out using client portals only)
		if (index == INDEX_MASK && prevPortal == ent && prevLinkedPortal)
		{
			index = prevLinkedPortal->entindex();
		}

		if (index != INDEX_MASK)
		{
			IClientEntity* portal = GetClientEntity(index - 1);
			prevLinkedPortal = portal;
			prevPortal = ent;

			return portal;
		}
		else
			return nullptr;
	}

	void* GetPropProxy(int iProp, IClientEntity* ent)
	{
		auto table = ent->GetClientClass()->m_pRecvTable;
	}

}