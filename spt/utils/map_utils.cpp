#include "stdafx.hpp"

#include <unordered_map>

#include "map_utils.hpp"
#include "spt/features/ent_props.hpp"

auto utils::GetLandmarksInLoadedMap() -> const SptLandmarkList&
{
	static std::unordered_map<std::string, SptLandmarkList> cache;
	static SptLandmarkList emptyList;

	auto [it, is_new] = cache.try_emplace(GetLoadedMap());
	if (!is_new)
		return it->second;

	static utils::CachedField<string_t, "CBaseEntity", "m_iClassname", true, false> f1;
	static utils::CachedField<string_t, "CBaseEntity", "m_iName", true, false> f2;
	static utils::CachedField<Vector, "CBaseEntity", "m_vecAbsOrigin", true, false> f3;
	static utils::CachedFields fields{f1, f2, f3};

	if (fields.HasAll())
	{
		if (!utils::spt_serverEntList.Valid())
		{
			cache.erase(it);
			return emptyList;
		}
		for (auto ent : utils::spt_serverEntList.GetEntList())
		{
			auto [cn, n, pos] = fields.GetAllPtrs(ent);
			if (strcmp(cn->ToCStr(), "info_landmark"))
				continue;
			it->second.emplace_back(n->ToCStr(), *pos);
		}
	}
	return it->second;
}

Vector utils::LandmarkDelta(const SptLandmarkList& from, const SptLandmarkList& to)
{
	for (auto& [fromName, fromPos] : from)
		for (auto& [toName, toPos] : to)
			if (fromName == toName)
				return fromPos - toPos;
	return vec3_origin;
}
