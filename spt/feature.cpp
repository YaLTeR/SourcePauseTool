#include "stdafx.h"
#include <future>
#include "feature.hpp"
#include "interfaces.hpp"
#include "cvars.hpp"
#include "SPTLib\sptlib.hpp"
#include "dbg.h"
#include "SPTLib\Windows\detoursutils.hpp"
#include "SPTLib\Hooks.hpp"

static std::unordered_map<std::string, ModuleHookData> moduleHookData;
static std::unordered_map<uintptr_t, int> patternIndices;

static std::vector<Feature*>& GetFeatures()
{
	static std::vector<Feature*> features;
	return features;
}

void Feature::LoadFeatures()
{
	Hooks::InitInterception(true);

	for (auto feature : GetFeatures())
	{
		if (!feature->moduleLoaded && feature->ShouldLoadFeature())
		{
			feature->startedLoading = true;
			feature->InitHooks();
		}
	}

	InitModules();

	for (auto feature : GetFeatures())
	{
		if (!feature->moduleLoaded && feature->startedLoading)
		{
			feature->PreHook();
		}
	}

	Hook();

	for (auto feature : GetFeatures())
	{
		if (!feature->moduleLoaded && feature->startedLoading)
		{
			feature->LoadFeature();
			feature->moduleLoaded = true;
		}
	}
}

void Feature::UnloadFeatures()
{
	for (auto feature : GetFeatures())
	{
		if (feature->moduleLoaded)
		{
			feature->UnloadFeature();
			feature->moduleLoaded = false;
		}
	}

	Unhook();
}

void Feature::AddVFTableHook(VFTableHook hook, std::string moduleEnum)
{
	if (moduleHookData.find(moduleEnum) == moduleHookData.end())
	{
		moduleHookData[moduleEnum] = ModuleHookData();
	}

	auto& mhd = moduleHookData[moduleEnum];
	mhd.vftableHooks.push_back(hook);
}

Feature::Feature()
{
	moduleLoaded = false;
	startedLoading = false;
	GetFeatures().push_back(this);
}

void Feature::InitModules()
{
	for (auto& pair : moduleHookData)
	{
		pair.second.InitModule(Convert(pair.first + ".dll"));
	}
}

void Feature::Hook()
{
	for (auto& pair : moduleHookData)
	{
		pair.second.HookModule(Convert(pair.first + ".dll"));
	}
}

void Feature::Unhook()
{
	for (auto& pair : moduleHookData)
	{
		pair.second.UnhookModule(Convert(pair.first + ".dll"));
	}
}

void Feature::AddOffsetHook(std::string moduleEnum,
                            int offset,
                            const char* patternName,
                            void** origPtr,
                            void* functionHook)
{
	if (moduleHookData.find(moduleEnum) == moduleHookData.end())
	{
		moduleHookData[moduleEnum] = ModuleHookData();
	}

	auto& mhd = moduleHookData[moduleEnum];
	mhd.offsetHooks.push_back(OffsetHook{offset, patternName, origPtr, functionHook});
}

int Feature::GetPatternIndex(void** origPtr)
{
	uintptr_t ptr = reinterpret_cast<uintptr_t>(origPtr);
	if (patternIndices.find(ptr) != patternIndices.end())
	{
		return patternIndices[ptr];
	}
	else
	{
		return -1;
	}
}

void Feature::AddRawHook(std::string moduleName, void** origPtr, void* functionHook)
{
	if (moduleHookData.find(moduleName) == moduleHookData.end())
	{
		moduleHookData[moduleName] = ModuleHookData();
	}

	auto& hookData = moduleHookData[moduleName];
	hookData.funcPairs.emplace_back(origPtr, functionHook);
	hookData.hookedFunctions.emplace_back(origPtr);
}

void Feature::AddPatternHook(PatternHook hook, std::string moduleName)
{
	if (moduleHookData.find(moduleName) == moduleHookData.end())
	{
		moduleHookData[moduleName] = ModuleHookData();
	}

	auto& mhd = moduleHookData[moduleName];
	mhd.patternHooks.push_back(hook);
}

