#include "stdafx.hpp"

#include "tr_binary_internal.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

template<typename T>
bool WriteLumpHeader(ITrWriter& wr,
                     const TrPlayerTrace& trace,
                     const std::vector<T>& vec,
                     uint32_t& fileOff,
                     uint32_t& lumpDataFileOff)
{
	TrLump lump{};
	strncpy(lump.name, TR_LUMP_NAME(T), sizeof lump.name);
	lump.structVersion = TR_LUMP_VERSION(T);
	lump.dataOff = lumpDataFileOff;
	lump.dataLenBytes = vec.size() * sizeof(T);
	lump.nElems = vec.size();
	lump.firstExportVersion = trace.GetFirstExportVersion<T>();

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
	if (!tr.hasStartRecordingBeenCalled)
		return false;

	size_t fileOff = 0;

	TrPreamble preamble{.fileVersion = TR_SERIALIZE_VERSION};
	memcpy(preamble.fileId, TR_FILE_ID, sizeof preamble.fileId);
	if (!wr.Write(preamble))
		return false;
	fileOff += sizeof preamble;

	constexpr uint32_t nLumps = std::tuple_size_v<decltype(tr._storage)>;

	TrHeader header{};
	header.lumpsOff = fileOff + sizeof header;
	header.nLumps = nLumps;
	header.numRecordedTicks = tr.numRecordedTicks;
	header.playerStandBboxIdx = tr.playerStandBboxIdx;
	header.playerDuckBboxIdx = tr.playerDuckBboxIdx;
	strncpy(header.lastExportSptVersion, SPT_VERSION, sizeof header.lastExportSptVersion);
	strncpy(header.gameInfo.gameName, tr.firstRecordedInfo.gameName.c_str(), sizeof header.gameInfo.gameName);
	strncpy(header.gameInfo.modName, tr.firstRecordedInfo.gameModName.c_str(), sizeof header.gameInfo.modName);
	header.gameInfo.gameVersion = tr.firstRecordedInfo.gameVersion;
	strncpy(header.playerName, tr.firstRecordedInfo.playerName.c_str(), sizeof header.playerName);

	if (!wr.Write(header))
		return false;
	fileOff += sizeof header;

	uint32_t lumpDataFileOff = fileOff + sizeof(TrLump) * nLumps;
	if (!std::apply([&](auto&... vecs) { return (WriteLumpHeader(wr, tr, vecs, fileOff, lumpDataFileOff) && ...); },
	                tr._storage))
	{
		return false;
	}

	if (!std::apply([&](auto&... vecs) { return (WriteLumpData(wr, vecs, fileOff) && ...); }, tr._storage))
		return false;

	return wr.DoneWritingTrace();
}

#endif
