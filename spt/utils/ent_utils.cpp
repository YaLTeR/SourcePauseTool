#include "stdafx.hpp"

#include "ent_utils.hpp"

#include <limits>
#include <regex>
#include <sstream>
#include <vector>

#include "spt\utils\portal_utils.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\strafe\strafestuff.hpp"
#include "SPTLib\sptlib.hpp"
#include "client_class.h"
#include "game_detection.hpp"
#include "..\features\property_getter.hpp"
#include "string_utils.hpp"
#include "math.hpp"
#include "game_detection.hpp"
#include "..\features\playerio.hpp"
#include "..\features\tickrate.hpp"
#include "..\features\tracing.hpp"
#include "..\spt-serverplugin.hpp"
#include "interfaces.hpp"

#undef max

#if !defined(OE)
#define GAME_DLL
#include "cbase.h"
#endif

extern ConVar y_spt_piwsave;

namespace utils
{
	IClientEntity* GetClientEntity(int index)
	{
		if (index >= interfaces::entList->GetHighestEntityIndex())
			return nullptr;
		return interfaces::entList->GetClientEntity(index + 1);
	}

	void PrintAllClientEntities()
	{
		int maxIndex = interfaces::entList->GetHighestEntityIndex();

		for (int i = 0; i <= maxIndex; ++i)
		{
			auto ent = GetClientEntity(i);
			if (ent)
			{
				Msg("%d : %s\n", i, ent->GetClientClass()->m_pNetworkName);
			}
		}
	}

	IClientEntity* GetPlayer()
	{
		return GetClientEntity(0);
	}

	const char* GetModelName(IClientEntity* ent)
	{
		if (ent)
			return interfaces::modelInfo->GetModelName(ent->GetModel());
		else
			return nullptr;
	}

