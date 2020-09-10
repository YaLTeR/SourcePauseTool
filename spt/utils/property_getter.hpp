#pragma once

#include <map>
#include <string>
#include "cdll_int.h"
#include "engine\ivmodelinfo.h"
#include "client_class.h"
#include "icliententity.h"
#include "icliententitylist.h"

namespace utils
{
	const int INVALID_OFFSET = -1;

	struct PropMap
	{
		std::map<std::string, RecvProp*> props;
		bool foundOffsets;
	};

	int GetOffset(int entindex, const std::string& key);
	RecvProp* GetRecvProp(int entindex, const std::string& key);

	template<typename T>
	T GetProperty(int entindex, const std::string& key)
	{
#ifdef OE
		return T();
#else
		auto ent = GetClientEntity(entindex);

		if (!ent)
			return T();

		int offset = GetOffset(entindex, key);
		if (offset == INVALID_OFFSET)
			return T();
		else
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ent) + offset);
		}
#endif
	}
} // namespace utils