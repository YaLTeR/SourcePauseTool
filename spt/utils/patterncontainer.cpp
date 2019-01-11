#include "stdafx.h"
#include "patterncontainer.hpp"
#include "SPTLib\detoursutils.hpp"
#include "SPTLib\sptlib.hpp"

void PatternContainer::Init(const std::wstring & moduleName, int moduleStart, int moduleLength)
{
	this->moduleName = moduleName;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
}

void PatternContainer::AddEntry(PVOID functionToHook, PVOID* origPtr, const MemUtils::ptnvec & pattern, const char* name)
{
	if (origPtr)
		*origPtr = nullptr;
	uintptr_t ptr = NULL;
	auto f = std::async(std::launch::async, MemUtils::FindUniqueSequence, moduleStart, moduleLength, pattern, &ptr);

	MemUtils::ptnvec_size ptnNumber = f.get();

	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		if (origPtr)
			*origPtr = (PVOID)ptr;

		patternMap[(int)origPtr] = pattern[ptnNumber].build.c_str();
		indexMap[(int)origPtr] = ptnNumber;

		if (functionToHook)
			AddHook(functionToHook, origPtr);
	}

	PrintFound(name, origPtr);
}

void PatternContainer::AddHook(PVOID functionToHook, PVOID * origPtr)
{
	entries.push_back(std::make_pair(origPtr, functionToHook));
}

void PatternContainer::Hook()
{
	DetoursUtils::AttachDetours(moduleName, entries);
}

void PatternContainer::Unhook()
{
	DetoursUtils::DetachDetours(moduleName, entries);
}

const char* PatternContainer::FindPatternName(PVOID* origPtr)
{
	if (patternMap.find((int)origPtr) != patternMap.end())
		return patternMap[(int)origPtr].c_str();
	else
		return "error";
}

MemUtils::ptnvec_size PatternContainer::FindPatternIndex(PVOID* origPtr)
{
	if (patternMap.find((int)origPtr) != patternMap.end())
		return indexMap[(int)origPtr];
	else
		return MemUtils::INVALID_SEQUENCE_INDEX;
}

void PatternContainer::PrintFound(const char* name, PVOID* origPtr)
{
	const char* pattern = FindPatternName(origPtr);

	if (patternMap.find((int)origPtr) != patternMap.end())
		EngineDevMsg("[%s] Found %s at %p (using the build %s pattern).\n", string_converter.to_bytes(moduleName).c_str(), name, origPtr, pattern);
	else
		EngineDevMsg("[%s] Could not find the %s pattern.\n", string_converter.to_bytes(moduleName).c_str(), name);
}
