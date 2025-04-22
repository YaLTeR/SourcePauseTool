#include "stdafx.hpp"

#include <unordered_set>

#include "tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

constexpr char TR_FILE_ID[8] = "sausage";
constexpr uint32_t TR_SERIALIZE_VERSION = 1;
constexpr size_t TR_MAX_LUMP_NAME_LEN = 32;

template<typename T, typename Tuple>
struct tuple_contains_type : std::false_type
{
};

template<typename T, typename... Tuple>
struct tuple_contains_type<T, std::tuple<Tuple...>> : std::disjunction<std::is_same<T, Tuple>...>
{
};

template<size_t N>
struct _tstring
{
	constexpr _tstring(const char (&str)[N])
	{
		std::copy_n(str, N, val);
	}
	char val[N];
};

template<_tstring>
struct TrUniqueLumpName;

template<typename T>
struct TrLumpInfo;

#define TR_REGISTER_LUMP(T, nameStr, versionNum) \
	template<> \
	struct TrUniqueLumpName<nameStr> \
	{ \
	}; \
	template<> \
	struct TrLumpInfo<T> \
	{ \
		static_assert(tuple_contains_type<std::vector<T>, TrPlayerTrace::storageType>::value, \
		              "TrPlayerTrace::storageType must contain std::vector<T>"); \
		static constexpr const char name[TR_MAX_LUMP_NAME_LEN] = nameStr; \
		static constexpr uint32_t version = versionNum; \
	};

#define TR_LUMP_TYPE_TO_NAME(T) (TrLumpInfo<T>::name)
#define TR_LUMP_TYPE_TO_VERSION(T) (TrLumpInfo<T>::version)

/*
* Give a unique name to each vector type in TrPlayerTrace. Anything that doesn't match the name
* and version exactly will have to be processed for backwards compat. If you get crazy long error
* messages during compilation from this file it probably means you either:
* 
* - added a new vector to the trace and didn't register it here
* - removed a vector from the trace and didn't remove it from here
* - registered a duplicate lump name
*/
TR_REGISTER_LUMP(Vector, "point", 0);
TR_REGISTER_LUMP(TrIdx<Vector>, "point_idx", 0);
TR_REGISTER_LUMP(QAngle, "angle", 0);
TR_REGISTER_LUMP(TrTransform_v1, "transform", 1);
TR_REGISTER_LUMP(TrIdx<TrTransform_v1>, "transform_idx", 0);
TR_REGISTER_LUMP(TrPhysicsObjectInfo_v1, "physics_object_info", 1);
TR_REGISTER_LUMP(TrPhysicsObject_v1, "physics_object", 1);
TR_REGISTER_LUMP(TrPhysMesh_v1, "physics_mesh", 1);
TR_REGISTER_LUMP(TrEntTransform_v1, "ent_transform", 1);
TR_REGISTER_LUMP(TrIdx<TrPhysicsObject_v1>, "physics_object_idx", 0);
TR_REGISTER_LUMP(TrAbsBox_v1, "aabb", 1);
TR_REGISTER_LUMP(TrEnt_v1, "ent", 0);
TR_REGISTER_LUMP(TrEntSnapshot_v1, "ent_snap", 1);
TR_REGISTER_LUMP(TrEntSnapshotDelta_v1, "ent_snap_delta", 1);
TR_REGISTER_LUMP(TrEntCreateDelta_v1, "ent_create_delta", 1);
TR_REGISTER_LUMP(TrEntTransformDelta_v1, "ent_trans_delta", 1);
TR_REGISTER_LUMP(TrEntDeleteDelta_v1, "ent_delete_delta", 1);
TR_REGISTER_LUMP(TrPortalSnapshot_v1, "portal_snap", 1);
TR_REGISTER_LUMP(TrPortal_v1, "portal", 1);
TR_REGISTER_LUMP(TrIdx<TrPortal_v1>, "portal_idx", 0);
TR_REGISTER_LUMP(char, "string", 0);
TR_REGISTER_LUMP(TrTraceState_v1, "trace_state", 1);
TR_REGISTER_LUMP(TrServerState_v1, "server_state", 1);
TR_REGISTER_LUMP(TrSegmentStart_v1, "segment", 1);
TR_REGISTER_LUMP(TrMap_v1, "map", 1);
TR_REGISTER_LUMP(TrMapTransition_v1, "map_transition", 1);
TR_REGISTER_LUMP(TrLandmark_v1, "landmark", 1);
TR_REGISTER_LUMP(TrPlayerData_v1, "player_data", 1);
TR_REGISTER_LUMP(TrPlayerContactPoint_v1, "contact_point", 1);
TR_REGISTER_LUMP(TrIdx<TrPlayerContactPoint_v1>, "contact_point_idx", 0);

