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

	#define dim(x) (sizeof(x) / sizeof((x)[0])) // calculates the size of an array at compile time, according to some spooky people on the internet this isn't type safe
	const char* RESET_VARS[] = {
		"cl_forwardspeed",
		"cl_sidespeed",
		"cl_yawspeed"
	};
	const int RESET_VARS_COUNT = dim(RESET_VARS);

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
		SetFpsAndPlayspeed();
		currentTick = 0;
		clientDLL.ResetAfterframesQueue();

		currentScript.Init(fileName);

		for (size_t i = 0; i < currentScript.afterFramesEntries.size(); ++i)
		{
			if (currentScript.afterFramesEntries[i].framesLeft == NO_AFTERFRAMES_BULK)
			{
				currentScript.AddDuringLoadCmd(currentScript.afterFramesEntries[i].command);
			}
		}

		std::string startCmd(currentScript.initCommand + ";" + currentScript.duringLoad);
		EngineConCmd(startCmd.c_str());
		DevMsg("Executing start command: %s\n", startCmd.c_str());	
		currentScript.AddAfterFramesEntry(demoDelay, "record " + demoName);

		for (size_t i = 0; i < currentScript.afterFramesEntries.size(); ++i)
		{
			if (currentScript.afterFramesEntries[i].framesLeft != NO_AFTERFRAMES_BULK)
			{
				clientDLL.AddIntoAfterframesQueue(currentScript.afterFramesEntries[i]);
			}
		}
	}

	void SourceTASReader::SetFpsAndPlayspeed()
	{
		std::ostringstream os;
		float fps = 1.0f / tickTime * playbackSpeed;
		os << "host_framerate " << tickTime << "; fps_max " << fps;
		currentScript.AddDuringLoadCmd(os.str());
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

	void SourceTASReader::ResetConvars()
	{
		auto icvar = GetCvarInterface();
		ConCommandBase * cmd = icvar->GetCommands();

		// Loops through the console variables and commands
		while (cmd != NULL)
		{
			const char* name = cmd->GetName();
			// Reset any variables that have been marked to be reset for TASes
			if (!cmd->IsCommand() && name != NULL && cmd->IsFlagSet(FCVAR_TAS_RESET))
			{			
				auto convar = icvar->FindVar(name);
				DevMsg("Trying to reset variable %s\n", name);

				// convar found
				if (convar != NULL) 
				{
					DevMsg("Resetting var %s to value %s\n", name, convar->GetDefault());
					convar->SetValue(convar->GetDefault());
				}		
				else
					throw std::exception("Unable to find listed console variable!");
			}

			// Issue minus commands to reset any keypresses
			else if (cmd->IsCommand() && cmd->GetName() != NULL && cmd->GetName()[0] == '-')
			{
				DevMsg("Running command %s\n", cmd->GetName());
				EngineConCmd(cmd->GetName());
			}

			cmd = cmd->GetNext();
		}


		// Reset any variables selected above
		for (int i = 0; i < RESET_VARS_COUNT; ++i)
		{
			auto resetCmd = icvar->FindVar(RESET_VARS[i]);
			if (cmd != NULL)
			{
				DevMsg("Resetting var %s to value %s\n", cmd->GetName(), resetCmd->GetDefault());
				resetCmd->SetValue(resetCmd->GetDefault());
			}
			else
				DevWarning("Unable to find console variable %s\n", RESET_VARS[i]);
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
		ResetConvars();
		conditions.clear();
		scriptStream.clear();
		lineStream.clear();
		line.clear();
		currentLine = 0;
		afterFramesTick = 0;
		tickTime = DEFAULT_TICK_TIME;
		searchType = SearchType::None;
		playbackSpeed = 1.0f;
		demoDelay = 0;
		demoName.clear();
		currentScript.Reset();
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

	void SourceTASReader::HandleSettings(std::string & value)
	{
		currentScript.AddDuringLoadCmd(value);
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
		else if (line.find("ss") == 0)
		{
			currentScript.AddSaveState();
		}
		else
		{
			FrameBulkInfo info(lineStream);
			auto output = HandleFrameBulk(info);

			currentScript.AddFrameBulk(output);
		}
	}

	void SourceTASReader::InitPropertyHandlers()
	{
		propertyHandlers["save"] = &SourceTASReader::HandleSave;
		propertyHandlers["demo"] = &SourceTASReader::HandleDemo;
		propertyHandlers["demodelay"] = &SourceTASReader::HandleDemoDelay;
		propertyHandlers["search"] = &SourceTASReader::HandleSearch;
		propertyHandlers["playspeed"] = &SourceTASReader::HandlePlaybackSpeed;
		propertyHandlers["ticktime"] = &SourceTASReader::HandleTickTime;
		propertyHandlers["settings"] = &SourceTASReader::HandleSettings;

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
		currentScript.SetSave(value);
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
