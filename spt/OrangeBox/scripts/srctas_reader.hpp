#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include "variable_container.hpp"
#include "..\modules\ClientDLL.hpp"
#include <map>

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
		bool hooked;
		bool inFrames;
		bool freezeVariables;
		std::string fileName;
		std::ifstream scriptStream;
		std::istringstream lineStream;
		std::string line;
		int currentLine;
		long long int currentTick;
		long long int afterFramesTick;
		SearchType searchType;

		VariableContainer variables;
		std::string startCommand;
		std::vector<afterframes_entry_t> afterFramesEntries;
		std::map<std::string, void(SourceTASReader::*)(std::string&)> propertyHandlers;

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