struct TrPreamble
{
	char fileId[sizeof TR_FILE_ID];
	uint32_t fileVersion;
};

struct TrHeader_v1
{
	char sptVersion[TR_MAX_SPT_VERSION_LEN];
	uint32_t lumpsOff;
	uint32_t nLumps;
	uint32_t numRecordedTicks;
	TrIdx<TrAbsBox_v1> playerStandBboxIdx, playerDuckBboxIdx;
};

struct TrLump_v1
{
	char name[TR_MAX_LUMP_NAME_LEN];
	uint32_t version;
	uint32_t dataOff;
	uint32_t dataLenBytes;
};

template<typename T>
static bool WriteLump(ITrWriter& wr, const std::vector<T>& vec, uint32_t& fileOff, uint32_t& lumpDataFileOff)
{
	TrLump_v1 lump{
	    .version = TR_LUMP_TYPE_TO_VERSION(T),
	    .dataOff = lumpDataFileOff,
	    .dataLenBytes = vec.size() * sizeof(T),
	};
	strncpy(lump.name, TR_LUMP_TYPE_TO_NAME(T), sizeof lump.name);
	if (!wr.Write(lump))
		return false;
	fileOff += sizeof lump;
	lumpDataFileOff += lump.dataLenBytes;
	return true;
}

template<typename T>
static bool WriteLumpData(ITrWriter& wr, const std::vector<T>& vec, uint32_t& fileOff)
{
	if (!wr.Write(vec.data(), vec.size() * sizeof(T)))
		return false;
	fileOff += vec.size() * sizeof(T);
	return true;
}

bool TrPlayerTrace::WriteBinaryStream(ITrWriter& wr) const
{
	size_t fileOff = 0;

	TrPreamble preamble{.fileVersion = TR_SERIALIZE_VERSION};
	memcpy(preamble.fileId, TR_FILE_ID, sizeof preamble.fileId);
	if (!wr.Write(preamble))
		return false;
	fileOff += sizeof preamble;

	constexpr uint32_t nLumps = std::tuple_size_v<decltype(storage)>;

	TrHeader_v1 header{
	    .lumpsOff = fileOff + sizeof header,
	    .nLumps = nLumps,
	    .numRecordedTicks = numRecordedTicks,
	    .playerStandBboxIdx = playerStandBboxIdx,
	    .playerDuckBboxIdx = playerDuckBboxIdx,
	};
	extern const char* SPT_VERSION;
	strncpy(header.sptVersion, SPT_VERSION, sizeof header.sptVersion);
	if (!wr.Write(header))
		return false;
	fileOff += sizeof header;

	uint32_t lumpDataFileOff = fileOff + sizeof(TrLump_v1) * nLumps;
	if (!std::apply([&](auto&... vecs) { return (WriteLump(wr, vecs, fileOff, lumpDataFileOff) && ...); }, storage))
		return false;

	if (!std::apply([&](auto&... vecs) { return (WriteLumpData(wr, vecs, fileOff) && ...); }, storage))
		return false;

	wr.Finish();
	return true;
}

