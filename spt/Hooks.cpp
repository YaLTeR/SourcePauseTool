#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <detours.h>

#include "../utf8conv/utf8conv.hpp"

#include "hooks.hpp"
#include "detoursutils.hpp"
#include "memutils.hpp"
#include "patterns.hpp"
#include "spt.hpp"
#include "OrangeBox\modules\EngineDLL.hpp"

HMODULE WINAPI HOOKED_LoadLibraryA(LPCSTR lpFileName)
{
	return Hooks::getInstance().HOOKED_LoadLibraryA_Func(lpFileName);
}

HMODULE WINAPI HOOKED_LoadLibraryW(LPCWSTR lpFileName)
{
	return Hooks::getInstance().HOOKED_LoadLibraryW_Func(lpFileName);
}

HMODULE WINAPI HOOKED_LoadLibraryExA(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
	return Hooks::getInstance().HOOKED_LoadLibraryExA_Func(lpFileName, hFile, dwFlags);
}

HMODULE WINAPI HOOKED_LoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
	return Hooks::getInstance().HOOKED_LoadLibraryExW_Func(lpFileName, hFile, dwFlags);
}

BOOL WINAPI HOOKED_FreeLibrary(HMODULE hModule)
{
	return Hooks::getInstance().HOOKED_FreeLibrary_Func(hModule);
}

void Hooks::Init()
{
	Clear();

	AddToHookedModules(&engineDLL);
	AddToHookedModules(&clientDLL);
	AddToHookedModules(&serverDLL);

	EngineDevMsg("SPT: Modules contain %d entries.\n", modules.size());

	// Try hooking each module in case it is already loaded
	for (auto it = modules.cbegin(); it != modules.cend(); ++it)
	{
		(*it)->TryHookAll();
	}

	ORIG_LoadLibraryA = LoadLibraryA;
	ORIG_LoadLibraryW = LoadLibraryW;
	ORIG_LoadLibraryExA = LoadLibraryExA;
	ORIG_LoadLibraryExW = LoadLibraryExW;
	ORIG_FreeLibrary = FreeLibrary;

	AttachDetours(L"WinApi", 10,
		&ORIG_LoadLibraryA, HOOKED_LoadLibraryA,
		&ORIG_LoadLibraryW, HOOKED_LoadLibraryW,
		&ORIG_LoadLibraryExA, HOOKED_LoadLibraryExA,
		&ORIG_LoadLibraryExW, HOOKED_LoadLibraryExW,
		&ORIG_FreeLibrary, HOOKED_FreeLibrary);
}

void Hooks::Free()
{
	EngineDevMsg("SPT: Modules contain %d entries.\n", modules.size());

	// Unhook everything
	for (auto it = modules.begin(); it != modules.end(); ++it)
	{
		EngineDevMsg("SPT: Unhooking %s...\n", utf8util::UTF8FromUTF16((*it)->GetHookedModuleName()).c_str());
		(*it)->Unhook();
	}

	DetachDetours(L"WinApi", 10,
		&ORIG_LoadLibraryA, HOOKED_LoadLibraryA,
		&ORIG_LoadLibraryW, HOOKED_LoadLibraryW,
		&ORIG_LoadLibraryExA, HOOKED_LoadLibraryExA,
		&ORIG_LoadLibraryExW, HOOKED_LoadLibraryExW,
		&ORIG_FreeLibrary, HOOKED_FreeLibrary);

	Clear();
}

void Hooks::Clear()
{
	ORIG_LoadLibraryA = nullptr;
	ORIG_LoadLibraryW = nullptr;
	ORIG_LoadLibraryExA = nullptr;
	ORIG_LoadLibraryExW = nullptr;
	ORIG_FreeLibrary = nullptr;
}

