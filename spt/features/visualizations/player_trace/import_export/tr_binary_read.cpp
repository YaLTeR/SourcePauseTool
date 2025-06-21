#include "stdafx.hpp"

#include <inttypes.h>

#include "tr_binary.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

#include "spt/spt-serverplugin.hpp"

using namespace player_trace;

/*
* Broken up into 2 cases:
* 1) There are no upgrade handlers. Read data straight into the trace vector.
* 2) There are upgrade handlers. Read data into a temporary buffer and feed that buffer through all
*    the required handlers one at a time.
*/
template<typename T>
static bool TrReadLumpData(TrRestore& restore, TrTopologicalNode& node)
{
	auto& tr = restore.trace;
	auto& rd = restore.reader;
	auto& lump = node.lump;

	DevMsg("SPT [%s]: reading lump '%s' with %d handlers\n", __FUNCTION__, lump.name, node.handlers.size());

	rd.SetPos(lump.dataOff);
	std::vector<std::byte> upgradeBuf;
	bool anyHandlers = !node.handlers.empty();

	if (anyHandlers)
	{
		restore.reader.Warn(
		    std::format("applying {} compatibility handler(s) to lump '{}' (upgrading from version {} to {})",
		                node.handlers.size(),
		                lump.name,
		                lump.structVersion,
		                TR_LUMP_VERSION(T)));

		upgradeBuf.resize(lump.dataLenBytes);
		if (!rd.ReadSpan(std::span{upgradeBuf}))
			return false;

		for (auto handler : node.handlers)
		{
			if (handler->lumpVersion != lump.structVersion)
				continue;
			if (!handler->HandleCompat(restore, lump, upgradeBuf))
			{
				restore.reader.Err(std::format("failed to upgrade lump '{}' to version {}",
				                               lump.name,
				                               TR_LUMP_VERSION(T)));
				return false;
			}
		}
	}

	// with the current static assert(s) and upgrade handlers this assert should always be true
	Assert(lump.structVersion != TR_INVALID_STRUCT_VERSION);

	if (lump.firstExportVersion > lump.structVersion)
	{
		restore.reader.Warn(std::format("clamping export version of lump '{}' to {} (was {})",
		                                lump.name,
		                                lump.structVersion,
		                                lump.firstExportVersion));
		lump.firstExportVersion = lump.structVersion;
	}

	tr.SetFirstExportVersion<T>(lump.firstExportVersion);

	if (!anyHandlers && lump.firstExportVersion != lump.structVersion)
	{
		DevMsg("SPT [%s]: lump '%s' has version %" PRIu32 " (originally exported as version %" PRIu32 ")\n",
		       __FUNCTION__,
		       lump.name,
		       lump.structVersion,
		       lump.firstExportVersion);
	}

	if (lump.structVersion != TR_LUMP_VERSION(T))
	{
		restore.reader.Err(std::format("bad lump version for lump '{}'{} (expected {}, got {})",
		                               lump.name,
		                               node.handlers.empty() ? "" : " after upgrading",
		                               TR_LUMP_VERSION(T),
		                               lump.structVersion));
		return false;
	}

	std::vector<T>& vec = tr.Get<T>();
	bool numBytesSeemsReasonable =
	    anyHandlers ? upgradeBuf.size() % sizeof(vec[0]) == 0 : sizeof(T) * lump.nElems == lump.dataLenBytes;

	if (!numBytesSeemsReasonable)
	{
		restore.reader.Err(std::format("unexpected number of bytes for lump '{}'", lump.name));
		return false;
	}

	if (anyHandlers)
	{
		vec.resize(upgradeBuf.size() / sizeof(vec[0]));
		memcpy(vec.data(), upgradeBuf.data(), upgradeBuf.size());
	}
	else
	{
		vec.resize(lump.nElems);
		if (!rd.ReadSpan(std::span{vec}))
			return false;
	}
	return true;
}

static bool TrTopologicalSortVisit(TrRestore& restore, TrTopologicalNode& node)
{
	if (node.dfsVisited)
		return true;
	if (node.dfsVisiting)
	{
		restore.reader.Err("cycle found while doing topological sort on lumps");
		return false;
	}
	node.dfsVisiting = true;

	// figure out which handlers are required by this lump
	auto& allHandlers = TrLumpUpgradeHandler::GetHandlersByName(node.lump.name);
	auto firstHandlerIt =
	    std::ranges::find(allHandlers, node.lump.structVersion, [](auto handler) { return handler->lumpVersion; });
	node.handlers = std::span{firstHandlerIt, allHandlers.cend()};

	/*
	* Recursively visit all dependencies. This step is the opposite of the version of DFS that is
	* on wikipedia, but the result is that it produces the final list in the order that we need it.
	*/
	for (auto handler : node.handlers)
	{
		for (auto& depName : handler->lumpDependencies)
		{
			auto it = restore.nameToNode.find(depName);
			if (it == restore.nameToNode.cend())
			{
				/*
				* This may fail for a sufficiently old file which is missing some lump that an
				* upgrade handler creates, and some other handler depends on that *new* lump. So:
				* 
				* - handler A creates a new lump L which does not exist in the file
				* - handler B upgrades lump L
				* 
				* That will require additional logic.
				*/
				restore.reader.Err(std::format(
				    "lump '{}' has an upgrade handler that depends on lump '{}' which does not exist in the file",
				    node.lump.name,
				    depName));
				return false;
			}
			if (!TrTopologicalSortVisit(restore, it->second))
				return false;
		}
	}

	restore.orderedNodes.push_back(&node);
	node.dfsVisited = true;
	return true;
}

