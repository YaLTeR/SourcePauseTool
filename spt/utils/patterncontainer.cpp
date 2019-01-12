#include "stdafx.h"
#include "patterncontainer.hpp"
#include "SPTLib\Windows\detoursutils.hpp"
#include "SPTLib\sptlib.hpp"
#include "string_parsing.hpp"

void PatternContainer::Init(const std::wstring & moduleName, int moduleStart, int moduleLength)
{
	this->moduleName = moduleName;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
}

void PatternContainer::AddHook(PVOID functionToHook, PVOID * origPtr)
{
	entries.push_back(std::make_pair(origPtr, functionToHook));
}

void PatternContainer::Hook()
{
	DetoursUtils::AttachDetours(moduleName, entries.size(), &entries[0]);
}

void PatternContainer::Unhook()
{
	DetoursUtils::DetachDetours(moduleName, entries.size(), NULL); // FIXME
}

const char* PatternContainer::FindPatternName(PVOID* origPtr)
{
	if (patternMap.find((int)origPtr) != patternMap.end())
		return patternMap[(int)origPtr].c_str();
	else
		return "error";
}

void PatternContainer::PrintFound(const char* name, PVOID* origPtr)
{
	const char* pattern = FindPatternName(origPtr);

	if (patternMap.find((int)origPtr) != patternMap.end())
		EngineDevMsg("[%s] Found %s at %p (using the build %s pattern).\n", moduleName.c_str(), name, origPtr, pattern);
	else
		EngineDevMsg("[%s] Could not find the %s pattern.\n", moduleName.c_str(), name);
}
