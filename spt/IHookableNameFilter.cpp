#include "stdafx.h"

#include "spt.hpp"
#include "memutils.hpp"
#include "IHookableNameFilter.hpp"

using std::uintptr_t;
using std::size_t;

bool IHookableNameFilter::CanHook(const std::wstring& moduleFullName)
{
	return (moduleNames.find( GetFileName(moduleFullName) ) != moduleNames.end());
}

void IHookableNameFilter::Clear()
{
	IHookableModule::Clear();
}

void IHookableNameFilter::TryHookAll()
{
	for (auto name : moduleNames)
	{
		HMODULE hModule;
		uintptr_t start;
		size_t size;
		if (MemUtils::GetModuleInfo(name, &hModule, &start, &size))
		{
			EngineDevMsg("SPT: Hooking %s (start: %p; size: %x)...\n", string_converter.to_bytes(name).c_str(), start, size);
			Hook(name, hModule, start, size);
			break;
		}
	}
}
