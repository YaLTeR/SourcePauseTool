#include "stdafx.h"

#include "srctas_reader.hpp"
#include "..\modules.hpp"
#include "framebulk_handler.hpp"
#include <fstream>
#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\string_parsing.hpp"
#include "..\cvars.hpp"
#include "..\spt-serverplugin.hpp"

namespace scripts
{
	const float DEFAULT_TICK_TIME = 0.015f;
	SourceTASReader g_TASReader;
	const std::string SCRIPT_EXT = ".srctas";

	SourceTASReader::SourceTASReader()
	{
		InitPropertyHandlers();
		iterationFinished = true;
		hooked = false;
	}

	void SourceTASReader::ExecuteScript(std::string script)
	{
		freezeVariables = false;
		fileName = script;
		CommonExecuteScript(false);
	}

	void SourceTASReader::StartSearch(std::string script)
	{
		freezeVariables = false;
		fileName = script;
		CommonExecuteScript(true);
		freezeVariables = true;
	}

	void SourceTASReader::SearchResult(scripts::SearchResult result)
	{
		try
		{
			variables.SetResult(result);
			CommonExecuteScript(true);
		}
		catch (const std::exception & ex)
		{
			Msg("Error setting result: %s\n", ex.what());
		}
		catch (const SearchDoneException&)
		{
			Msg("Search done.\n");
			variables.PrintBest();
			iterationFinished = true;
		}
		catch (...)
		{
			Msg("Unexpected exception on line %i\n", currentLine);
		}
	}

	void SourceTASReader::CommonExecuteScript(bool search)
	{
		try
		{
			HookAfterFrames();
			Reset();
			std::string gameDir = GetGameDir();
			scriptStream.open(gameDir + "\\" + fileName + SCRIPT_EXT);

			if (!scriptStream.is_open())
				throw std::exception("File does not exist!");
			ParseProps();

			if (search && searchType == SearchType::None)
				throw std::exception("In search mode but search property is not set!");
			else if (!search && searchType != SearchType::None)
				throw std::exception("Not in search mode but search property is set!");

			while (!scriptStream.eof())
			{
				if (IsFramesLine())
					ParseFrames();
				else if (IsVarsLine())
					ParseVariables();
				else
					throw std::exception("Unexpected section order in file. Expected order is props - variables - frames");
			}

			Execute();
		}
		catch (const std::exception & ex)
		{
			Msg("Error in line %i: %s\n", currentLine, ex.what());
		}
		catch (const SearchDoneException&)
		{
			Msg("Search done.\n");
			variables.PrintBest();
		}
		catch (...)
		{
			Msg("Unexpected exception on line %i\n", currentLine);
		}

		scriptStream.close();
	}

	void SourceTASReader::HookAfterFrames()
	{
		if (!hooked)
		{
			clientDLL.AfterFrames += Simple::slot(this, &SourceTASReader::OnAfterFrames);
			hooked = true;
		}
	}

	void SourceTASReader::OnAfterFrames()
	{
		if (conditions.size() == 0 || iterationFinished)
			return;

		++currentTick;

		bool allTrue = true;

		for (auto& pointer : conditions)
		{
			allTrue = allTrue && pointer->IsTrue(currentTick, afterFramesTick);
			if (pointer->ShouldTerminate(currentTick, afterFramesTick))
			{
				iterationFinished = true;
				SearchResult(SearchResult::Fail);

				return;
			}
		}

		if (allTrue)
		{
			iterationFinished = true;
			SearchResult(SearchResult::Success);
		}	
	}

	void SourceTASReader::Execute()
	{
		iterationFinished = false;
		std::ostringstream os;
		float fps = 1.0f / tickTime * playbackSpeed;
		os << ";host_framerate " << tickTime << "; sv_cheats 1; fps_max " << fps << "; y_spt_pause 0;_y_spt_afterframes_await_load; _y_spt_afterframes_reset_on_server_activate 0";
		startCommand += os.str();
		EngineConCmd(startCommand.c_str());

		currentTick = 0;
		clientDLL.ResetAfterframesQueue();

		if(demoName.length() > 0)
			AddAfterframesEntry(demoDelay, "record " + demoName);

		for (size_t i = 0; i < afterFramesEntries.size(); ++i)
		{
			clientDLL.AddIntoAfterframesQueue(afterFramesEntries[i]);
		}
	}

	bool SourceTASReader::ParseLine()
	{
		if (!scriptStream.good())
		{
			return false;
		}

		std::getline(scriptStream, line);
		SetNewLine();

		return true;
	}

	void SourceTASReader::SetNewLine()
	{
		const std::string COMMENT_START = "//";
		int end = line.find(COMMENT_START); // ignore comments
		if (end != std::string::npos)
			line.erase(end);

		ReplaceVariables();
		lineStream.str(line);
		lineStream.clear();
		++currentLine;
	}

	void SourceTASReader::ReplaceVariables()
	{
		for(auto& variable : variables.variableMap)
		{
			ReplaceAll(line, GetVarIdentifier(variable.first), variable.second.GetValue());
		}
	}

	void SourceTASReader::Reset()
	{
		if (!freezeVariables)
		{
			variables.Clear();
		}
		ResetIterationState();
	}

	void SourceTASReader::ResetIterationState()
	{
		conditions.clear();
		startCommand.clear();
		scriptStream.clear();
		lineStream.clear();
		line.clear();
		currentLine = 0;
		afterFramesTick = 0;
		afterFramesEntries.clear();
		tickTime = DEFAULT_TICK_TIME;
		searchType = SearchType::None;
		playbackSpeed = 1.0f;
		demoDelay = 0;
		demoName.clear();
	}

	void SourceTASReader::AddAfterframesEntry(long long int tick, std::string command)
	{
		afterFramesEntries.push_back(afterframes_entry_t(tick, command));
	}

