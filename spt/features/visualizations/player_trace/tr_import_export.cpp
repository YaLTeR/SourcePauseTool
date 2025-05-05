#include "stdafx.hpp"

#include <unordered_set>

#include "tr_import_export.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

extern const char* SPT_VERSION;

constexpr char TR_FILE_ID[8] = "sausage";
constexpr uint32_t TR_SERIALIZE_VERSION = 1;
constexpr size_t TR_MAX_LUMP_NAME_LEN = 32;

namespace player_trace
{

	template<typename T, typename Tuple>
	struct tuple_contains_type : std::false_type
	{
	};

	template<typename T, typename... Tuple>
	struct tuple_contains_type<T, std::tuple<Tuple...>> : std::disjunction<std::is_same<T, Tuple>...>
	{
	};

	using tr_struct_version = uint32_t;

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
		static_assert(tuple_contains_type<std::vector<T>, decltype(TrPlayerTrace::_storage)>::value, \
		              "TrPlayerTrace::_storage must contain std::vector<T>"); \
		static constexpr const char name[TR_MAX_LUMP_NAME_LEN] = nameStr; \
		static constexpr tr_struct_version struct_version = versionNum; \
	}

#define TR_LUMP_TYPE_TO_NAME(T) (TrLumpInfo<T>::name)
#define TR_LUMP_TYPE_TO_VERSION(T) (TrLumpInfo<T>::struct_version)

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

	struct TrHeader
	{
		char sptVersion[TR_MAX_SPT_VERSION_LEN];
		uint32_t lumpsOff;
		uint32_t nLumps;
		uint32_t numRecordedTicks;
		TrIdx<TrAbsBox> playerStandBboxIdx, playerDuckBboxIdx;
	};

	struct TrLump_v1
	{
		char name[TR_MAX_LUMP_NAME_LEN];
		tr_struct_version struct_version;
		uint32_t dataOff;
		uint32_t dataLenBytes;
		// not necessary but useful for external tools so they don't have to know how to parse the each lump
		uint32_t nElems;
	};

	constexpr char TR_XZ_FILE_ID[] = "omg_hi!";
	constexpr uint32_t TR_XZ_FILE_VERSION = 1;

	struct TrXzFooter
	{
		uint32_t numCompressedBytes;
		uint32_t numUncompressedBytes;
		char id[sizeof TR_XZ_FILE_ID];
		uint32_t version;
	};

} // namespace player_trace

using namespace player_trace;

template<typename T>
bool ITrWriter::WriteLumpHeader(const std::vector<T>& vec, uint32_t& fileOff, uint32_t& lumpDataFileOff)
{
	TrLump lump{
	    .struct_version = TR_LUMP_TYPE_TO_VERSION(T),
	    .dataOff = lumpDataFileOff,
	    .dataLenBytes = vec.size() * sizeof(T),
	    .nElems = vec.size(),
	};
	strncpy(lump.name, TR_LUMP_TYPE_TO_NAME(T), sizeof lump.name);
	if (!Write(lump))
		return false;
	fileOff += sizeof lump;
	lumpDataFileOff += lump.dataLenBytes;
	return true;
}

template<typename T>
bool ITrWriter::WriteLumpData(const std::vector<T>& vec, uint32_t& fileOff)
{
	if (!Write(vec.data(), vec.size() * sizeof(T)))
		return false;
	fileOff += vec.size() * sizeof(T);
	return true;
}

bool ITrWriter::WriteTrace(const TrPlayerTrace& tr)
{
	size_t fileOff = 0;

	TrPreamble preamble{.fileVersion = TR_SERIALIZE_VERSION};
	memcpy(preamble.fileId, TR_FILE_ID, sizeof preamble.fileId);
	if (!Write(preamble))
		return false;
	fileOff += sizeof preamble;

	constexpr uint32_t nLumps = std::tuple_size_v<decltype(tr._storage)>;

	TrHeader header{
	    .lumpsOff = fileOff + sizeof header,
	    .nLumps = nLumps,
	    .numRecordedTicks = tr.numRecordedTicks,
	    .playerStandBboxIdx = tr.playerStandBboxIdx,
	    .playerDuckBboxIdx = tr.playerDuckBboxIdx,
	};
	strncpy(header.sptVersion, SPT_VERSION, sizeof header.sptVersion);
	if (!Write(header))
		return false;
	fileOff += sizeof header;

	uint32_t lumpDataFileOff = fileOff + sizeof(TrLump) * nLumps;
	if (!std::apply([&](auto&... vecs) { return (WriteLumpHeader(vecs, fileOff, lumpDataFileOff) && ...); },
	                tr._storage))
		return false;

	if (!std::apply([&](auto&... vecs) { return (WriteLumpData(vecs, fileOff) && ...); }, tr._storage))
		return false;

	DoneWritingTrace();
	return true;
}

