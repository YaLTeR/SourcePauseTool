#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "spt.hpp"
#include "memutils.hpp"
#include "..\utf8conv\utf8conv.hpp"
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
	moduleNames.clear();
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
			EngineMsg("SPT: Hooking %s (start: %p; size: %x)...\n", utf8util::UTF8FromUTF16(name).c_str(), start, size);
			Hook(name, hModule, start, size);
			break;
		}
	}
}