template<typename T>
static bool ReadLumpData(ITrReader& rd,
                         std::vector<T>& vec,
                         std::unordered_map<std::string_view, const TrLump_v1*>& lumpMap,
                         std::string& errMsg)
{
	auto it = lumpMap.find(TR_LUMP_TYPE_TO_NAME(T));
	if (it == lumpMap.cend())
		return true; // compat with other versions, don't fill non-existent lumps

	const TrLump_v1& lump = *it->second;
	if (lump.version != TR_LUMP_TYPE_TO_VERSION(T))
	{
		errMsg = std::format("bad lump version for lump '{}' (expected {}, got {})",
		                     TR_LUMP_TYPE_TO_NAME(T),
		                     TR_LUMP_TYPE_TO_VERSION(T),
		                     lump.version);
		return false;
	}
	if (lump.dataLenBytes % sizeof(T) != 0)
	{
		errMsg = std::format("unexpected number of bytes for lump '{}'", TR_LUMP_TYPE_TO_NAME(T));
		return false;
	}
	vec.resize(lump.dataLenBytes / sizeof(T));
	if (!rd.ReadTo(std::span<char>{(char*)vec.data(), lump.dataLenBytes}, lump.dataOff))
	{
		errMsg = std::format("failed to read lump data for lump '{}'", TR_LUMP_TYPE_TO_NAME(T));
		return false;
	}

	lumpMap.erase(it);
	return true;
}

bool TrPlayerTrace::ReadBinaryStream(ITrReader& rd, char sptVersion[TR_MAX_SPT_VERSION_LEN], std::string& errMsg)
{
	Clear();

	TrPreamble preamble;
	if (!rd.ReadTo(preamble, 0))
	{
		errMsg = "failed to read preamble";
		return false;
	}

	if (sizeof(preamble.fileId) != sizeof(TR_FILE_ID)
	    || memcmp(preamble.fileId, TR_FILE_ID, sizeof(preamble.fileId)))
	{
		errMsg = "bad file ID";
		return false;
	}

	if (preamble.fileVersion != TR_SERIALIZE_VERSION)
	{
		// for now, no backwards or forwards compat
		errMsg = std::format("expected file version {}, got {}", TR_SERIALIZE_VERSION, preamble.fileVersion);
		return false;
	}

	TrHeader_v1 header;
	if (!rd.ReadTo(header, sizeof preamble))
	{
		errMsg = "failed to read header";
		return false;
	}

	memcpy(sptVersion, header.sptVersion, sizeof header.sptVersion);
	numRecordedTicks = header.numRecordedTicks;
	playerStandBboxIdx = header.playerStandBboxIdx;
	playerDuckBboxIdx = header.playerDuckBboxIdx;

	std::vector<TrLump_v1> lumps{header.nLumps};
	if (!rd.ReadTo(std::span<char>{(char*)lumps.data(), lumps.size() * sizeof(lumps[0])}, header.lumpsOff))
	{
		errMsg = "failed to read lump headers";
		return false;
	}
	std::unordered_map<std::string_view, const TrLump_v1*> lumpMap;
	for (auto& lump : lumps)
		lumpMap[std::string_view{lump.name, strnlen(lump.name, sizeof lump.name)}] = &lump;

	errMsg.clear();
	// read all lumps and remove them from the lump map
	if (!std::apply([&](auto&... vecs) { return (ReadLumpData(rd, vecs, lumpMap, errMsg) && ...); }, storage))
	{
		if (errMsg.empty())
			errMsg = "failed to read lump data";
		return false;
	}

	rd.Finish();

	if (!lumpMap.empty())
	{
		// these are warnings I guess?
		errMsg = "unprocessed lumps remaining: [";
		for (auto& [lumpName, _] : lumpMap)
		{
			errMsg += lumpName;
			if (lumpName != (--lumpMap.cend())->first)
				errMsg += ", ";
		}
		errMsg += "]";
	}

	return true;
}

#endif
