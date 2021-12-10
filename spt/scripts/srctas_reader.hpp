#pragma once
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "condition.hpp"
#include "parsed_script.hpp"
#include "range_variable.hpp"
#include "variable_container.hpp"

namespace scripts
{
	extern const std::string SCRIPT_EXT;

	class SourceTASReader
	{
	public:
		SourceTASReader();
		void ExecuteScript(const std::string& script);
		void ExecuteScriptWithResume(const std::string& script, int resumeTicks);
		void StartSearch(const std::string& script);
		void SearchResult(scripts::SearchResult result);
		void OnAfterFrames();
		int GetCurrentTick();
		int GetCurrentScriptLength();
		bool IsExecutingScript();

	private:
		bool iterationFinished;
		bool freezeVariables;
		std::string fileName;
		std::ifstream scriptStream;
		std::istringstream lineStream;
		std::string line;
		int currentLine;
		long long int currentTick;
		SearchType searchType;
		float tickTime;
		float playbackSpeed;
		int demoDelay;

		VariableContainer variables;
		ParsedScript currentScript;
		std::map<std::string, void (SourceTASReader::*)(const std::string&)> propertyHandlers;
		std::vector<std::unique_ptr<Condition>> conditions;

		void CommonExecuteScript(bool search);
		void Reset();
		void ResetIterationState();
		void Execute();
		void SetFpsAndPlayspeed();

		bool ParseLine();
		void SetNewLine();
		void ReplaceVariables();
		void ResetConvars();

		void InitPropertyHandlers();
		void ParseProps();
		void ParseProp();
		void HandleSettings(const std::string& value);
		void HandleSave(const std::string& value);
		void HandleDemo(const std::string& value);
		void HandleDemoDelay(const std::string& value);
		void HandleSearch(const std::string& value);
		void HandlePlaybackSpeed(const std::string& value);
		void HandleTickRange(const std::string& value);
		void HandleTicksFromEndRange(const std::string& value);
		void HandleJBCondition(const std::string& value);
		void HandleAliveCondition(const std::string& value);
		void HandleCLCondition(const std::string& value);
		void HandleVelPitch(const std::string& value);
		void HandleVelYaw(const std::string& value);
#if SSDK2007
		void HandlePBubbleCondition(const std::string& value);
#endif

		void HandleXPos(const std::string& value)
		{
			HandlePosVel(value, Axis::AxisX, true);
		}
		void HandleYPos(const std::string& value)
		{
			HandlePosVel(value, Axis::AxisY, true);
		}
		void HandleZPos(const std::string& value)
		{
			HandlePosVel(value, Axis::AxisZ, true);
		}

		void HandleXVel(const std::string& value)
		{
			HandlePosVel(value, Axis::AxisX, false);
		}
		void HandleYVel(const std::string& value)
		{
			HandlePosVel(value, Axis::AxisY, false);
		}
		void HandleZVel(const std::string& value)
		{
			HandlePosVel(value, Axis::AxisZ, false);
		}
		void HandleAbsVel(const std::string& value)
		{
			HandlePosVel(value, Axis::Abs, false);
		}
		void Handle2DVel(const std::string& value)
		{
			HandlePosVel(value, Axis::TwoD, false);
		}

		void HandlePosVel(const std::string& value, Axis axis, bool isPos);

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
} // namespace scripts
