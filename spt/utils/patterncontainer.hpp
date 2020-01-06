#pragma once
#include <map>
#include <string>
#include <vector>
#include "SPTLib\memutils.hpp"

using PVOID = void*;

struct VFTableHook
{
	VFTableHook(void** vftable, int index, PVOID functionToHook, PVOID* origPtr);

	void** vftable;
	int index;
	PVOID functionToHook;
	PVOID* origPtr;
};

class PatternContainer
{
public:
	PatternContainer(){};
	int FindPatternIndex(PVOID* origPtr);
	const std::string& FindPatternName(PVOID* origPtr);
	void Init(const std::wstring& moduleName);
	void AddHook(PVOID functionToHook, PVOID* origPtr);
	void AddVFTableHook(VFTableHook hook);
	void AddIndex(PVOID* origPtr, int index, std::string name);
	void Hook();
	void Unhook();

private:
	std::map<int, int> patterns;
	std::map<int, std::string> patternNames;
	std::vector<VFTableHook> vftable_hooks;
	std::vector<std::pair<PVOID*, PVOID>> entries;
	std::vector<PVOID*> functions;
	std::wstring moduleName;
};