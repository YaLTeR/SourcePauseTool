#include "stdafx.h"

#include "property_getter.hpp"
#include <vector>
#include "ent_utils.hpp"

#ifndef OE
PropMap PropertyGetterFeature::FindOffsets(IClientEntity* ent)
{
	PropMap out;
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
		out.props[prop1.name] = prop1.prop;
	}

	out.foundOffsets = true;
	return out;
}
#endif

void PropertyGetterFeature::UnloadFeature()
{
	classToOffsetsMap.clear();
}

int PropertyGetterFeature::GetOffset(int entindex, const std::string& key)
{
#ifdef OE
	return INVALID_OFFSET;
#else
	auto prop = GetRecvProp(entindex, key);
	if (prop)
		return prop->GetOffset();
	else
		return INVALID_OFFSET;
#endif
}

RecvProp* PropertyGetterFeature::GetRecvProp(int entindex, const std::string& key)
{
#ifdef OE
	return nullptr;
#else
	auto ent = utils::GetClientEntity(entindex);
	std::string className = ent->GetClientClass()->m_pNetworkName;

	if (classToOffsetsMap.find(className) == classToOffsetsMap.end())
	{
		auto map = FindOffsets(ent);
		if (!map.foundOffsets)
			return nullptr;
		classToOffsetsMap[className] = map;
	}

	auto& classMap = classToOffsetsMap[className].props;

	if (classMap.find(key) != classMap.end())
		return classMap[key];
	else
		return nullptr;
#endif
}
