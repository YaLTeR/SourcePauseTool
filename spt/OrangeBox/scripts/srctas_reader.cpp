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
	SourceTASReader g_TASReader;
	const std::string SCRIPT_EXT = ".stas";

	SourceTASReader::SourceTASReader()
	{
		InitPropertyHandlers();
	}

	void SourceTASReader::ExecuteScript(std::string script)
	{
		hooked = false;
		searchType = None;
		freezeVariables = false;
		fileName = script;
		CommonExecuteScript();
	}

	void SourceTASReader::StartSearch(std::string script)
	{
		if(!FindSearchType())
			return;

		freezeVariables = false;
		fileName = script;
		CommonExecuteScript();
		freezeVariables = true;
	}

	void SourceTASReader::SearchResult(std::string result)
	{
		try
		{
			if (result == "low")
				variables.SetResult(Low);
			else if (result == "high")
				variables.SetResult(High);
			else if (result == "success")
			{
				Msg("Iteration was successful!\n");
				variables.SetResult(Success);
			}
			else if (result == "fail")
				variables.SetResult(Fail);
			else
				throw std::exception("invalid result type");

			CommonExecuteScript();
		}
		catch (const std::exception & ex)
		{
			Msg("Error setting result: %s\n", ex.what());
		}
	}

	bool SourceTASReader::FindSearchType()
	{
		std::string searchT(tas_script_search.GetString());
		if (searchT == "low")
			searchType = Lowest;
		else if (searchT == "high")
			searchType = Highest;
		else if (searchT == "random")
			searchType = Random;
		else
		{
			Msg("Search type was invalid!\n");
			return false;
		}

		return true;
	}

	void SourceTASReader::CommonExecuteScript()
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
		++currentTick;

		if (conditions.size() == 0)
			return;

		bool allTrue = true;

		for (auto& pointer : conditions)
		{
			allTrue = allTrue && pointer->IsTrue(currentTick, afterFramesTick);
			if (pointer->ShouldTerminate(currentTick, afterFramesTick))
			{
				SearchResult("fail");
			}
		}

		if (allTrue)
			SearchResult("success");
	}

	void SourceTASReader::Execute()
	{
		const std::string DEFAULT_START = ";host_framerate 0.015; sv_cheats 1; fps_max 66.66666; y_spt_pause 0;_y_spt_afterframes_await_load; _y_spt_afterframes_reset_on_server_activate 0";
		startCommand += DEFAULT_START;
		EngineConCmd(startCommand.c_str());

		currentTick = 0;
		clientDLL.ResetAfterframesQueue();

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
		inFrames = false;
		startCommand.clear();
		scriptStream.clear();
		lineStream.clear();
		line.clear();
		currentLine = 0;
		afterFramesTick = 0;
		afterFramesEntries.clear();
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
		inFrames = true;
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
		AddAfterframesEntry(afterFramesTick, output.command);
		afterFramesTick += output.ticks;
	}

	void SourceTASReader::InitPropertyHandlers()
	{
		propertyHandlers["save"] = &SourceTASReader::HandleSave;
		propertyHandlers["demo"] = &SourceTASReader::HandleDemo;

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
		startCommand = "load " + value;
	}

	void SourceTASReader::HandleDemo(std::string & value)
	{
		AddAfterframesEntry(0, "record " + value);
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
