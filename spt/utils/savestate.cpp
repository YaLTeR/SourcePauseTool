#include "stdafx.h"
#include "savestate.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "..\OrangeBox\modules.hpp"
#include "..\OrangeBox\modules\ServerDLL.hpp"
#include "edict.h"
#include "string_parsing.hpp"
#include "gamestringpool.h"
#include "server_class.h"

namespace utils
{
	/*
	const int INDEX_MASK = MAX_EDICTS - 1;
	static IServerGameDLL* g_serverGameDll;
	static std::map<std::string, datamap_t*> classToDatamap;

	void SetGameDLL(IServerGameDLL* serverDll)
	{
		g_serverGameDll = serverDll;
	}

	edict_t* FindEntity(void* ent)
	{
		auto engine = GetEngine();

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			auto e = engine->PEntityOfEntIndex(i);
			if (e && e->GetUnknown() == ent)
			{
				return e;
			}
		}

		return nullptr;
	}

	void AddDatamap(datamap_t* map, void* ent)
	{
		auto edict = FindEntity(ent);

		if (edict)
		{
			std::string name(edict->GetClassName());
			if (classToDatamap.find(name) == classToDatamap.end())
			{
				classToDatamap[name] = map;
			}
		}

	}

	datamap_t* GetDatamap(edict_t* e)
	{
		std::string name(e->GetClassName());

		if (classToDatamap.find(name) != classToDatamap.end() && name != "env_soundscape" && name != "info_particle_system" && name != "worldspawn")
		{
			return classToDatamap[name];
		}
		else
			return nullptr;
	}

	void GetFields(datamap_t* map, SaveStateEntry& entry, edict_t* edict)
	{
		if (map->baseMap)
			GetFields(map->baseMap, entry, edict);

		for (int i = 0; i < map->dataNumFields; ++i)
		{
			auto& t = map->dataDesc[i];
			Datum item;
			void* value = reinterpret_cast<void*>((uintptr_t)edict->GetUnknown() + t.fieldOffset[0]);
			string_t stringVal;
			const char* pointer;
			std::string str;

			switch (t.fieldType)
			{
			case FIELD_EHANDLE:
				entry.ehandles.push_back(EHANDLEData(*reinterpret_cast<int*>(value) & INDEX_MASK, t.fieldOffset[0]));
				break;
			case FIELD_FLOAT:
			case FIELD_INTEGER:
			case FIELD_BOOLEAN:
			case FIELD_VECTOR:
			case FIELD_SHORT:
			case FIELD_CHARACTER:
			case FIELD_COLOR32:
			case FIELD_POSITION_VECTOR:
			case FIELD_TIME:
			case FIELD_TICK:
			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
			case FIELD_MATRIX3X4_WORLDSPACE:
			case FIELD_INTERVAL:
			case FIELD_VECTOR2D:
				if (t.fieldSizeInBytes <= MAX_BYTES_IN_DATUM && t.fieldSizeInBytes > 0)
				{
					item.CopyData(value, t.fieldOffset[0], t.fieldSizeInBytes);
					entry.data.push_back(item);
				}
				break;
			//case FIELD_STRING:
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
				if (stringVal.ToCStr() && *stringVal.ToCStr())
				{
					stringVal = *reinterpret_cast<string_t*>(value);
					item.CopyData((void*)stringVal.ToCStr(), t.fieldOffset[0], strlen(stringVal.ToCStr()) + 1);
					entry.stringData.push_back(item);
				}
				break;
			default:
				break;
			}

		}
	}

	SaveStateEntry GetSaveStateFromEdict(edict_t* edict)
	{
		SaveStateEntry entry;
		std::string name(edict->GetClassName());

		if (classToDatamap.find(name) != classToDatamap.end())
		{
			auto map = classToDatamap[name];
			if (map)
			{
				GetFields(map, entry, edict);
				entry.className = name;
				entry.stateFlags = edict->m_fStateFlags;
				entry.index = edict->GetUnknown()->GetRefEHandle().GetEntryIndex();
			}
		}

		return entry;
	}

	void LoadDataFromSaveState(void* ent, const SaveStateEntry& entry, edict_t* edict)
	{
		for (int i = 0; i < entry.data.size(); ++i)
		{
			auto& item = entry.data[i];
			void* value = reinterpret_cast<void*>((uintptr_t)ent + item.offset);
			memcpy(value, item.data, item.count);
		}

		for (int i = 0; i < entry.stringData.size(); ++i)
		{
			auto& item = entry.stringData[i];
			string_t result = serverDLL.ORIG_AllocPooledString(item.data, 0);
			string_t* value = reinterpret_cast<string_t*>((uintptr_t)ent + item.offset);
			*value = result;
		}

		edict->m_fStateFlags = entry.stateFlags;
	}

	void DeleteSavedEntities()
	{
		auto engine = GetEngine();
		for (int i = 2; i < MAX_EDICTS; ++i)
		{
			auto edict = engine->PEntityOfEntIndex(i);

			// Make sure this entity is one that can be restored before deleting
			if (edict && GetDatamap(edict))
			{
				edict->SetFree();
			}
		}

	}

	void SpawnEntityWithEntry(const SaveStateEntry& entry, std::map<int, int>& indexToNewEhandleMap, std::vector<edict_t*>& edicts)
	{
		auto engine = GetEngine();
		void* ent;
		edict_t* edict;

		Msg("Spawning %s\n", entry.className.c_str());

		if (entry.index > 1)
		{
			ent = serverDLL.ORIG_CreateEntityByName(entry.className.c_str(), -1);
			edict = FindEntity(ent);
		}
		else // Player and worldspawn are not respawned while restoring a savestate
		{
			edict = engine->PEntityOfEntIndex(entry.index);
			ent = edict->GetUnknown();
		}

		LoadDataFromSaveState(ent, entry, edict);
		// Respawn entities
		if (entry.index > 1)
		{
			int result = serverDLL.ORIG_DispatchSpawn(ent);
		}

		indexToNewEhandleMap[entry.index] = edict->GetIServerEntity()->GetRefEHandle().ToInt();
		edicts.push_back(edict);
	}

	void MapEHandles(const SaveStateEntry& entry, void* ent, std::map<int, int>& indexToNewEhandleMap)
	{
		for (auto& ehandle : entry.ehandles)
		{
			if (indexToNewEhandleMap.find(ehandle.index) != indexToNewEhandleMap.end())
			{
				int* value = reinterpret_cast<int*>((uintptr_t)ent + ehandle.offset);
				*value = indexToNewEhandleMap[ehandle.index];
			}
		}
	}

	void LoadSaveState(const SaveState& ss)
	{
		DeleteSavedEntities();
		std::map<int, int> indexToNewEhandleMap;
		std::vector<edict_t*> edicts;

		for(int i=0; i < ss.entries.size(); ++i)
		{
			auto& entry = ss.entries[i];
			SpawnEntityWithEntry(entry, indexToNewEhandleMap, edicts);
		}

		for (int i = 0; i < ss.entries.size(); ++i)
		{
			auto& entry = ss.entries[i];
			auto edict = edicts[i];

			MapEHandles(entry, edict->GetUnknown(), indexToNewEhandleMap);
		}
	}

	static SaveState ss;

	void GetSaveState()
	{
		auto engine = GetEngine();
		ss.entries.clear();

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			auto edict = engine->PEntityOfEntIndex(i);
			if (edict && GetDatamap(edict))
			{
				ss.entries.push_back(GetSaveStateFromEdict(edict));
			}
		}

	}

	void LoadSaveState()
	{
		LoadSaveState(ss);
	}

	void SpawnEntity(const char* name)
	{
		auto engine = GetEngine();
		auto e = engine->PEntityOfEntIndex(1000);
		e->SetFree();
		e->freetime = 0;


		e = engine->PEntityOfEntIndex(1000);
		auto handle = e->GetNetworkable()->GetEntityHandle();
		handle->SetRefEHandle(CBaseHandle(1000, 2048));


		Msg("result %d\n");
	}

	void PrintEntities()
	{
		auto engine = GetEngine();
		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			auto e = engine->PEntityOfEntIndex(i);
			if (e && !whiteSpacesOnly(e->GetClassName()))
			{
				Msg("%d : %s : %d\n", i, e->GetClassName(), GetDatamap(e) != nullptr);
			}
		}
	}

	void RestoreState(const SaveState& data)
	{
		if (!serverDLL.ORIG_CreateEntityByName)
		{
			Msg("CreateEntityByName function not hooked! Unable to load savestate.");
		}

		auto engine = GetEngine();
		for (int i = 1; i < MAX_EDICTS; ++i)
		{
			auto e = engine->PEntityOfEntIndex(i);
			if (e)
				engine->RemoveEdict(e);
		}

		for (auto& entry : data.entries)
		{
			if (entry.index != 0)
			{
				CBaseEntity* ent = serverDLL.ORIG_CreateEntityByName(entry.className.c_str(), entry.index);
				if (!ent)
				{
					Msg("Loading savestate failed! Unable to create entity.");
					return; // This will probably crash the game so an instant disconnect would be a good idea
				}

			}

			auto e = engine->PEntityOfEntIndex(entry.index);
			auto handle = e->GetNetworkable()->GetEntityHandle();
			handle->SetRefEHandle(CBaseHandle(entry.index, entry.serial));
			//RestoreEntityState(e, entry);
		}
	}
	Datum::Datum()
	{
		Reset();
	}
	void Datum::Reset()
	{
		offset = 0;
		count = 0;
		data = nullptr;
	}
	void Datum::CopyData(void * src, int offset, int count)
	{
		Allocate(count);
		this->offset = offset;
		this->count = count;
		memcpy(data, src, count);
	}
	void Datum::Allocate(int count)
	{
		if (data)
			delete[] data;
		data = new char[count];
	}
	Datum::~Datum()
	{
		if (data)
			delete[] data;
	}
	Datum::Datum(const Datum & rhs)
	{
		Reset();
		CopyData(rhs.data, rhs.offset, rhs.count);
	}*/
}