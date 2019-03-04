#include "stdafx.h"
#include "savestate.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "..\OrangeBox\modules.hpp"
#include "..\OrangeBox\modules\ServerDLL.hpp"
#include "edict.h"
#include "string_parsing.hpp"
#include "gamestringpool.h"

namespace utils
{
	static std::map<std::string, datamap_t*> classToDatamap;

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
				Msg("Added class %s\n", name.c_str());
			}
		}

	}

	datamap_t* GetDatamap(edict_t* e)
	{
		std::string name(e->GetClassName());

		if (classToDatamap.find(name) != classToDatamap.end())
		{
			return classToDatamap[name];
		}
		else
			return nullptr;
	}

	void RestoreEntityState(edict_t* e, const SaveStateEntry& entry)
	{
		//e->SetChangeInfoSerialNumber(entry.serial);
		/*
		for (auto& intEntry : entry.ints)
		{
			WriteInt(e, intEntry.first, intEntry.second);
		}*/
	}


	/*	FIELD_VOID = 0,			// No type or value
	FIELD_FLOAT,			// Any floating point value
	FIELD_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_INTEGER,			// Any integer or enum
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,			// 2 byte integer
	FIELD_CHARACTER,		// a byte
	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions

	FIELD_CLASSPTR,			// CBaseEntity *
	FIELD_EHANDLE,			// Entity handle
	FIELD_EDICT,			// edict_t *

	FIELD_POSITION_VECTOR,	// A world coordinate (these are fixed up across level transitions automagically)
	FIELD_TIME,				// a floating point time (these are fixed up automatically too!)
	FIELD_TICK,				// an integer tick count( fixed up similarly to time)
	FIELD_MODELNAME,		// Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,		// Engine string that is a sound name (needs precache)

	FIELD_INPUT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)

	FIELD_VMATRIX,			// a vmatrix (output coords are NOT worldspace)

	// NOTE: Use float arrays for local transformations that don't need to be fixed up.
	FIELD_VMATRIX_WORLDSPACE,// A VMatrix that maps some local space to world space (translation is fixed up on level transitions)
	FIELD_MATRIX3X4_WORLDSPACE,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)

	FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
	FIELD_MODELINDEX,		// a model index
	FIELD_MATERIALINDEX,	// a material index (using the material precache string table)
	
	FIELD_VECTOR2D,			// 2 floats

	FIELD_TYPECOUNT,		// MUST BE LAST*/

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
			case FIELD_FLOAT:
			case FIELD_INTEGER:
			case FIELD_BOOLEAN:
			case FIELD_VECTOR:
			case FIELD_SHORT:
			case FIELD_CHARACTER:
			case FIELD_COLOR32:
			case FIELD_EHANDLE:
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
					memcpy(item.data, value, t.fieldSizeInBytes);
					item.count = t.fieldSizeInBytes;
					item.offset = t.fieldOffset[0];
					entry.data.push_back(item);
				}
				break;
			case FIELD_STRING:
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
				stringVal = *reinterpret_cast<string_t*>(value);
				entry.stringData.push_back(StringDatum(stringVal, t.fieldOffset[0]));
				break;
			default:
				if (t.fieldName)
					Msg("%s : type %d : offset %d : count %d\n", t.fieldName, t.fieldType, t.fieldOffset, t.fieldSizeInBytes);
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
			string_t* value = reinterpret_cast<string_t*>((uintptr_t)ent + item.offset);
			*value = item.data;
		}

		edict->m_fStateFlags = entry.stateFlags;

	}

	static SaveStateEntry savedEntry;

	void WriteFloat(edict_t * ent, const std::string & propName, float value)
	{
	}

	void WriteInt(edict_t * ent, const std::string & propName, int value)
	{
	}

	void WriteVector(edict_t * ent, const std::string & propName, const Vector & value)
	{
	}

	void GetSaveState(int index)
	{
		auto engine = GetEngine();
		auto edict = engine->PEntityOfEntIndex(index);
		if (edict && !whiteSpacesOnly(edict->GetClassName()))
		{
			savedEntry = GetSaveStateFromEdict(edict);
		}
	}

	void LoadSaveState()
	{
		void* ent = serverDLL.ORIG_CreateEntityByName(savedEntry.className.c_str(), -1);
		edict_t* edict = FindEntity(ent);
		LoadDataFromSaveState(ent, savedEntry, edict);
		int result = serverDLL.ORIG_DispatchSpawn(ent);
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
			if(e)
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
			RestoreEntityState(e, entry);
		}
	}
}