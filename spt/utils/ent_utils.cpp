#include "stdafx.h"

#include "ent_utils.hpp"
#include "client_class.h"
#include "..\OrangeBox\overlay\portal_camera.hpp"
#include <vector>
#include <regex>
#include "string_parsing.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "..\OrangeBox\modules\ClientDLL.hpp"
#include "..\OrangeBox\modules\ServerDLL.hpp"
#include "..\OrangeBox\modules.hpp"
#include <limits>
#include "property_getter.hpp"
#include "..\strafestuff.hpp"
#undef max

#ifndef OE
static IClientEntityList* entList;
static IVModelInfo* modelInfo;
static IBaseClientDLL* clientInterface;
const int INDEX_MASK = MAX_EDICTS - 1;
#endif

namespace utils
{

#ifndef OE
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
				sprintf_s(buffer, size, "(index %d, serial %d)", ehandle & INDEX_MASK, (ehandle & (~INDEX_MASK)) >> MAX_EDICT_BITS);
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
		case DPT_String:
			sprintf_s(buffer, size, "%s", *reinterpret_cast<const char**>(value));
			break;
		default:
			sprintf_s(buffer, size, "unable to parse");
			break;
		}
	}

	const size_t BUFFER_SIZE = 256;
	static char BUFFER[BUFFER_SIZE];

	void AddProp(std::vector<propValue>& props, const char* name, int offset)
	{
		props.push_back(propValue(name, BUFFER, offset));
	}


	void GetAllProps(RecvTable* table, void* ptr, std::vector<propValue>& props)
	{
		int numProps = table->m_nProps;

		for (int i = 0; i < numProps; ++i)
		{
			auto prop = table->GetProp(i);
			
			if (strcmp(prop->GetName(), "baseclass") == 0)
			{
				RecvTable* base = prop->GetDataTable();
				
				if (base)
					GetAllProps(base, ptr, props);
			}
			else if (prop->GetOffset() != 0) // There's various garbage attributes at offset 0 for a thusfar unbeknownst reason
			{
				GetPropValue(prop, ptr, BUFFER, BUFFER_SIZE);
				AddProp(props, prop->GetName(), prop->GetOffset());
			}				
		}
	}

	void GetAllProps(IClientEntity* ent, std::vector<propValue>& props)
	{
		props.clear();

		if (ent)
		{
			snprintf(BUFFER, BUFFER_SIZE, "(%.3f, %.3f, %.3f)", ent->GetAbsOrigin().x, ent->GetAbsOrigin().y, ent->GetAbsOrigin().z);
			AddProp(props, "AbsOrigin", -1);
			snprintf(BUFFER, BUFFER_SIZE, "(%.3f, %.3f, %.3f)", ent->GetAbsAngles().x, ent->GetAbsAngles().y, ent->GetAbsAngles().z);
			AddProp(props, "AbsAngles", -1);
			GetAllProps(ent->GetClientClass()->m_pRecvTable, ent, props);
		}

	}

	void PrintAllProps(int index)
	{
		IClientEntity* ent = GetClientEntity(index);

		if (ent)
		{
			std::vector<propValue> props;
			GetAllProps(ent, props);

			Msg("Class %s props:\n\n", ent->GetClientClass()->m_pNetworkName);

			for (auto& prop : props)
				Msg("Name: %s, offset %d, value %s\n", prop.name.c_str(), prop.offset, prop.value.c_str());
		}
		else
			Msg("Entity doesn't exist!");
	}

	Vector GetPortalPosition(IClientEntity* ent)
	{
		return ent->GetAbsOrigin();
	}

	QAngle GetPortalAngles(IClientEntity* ent)
	{
		return ent->GetAbsAngles();
	}

	Vector GetPlayerEyePosition()
	{
		auto ply = GetPlayer();

		if (ply)
		{
			auto offset = utils::GetProperty<Vector>(0, "m_vecViewOffset[0]");

			return ply->GetAbsOrigin() + offset;
		}
		else
			return Vector();
	}

	QAngle GetPlayerEyeAngles()
	{
		return utils::GetProperty<QAngle>(0, "m_angEyeAngles[0]");
	}

	int PortalIsOrange(IClientEntity* ent)
	{
		return utils::GetProperty<int>(ent->entindex() - 1, "m_bIsPortal2");
	}

	static IClientEntity* prevPortal = nullptr;
	static IClientEntity* prevLinkedPortal = nullptr;

	IClientEntity * FindLinkedPortal(IClientEntity * ent)
	{
		int ehandle = utils::GetProperty<int>(ent->entindex() - 1, "m_hLinkedPortal");
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

	void GetValuesMatchingRegex(const char* regex, const char* end, int& entries, wchar* arr, int maxEntries, int bufferSize, const std::vector<propValue>& props)
	{
		std::regex r(regex, end);

		for (auto& prop : props)
		{
			if (entries >= maxEntries)
				break;

			// Prop name matches regex
			if (std::regex_match(prop.name.c_str(), r))
			{
				std::wstring name = s2ws(prop.name);
				std::wstring value = s2ws(prop.value);

				swprintf_s(arr + (entries * bufferSize), bufferSize, L"%s : %s", name.c_str(), value.c_str());
				++entries;
			}
		}
	}

	int FillInfoArray(std::string argString, wchar* arr, int maxEntries, int bufferSize, char sep, char entSep)
	{
		int entries = 0;
		int i = 0;
		IClientEntity* ent = nullptr;
		const char* args = argString.c_str();
		int argSize = argString.size();
		std::vector<propValue> props;

		while (i < argSize && entries < maxEntries)
		{
			bool readEntityIndex = false;

			// Read entity index
			if (args[i] == entSep)
			{
				++i;
				readEntityIndex = true;
			}
			// Also read if at the start of the string. Note: does not increment the index since we're at the start of the string.
			else if (i == 0)
			{
				readEntityIndex = true;
			}
			else if(args[i] == sep)
			{
				++i;
			}
			else // error occurred
			{
				i = argSize;
				arr = L"error occurred in parsing";
				entries = 1;
				break;
			}

			int endIndex = i;

			// Go to the next separator
			while (args[endIndex] != sep && args[endIndex] != entSep && endIndex < argSize)
				++endIndex;

			if (readEntityIndex)
			{
				int entIndex = atoi(args + i);
				ent = GetClientEntity(entIndex);
				// Get all the props for this entity before hand
				GetAllProps(ent, props);

				if (!ent)
				{
					swprintf_s(arr + (entries * bufferSize), bufferSize, L"entity %d does not exist", entIndex);
					++entries;
				}
				else
				{
					std::string temp(ent->GetClientClass()->GetName());
					std::wstring className = s2ws(temp);

					// Add empty line in between entities
					if (entries != 0)
					{
						swprintf_s(arr + (entries * bufferSize), bufferSize, L"");
						++entries;
					}

					swprintf_s(arr + (entries * bufferSize), bufferSize, L"entity %d : %s", entIndex, className.c_str());
					++entries;
				}
			}
			// Read prop regex, make sure entity exists as well
			else if(ent)
			{
				const char* end = args + endIndex;
				GetValuesMatchingRegex(args + i, end, entries, arr, maxEntries, bufferSize, props);
			}

			i = endIndex;
		}

		return entries;
	}

	void SimulateFrames(int frames)
	{
		auto vars = clientDLL.GetMovementVars();
		auto player = clientDLL.GetPlayerData();
		auto type = Strafe::GetPositionType(player, Strafe::HullType::NORMAL);

		for (int i = 0; i < frames; ++i)
		{
			Msg("%d: pos: (%.3f, %.3f, %.3f), vel: (%.3f, %.3f, %.3f), ducked %d, onground %d\n", i, player.UnduckedOrigin.x, player.UnduckedOrigin.y, player.UnduckedOrigin.z,
				player.Velocity.x, player.Velocity.y, player.Velocity.z, player.Ducking, type == Strafe::PositionType::GROUND);
			type = Strafe::Move(player, vars);
		}
	}
	int GetIndex(void* ent)
	{
		if (!ent)
			return -1;

		auto engine = GetEngine();

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			auto e = engine->PEntityOfEntIndex(i);
			if (e && e->GetUnknown() == ent)
				return i;
		}

		return -1;
	}
