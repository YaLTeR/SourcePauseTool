#pragma once

#include <map>
#include <string>
#include "spt/feature.hpp"

#include "cdll_int.h"
#include "engine\ivmodelinfo.h"
#include "client_class.h"
#include "icliententity.h"
#include "icliententitylist.h"
#include "ent_utils.hpp"
#include "spt\utils\ent_list.hpp"

#define INVALID_OFFSET -1

struct PropMap
{
	std::map<std::string, RecvProp*> props;
	bool foundOffsets;
};

class PropertyGetterFeature : public FeatureWrapper<PropertyGetterFeature>
{
private:
	std::map<std::string, PropMap> classToOffsetsMap;

	PropMap FindOffsets(IClientEntity* ent);

protected:
	void UnloadFeature() override;

public:
	int GetOffset(int entindex, const std::string& key);

	RecvProp* GetRecvProp(int entindex, const std::string& key);

	template<typename T>
	T GetProperty(int entindex, const std::string& key)
	{
		auto ent = utils::spt_clientEntList.GetEnt(entindex);

		if (!ent)
			return T();

		int offset = GetOffset(entindex, key);
		if (offset == INVALID_OFFSET)
			return T();
		else
		{
			return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ent) + offset);
		}
	}
};

inline PropertyGetterFeature spt_propertyGetter;
