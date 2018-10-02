#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "variable_container.hpp"
#include "..\modules\ClientDLL.hpp"
#include <map>
#include "condition.hpp"

namespace scripts
{
	class SourceTASReader
	{
	public:
		SourceTASReader();
		void ExecuteScript(std::string script);
		void StartSearch(std::string script);
		void SearchResult(std::string result);
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
		std::string failResult;
		SearchType searchType;

		VariableContainer variables;
		std::string startCommand;
		std::vector<afterframes_entry_t> afterFramesEntries;
		std::map<std::string, void(SourceTASReader::*)(std::string&)> propertyHandlers;
		std::vector<std::unique_ptr<Condition>> conditions;

		bool FindSearchType();
		void CommonExecuteScript();
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
		void HandleFail(std::string& value);
		void HandleTickRange(std::string& value);
		void HandleTicksFromEndRange(std::string& value);
		
		void HandleXPos(std::string& value) { HandlePosVel(value, AxisX, true); }
		void HandleYPos(std::string& value) { HandlePosVel(value, AxisY, true); }
		void HandleZPos(std::string& value) { HandlePosVel(value, AxisZ, true); }

		void HandleXVel(std::string& value) { HandlePosVel(value, AxisX, false); }
		void HandleYVel(std::string& value) { HandlePosVel(value, AxisY, false); }
		void HandleZVel(std::string& value) { HandlePosVel(value, AxisZ, false); }
		void HandleAbsVel(std::string& value) { HandlePosVel(value, Abs, false); }
		void Handle2DVel(std::string& value) { HandlePosVel(value, TwoD, false); }

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

