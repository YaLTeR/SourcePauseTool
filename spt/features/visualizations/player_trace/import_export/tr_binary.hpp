#pragma once

#include "../tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

#include "spt/utils/game_detection.hpp"

namespace player_trace
{
	constexpr char TR_FILE_ID[8] = "sausage";
	constexpr uint32_t TR_SERIALIZE_VERSION = 2;
	constexpr size_t TR_MAX_SPT_VERSION_LEN = 32;

	struct TrPreamble
	{
		char fileId[sizeof TR_FILE_ID];
		uint32_t fileVersion;
	};

	struct TrHeader_v1
	{
		char lastExportSptVersion[TR_MAX_SPT_VERSION_LEN];
		uint32_t lumpsOff;
		uint32_t nLumps;
		uint32_t numRecordedTicks;
		TrIdx<TrAbsBox> playerStandBboxIdx, playerDuckBboxIdx;
	};

	struct TrHeader_v2 : TrHeader_v1
	{
		TrHeader_v2() {}
		TrHeader_v2(const TrHeader_v1& v1)
		{
			memcpy(this, &v1, sizeof v1);

			strncpy(gameInfo.gameName, "<NOT_RECORDED>", sizeof gameInfo.gameName);
			strncpy(gameInfo.modName, "<NOT_RECORDED>", sizeof gameInfo.modName);
			gameInfo.gameVersion = -666;
			strncpy(playerName, "<NOT_RECORDED>", sizeof playerName);
		}

		// a bunch of info from when the trace was first recorded

		struct
		{
			char gameName[16]; // max length returned by utils::GetGameName()
			char modName[260]; // MAX_OSPATH
			int32_t gameVersion;
		} gameInfo;

		char playerName[32]; // MAX_PLAYER_NAME_LENGTH
	};

	using TrHeader = TrHeader_v2;

	struct TrLump_v1
	{
		char name[TR_MAX_LUMP_NAME_LEN];
		tr_struct_version structVersion;
		uint32_t dataOff;
		uint32_t dataLenBytes;
		// not necessary but useful for external tools so they don't have to know how to parse the each lump
		uint32_t nElems;
	};

	struct TrLump_v2 : TrLump_v1
	{
		TrLump_v2() {};

		TrLump_v2(const TrLump_v1& v1) : firstExportVersion{v1.structVersion}
		{
			memcpy(this, &v1, sizeof v1);
		}

		tr_struct_version firstExportVersion;
	};

	using TrLump = struct TrLump_v2;

	struct TrRestore;

	// handler for handling lump version upgrades
	struct TrLumpUpgradeHandler
	{
		/*
		* lump name -> list of all handlers for that lump
		*
		* The handlers are sorted by the (input) lump version, so the idea is that you find the
		* first handler which has the same version as whatever lump you're upgrading, then call its
		* HandleCompat method and HandleCompat of all the subsequent handlers to upgrade the lump
		* to the most up-to-date version.
		*/
		static auto& GetHandlersByName(const std::string& s)
		{
			static std::unordered_map<std::string, std::vector<const TrLumpUpgradeHandler*>> map;
			return map[s];
		}

		const char* lumpName;
		tr_struct_version lumpVersion;
		std::vector<std::string> lumpDependencies;

		TrLumpUpgradeHandler(const char* lumpName,
		                     tr_struct_version lumpVersion,
		                     const std::vector<std::string>& lumpDependencies)
		    : lumpName{lumpName}, lumpVersion{lumpVersion}, lumpDependencies{lumpDependencies}
		{
			auto& vec = GetHandlersByName(lumpName);
			auto biggerVerHandlerIt = std::ranges::find_if(vec,
			                                               [lumpVersion](auto handler)
			                                               { return handler->lumpVersion > lumpVersion; });
			AssertMsg2(biggerVerHandlerIt == vec.cend()
			               || (*biggerVerHandlerIt)->lumpVersion != lumpVersion,
			           "SPT: duplicate TrLumpUpgradeHandler version %d for lump %s",
			           lumpVersion,
			           lumpName);
			vec.insert(biggerVerHandlerIt, this);
		}

		/*
		* Given a lump, upgrades the lump to some subsequent version. Returns true on success and
		* false on failure. The data is read into the buffer by using lump.dataOff/dataLenBytes.
		* This function should modify the lump and data in the buffer to have the new lump data.
		* 
		* - changing the structVersion & nElems is REQUIRED
		* - changing dataOff or dataLenBytes has no effect (the data has already been read)
		* - changing the name is UB (you can read/write/edit other vectors in the trace though)
		* - changing firstExportVersion should only be done when upgrading from version 0
		* 
		* If reading data from other lumps, you must set them as a dependency in the constructor.
		* By the time this is called:
		* - TrTreadContextScope is set
		* - lump.name == lumpName
		* - lump.structVersion == lumpVersion
		*/
		virtual bool HandleCompat(TrRestore& restore,
		                          TrLump& lump,
		                          std::vector<std::byte>& dataInOut) const = 0;
	};

	struct TrRestore;

	struct TrTopologicalNode
	{
		TrLump& lump;
		std::span<const TrLumpUpgradeHandler*> handlers;
		bool (*readFunc)(TrRestore& restore, TrTopologicalNode& node);
		bool dfsVisited, dfsVisiting;
	};

	struct TrRestore
	{
		TrPlayerTrace& trace;
		ser::IReader& reader;

		std::unordered_map<std::string_view, TrTopologicalNode> nameToNode;
		std::vector<TrTopologicalNode*> orderedNodes;
	};
} // namespace player_trace

#endif
