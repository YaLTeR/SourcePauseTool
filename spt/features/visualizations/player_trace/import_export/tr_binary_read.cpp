#include "stdafx.hpp"

#include <inttypes.h>

#include "tr_binary_internal.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

/*
* Broken up into 2 cases:
* 1) There are no upgrade handlers. Read data straight into the trace vector.
* 2) There are upgrade handlers. Read data into a temporary buffer and feed that buffer through all
*    the required handlers one at a time.
*/
template<typename T>
static bool TrReadLumpData(TrRestoreInternal& internal, TrTopologicalNode& node)
{
	auto& tr = internal.trace;
	auto& rd = internal.reader;
	auto& restore = internal.restore;
	auto& lump = node.lump;

	DevMsg("SPT [%s]: reading lump '%s' with %d handlers\n", __FUNCTION__, lump.name, node.handlers.size());

	std::vector<std::byte> upgradeBuf;
	bool anyHandlers = !node.handlers.empty();

	if (anyHandlers)
	{
		restore.warnings.push_back(
		    std::format("applying {} compatibility handler(s) to lump '{}' (upgrading from version {} to {})",
		                node.handlers.size(),
		                lump.name,
		                lump.structVersion,
		                TR_LUMP_VERSION(T)));

		upgradeBuf.resize(lump.dataLenBytes);
		if (!rd.ReadTo(std::span{upgradeBuf}, lump.dataOff))
			return false;

		for (auto handler : node.handlers)
		{
			if (handler->lumpVersion != lump.structVersion)
				continue;
			if (!handler->HandleCompat(internal, lump, upgradeBuf))
			{
				if (restore.errMsg.empty())
				{
					restore.errMsg = std::format("failed to upgrade lump '{}' to version {}",
					                             lump.name,
					                             TR_LUMP_VERSION(T));
				}
			}
		}
	}

	// with the current static assert(s) and upgrade handlers this assert should always be true
	Assert(lump.structVersion != TR_INVALID_STRUCT_VERSION);

	if (lump.firstExportVersion > lump.structVersion)
	{
		restore.warnings.push_back(std::format("clamping export version of lump '{}' to {} (was {})",
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
		restore.errMsg = std::format("bad lump version for lump '{}'{} (expected {}, got {})",
		                             lump.name,
		                             node.handlers.empty() ? "" : " after upgrading",
		                             TR_LUMP_VERSION(T),
		                             lump.structVersion);
		return false;
	}

	std::vector<T>& vec = tr.Get<T>();
	bool numBytesSeemsReasonable =
	    anyHandlers ? upgradeBuf.size() % sizeof(vec[0]) == 0 : sizeof(T) * lump.nElems == lump.dataLenBytes;

	if (!numBytesSeemsReasonable)
	{
		restore.errMsg = std::format("unexpected number of bytes for lump '{}'", lump.name);
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
		if (!rd.ReadTo(std::as_writable_bytes(std::span{vec}), lump.dataOff))
			return false;
	}
	return true;
}

static bool TrTopologicalSortVisit(TrRestoreInternal& internal, TrTopologicalNode& node)
{
	if (node.dfsVisited)
		return true;
	if (node.dfsVisiting)
	{
		internal.restore.errMsg = "cycle found while doing topological sort on lumps";
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
			auto it = internal.nameToNode.find(depName);
			if (it == internal.nameToNode.cend())
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
				internal.restore.errMsg = std::format(
				    "lump '{}' has an upgrade handler that depends on lump '{}' which does not exist in the file",
				    node.lump.name,
				    depName);
				return false;
			}
			if (!TrTopologicalSortVisit(internal, it->second))
				return false;
		}
	}

	internal.orderedNodes.push_back(&node);
	node.dfsVisited = true;
	return true;
}

// https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
static bool TrTopologicalSort(TrRestoreInternal& internal)
{
	for (auto& [_, node] : internal.nameToNode)
		if (!TrTopologicalSortVisit(internal, node))
			return false;
	return true;
}

