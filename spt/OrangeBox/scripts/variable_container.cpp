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
		lastResult = NoSearch;
		lastSuccessPrint.clear();
		iterationPrint.clear();
	}

	void VariableContainer::Iteration(SearchType type)
	{
		int maxChanges;
		int changes = 0;
		searchType = type;

		if (type == None)
			maxChanges = 0;
		else if (type != Random)
			maxChanges = 1;
		else
			maxChanges = variableMap.size();

		for (auto& var : variableMap)
		{
			if (var.second.Iteration(lastResult, type))
				++changes;

			if (changes > maxChanges)
			{
				if(type == None)
					throw std::exception("Not in search mode, range variables are illegal.");
				else 
					throw std::exception("Binary search only accepts one range variable.");
			}
				
		}
	}

	void VariableContainer::AddNewVariable(std::string type, std::string name, std::string value)
	{
		auto newVar = ScriptVariable(type, value);
		variableMap[name] = newVar;
	}

	void VariableContainer::SetResult(SearchResult result)
	{
		if (searchType == None)
			throw std::exception("Set result while not in search mode!");

		if (result == Success)
			lastSuccessPrint = iterationPrint;

		lastResult = result;
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

	ScriptVariable::ScriptVariable(std::string type, std::string value)
	{
		if (type == "var")
		{
			variableType = Var;
			data.value = value;
		}
		else if (type == "int")
		{
			variableType = IntRange;
			data.intRange.ParseInput(value, false);
		}			
		else if (type == "float")
		{
			variableType = FloatRange;
			data.floatRange.ParseInput(value, false);
		}
		else if (type == "angle")
		{
			variableType = AngleRange;
			data.floatRange.ParseInput(value, true);
		}
		else
			throw std::exception("Unknown typename for variable!");
	}

	std::string ScriptVariable::GetPrint()
	{
		switch (variableType)
		{
		case Var:
			return data.value;
		case IntRange:
			return data.intRange.GetRangeString();
		case FloatRange: case AngleRange:
			return data.floatRange.GetRangeString();
		default:
			throw std::exception("Unexpected variable type while printing!");
		}
	}

	std::string ScriptVariable::GetValue()
	{
		switch (variableType)
		{
		case Var:
			return data.value;
		case IntRange:
			return data.intRange.GetValue();
		case FloatRange: case AngleRange:
			return data.floatRange.GetValue();
		default:
			throw std::exception("Unexpected variable type while getting value!");
		}
	}

	bool ScriptVariable::Iteration(SearchResult search, SearchType type)
	{
		switch (variableType)
		{
		case Var:
			return false;
		case IntRange: 
			data.intRange.Select(search, type);
			return true;
		case FloatRange: case AngleRange:
			data.floatRange.Select(search, type);
			return true;
		default:
			throw std::exception("Unexpected variable type while starting iteration!");
		}
	}

}