#pragma once

#include <string>
#include <map>
#include <random>
#include "range_variable.hpp"

namespace scripts
{
	enum class VariableType { Error, Var, IntRange, FloatRange, AngleRange };

	// A pseudo-union for storing variable data. Depending on the variable type, one is used and others are not.
	// There's some issue with unions with the destructor not being well defined I couldn't figure out how to solve so I just made this a class
	class VarData
	{
	public:
		std::string value; // for var type
		RangeVariable<int> intRange; // for int type
		RangeVariable<float> floatRange; // for float/angle types
	};

	// A variable holding a singular value or a range of values
	class ScriptVariable
	{
	public:
		ScriptVariable() { variableType = VariableType::Error; }
		ScriptVariable(std::string type, std::string value);
		std::string GetPrint(); // For printing the variable state
		std::string GetValue(); // Returns the actual value of the variable in a string
		bool Iteration(SearchResult search, SearchType type);
	private:
		VariableType variableType;
		VarData data;
	};
	
	// Contains all variables for the SourceTASReader
	class VariableContainer
	{
	public:	
		std::string lastSuccessPrint;
		std::string iterationPrint;
		SearchType searchType;
		std::map<std::string, ScriptVariable> variableMap;

		void PrintBest();
		void Clear();
		void Iteration(SearchType type);
		void AddNewVariable(std::string type, std::string name, std::string value);
		void SetResult(SearchResult result);
		void PrintState();
	private:
		SearchResult lastResult;
	};
}
