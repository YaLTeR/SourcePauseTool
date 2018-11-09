#include "stdafx.h"
#include "variable_container.hpp"
#include "dbg.h"
#include "..\cvars.hpp"

namespace scripts 
{
	void VariableContainer::PrintBest()
	{
		if (lastSuccessPrint.empty())
		{
			Msg("No results found.\n");
		}
		else
		{
			Msg("Best result was with:\n");
			Msg(lastSuccessPrint.c_str());
		}
	}

	void VariableContainer::Clear()
	{
		variableMap.clear();
		lastResult = SearchResult::NoSearch;
		lastSuccessPrint.clear();
		iterationPrint.clear();
	}

	void VariableContainer::Iteration(SearchType type)
	{
		int maxChanges;
		int changes = 0;
		searchType = type;

		if (type == SearchType::None)
			maxChanges = 0;
		else if (type != SearchType::Random)
			maxChanges = 1;
		else
			maxChanges = variableMap.size();

		for (auto& var : variableMap)
		{
			if (var.second.Iteration(lastResult, type))
				++changes;

			if (changes > maxChanges)
			{
				if(type == SearchType::None)
					throw std::exception("Not in search mode, range variables are illegal");
				else 
					throw std::exception("Binary search only accepts one range variable");
			}
				
		}
	}

	void VariableContainer::AddNewVariable(const std::string& type, const std::string& name, const std::string& value)
	{
		auto newVar = ScriptVariable(type, value);
		variableMap[name] = newVar;
	}

	void VariableContainer::SetResult(SearchResult result)
	{
		if (searchType == SearchType::None)
			throw std::exception("Set result while not in search mode");

		lastResult = result;

		if (result == SearchResult::Success)
		{
			lastSuccessPrint = iterationPrint;
			if (searchType == SearchType::Random)
				throw SearchDoneException();
		}			
		
	}

	void VariableContainer::PrintState()
	{
		if (variableMap.size() == 0)
			return;

		iterationPrint = "Variables:\n";
		for (auto& var : variableMap)
		{
			iterationPrint += "\t - " + var.first + " : " + var.second.GetPrint() + "\n";
		}

		if (tas_script_printvars.GetBool())
		{
			Msg(iterationPrint.c_str());
		}
	}

	ScriptVariable::ScriptVariable(const std::string& type, const std::string& value)
	{
		if (type == "var")
		{
			variableType = VariableType::Var;
			data.value = value;
		}
		else if (type == "int")
		{
			variableType = VariableType::IntRange;
			data.intRange.ParseInput(value, false);
		}			
		else if (type == "float")
		{
			variableType = VariableType::FloatRange;
			data.floatRange.ParseInput(value, false);
		}
		else if (type == "angle")
		{
			variableType = VariableType::AngleRange;
			data.floatRange.ParseInput(value, true);
		}
		else
			throw std::exception("Unknown typename for variable");
	}

	std::string ScriptVariable::GetPrint()
	{
		switch (variableType)
		{
		case VariableType::Var:
			return data.value;
		case VariableType::IntRange:
			return data.intRange.GetRangeString();
		case VariableType::FloatRange: case VariableType::AngleRange:
			return data.floatRange.GetRangeString();
		default:
			throw std::exception("Unexpected variable type while printing");
		}
	}

	std::string ScriptVariable::GetValue()
	{
		switch (variableType)
		{
		case VariableType::Var:
			return data.value;
		case VariableType::IntRange:
			return data.intRange.GetValue();
		case VariableType::FloatRange: case VariableType::AngleRange:
			return data.floatRange.GetValue();
		default:
			throw std::exception("Unexpected variable type while getting value");
		}
	}

	bool ScriptVariable::Iteration(SearchResult search, SearchType type)
	{
		switch (variableType)
		{
		case VariableType::Var:
			return false;
		case VariableType::IntRange:
			data.intRange.Select(search, type);
			return true;
		case VariableType::FloatRange: case VariableType::AngleRange:
			data.floatRange.Select(search, type);
			return true;
		default:
			throw std::exception("Unexpected variable type while starting iteration");
		}
	}
}
