#include "stdafx.h"
#include "datamap_wrapper.hpp"

namespace utils
{
	DatamapWrapper::DatamapWrapper()
	{
		_serverMap = nullptr;
		_clientMap = nullptr;
		offsetsCached = false;
	}

	int DatamapWrapper::GetClientOffset(const std::string& key)
	{
		if (!offsetsCached)
			CacheOffsets();
		auto search = clientOffsets.find(key);
		if (search != clientOffsets.end())
			return search->second;
		else
			return INVALID_DATAMAP_OFFSET;
	}

	int DatamapWrapper::GetServerOffset(const std::string& key)
	{
		if (!offsetsCached)
			CacheOffsets();
		auto search = serverOffsets.find(key);
		if (search != serverOffsets.end())
			return search->second;
		else
			return INVALID_DATAMAP_OFFSET;
	}

	void DatamapWrapper::ExploreOffsets()
	{
		auto map = _serverMap;
		if (map)
		{
			Msg("%s server map:\n", map->dataClassName);
			ExploreOffsetsHelper(map, 0, "");
			Msg("\n");
		}
		map = _clientMap;
		if (map)
		{
			Msg("%s client map:\n", map->dataClassName);
			ExploreOffsetsHelper(map, 0, "");
			Msg("\n");
		}
	}

	void DatamapWrapper::ExploreOffsetsHelper(datamap_t* map, int prefixOffset, std::string prefix)
	{
		if (map->baseMap)
			ExploreOffsetsHelper(map->baseMap, prefixOffset, prefix);

		for (int i = 0; i < map->dataNumFields; ++i)
		{
			auto typedescription = map->dataDesc[i];

			switch (typedescription.fieldType)
			{
				// Some weird stuff so ignore it
			case FIELD_VOID:
			case FIELD_CUSTOM:
			case FIELD_FUNCTION:
			case FIELD_TYPECOUNT:
				continue;
			default:
				break;
			}

			int offset = typedescription.fieldOffset[0] + prefixOffset;
			std::string prefixedName = prefix + typedescription.fieldName;

			switch (typedescription.fieldType)
			{
				// Datamaps for "embedded" members (read: struct)
				// The child fields will be named parentField.childField in the offset map
			case FIELD_EMBEDDED:
				ExploreOffsetsHelper(typedescription.td, offset, prefixedName + ".");
				break;
			default:
				Msg("\t%s : %d\n", prefixedName.c_str(), offset);
				break;
			}
		}
	}

	void DatamapWrapper::CacheOffsets()
	{
		if (_serverMap)
			CacheOffsetsHelper(serverOffsets, 0, "", _serverMap);
		if (_clientMap)
			CacheOffsetsHelper(clientOffsets, 0, "", _clientMap);
		offsetsCached = true;
	}

	void DatamapWrapper::CacheOffsetsHelper(std::unordered_map<std::string, int>& offsetMap,
	                                        int prefixOffset,
	                                        std::string prefix,
	                                        datamap_t* map)
	{
		if (map->baseMap)
			CacheOffsetsHelper(offsetMap, prefixOffset, prefix, map->baseMap);

		for (int i = 0; i < map->dataNumFields; ++i)
		{
			auto typedescription = map->dataDesc[i];

			switch (typedescription.fieldType)
			{
			// Some weird stuff so ignore it
			case FIELD_VOID:
			case FIELD_CUSTOM:
			case FIELD_FUNCTION:
			case FIELD_TYPECOUNT:
				continue;
			default:
				break;
			}

			int offset = typedescription.fieldOffset[0] + prefixOffset;
			std::string prefixedName = prefix + typedescription.fieldName;

			switch (typedescription.fieldType)
			{
			// Datamaps for "embedded" members (read: struct)
			// The child fields will be named parentField.childField in the offset map
			case FIELD_EMBEDDED:
				CacheOffsetsHelper(offsetMap, offset, prefixedName + ".", typedescription.td);
				break;
			default:
				offsetMap[prefixedName] = offset;
				break;
			}
		}
	}
} // namespace utils