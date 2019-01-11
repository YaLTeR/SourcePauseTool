#pragma once
#include "SPTLib\memutils.hpp"
#include <map>
#include <string>

class PatternContainer
{
public:
	PatternContainer() {};
	void Init(const std::wstring& moduleName, int moduleStart, int moduleLength);
	void AddEntry(PVOID functionToHook, PVOID * origPtr, const MemUtils::ptnvec& pattern, const char* name);
	void AddHook(PVOID functionToHook, PVOID * origPtr);
	void Hook();
	void Unhook();
	const char* FindPatternName(PVOID* origPtr);
	MemUtils::ptnvec_size FindPatternIndex(PVOID* origPtr);
private:
	void PrintFound(const char* name, PVOID* origPtr);
	std::vector<std::pair<PVOID *, PVOID>> entries;
	std::map<int, std::string> patternMap;
	std::map<int, MemUtils::ptnvec_size> indexMap;
	int moduleStart;
	int moduleLength;
	std::wstring moduleName;
};