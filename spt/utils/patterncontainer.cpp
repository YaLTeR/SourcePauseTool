#include "stdafx.h"
#include "patterncontainer.hpp"
#include "SPTLib\Windows\detoursutils.hpp"
#include "SPTLib\sptlib.hpp"
#include "string_parsing.hpp"

int PatternContainer::FindPatternIndex(PVOID * origPtr)
{
	return patterns[(int)origPtr];
}

const std::string& PatternContainer::FindPatternName(PVOID * origPtr)
{
	return patternNames[(int)origPtr];
}

void PatternContainer::Init(const std::wstring & moduleName)
{
	this->moduleName = moduleName;
}

void PatternContainer::AddHook(PVOID functionToHook, PVOID* origPtr)
{
	entries.push_back(std::make_pair(origPtr, functionToHook));
	functions.push_back(origPtr);
}

void PatternContainer::AddIndex(PVOID * origPtr, int index, std::string name)
{
	patterns[(int)origPtr] = index;
	patternNames[(int)origPtr] = name;
}

void PatternContainer::Hook()
{
	for (auto& entry : entries)
		MemUtils::MarkAsExecutable(*(entry.first));

	DetoursUtils::AttachDetours(moduleName, entries.size(), &entries[0]);
}

void PatternContainer::Unhook()
{
	DetoursUtils::DetachDetours(moduleName, entries.size(), &functions[0]);
}

