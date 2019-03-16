#pragma once
#include <map>
#include <vector>
#include "edict.h"
#include "eiface.h"

#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

namespace utils
{
	/*
	const int MAX_BYTES_IN_DATUM = 64;

	struct Datum
	{
		char* data;
		int offset;
		int count;

		Datum();
		void Reset();
		void CopyData(void* src, int offset, int count);
		void Allocate(int count);
		~Datum();
		Datum(const Datum& rhs);
		Datum& operator=(const Datum& rhs) = delete;
	};

	struct EHANDLEData
	{
		int index;
		int offset;

		EHANDLEData(int index, int offset) : index(index), offset(offset) {}
	};

	struct SaveStateEntry
	{
		std::vector<Datum> data;
		std::vector<Datum> stringData;
		std::vector<EHANDLEData> ehandles;
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

	void GetSaveState();
	void LoadSaveState();

	void WriteFloat(edict_t* ent, const std::string& propName, float value);
	void WriteInt(edict_t* ent, const std::string& propName, int value);
	void WriteVector(edict_t* ent, const std::string& propName, const Vector& value);
	void RestoreState(const SaveState& data);
	void PrintEntities();
	void AddDatamap(datamap_t* map, void* ent);
	void SpawnEntity(const char* name);
	void SetGameDLL(IServerGameDLL* serverDll);*/
}