#endif

	JBData CanJB(float height)
	{
		JBData data;
		data.ticks = -1;
		data.canJB = false;
		data.landingHeight = std::numeric_limits<float>::max();

		Vector player_origin = clientDLL.GetPlayerEyePos();
        Vector vel = clientDLL.GetPlayerVelocity();

		constexpr float gravity = 600;
		constexpr float groundThreshold = 2.0f;
		float ticktime = engineDLL.GetTickrate();
		constexpr int maxIterations = 1000;

		for (int i = 0; i < maxIterations; ++i)
		{
			auto absDiff = std::abs(player_origin.z - height);

			// If distance to ground is closer than any other attempt, update the data
			if (absDiff < std::abs(data.landingHeight - height) && vel.z < 0)
			{
				data.landingHeight = player_origin.z;
				data.ticks = i;
			
				// Can jb, exit the loop
				if (player_origin.z - height <= groundThreshold && player_origin.z - height >= 0)
				{
					data.canJB = true;
					break;
				}
				else if (player_origin.z < height && vel.z < 0) // fell past the jb point
				{
					break;
				}
			}

			vel.z -= (gravity * ticktime / 2);
			player_origin.z += vel.z * ticktime;
			vel.z -= (gravity * ticktime / 2);
		}

		return data;
	}

	bool serverActive()
	{
		return GetEngine()->PEntityOfEntIndex(0);
	}
}

