#include "stdafx.h"
#include "parsed_script.hpp"
#include "..\..\utils\md5.hpp"
#include "..\spt-serverplugin.hpp"
#include "framebulk_handler.hpp"

namespace scripts
{
	void ParsedScript::AddDuringLoadCmd(std::string cmd)
	{
		duringLoad += ";" + cmd;
	}

	void ParsedScript::AddInitCommand(std::string cmd)
	{
		initCommand += ";" + cmd;
	}

	void ParsedScript::AddFrameBulk(FrameBulkOutput& output)
	{
		if (output.ticks >= 0)
		{
			AddAfterFramesEntry(afterFramesTick, output.initialCommand);
			AddAfterFramesEntry(afterFramesTick, output.repeatingCommand);

			afterFramesTick += output.ticks;
			for (int i = 1; i < output.ticks; ++i)
				AddAfterFramesEntry(afterFramesTick + i, output.repeatingCommand);
		}
		else if (output.ticks == NO_AFTERFRAMES_BULK)
		{
			AddAfterFramesEntry(NO_AFTERFRAMES_BULK, output.initialCommand);
			AddAfterFramesEntry(NO_AFTERFRAMES_BULK, output.repeatingCommand);
		}
		else
		{
			throw std::exception("Frame bulk length was negative!");
		}
	}

	void ParsedScript::AddSaveState()
	{
		saveStates.push_back(GetSaveStateInfo());
	}

	void ParsedScript::Reset()
	{
		scriptName.clear();
		afterFramesTick = 0;
		afterFramesEntries.clear();
		saveStateIndexes.clear();
		initCommand = "sv_cheats 1; y_spt_pause 0;_y_spt_afterframes_await_load; _y_spt_afterframes_reset_on_server_activate 0";
		duringLoad.clear();
		saveName.clear();
		saveStates.clear();
	}

	void ParsedScript::Init(std::string scriptName)
	{
		this->scriptName = scriptName;
		int saveStateIndex = -1;

		for (int i = 0; i < saveStates.size(); ++i)
		{
			if (saveStates[i].exists)
				saveStateIndex = i;
			else
			{
				std::string entry("save " + saveStates[i].key);
				AddAfterFramesEntry(saveStates[i].tick, entry);
				Msg("Adding savestate entry %s\n", entry.c_str());
			}
		}

		if (saveStateIndex != -1)
		{
			int tick = saveStates[saveStateIndex].tick;
			saveName = saveStates[saveStateIndex].key;

			for (int i = 0; i < afterFramesEntries.size(); ++i)
			{
				if (afterFramesEntries[i].framesLeft == -1)
					continue;
				else if (afterFramesEntries[i].framesLeft >= tick)
				{
					// If the entry is on the same tick as the save, add a new during load entry
					if (afterFramesEntries[i].framesLeft == tick)
					{
						afterframes_entry_t copy = afterFramesEntries[i];
						copy.framesLeft = NO_AFTERFRAMES_BULK;
						afterFramesEntries.push_back(copy);
						afterFramesEntries[i].framesLeft = 0;
					}
					else
					{
						afterFramesEntries[i].framesLeft -= tick;
					}
				}
				else
				{
					afterFramesEntries.erase(afterFramesEntries.begin() + i);
					--i;
				}
					
			}
		}

		if (saveName.size() > 0)
			AddInitCommand("load " + saveName);
	}

	void ParsedScript::AddAfterFramesEntry(long long int tick, std::string command)
	{
		afterFramesEntries.push_back(afterframes_entry_t(tick, command));
	}

	Savestate ParsedScript::GetSaveStateInfo()
	{
		std::string data = saveName;

		for (int i = 0; i < afterFramesEntries.size(); ++i)
		{
			data += std::to_string(afterFramesEntries[i].framesLeft);
			data += afterFramesEntries[i].command;
		}
		
		MD5 hash(data);
		std::string digest = hash.hexdigest();

		Savestate ss(afterFramesTick, afterFramesEntries.size(), "ss-" + digest + "-" + std::to_string(afterFramesTick));
		Msg("Got savestate with hash %s\n", digest.c_str());

		return ss;
	}

	Savestate::Savestate(int tick, int index, std::string key) : tick(tick), index(index), key(key)
	{
		TestExists();
	}

	void Savestate::TestExists()
	{
		std::string dir = GetGameDir() + "\\SAVE\\" + key + ".sav";
		std::ifstream is;
		is.open(dir);
		exists = is.is_open();
	}
}