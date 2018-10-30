#pragma once

#include <string>
#include <vector>
#include "..\modules\ClientDLL.hpp"
#include "framebulk_handler.hpp"

namespace scripts
{
	struct Savestate
	{
		Savestate() {}
		Savestate(int tick, int index, std::string key);
		int index;
		int tick;
		std::string key;
		bool exists;

	private:
		void TestExists();
	};

	class ParsedScript
	{
	public:
		std::string initCommand;
		std::string duringLoad;
		std::vector<afterframes_entry_t> afterFramesEntries;
		std::vector<int> saveStateIndexes;
		
		void Reset();
		void Init(std::string scriptName);

		void AddDuringLoadCmd(std::string cmd);
		void AddInitCommand(std::string cmd);
		void AddFrameBulk(FrameBulkOutput output);
		void AddSaveState();
		void AddAfterFramesEntry(long long int tick, std::string command);
		void SetSave(std::string save) { saveName = save };

	private:
		std::string saveName;
		std::string scriptName;
		int afterFramesTick;
		Savestate GetSaveStateInfo(int tick);
		std::vector<Savestate> saveStates;
	};
}