void ModuleHookData::UnhookModule(const std::wstring& moduleName)
{
	if (!hookedFunctions.empty())
		DetoursUtils::DetachDetours(moduleName, hookedFunctions.size(), &hookedFunctions[0]);

	for (auto& vft_hook : existingVTableHooks)
		MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, *vft_hook.origPtr);
}

void ModuleHookData::InitModule(const std::wstring& moduleName)
{
	void* handle;
	void* moduleStart;
	size_t moduleSize;

	if (MemUtils::GetModuleInfo(moduleName, &handle, &moduleStart, &moduleSize))
	{
		DevMsg("Hooking %s (start: %p; size: %x)...\n", Convert(moduleName).c_str(), moduleStart, moduleSize);
	}
	else
	{
		DevMsg("Couldn't hook %s, not loaded\n", Convert(moduleName).c_str());
		return;
	}

	std::vector<std::future<patterns::PatternWrapper*>> hooks;
	hooks.reserve(patternHooks.size());

	for (auto& pattern : patternHooks)
	{
		hooks.emplace_back(MemUtils::find_unique_sequence_async(*pattern.origPtr,
		                                                        moduleStart,
		                                                        moduleSize,
		                                                        pattern.patternArr,
		                                                        pattern.patternArr + pattern.size));
	}

	funcPairs.reserve(funcPairs.size() + patternHooks.size());
	hookedFunctions.reserve(hookedFunctions.size() + patternHooks.size());

	for (std::size_t i = 0; i < hooks.size(); ++i)
	{
		auto foundPattern = hooks[i].get();
		auto modulePattern = patternHooks[i];

		if (*modulePattern.origPtr)
		{
			if (modulePattern.functionHook)
			{
				funcPairs.emplace_back(modulePattern.origPtr, modulePattern.functionHook);
				hookedFunctions.emplace_back(modulePattern.origPtr);
			}

			DevMsg("[%s] Found %s at %p (using the %s pattern).\n",
			       Convert(moduleName).c_str(),
			       modulePattern.patternName,
			       *modulePattern.origPtr,
			       foundPattern->name());
			patternIndices[reinterpret_cast<uintptr_t>(modulePattern.origPtr)] =
			    foundPattern - modulePattern.patternArr;
		}
		else
		{
			DevWarning("[%s] Could not find %s.\n", Convert(moduleName).c_str(), modulePattern.patternName);
		}
	}

	for (auto& offset : offsetHooks)
	{
		*offset.origPtr = reinterpret_cast<char*>(moduleStart) + offset.offset;

		DevMsg("[%s] Found %s at %p via a fixed offset.\n",
		       Convert(moduleName).c_str(),
		       offset.patternName,
		       *offset.origPtr);

		if (offset.functionHook)
		{
			funcPairs.emplace_back(offset.origPtr, offset.functionHook);
			hookedFunctions.emplace_back(offset.origPtr);
		}
	}
}

void ModuleHookData::HookModule(const std::wstring& moduleName)
{
	if (!vftableHooks.empty())
	{
		for (auto& vft_hook : vftableHooks)
		{
			*vft_hook.origPtr = vft_hook.vftable[vft_hook.index];
			MemUtils::HookVTable(vft_hook.vftable, vft_hook.index, vft_hook.functionToHook);
		}
	}

	if (!funcPairs.empty())
	{
		for (auto& entry : funcPairs)
			MemUtils::MarkAsExecutable(*(entry.first));

		DetoursUtils::AttachDetours(moduleName, funcPairs.size(), &funcPairs[0]);
	}

	// Clear any hooks that were added
	offsetHooks.clear();
	patternHooks.clear();
	// VTable hooks have to be stored for the unhooking code
	existingVTableHooks.insert(existingVTableHooks.end(), vftableHooks.begin(), vftableHooks.end());
	vftableHooks.clear();
}

VFTableHook::VFTableHook(void** vftable, int index, void* functionToHook, void** origPtr)
{
	this->vftable = vftable;
	this->index = index;
	this->functionToHook = functionToHook;
	this->origPtr = origPtr;
}