	ClientClass* GetClass(const char* name)
	{
		auto list = interfaces::clientInterface->GetAllClasses();

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
				sprintf_s(buffer,
				          size,
				          "(index %d, serial %d)",
				          ehandle & INDEX_MASK,
				          (ehandle & (~INDEX_MASK)) >> MAX_EDICT_BITS);
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
#ifdef SSDK2007
		case DPT_String:
			sprintf_s(buffer, size, "%s", *reinterpret_cast<const char**>(value));
			break;
#endif
		default:
			sprintf_s(buffer, size, "unable to parse");
			break;
		}
	}

	const size_t BUFFER_SIZE = 256;
	static char BUFFER[BUFFER_SIZE];

	void AddProp(std::vector<propValue>& props, const char* name, RecvProp* prop)
	{
		props.push_back(propValue(name, BUFFER, prop));
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
			else if (
			    prop->GetOffset()
			    != 0) // There's various garbage attributes at offset 0 for a thusfar unbeknownst reason
			{
				GetPropValue(prop, ptr, BUFFER, BUFFER_SIZE);
				AddProp(props, prop->GetName(), prop);
			}
		}
	}

	void GetAllProps(IClientEntity* ent, std::vector<propValue>& props)
	{
		props.clear();

		if (ent)
		{
			snprintf(BUFFER,
			         BUFFER_SIZE,
			         "(%.3f, %.3f, %.3f)",
			         ent->GetAbsOrigin().x,
			         ent->GetAbsOrigin().y,
			         ent->GetAbsOrigin().z);
			AddProp(props, "AbsOrigin", nullptr);
			snprintf(BUFFER,
			         BUFFER_SIZE,
			         "(%.3f, %.3f, %.3f)",
			         ent->GetAbsAngles().x,
			         ent->GetAbsAngles().y,
			         ent->GetAbsAngles().z);
			AddProp(props, "AbsAngles", nullptr);
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
				Msg("Name: %s, offset %d, value %s\n",
				    prop.name.c_str(),
				    prop.GetOffset(),
				    prop.value.c_str());
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
		return spt_playerio.m_vecAbsOrigin.GetValue() + spt_playerio.m_vecViewOffset.GetValue();
	}

	QAngle GetPlayerEyeAngles()
	{
		float va[3];
		EngineGetViewAngles(va);
		return QAngle(va[0], va[1], va[2]);
	}

	int PortalIsOrange(IClientEntity* ent)
	{
		return spt_propertyGetter.GetProperty<int>(ent->entindex() - 1, "m_bIsPortal2");
	}

	static IClientEntity* prevPortal = nullptr;
	static IClientEntity* prevLinkedPortal = nullptr;

	IClientEntity* FindLinkedPortal(IClientEntity* ent)
	{
		int ehandle = spt_propertyGetter.GetProperty<int>(ent->entindex() - 1, "m_hLinkedPortal");
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

	void GetValuesMatchingRegex(const char* regex,
	                            const char* end,
	                            int& entries,
	                            wchar* arr,
	                            int maxEntries,
	                            int bufferSize,
	                            const std::vector<propValue>& props)
	{
		std::regex r(regex, end);

		for (auto& prop : props)
		{
			if (entries >= maxEntries)
				break;

			// Prop name matches regex
			if (std::regex_match(prop.name.c_str(), r))
			{
				std::wstring name = Convert(prop.name);
				std::wstring value = Convert(prop.value);

				swprintf_s(arr + (entries * bufferSize),
				           bufferSize,
				           L"%s : %s",
				           name.c_str(),
				           value.c_str());
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
			bool readEntityIndex = i == 0 || args[i] == entSep;
			i += i > 0 || entries > 0; // i is at a separtor from the previous loop, go to next char
			int endIndex = i;

			// Go to the next separator
			while (endIndex < argSize && args[endIndex] != sep && args[endIndex] != entSep)
				++endIndex;

			if (readEntityIndex)
			{
				int entIndex = atoi(args + i);
				ent = GetClientEntity(entIndex);
				// Get all the props for this entity before hand
				GetAllProps(ent, props);

				if (!ent)
				{
					swprintf_s(arr + (entries * bufferSize),
					           bufferSize,
					           L"entity %d does not exist",
					           entIndex);
					++entries;
				}
				else
				{
					std::string temp(ent->GetClientClass()->GetName());
					std::wstring className = Convert(temp);

					// Add empty line in between entities
					if (entries != 0)
					{
						swprintf_s(arr + (entries * bufferSize), bufferSize, L"");
						++entries;
					}

					swprintf_s(arr + (entries * bufferSize),
					           bufferSize,
					           L"entity %d : %s",
					           entIndex,
					           className.c_str());
					++entries;
				}
			}
			// Read prop regex, make sure entity exists as well
			else if (ent)
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
		auto vars = spt_playerio.GetMovementVars();
		auto player = spt_playerio.GetPlayerData();
		auto type = Strafe::GetPositionType(player, Strafe::HullType::NORMAL);

		for (int i = 0; i < frames; ++i)
		{
			Msg("%d: pos: (%.3f, %.3f, %.3f), vel: (%.3f, %.3f, %.3f), ducked %d, onground %d\n",
			    i,
			    player.UnduckedOrigin.x,
			    player.UnduckedOrigin.y,
			    player.UnduckedOrigin.z,
			    player.Velocity.x,
			    player.Velocity.y,
			    player.Velocity.z,
			    player.Ducking,
			    type == Strafe::PositionType::GROUND);
			type = Strafe::Move(player, vars);
		}
	}
	int GetIndex(void* ent)
	{
		if (!ent)
			return -1;

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			auto e = interfaces::engine_server->PEntityOfEntIndex(i);
			if (e && e->GetUnknown() == ent)
				return i;
		}

		return -1;
	}

	int propValue::GetOffset()
	{
		if (prop)
		{
			return prop->GetOffset();
		}
		else
		{
			return INVALID_OFFSET;
		}
	}

	IServerUnknown* GetServerPlayer()
	{
		if (!interfaces::engine_server)
			return nullptr;

		auto edict = interfaces::engine_server->PEntityOfEntIndex(1);
		if (!edict)
			return nullptr;

		return edict->GetUnknown();
	}

	JBData CanJB(float height)
	{
		JBData data;
		data.ticks = -1;
		data.canJB = false;
		data.landingHeight = std::numeric_limits<float>::max();

		if (!utils::playerEntityAvailable())
			return data;

		Vector player_origin = spt_playerio.GetPlayerEyePos();
		Vector vel = spt_playerio.GetPlayerVelocity();

		constexpr float gravity = 600;
		constexpr float groundThreshold = 2.0f;
		float ticktime = spt_tickrate.GetTickrate();
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

	bool playerEntityAvailable()
	{
		return GetClientEntity(0) != nullptr;
	}

	static CBaseEntity* GetServerEntity(int index)
	{
		if (!interfaces::engine_server)
			return nullptr;

		auto edict = interfaces::engine_server->PEntityOfEntIndex(index);
		if (!edict)
			return nullptr;

		auto unknown = edict->GetUnknown();
		if (!unknown)
			return nullptr;

		return unknown->GetBaseEntity();
	}

	bool GetPunchAngleInformation(QAngle& punchAngle, QAngle& punchAngleVel)
	{
		auto ply = GetServerEntity(1);

		if (ply)
		{
			punchAngle = spt_playerio.m_vecPunchAngle.GetValue();
			punchAngleVel = spt_playerio.m_vecPunchAngleVel.GetValue();
			return true;
		}
		else
		{
			return false;
		}
	}

#if !defined(OE)
	static int GetServerEntityCount()
	{
		if (!interfaces::engine_server)
			return 0;

		return interfaces::engine_server->GetEntityCount();
	}

	void CheckPiwSave(bool simulating)
	{
		if (!simulating || y_spt_piwsave.GetString()[0] == '\0')
			return;

		auto ply = GetServerEntity(1);
		if (!ply)
			return;

		auto pphys = ply->VPhysicsGetObject();
		if (!pphys || (pphys->GetGameFlags() & FVPHYSICS_PENETRATING))
			return;

		int count = GetServerEntityCount();
		for (int i = 0; i < count; ++i)
		{
			auto ent = GetServerEntity(i);
			if (!ent)
				continue;

			auto phys = ent->VPhysicsGetObject();
			if (!phys)
				continue;

			const auto mask = FVPHYSICS_PLAYER_HELD | FVPHYSICS_PENETRATING;

			if ((phys->GetGameFlags() & mask) == mask)
			{
				std::ostringstream oss;
				oss << "save " << y_spt_piwsave.GetString();
				EngineConCmd(oss.str().c_str());
				y_spt_piwsave.SetValue("");
			}
		}
	}
#endif

} // namespace utils
