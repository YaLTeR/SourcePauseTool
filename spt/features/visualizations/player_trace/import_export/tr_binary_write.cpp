#include "stdafx.hpp"

#include "tr_binary.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

#include "spt/spt-serverplugin.hpp"

using namespace player_trace;

template<typename T>
void WriteLumpHeader(ser::IWriter& wr, const TrPlayerTrace& trace, const std::vector<T>& vec, uint32_t& lumpDataFileOff)
{
	TrLump lump{};
	strncpy(lump.name, TR_LUMP_NAME(T), sizeof lump.name);
	lump.structVersion = TR_LUMP_VERSION(T);
	lump.dataOff = lumpDataFileOff;
	lump.dataLenBytes = vec.size() * sizeof(T);
	lump.nElems = vec.size();
	lump.firstExportVersion = trace.GetFirstExportVersion<T>();

	wr.WritePod(lump);
	lumpDataFileOff += lump.dataLenBytes;
}

void TrPlayerTrace::Serialize(ser::IWriter& wr) const
{
	if (!hasStartRecordingBeenCalled)
	{
		wr.Err("trace has not been initialized");
		return;
	}

	wr.Rebase();

	TrPreamble preamble{.fileVersion = TR_SERIALIZE_VERSION};
	memcpy(preamble.fileId, TR_FILE_ID, sizeof preamble.fileId);
	if (!wr.WritePod(preamble))
		return;

	constexpr uint32_t nLumps = std::tuple_size_v<decltype(storage)>;

	TrHeader header{};
	header.lumpsOff = wr.GetRelPos() + sizeof header;
	header.nLumps = nLumps;
	header.numRecordedTicks = numRecordedTicks;
	header.playerStandBboxIdx = playerStandBboxIdx;
	header.playerDuckBboxIdx = playerDuckBboxIdx;
	strncpy(header.lastExportSptVersion, SPT_VERSION, sizeof header.lastExportSptVersion);
	strncpy(header.gameInfo.gameName, firstRecordedInfo.gameName.c_str(), sizeof header.gameInfo.gameName);
	strncpy(header.gameInfo.modName, firstRecordedInfo.gameModName.c_str(), sizeof header.gameInfo.modName);
	header.gameInfo.gameVersion = firstRecordedInfo.gameVersion;
	strncpy(header.playerName, firstRecordedInfo.playerName.c_str(), sizeof header.playerName);

	if (!wr.WritePod(header))
		return;

	uint32_t lumpOff = wr.GetRelPos() + sizeof(TrLump) * nLumps;
	std::apply([&](auto&... vecs) { return ((WriteLumpHeader(wr, *this, vecs, lumpOff), wr.Ok()), ...); }, storage);
	std::apply([&](auto&... vecs) { return (wr.WriteSpan(std::span{vecs}) && ...); }, storage);
}

#endif
