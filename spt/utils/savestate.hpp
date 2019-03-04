#pragma once
#include <map>
#include <vector>
#include "edict.h"

#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

namespace utils
{
	const int MAX_BYTES_IN_DATUM = 64;

	struct Datum
	{
		char data[MAX_BYTES_IN_DATUM];
		int offset;
		int count;

		Datum()
		{
			offset = 0;
			count = 0;
		}
	};

	struct StringDatum
	{
		string_t data;
		int offset;

		StringDatum(const string_t& d, int offset) : data(d), offset(offset) {}
	};

	struct SaveStateEntry
	{
		std::vector<Datum> data;
		std::vector<StringDatum> stringData;
		std::string className;
		int serial;
		int index;
		int modelIndex;
		int stateFlags;
	};

	struct SaveState
	{
		std::vector<SaveStateEntry> entries;
		float time;
	};

	void GetSaveState(int index);
	void LoadSaveState();

	void WriteFloat(edict_t* ent, const std::string& propName, float value);
	void WriteInt(edict_t* ent, const std::string& propName, int value);
	void WriteVector(edict_t* ent, const std::string& propName, const Vector& value);
	void RestoreState(const SaveState& data);
	void PrintEntities();
	void AddDatamap(datamap_t* map, void* ent);
	void SpawnEntity(const char* name);
}