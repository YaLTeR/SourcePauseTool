#include "stdafx.h"

#include "ent_utils.hpp"

#include <limits>
#include <regex>
#include <sstream>
#include <vector>

#include "..\cvars.hpp"
#include "..\overlay\portal_camera.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\strafe\strafestuff.hpp"
#include "SPTLib\sptlib.hpp"
#include "client_class.h"
#include "game_detection.hpp"
#include "property_getter.hpp"
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

namespace utils
{
#ifndef OE
	IClientEntity* GetClientEntity(int index)
	{
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

	void PrintAllPortals()
	{
		int maxIndex = interfaces::entList->GetHighestEntityIndex();

		for (int i = 0; i <= maxIndex; ++i)
		{
			auto ent = GetClientEntity(i);
			if (!invalidPortal(ent))
			{
				auto& pos = ent->GetAbsOrigin();
				auto& angles = ent->GetAbsAngles();
				Msg("%d : %s, position (%.3f, %.3f, %.3f), angles (%.3f, %.3f, %.3f)\n",
				    i,
				    ent->GetClientClass()->m_pNetworkName,
				    pos.x,
				    pos.y,
				    pos.z,
				    angles.x,
				    angles.y,
				    angles.z);
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
		if (DoesGameLookLikePortal())
			return utils::GetProperty<QAngle>(0, "m_angEyeAngles[0]");
		else
			return utils::GetProperty<QAngle>(0, "m_angRotation");
	}

	int PortalIsOrange(IClientEntity* ent)
	{
		return utils::GetProperty<int>(ent->entindex() - 1, "m_bIsPortal2");
	}

	static IClientEntity* prevPortal = nullptr;
	static IClientEntity* prevLinkedPortal = nullptr;

	IClientEntity* FindLinkedPortal(IClientEntity* ent)
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
			else if (args[i] == sep)
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
#endif

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
#ifdef OE
		return false;
#else
		return GetClientEntity(0) != nullptr;
#endif
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

	void CheckPiwSave()
	{
		if (y_spt_piwsave.GetString()[0] != '\0')
		{
			auto ply = GetServerEntity(1);
			if (ply)
			{
				auto pphys = ply->VPhysicsGetObject();
				if (pphys && (pphys->GetGameFlags() & FVPHYSICS_PENETRATING) == 0)
				{
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
			}
		}
	}
#endif

	void FindClosestPlane(const trace_t& tr, trace_t& out, float maxDistSqr)
	{
		//look for an edge point
		Vector checkDirs[4];
		const float invalidNormalLength = 1.1f;
		out.fraction = 1.0f;

		if (tr.startsolid || tr.allsolid || !tr.plane.normal.IsValid()
		    || std::abs(tr.plane.normal.LengthSqr()) > invalidNormalLength)
		{
			return;
		}

		float planeDistanceDiff = tr.plane.normal.Dot(tr.endpos) - tr.plane.dist;
		const float INVALID_PLANE_DISTANCE_DIFF = 1.0f;

		// Sometimes the trace end position isnt on the plane? Why?
		if (std::abs(planeDistanceDiff) > INVALID_PLANE_DISTANCE_DIFF)
		{
			return;
		}

		//a vector lying on a plane
		Vector upVector = Vector(0, 0, 1);

		if (tr.plane.normal.z != 0)
		{
			upVector = Vector(1, 0, 0);
			upVector -= tr.plane.normal * upVector.Dot(tr.plane.normal);
			upVector.NormalizeInPlace();

			if (!upVector.IsValid())
			{
				Warning("Invalid upvector calculated\n");
				return;
			}
		}
		checkDirs[0] = tr.plane.normal.Cross(upVector);
		//a vector crossing the previous one
		checkDirs[1] = tr.plane.normal.Cross(checkDirs[0]);

		checkDirs[0].NormalizeInPlace();
		checkDirs[1].NormalizeInPlace();

		//the rest is the inverse of other vectors to get 4 vectors in all directions
		checkDirs[2] = checkDirs[0] * -1;
		checkDirs[3] = checkDirs[1] * -1;

		for (int i = 0; i < 4; i++)
		{
			trace_t newEdgeTr;
			spt_tracing.TraceFirePortal(newEdgeTr, tr.endpos, checkDirs[i]);

			if (utils::TraceHit(newEdgeTr, maxDistSqr))
			{
				if (newEdgeTr.fraction < out.fraction)
				{
					out = newEdgeTr;
				}
			}
		}

		if (out.fraction < 1.0f)
		{
			// Hack the seam position to lie exactly on the intersection of the two surfaces
			out.endpos -= (out.plane.normal.Dot(out.endpos) - out.plane.dist) * out.plane.normal;
			out.endpos -= (tr.plane.normal.Dot(out.endpos) - tr.plane.dist) * tr.plane.normal;
		}
	}

	bool TraceHit(const trace_t& tr, float maxDistSqr)
	{
		if (tr.fraction == 1.0f)
			return false;

		float lengthSqr = (tr.endpos - tr.startpos).LengthSqr();
		return lengthSqr < maxDistSqr;
	}

	bool TestSeamshot(const Vector& cameraPos,
	                  const Vector& seamPos,
	                  const cplane_t& plane1,
	                  const cplane_t& plane2,
	                  QAngle& seamAngle)
	{
		Vector diff1 = (seamPos - cameraPos);
		VectorAngles(diff1, seamAngle);
		seamAngle.x = utils::NormalizeDeg(seamAngle.x);
		seamAngle.y = utils::NormalizeDeg(seamAngle.y);

		trace_t seamTrace;
		Vector dir = seamPos - cameraPos;
		dir.NormalizeInPlace();
		spt_tracing.TraceFirePortal(seamTrace, cameraPos, dir);

		if (seamTrace.fraction == 1.0f)
		{
			return true;
		}
		else
		{
			float dot1 = plane1.normal.Dot(seamTrace.endpos) - plane1.dist;
			float dot2 = plane2.normal.Dot(seamTrace.endpos) - plane2.dist;

			if (dot1 < 0 || dot2 < 0)
			{
				return true;
			}
		}

		return false;
	}

} // namespace utils
