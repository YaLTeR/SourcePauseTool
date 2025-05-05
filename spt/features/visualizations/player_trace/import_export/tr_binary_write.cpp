#include "stdafx.hpp"

#include "tr_binary_internal.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

template<typename T>
bool WriteLumpHeader(ITrWriter& wr, const std::vector<T>& vec, uint32_t& fileOff, uint32_t& lumpDataFileOff)
{
	TrLump lump{
	    .structVersion = TR_LUMP_VERSION(T),
	    .dataOff = lumpDataFileOff,
	    .dataLenBytes = vec.size() * sizeof(T),
	    .nElems = vec.size(),
	};
	strncpy(lump.name, TR_LUMP_NAME(T), sizeof lump.name);
	if (!wr.Write(lump))
		return false;
	fileOff += sizeof lump;
	lumpDataFileOff += lump.dataLenBytes;
	return true;
}

template<typename T>
bool WriteLumpData(ITrWriter& wr, const std::vector<T>& vec, uint32_t& fileOff)
{
	if (!wr.Write(std::as_bytes(std::span{vec})))
		return false;
	fileOff += vec.size() * sizeof(T);
	return true;
}

bool TrWrite::Write(const TrPlayerTrace& tr, ITrWriter& wr)
{
	size_t fileOff = 0;

	TrPreamble preamble{.fileVersion = TR_SERIALIZE_VERSION};
	memcpy(preamble.fileId, TR_FILE_ID, sizeof preamble.fileId);
	if (!wr.Write(preamble))
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
	if (!wr.Write(header))
		return false;
	fileOff += sizeof header;

	uint32_t lumpDataFileOff = fileOff + sizeof(TrLump) * nLumps;
	if (!std::apply([&](auto&... vecs) { return (WriteLumpHeader(wr, vecs, fileOff, lumpDataFileOff) && ...); },
	                tr._storage))
	{
		return false;
	}

	if (!std::apply([&](auto&... vecs) { return (WriteLumpData(wr, vecs, fileOff) && ...); }, tr._storage))
		return false;

	return wr.DoneWritingTrace();
}

#endif
