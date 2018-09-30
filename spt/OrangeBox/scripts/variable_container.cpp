#include "stdafx.h"
#include "variable_container.hpp"
#include "dbg.h"

namespace scripts 
{
	void VariableContainer::Clear()
	{
		variableMap.clear();
	}

	void VariableContainer::Iteration()
	{
		int maxChanges;
		int changes = 0;

		if (currentSearch == None)
			maxChanges = 0;
		else if (currentSearch != Random)
			maxChanges = 1;
		else
			maxChanges = variableMap.size();

		for (auto& var : variableMap)
		{
			if (var.second.Iteration(lastResult, currentSearch))
				++changes;

			if (changes > maxChanges)
			{
				if(currentSearch == None)
					throw std::exception("Not in search mode, range variables are illegal.");
				else 
					throw std::exception("Binary search only accepts one range variable.");
			}
				
		}
	}

	void VariableContainer::AddNewVariable(std::string type, std::string name, std::string value)
	{
		variableMap[name] = ScriptVariable(type, value);
	}

	void VariableContainer::SetResult(SearchResult result)
	{
		lastResult = result;
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