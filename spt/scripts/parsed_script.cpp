#include "stdafx.hpp"
#include "parsed_script.hpp"
#include "..\spt-serverplugin.hpp"
#include "file.hpp"
#include "..\features\demo.hpp"
#include "framebulk_handler.hpp"
#include "thirdparty\md5.hpp"

extern ConVar tas_script_savestates;

namespace scripts
{
	void ParsedScript::AddDuringLoadCmd(const std::string& cmd)
	{
		duringLoad.push_back(';');
		duringLoad += cmd;
	}

	void ParsedScript::AddInitCommand(const std::string& cmd)
	{
		initCommand.push_back(';');
		initCommand += cmd;
	}

	void ParsedScript::AddFrameBulk(FrameBulkOutput& output)
	{
		if (output.ticks >= 0)
		{
			AddAfterFramesEntry(afterFramesTick, output.initialCommand);
			AddAfterFramesEntry(afterFramesTick, output.repeatingCommand);

			for (int i = 1; i < output.ticks; ++i)
				AddAfterFramesEntry(afterFramesTick + i, output.repeatingCommand);

			afterFramesTick += output.ticks;
		}
		else if (output.ticks == NO_AFTERFRAMES_BULK)
		{
			AddAfterFramesEntry(NO_AFTERFRAMES_BULK, output.initialCommand);
			AddAfterFramesEntry(NO_AFTERFRAMES_BULK, output.repeatingCommand);
		}
		else
		{
			throw std::exception("Frame bulk length was negative");
		}
	}

	void ParsedScript::AddSaveState()
	{
		saveStates.push_back(GetSaveStateInfo());
	}

	void ParsedScript::AddSaveLoad()
	{
		std::string sName;
		if (demoCount == 1)
		{
			sName = demoName;
		}
		else
		{
			sName = demoName + "_" + std::to_string(demoCount);
		}

		if (sName == saveName)
		{
			std::string error =
			    "Save name " + saveName + " is the same as the initial save, cannot create saveload bulk";
			throw std::exception(error.c_str());
		}

		++demoCount;
		AddAfterFramesEntry(afterFramesTick,
		                    "save " + sName + "; load " + sName + +";  spt_afterframes_await_load");
		// Only manually record if autorecording isn't available
		if (!spt_demostuff.Demo_IsAutoRecordingAvailable() && !demoName.empty())
			AddAfterFramesEntry(afterFramesTick + 1,
			                    "record " + demoName + "-" + std::to_string(demoCount));
		++afterFramesTick;
	}

	void ParsedScript::Reset()
	{
		demoName.clear();
		scriptName.clear();
		demoCount = 1;
		afterFramesTick = 0;
		afterFramesEntries.clear();
		saveStateIndexes.clear();
		initCommand =
		    "sv_cheats 1; spt_pause 0; spt_afterframes_await_load; spt_afterframes_reset_on_server_activate 0; spt_resetpitchyaw;"
		    "tas_aim_reset";
		duringLoad.clear();
		saveName.clear();
		saveStates.clear();
	}

	void ParsedScript::Init(std::string name)
	{
		this->scriptName = std::move(name);
		int saveStateIndex = -1;

		for (size_t i = 0; i < saveStates.size(); ++i)
		{
			if (saveStates[i].exists && tas_script_savestates.GetBool())
				saveStateIndex = i;
			else
			{
				AddAfterFramesEntry(saveStates[i].tick, "save " + saveStates[i].key);
			}
		}

		if (saveStateIndex != -1)
		{
			int tick = saveStates[saveStateIndex].tick;
			saveName = saveStates[saveStateIndex].key;

			for (size_t i = 0; i < afterFramesEntries.size(); ++i)
			{
				if (afterFramesEntries[i].framesLeft >= tick)
				{
					// If the entry is on the same tick as the save, add a new during load entry
					if (afterFramesEntries[i].framesLeft == tick)
					{
						afterframes_entry_t copy = afterFramesEntries[i];
						copy.framesLeft = NO_AFTERFRAMES_BULK;
						afterFramesEntries.push_back(copy);
					}

					afterFramesEntries[i].framesLeft -= tick;
				}
				else
				{
					afterFramesEntries.erase(afterFramesEntries.begin() + i);
					--i;
				}
			}
		}

		if (!saveName.empty())
			AddInitCommand("load " + saveName);
	}

	void ParsedScript::SetDemoName(std::string name)
	{
		rtrim(name);
		demoName = std::move(name);
	}

	void ParsedScript::AddAfterFramesEntry(long long int tick, std::string command)
	{
		afterFramesEntries.push_back(afterframes_entry_t(tick, std::move(command)));
	}

	void ParsedScript::SetSave(std::string save)
	{
		rtrim(save);
		saveName = std::move(save);
	}

	Savestate ParsedScript::GetSaveStateInfo()
	{
		std::string data = saveName;

		for (auto& entry : afterFramesEntries)
		{
			data += std::to_string(entry.framesLeft);
			data += entry.command;
		}

		MD5 hash(data);
		std::string digest = hash.hexdigest();

		return Savestate(afterFramesTick,
		                 afterFramesEntries.size(),
		                 "ss-" + digest + "-" + std::to_string(afterFramesTick));
	}

	Savestate::Savestate(int tick, int index, std::string key) : tick(tick), index(index), key(std::move(key))
	{
		TestExists();
	}

	void Savestate::TestExists()
	{
		exists = FileExists(GetGameDir() + "\\SAVE\\" + key + ".sav");
	}
} // namespace scripts
