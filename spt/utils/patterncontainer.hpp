#pragma once
#include <map>
#include <string>
#include <vector>
#include "SPTLib\memutils.hpp"

#define PVOID void*

class PatternContainer
{
public:
	PatternContainer() {};
	void Init(const std::wstring& moduleName, int moduleStart, int moduleLength);
	void AddHook(PVOID functionToHook, PVOID* origPtr);
	void Hook();
	void Unhook();
	const char* FindPatternName(PVOID* origPtr);
private:
	void PrintFound(const char* name, PVOID* origPtr);
	std::vector<std::pair<PVOID *, PVOID>> entries;
	std::map<int, std::string> patternMap;
	int moduleStart;
	int moduleLength;
	std::wstring moduleName;
};