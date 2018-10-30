#include "stdafx.h"
#include "parsed_script.hpp"
#include "..\..\utils\tweetnacl.h"
#include "..\spt-serverplugin.hpp"

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

	void ParsedScript::AddFrameBulk(FrameBulkOutput output)
	{
		if (output.repeatingCommand.length() > 0)
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
		saveStateIndexes.push_back(afterFramesEntries.size());
	}

	void ParsedScript::Reset()
	{
		scriptName = "";
		afterFramesTick = 0;
		afterFramesEntries.clear();
		saveStateIndexes.clear();
		initCommand = "sv_cheats 1; y_spt_pause 0;_y_spt_afterframes_await_load; _y_spt_afterframes_reset_on_server_activate 0";
		duringLoad.clear();
		saveName.clear();
	}

	void ParsedScript::Init(std::string scriptName)
	{
		this->scriptName = scriptName;

		for (int i = 0; i < saveStateIndexes.size(); ++i)
		{
			saveStates.push_back(GetSaveStateInfo(i));
		}

		int saveStateIndex = -1;

		for (int i = 0; i < saveStates.size(); ++i)
		{
			if (saveStates[i].exists)
				saveStateIndex = i;
		}

		if (saveStateIndex != -1)
		{
			int tick = saveStates[saveStateIndex].tick;
			saveName = saveStates[saveStateIndex].key;

			for (int i = 0; i < afterFramesEntries.size(); ++i)
			{
				if (afterFramesEntries[i].framesLeft == -1)
					continue;
				else if (afterFramesEntries[i].framesLeft > tick)
				{
					afterFramesEntries[i].framesLeft -= tick;
				}
				else
					afterFramesEntries[i].framesLeft = 0;
			}
		}

		if (saveName.size() > 0)
			AddInitCommand("load " + saveName);
	}

	void ParsedScript::AddAfterFramesEntry(long long int tick, std::string command)
	{
		afterFramesEntries.push_back(afterframes_entry_t(tick, command));
	}

	Savestate ParsedScript::GetSaveStateInfo(int index)
	{
		static unsigned char hash[crypto_hash_sha256_BYTES];
		std::string data;
		int tick = 0;
		
		for (int i = 0; i < afterFramesEntries.size() && i < index; ++i)
		{
			if (afterFramesEntries[i].framesLeft > 0)
				tick = afterFramesEntries[i].framesLeft;

			data += std::to_string(afterFramesEntries[i].framesLeft);
			data += afterFramesEntries[i].command;
		}
		
		crypto_hashblocks_sha256_tweet(hash, (const unsigned char*)data.c_str(), data.size());

		Savestate ss(tick, index,
			scriptName + std::string((char*)&hash, crypto_hash_sha256_BYTES) + "-" + std::to_string(tick));

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