template<typename T>
static void TrSetupReadFunc(TrRestoreInternal& internal)
{
	const char* lumpName = TR_LUMP_NAME(T);
	auto it = internal.nameToNode.find(lumpName);
	if (it == internal.nameToNode.cend())
	{
		internal.restore.warnings.push_back(
		    std::format("lump '{}' declared in TrPlayerTrace but not found in file", lumpName));
	}
	else
	{
		it->second.readFunc = TrReadLumpData<T>;
	}
}

bool TrRestore::Restore(TrPlayerTrace& tr, ITrReader& rd)
{
	tr.Clear();
	errMsg.clear();
	warnings.clear();

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

	if (preamble.fileVersion == 0 || preamble.fileVersion > TR_SERIALIZE_VERSION)
	{
		errMsg = std::format("expected file version {}, got {}", TR_SERIALIZE_VERSION, preamble.fileVersion);
		return false;
	}

	TrHeader header;
	if (!rd.ReadTo(header, sizeof preamble))
	{
		errMsg = "failed to read header";
		return false;
	}
	if (strncmp(header.sptVersion, SPT_VERSION, TR_MAX_SPT_VERSION_LEN))
	{
		warnings.push_back(
		    std::format("trace was exported with different SPT version '{}' (current version is '{}')",
		                header.sptVersion,
		                SPT_VERSION));
	}

	tr.numRecordedTicks = header.numRecordedTicks;
	tr.playerStandBboxIdx = header.playerStandBboxIdx;
	tr.playerDuckBboxIdx = header.playerDuckBboxIdx;

	std::vector<TrLump> lumps(header.nLumps);

	switch (preamble.fileVersion)
	{
	case 1:
	{
		std::vector<TrLump_v1> v1Lumps(header.nLumps);
		if (!rd.ReadTo(std::as_writable_bytes(std::span{v1Lumps}), header.lumpsOff))
		{
			errMsg = "failed to read lump headers";
			return false;
		}
		lumps.assign(v1Lumps.cbegin(), v1Lumps.cend());
		break;
	}
	default:
		if (!rd.ReadTo(std::as_writable_bytes(std::span{lumps}), header.lumpsOff))
		{
			errMsg = "failed to read lump headers";
			return false;
		}
		break;
	}

	TrRestoreInternal internal{
	    .restore = *this,
	    .trace = tr,
	    .reader = rd,
	};

	// step 0: create a map name->lump for quick lookup
	for (auto& lump : lumps)
	{
		lump.name[sizeof(lump.name) - 1] = '\0'; // explicitly null terminate so we can use it in errors
		auto [_, isNew] = internal.nameToNode.try_emplace(lump.name, lump);
		if (!isNew)
			warnings.push_back(std::format("duplicate lump '{}', ignoring the second one", lump.name));
	}

	// step 1: sort based on recursive handler dependencies
	if (!TrTopologicalSort(internal))
	{
		if (errMsg.empty())
			errMsg = "topological sort failed";
		return false;
	}

	// step 2: populate all node.readFuncs for all types in TrPlayerTrace
	std::apply([&](auto&... vecs)
	           { (TrSetupReadFunc<typename std::decay_t<decltype(vecs)>::value_type>(internal), ...); },
	           tr._storage);

	// step 3: execute node.readFuncs in order
	TrReadContextScope scope{tr};
	for (auto node : internal.orderedNodes)
	{
		if (!node->readFunc)
		{
			warnings.push_back(std::format("ignored lump '{}'", node->lump.name));
		}
		else if (!node->readFunc(internal, *node))
		{
			if (errMsg.empty())
				errMsg = std::format("readFunc faied for lump '{}'", node->lump.name);
			return false;
		}
	}

	tr.hasStartRecordingBeenCalled = true;

	return true;
}

#endif
