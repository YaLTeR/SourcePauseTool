#pragma once

#include <string>
#include <map>
#include <random>
#include "range_variable.hpp"

namespace scripts
{
	enum VariableType { Error, Var, IntRange, FloatRange, AngleRange };

	class VarData
	{
	public:
		std::string value;
		RangeVariable<int> intRange;
		RangeVariable<float> floatRange;
	};

	class ScriptVariable
	{
	public:
		ScriptVariable() { variableType = Error; }
		ScriptVariable(std::string type, std::string value);
		std::string GetPrint(); // For printing the variable state
		std::string GetValue(); // Returns the actual value of the variable in a string
		bool Iteration(SearchResult search, SearchType type);
	private:
		VariableType variableType;
		VarData data;
	};

	class VariableContainer
	{
	public:
		std::string lastSuccessPrint;
		std::string prevIterationPrint;
		std::vector<std::string> successes;

		std::map<std::string, ScriptVariable> variableMap;
		void Clear();
		void Iteration();
		void AddNewVariable(std::string type, std::string name, std::string value);
		void SetResult(SearchResult result);
	private:
		SearchType currentSearch;
		SearchResult lastResult;
	};

}