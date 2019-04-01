#pragma once

#include <string>
#include <map>
#include "icliententitylist.h"
#include "icliententity.h"
#include "engine\ivmodelinfo.h"
#include "cdll_int.h"

namespace utils
{
	const int INVALID_OFFSET = -1;

	struct OffsetMap
	{
		std::map<std::string, int> offsets;
		bool foundOffsets;
	};
	
	int GetOffset(int entindex, const std::string& key);

	template<typename T>
	T GetProperty(int entindex, const std::string & key)
	{
#ifdef OE
		return T();
#else
		int offset = GetOffset(entindex, key);
		if (offset == INVALID_OFFSET)
			return T();
		else
		{
			IClientEntity* ent = GetClientEntity(entindex);
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ent) + offset);
		}
#endif
	}
}