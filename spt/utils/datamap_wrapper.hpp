#pragma once
#include "datamap.h"
#include <unordered_map>
#include "ent_utils.hpp"

namespace utils
{
	const int INVALID_DATAMAP_OFFSET = -1;

	class DatamapWrapper
	{
	public:
		datamap_t* _serverMap;
		datamap_t* _clientMap;

		DatamapWrapper();
		int GetClientOffset(const std::string& key);
		int GetServerOffset(const std::string& key);
		void ExploreOffsets();

	private:
		void ExploreOffsetsHelper(std::vector<std::pair<int, std::string>>& outVec,
		                          datamap_t* map,
		                          int prefixOffset,
		                          const std::string& prefix);
		void CacheOffsets();
		void CacheOffsetsHelper(std::unordered_map<std::string, int>& offsetMap,
		                        int prefixOffset,
		                        std::string prefix,
		                        datamap_t* map);
		std::unordered_map<std::string, int> clientOffsets;
		std::unordered_map<std::string, int> serverOffsets;
		bool offsetsCached;
	};
} // namespace utils