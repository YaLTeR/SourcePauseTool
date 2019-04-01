#include "stdafx.h"
#include "property_getter.hpp"
#include "ent_utils.hpp"
#include <vector>

namespace utils
{
	static std::map<std::string, OffsetMap> classToOffsetsMap;
#ifndef OE
	OffsetMap FindOffsets(IClientEntity* ent)
	{
		OffsetMap out;
		if (!ent)
		{
			out.foundOffsets = false;
			return out;
		}

		auto clientClass = ent->GetClientClass();
		std::vector<utils::propValue> props;
		utils::GetAllProps(clientClass->m_pRecvTable, ent, props);

		for (auto prop1 : props)
		{
			out.offsets[prop1.name] = prop1.offset;
		}

		out.foundOffsets = true;
		return out;
	}
#endif

	int GetOffset(int entindex, const std::string& key)
	{
#ifdef OE
		return INVALID_OFFSET;
#else
		auto ent = GetClientEntity(entindex);
		std::string className = ent->GetClientClass()->m_pNetworkName;

		if (classToOffsetsMap.find(className) == classToOffsetsMap.end())
		{
			auto map = FindOffsets(ent);
			if (!map.foundOffsets)
				return INVALID_OFFSET;
			classToOffsetsMap[className] = map;
		}

		auto& classMap = classToOffsetsMap[className].offsets;

		if (classMap.find(key) != classMap.end())
			return classMap[key];
		else
			return INVALID_OFFSET;
#endif
	}
}