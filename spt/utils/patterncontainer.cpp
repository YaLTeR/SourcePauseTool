#include "stdafx.h"
#include "patterncontainer.hpp"
#include "SPTLib\Windows\detoursutils.hpp"
#include "SPTLib\sptlib.hpp"
#include "string_parsing.hpp"

int PatternContainer::FindPatternIndex(PVOID* origPtr)
{
	return patterns[(int)origPtr];
}

const std::string& PatternContainer::FindPatternName(PVOID* origPtr)
{
	return patternNames[(int)origPtr];
}

void PatternContainer::Init(const std::wstring& moduleName)
{
	this->moduleName = moduleName;
}

void PatternContainer::AddHook(PVOID functionToHook, PVOID* origPtr)
{
	entries.push_back(std::make_pair(origPtr, functionToHook));
	functions.push_back(origPtr);
}

void PatternContainer::AddVFTableHook(VFTableHook hook)
{
	vftable_hooks.push_back(hook);
	*hook.origPtr = nullptr;
}

void PatternContainer::AddIndex(PVOID* origPtr, int index, std::string name)
{
	patterns[(int)origPtr] = index;
	patternNames[(int)origPtr] = name;
}

void PatternContainer::Hook()
{
	for (auto& entry : entries)
		MemUtils::MarkAsExecutable(*(entry.first));

	for (auto& vft_hook : vftable_hooks)
		*vft_hook.origPtr = MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, vft_hook.functionToHook);

	if (!entries.empty())
		DetoursUtils::AttachDetours(moduleName, entries.size(), &entries[0]);
}

void PatternContainer::Unhook()
{
	if (!entries.empty())
		DetoursUtils::DetachDetours(moduleName, entries.size(), &functions[0]);

	for (auto& vft_hook : vftable_hooks)
		MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, *vft_hook.origPtr);
}

VFTableHook::VFTableHook(void** vftable, int index, PVOID functionToHook, PVOID* origPtr)
{
	this->vftable = vftable;
	this->index = index;
	this->functionToHook = functionToHook;
	this->origPtr = origPtr;
}