void Hooks::HookModule(std::wstring moduleName)
{
	HMODULE module = NULL;
	uintptr_t start = 0;
	size_t size = 0;

	for (auto it = modules.cbegin(); it != modules.cend(); ++it)
	{
		if ((*it)->CanHook(moduleName))
		{
			if ((module != NULL) || (MemUtils::GetModuleInfo(moduleName, &module, &start, &size)))
			{
				EngineDevMsg("SPT: Hooking %s (start: %p; size: %x)...\n", utf8util::UTF8FromUTF16(moduleName).c_str(), start, size);
				(*it)->Hook(moduleName, module, start, size);
			}
			else
			{
				EngineWarning("SPT: Unable to obtain the %s module info!\n", utf8util::UTF8FromUTF16(moduleName).c_str());
				return;
			}
		}
	}

	if (module == NULL)
	{
		EngineDevMsg("SPT: Tried to hook an unlisted module: %s\n", utf8util::UTF8FromUTF16(moduleName).c_str());
	}
}

void Hooks::UnhookModule(std::wstring moduleName)
{
	bool unhookedSomething = false;
	for (auto it = modules.cbegin(); it != modules.cend(); ++it)
	{
		if ((*it)->GetHookedModuleName().compare(moduleName) == 0)
		{
			EngineDevMsg("SPT: Unhooking %s...\n", utf8util::UTF8FromUTF16(moduleName).c_str());
			(*it)->Unhook();
			unhookedSomething = true;
		}
	}
	
	if (!unhookedSomething)
	{
		EngineDevMsg("SPT: Tried to unhook an unlisted module: %s\n", utf8util::UTF8FromUTF16(moduleName).c_str());
	}
}

void Hooks::AddToHookedModules(IHookableModule* module)
{
	if (!module)
	{
		EngineWarning("SPT: Tried to add a nullptr module!\n");
		return;
	}

	modules.push_back(module);
}

HMODULE WINAPI Hooks::HOOKED_LoadLibraryA_Func(LPCSTR lpFileName)
{
	HMODULE rv = ORIG_LoadLibraryA(lpFileName);

	EngineDevMsg("SPT: Engine call: LoadLibraryA( \"%s\" ) => %p\n", lpFileName, rv);

	if (rv != NULL)
	{
		HookModule( utf8util::UTF16FromUTF8(lpFileName) );
	}

	return rv;
}

HMODULE WINAPI Hooks::HOOKED_LoadLibraryW_Func(LPCWSTR lpFileName)
{
	HMODULE rv = ORIG_LoadLibraryW(lpFileName);

	EngineDevMsg("SPT: Engine call: LoadLibraryW( \"%s\" ) => %p\n", utf8util::UTF8FromUTF16(lpFileName).c_str(), rv);

	if (rv != NULL)
	{
		HookModule( std::wstring(lpFileName) );
	}

	return rv;
}

HMODULE WINAPI Hooks::HOOKED_LoadLibraryExA_Func(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE rv = ORIG_LoadLibraryExA(lpFileName, hFile, dwFlags);

	EngineDevMsg("SPT: Engine call: LoadLibraryExA( \"%s\" ) => %p\n", lpFileName, rv);

	if (rv != NULL)
	{
		HookModule( utf8util::UTF16FromUTF8(lpFileName) );
	}

	return rv;
}

HMODULE WINAPI Hooks::HOOKED_LoadLibraryExW_Func(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE rv = ORIG_LoadLibraryExW(lpFileName, hFile, dwFlags);

	EngineDevMsg("SPT: Engine call: LoadLibraryExW( \"%s\" ) => %p\n", utf8util::UTF8FromUTF16(lpFileName).c_str(), rv);

	if (rv != NULL)
	{
		HookModule( std::wstring(lpFileName) );
	}

	return rv;
}

BOOL WINAPI Hooks::HOOKED_FreeLibrary_Func(HMODULE hModule)
{
	for (auto it = modules.cbegin(); it != modules.cend(); ++it)
	{
		if ((*it)->GetModule() == hModule)
			(*it)->Unhook();
	}

	BOOL rv = ORIG_FreeLibrary(hModule);

	EngineDevMsg("SPT: Engine call: FreeLibrary( %p ) => %s\n", hModule, (rv ? "true" : "false"));

	return rv;
}