template<typename T>
bool ITrReader::ReadLumpData(std::vector<T>& vec,
                             std::unordered_map<std::string_view, const TrLump*>& lumpMap,
                             std::string& errMsg)
{
	auto it = lumpMap.find(TR_LUMP_TYPE_TO_NAME(T));
	if (it == lumpMap.cend())
		return true; // compat with other versions, don't fill non-existent lumps

	const TrLump& lump = *it->second;
	if (lump.struct_version != TR_LUMP_TYPE_TO_VERSION(T))
	{
		errMsg = std::format("bad lump version for lump '{}' (expected {}, got {})",
		                     TR_LUMP_TYPE_TO_NAME(T),
		                     TR_LUMP_TYPE_TO_VERSION(T),
		                     lump.struct_version);
		return false;
	}
	if (sizeof(T) * lump.nElems != lump.dataLenBytes)
	{
		errMsg = std::format("unexpected number of bytes for lump '{}'", TR_LUMP_TYPE_TO_NAME(T));
		return false;
	}
	vec.resize(lump.dataLenBytes / sizeof(T));
	if (!ReadTo(std::span<char>{(char*)vec.data(), lump.dataLenBytes}, lump.dataOff))
	{
		errMsg = std::format("failed to read lump data for lump '{}'", TR_LUMP_TYPE_TO_NAME(T));
		return false;
	}

	lumpMap.erase(it);
	return true;
}