// https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
static bool TrTopologicalSort(TrRestore& restore)
{
	for (auto& [_, node] : restore.nameToNode)
		if (!TrTopologicalSortVisit(restore, node))
			return false;
	return true;
}

template<typename T>
static void TrSetupReadFunc(TrRestore& restore)
{
	const char* lumpName = TR_LUMP_NAME(T);
	auto it = restore.nameToNode.find(lumpName);
	if (it == restore.nameToNode.cend())
		restore.reader.Warn(std::format("lump '{}' declared in TrPlayerTrace but not found in file", lumpName));
	else
		it->second.readFunc = TrReadLumpData<T>;
}

void TrPlayerTrace::Deserialize(ser::IReader& rd)
{
	Clear();

	rd.Rebase();

	TrPreamble preamble;
	if (!rd.ReadPod(preamble))
		return;

	if (sizeof(preamble.fileId) != sizeof(TR_FILE_ID)
	    || memcmp(preamble.fileId, TR_FILE_ID, sizeof(preamble.fileId)))
	{
		rd.Err("bad file ID in trace preamble");
		return;
	}

	if (preamble.fileVersion == 0 || preamble.fileVersion > TR_SERIALIZE_VERSION)
	{
		rd.Err(std::format("expected file version {}, got {} in trace preamble",
		                   TR_SERIALIZE_VERSION,
		                   preamble.fileVersion));
		return;
	}

	TrHeader header;

	switch (preamble.fileVersion)
	{
	case 1:
	{
		TrHeader_v1 v1Header;
		if (!rd.ReadPod(v1Header))
			return;
		header = v1Header;
		break;
	}
	default:
		if (!rd.ReadPod(header))
			return;
		break;
	}

	TrRestore restore{
	    .trace = *this,
	    .reader = rd,
	};

	if (strncmp(header.lastExportSptVersion, SPT_VERSION, TR_MAX_SPT_VERSION_LEN))
	{
		rd.Warn(std::format("trace was exported with different SPT version '{}' (current version is '{}')",
		                    header.lastExportSptVersion,
		                    SPT_VERSION));
	}

	auto cArrToStdStr = [](const auto& cArr)
	{ return std::string(std::begin(cArr), std::find(std::begin(cArr), std::end(cArr), '\0')); };

	numRecordedTicks = header.numRecordedTicks;
	playerStandBboxIdx = header.playerStandBboxIdx;
	playerDuckBboxIdx = header.playerDuckBboxIdx;
	firstRecordedInfo.gameName = cArrToStdStr(header.gameInfo.gameName);
	firstRecordedInfo.gameModName = cArrToStdStr(header.gameInfo.modName);
	firstRecordedInfo.playerName = cArrToStdStr(header.playerName);
	firstRecordedInfo.gameVersion = header.gameInfo.gameVersion;

	rd.SetPos(header.lumpsOff);
	std::vector<TrLump> lumps(header.nLumps);

	switch (preamble.fileVersion)
	{
	case 1:
	{
		std::vector<TrLump_v1> v1Lumps(header.nLumps);
		if (!rd.ReadSpan(std::span{v1Lumps}))
			return;
		lumps.assign(v1Lumps.cbegin(), v1Lumps.cend());
		break;
	}
	default:
		if (!rd.ReadSpan(std::span{lumps}))
			return;
		break;
	}

	// step 0: create a map name->lump for quick lookup
	for (auto& lump : lumps)
	{
		lump.name[sizeof(lump.name) - 1] = '\0'; // explicitly null terminate so we can use it in errors
		auto [_, isNew] = restore.nameToNode.try_emplace(lump.name, lump);
		if (!isNew)
			rd.Warn(std::format("duplicate lump '{}', ignoring the second one", lump.name));
	}

	// step 1: sort based on recursive handler dependencies
	if (!TrTopologicalSort(restore))
		return;

	// step 2: populate all node.readFuncs for all types in TrPlayerTrace
	std::apply([&](auto&... vecs)
	           { (TrSetupReadFunc<typename std::decay_t<decltype(vecs)>::value_type>(restore), ...); },
	           storage);

	// step 3: execute node.readFuncs in order
	TrReadContextScope scope{*this};
	for (auto node : restore.orderedNodes)
	{
		if (!node->readFunc)
			rd.Warn(std::format("ignored lump '{}'", node->lump.name));
		else if (!node->readFunc(restore, *node))
			return;
	}

	hasStartRecordingBeenCalled = true;
}

#endif