	void SourceTASReader::ParseProps()
	{
		while (ParseLine())
		{
			if (IsFramesLine() || IsVarsLine())
			{
				break;
			}
			ParseProp();
		}
	}

	void SourceTASReader::ParseProp()
	{
		if (isLineEmpty())
			return;

		std::string prop;
		std::string value;
		GetDoublet(lineStream, prop, value, ' ');

		if (propertyHandlers.find(prop) != propertyHandlers.end())
		{
			(this->*propertyHandlers[prop])(value);
		}
		else
			throw std::exception("Unknown property name!");
	}

	void SourceTASReader::ParseVariables()
	{
		while (ParseLine())
		{
			if (IsFramesLine())
			{
				break;
			}

			if (!freezeVariables)
				ParseVariable();
		}

		variables.Iteration(searchType);
		variables.PrintState();
	}

	void SourceTASReader::ParseVariable()
	{
		if (isLineEmpty())
			return;

		std::string type;
		std::string name;
		std::string value;
		GetTriplet(lineStream, type, name, value, ' ');
		variables.AddNewVariable(type, name, value);
	}

	void SourceTASReader::ParseFrames()
	{
		while (ParseLine())
		{
			ParseFrameBulk();
		}
	}

	void SourceTASReader::ParseFrameBulk()
	{
		if (isLineEmpty())
			return;

		FrameBulkInfo info(lineStream);
		auto output = HandleFrameBulk(info);
		AddAfterframesEntry(afterFramesTick, output.initialCommand);
		
		if (output.repeatingCommand.length() > 0)
		{
			AddAfterframesEntry(afterFramesTick, output.repeatingCommand);
			for (int i = 1; i < output.ticks; ++i)
				AddAfterframesEntry(afterFramesTick + i, output.repeatingCommand);
		}	

		afterFramesTick += output.ticks;
	}

	void SourceTASReader::InitPropertyHandlers()
	{
		propertyHandlers["save"] = &SourceTASReader::HandleSave;
		propertyHandlers["demo"] = &SourceTASReader::HandleDemo;
		propertyHandlers["demodelay"] = &SourceTASReader::HandleDemoDelay;
		propertyHandlers["search"] = &SourceTASReader::HandleSearch;
		propertyHandlers["playspeed"] = &SourceTASReader::HandlePlaybackSpeed;
		propertyHandlers["ticktime"] = &SourceTASReader::HandleTickTime;

		// Conditions for automated searching
		propertyHandlers["tick"] = &SourceTASReader::HandleTickRange;
		propertyHandlers["tickend"] = &SourceTASReader::HandleTicksFromEndRange;
		propertyHandlers["posx"] = &SourceTASReader::HandleXPos;
		propertyHandlers["posy"] = &SourceTASReader::HandleYPos;
		propertyHandlers["posz"] = &SourceTASReader::HandleZPos;
		propertyHandlers["velx"] = &SourceTASReader::HandleXVel;
		propertyHandlers["vely"] = &SourceTASReader::HandleYVel;
		propertyHandlers["velz"] = &SourceTASReader::HandleZVel;
		propertyHandlers["vel2d"] = &SourceTASReader::Handle2DVel;
		propertyHandlers["velabs"] = &SourceTASReader::HandleAbsVel;
	}

	void SourceTASReader::HandleSave(std::string& value)
	{
		startCommand += "load " + value;
	}

	void SourceTASReader::HandleDemo(std::string& value)
	{
		demoName = value;
	}

	void SourceTASReader::HandleDemoDelay(std::string& value)
	{
		demoDelay = ParseValue<int>(value);
	}

	void SourceTASReader::HandleSearch(std::string& value)
	{
		if (value == "low")
			searchType = SearchType::Lowest;
		else if (value == "high")
			searchType = SearchType::Highest;
		else if (value == "random")
			searchType = SearchType::Random;
		else
		{
			throw std::exception("Search type was invalid!");
		}
	}

	void SourceTASReader::HandlePlaybackSpeed(std::string & value)
	{
		playbackSpeed = ParseValue<float>(value);
		if (playbackSpeed <= 0.0f)
			throw std::exception("Playback speed has to be positive!");
	}

	void SourceTASReader::HandleTickTime(std::string & value)
	{
		tickTime = ParseValue<float>(value);
		if (tickTime <= 0.0f)
			throw std::exception("Tick time has to be positive!");
		else if (tickTime > 1.0f)
			throw std::exception("Ticks are not this long.");
	}

	void SourceTASReader::HandleTickRange(std::string & value)
	{
		int min, max;
		GetDoublet(value, min, max, '|');
		conditions.push_back(std::unique_ptr<Condition>(new TickRangeCondition(min, max, false)));
	}

	void SourceTASReader::HandleTicksFromEndRange(std::string & value)
	{
		int min, max;
		GetDoublet(value, min, max, '|');
		conditions.push_back(std::unique_ptr<Condition>(new TickRangeCondition(min, max, true)));
	}

	void SourceTASReader::HandlePosVel(std::string & value, Axis axis, bool isPos)
	{
		float min, max;
		GetDoublet(value, min, max, '|');
		conditions.push_back(std::unique_ptr<Condition>(new PosSpeedCondition(min, max, axis, isPos)));
	}

	bool SourceTASReader::isLineEmpty()
	{
		return line.find_first_not_of(' ') == std::string::npos;
	}

	bool SourceTASReader::IsFramesLine()
	{
		return line.find("frames") == 0;
	}

	bool SourceTASReader::IsVarsLine()
	{
		return line.find("vars") == 0;
	}

	std::string GetVarIdentifier(std::string name)
	{
		std::ostringstream os;
		os << "[" << name << "]";

		return os.str();
	}

}