bool ITrReader::ReadTrace(TrPlayerTrace& tr, char sptVersionOut[TR_MAX_SPT_VERSION_LEN], std::string& errMsg)
{
	tr.Clear();

	TrPreamble preamble;
	if (!ReadTo(preamble, 0))
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

	TrHeader header;
	if (!ReadTo(header, sizeof preamble))
	{
		errMsg = "failed to read header";
		return false;
	}

	memcpy(sptVersionOut, header.sptVersion, sizeof header.sptVersion);
	tr.numRecordedTicks = header.numRecordedTicks;
	tr.playerStandBboxIdx = header.playerStandBboxIdx;
	tr.playerDuckBboxIdx = header.playerDuckBboxIdx;

	std::vector<TrLump> lumps{header.nLumps};
	if (!ReadTo(std::span<char>{(char*)lumps.data(), lumps.size() * sizeof(lumps[0])}, header.lumpsOff))
	{
		errMsg = "failed to read lump headers";
		return false;
	}
	std::unordered_map<std::string_view, const TrLump*> lumpMap;
	for (auto& lump : lumps)
		lumpMap[std::string_view{lump.name, strnlen(lump.name, sizeof lump.name)}] = &lump;

	errMsg.clear();
	// read all lumps and remove them from the lump map
	if (!std::apply([&](auto&... vecs) { return (ReadLumpData(vecs, lumpMap, errMsg) && ...); }, tr._storage))
	{
		if (errMsg.empty())
			errMsg = "failed to read lump data";
		return false;
	}

	DoneReadingTrace();

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

TrXzFileWriter::TrXzFileWriter(std::ostream& oStream, uint32_t compressionLevel)
    : oStream{oStream}, lzma_strm{lzma_stream LZMA_STREAM_INIT}, alive{true}
{
	lzma_mt mt{
	    .threads = std::thread::hardware_concurrency(),
	    .timeout = 0,
	    .preset = compressionLevel,
	    .check = LZMA_CHECK_CRC64,
	};
	alive = lzma_stream_encoder_mt(&lzma_strm, &mt) == LZMA_OK;
	if (alive)
		outBuf.resize(1 << 16);
}

TrXzFileWriter::~TrXzFileWriter()
{
	lzma_end(&lzma_strm);
}

bool TrXzFileWriter::Write(std::span<const char> sp)
{
	if (!alive)
		return false;
	lzma_strm.avail_in = sp.size_bytes();
	lzma_strm.next_in = (unsigned char*)sp.data();
	return LzmaToOfStream(false);
}

void TrXzFileWriter::DoneWritingTrace()
{
	alive = LzmaToOfStream(true);
	if (alive)
	{
		TrXzFooter footer{
		    .numCompressedBytes = (uint32_t)lzma_strm.total_out,
		    .numUncompressedBytes = (uint32_t)lzma_strm.total_in,
		    .version = TR_XZ_FILE_VERSION,
		};
		memcpy(footer.id, TR_XZ_FILE_ID, sizeof footer.id);
		alive = oStream.write((char*)&footer, sizeof footer).good();
	}
	if (alive)
		alive = oStream.flush().good();
}

bool TrXzFileWriter::LzmaToOfStream(bool finish)
{
	while (alive)
	{
		lzma_strm.avail_out = outBuf.size();
		lzma_strm.next_out = outBuf.data();
		lzma_ret ret = lzma_code(&lzma_strm, finish ? LZMA_FINISH : LZMA_RUN);
		alive = ret == LZMA_OK || ret == LZMA_STREAM_END;
		size_t nBytesToWrite = outBuf.size() - lzma_strm.avail_out;
		if (alive && nBytesToWrite > 0)
			alive = oStream.write((char*)outBuf.data(), nBytesToWrite).good();
		if ((!finish && lzma_strm.avail_out > 0) || (finish && ret == LZMA_STREAM_END))
			break;
	}
	return alive;
}

TrXzFileReader::TrXzFileReader(std::istream& iStream)
{
	TrXzFooter footer;
	bool alive = iStream.seekg(-(int)sizeof(TrXzFooter), std::ios_base::end)
	                 .read((char*)&footer, sizeof footer)
	                 .seekg(0)
	                 .good();
	if (!alive)
	{
		errMsg = "failed to read footer";
		return;
	}
	if (memcmp(footer.id, TR_XZ_FILE_ID, sizeof footer.id) || footer.version != TR_XZ_FILE_VERSION)
	{
		errMsg = "invalid footer";
		return;
	}

	lzma_stream lzma_strm LZMA_STREAM_INIT;

	lzma_mt mt{
	    .flags = LZMA_FAIL_FAST,
	    .threads = std::thread::hardware_concurrency(),
	    .timeout = 0,
	    .memlimit_threading = 1 << 29,
	    .memlimit_stop = 1 << 29,
	};
	lzma_ret ret = lzma_stream_decoder_mt(&lzma_strm, &mt);
	alive = ret == LZMA_OK;
	if (!alive)
	{
		errMsg = std::format("failed to initialize lzma decoder (error {})", (int)ret);
		return;
	}

	outBuf.resize(footer.numUncompressedBytes);
	lzma_strm.avail_out = outBuf.size();
	lzma_strm.next_out = outBuf.data();

	std::vector<uint8_t> inBuf(1 << 16);
	do
	{
		uint32_t nBytesToRead = MIN(inBuf.size(), footer.numCompressedBytes - lzma_strm.total_in);
		if (nBytesToRead == 0)
			break;
		alive = iStream.read((char*)inBuf.data(), nBytesToRead).good();
		if (alive)
		{
			lzma_strm.avail_in = inBuf.size();
			lzma_strm.next_in = inBuf.data();
			ret = lzma_code(&lzma_strm, LZMA_RUN);
			alive = ret == LZMA_OK || ret == LZMA_STREAM_END;
			if (!alive)
				errMsg = std::format("lzma_code error: {}", (int)ret);
		}
		else
		{
			errMsg = "file read error";
		}
	} while (alive && lzma_strm.avail_out > 0 && ret != LZMA_STREAM_END);

	if (alive && lzma_strm.total_out != outBuf.size())
	{
		errMsg = "stream ran out of bytes";
		alive = false;
	}

	if (!alive)
		outBuf = std::move(std::vector<uint8_t>{});

	lzma_end(&lzma_strm);
}

bool TrXzFileReader::ReadTo(std::span<char> sp, uint32_t at)
{
	if (at + sp.size() <= outBuf.size())
	{
		memcpy(sp.data(), outBuf.data() + at, sp.size());
		return true;
	}
	return false;
}

#endif
