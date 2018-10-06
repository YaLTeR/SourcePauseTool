#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "variable_container.hpp"
#include "..\modules\ClientDLL.hpp"
#include <map>
#include "condition.hpp"
#include "range_variable.hpp"

namespace scripts
{
	class SourceTASReader
	{
	public:
		SourceTASReader();
		void ExecuteScript(std::string script);
		void StartSearch(std::string script);
		void SearchResult(scripts::SearchResult result);
	private:
		bool iterationFinished;
		bool hooked;
		bool freezeVariables;
		std::string fileName;
		std::ifstream scriptStream;
		std::istringstream lineStream;
		std::string line;
		int currentLine;
		long long int currentTick;
		long long int afterFramesTick;
		SearchType searchType;
		float tickTime;
		float playbackSpeed;

		VariableContainer variables;
		std::string startCommand;
		std::vector<afterframes_entry_t> afterFramesEntries;
		std::map<std::string, void(SourceTASReader::*)(std::string&)> propertyHandlers;
		std::vector<std::unique_ptr<Condition>> conditions;

		void CommonExecuteScript(bool search);
		void Reset();
		void ResetIterationState();
		void Execute();
		void AddAfterframesEntry(long long int tick, std::string command);

		void HookAfterFrames();
		void OnAfterFrames();
		
		bool ParseLine();
		void SetNewLine();
		void ReplaceVariables();

		void InitPropertyHandlers();
		void ParseProps();
		void ParseProp();
		void HandleSave(std::string& value);
		void HandleDemo(std::string& value);
		void HandleSearch(std::string& value);
		void HandlePlaybackSpeed(std::string& value);
		void HandleTickTime(std::string& value);
		void HandleTickRange(std::string& value);
		void HandleTicksFromEndRange(std::string& value);
		
		void HandleXPos(std::string& value) { HandlePosVel(value, Axis::AxisX, true); }
		void HandleYPos(std::string& value) { HandlePosVel(value, Axis::AxisY, true); }
		void HandleZPos(std::string& value) { HandlePosVel(value, Axis::AxisZ, true); }

		void HandleXVel(std::string& value) { HandlePosVel(value, Axis::AxisX, false); }
		void HandleYVel(std::string& value) { HandlePosVel(value, Axis::AxisY, false); }
		void HandleZVel(std::string& value) { HandlePosVel(value, Axis::AxisZ, false); }
		void HandleAbsVel(std::string& value) { HandlePosVel(value, Axis::Abs, false); }
		void Handle2DVel(std::string& value) { HandlePosVel(value, Axis::TwoD, false); }

		void HandlePosVel(std::string& value, Axis axis, bool isPos);

		void ParseVariables();
		void ParseVariable();

		void ParseFrames();
		void ParseFrameBulk();

		bool isLineEmpty();
		bool IsFramesLine();
		bool IsVarsLine();
	};

	std::string GetVarIdentifier(std::string name);
	extern SourceTASReader g_TASReader;
}

