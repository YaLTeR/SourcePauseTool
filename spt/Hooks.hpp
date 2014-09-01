#include "stdafx.h"
#pragma once

#include <vector>

#include "IHookableModule.hpp"

typedef HMODULE(WINAPI *_LoadLibraryA) (LPCSTR lpLFileName);
typedef HMODULE(WINAPI *_LoadLibraryW) (LPCWSTR lpFileName);
typedef HMODULE(WINAPI *_LoadLibraryExA) (LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI *_LoadLibraryExW) (LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);
typedef BOOL(WINAPI *_FreeLibrary) (HMODULE hModule);

class Hooks
{
public:
	static Hooks& getInstance()
	{
		static Hooks instance;
		return instance;
	}

	void Init();
	void Free();
	void Clear();

	void HookModule(std::wstring moduleName);
	void UnhookModule(std::wstring moduleName);

	void AddToHookedModules(IHookableModule* module);

	HMODULE WINAPI HOOKED_LoadLibraryA_Func(LPCSTR lpFileName);
	HMODULE WINAPI HOOKED_LoadLibraryW_Func(LPCWSTR lpFileName);
	HMODULE WINAPI HOOKED_LoadLibraryExA_Func(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags);
	HMODULE WINAPI HOOKED_LoadLibraryExW_Func(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);
	BOOL WINAPI HOOKED_FreeLibrary_Func(HMODULE hModule);

private:
	Hooks() {}
	Hooks(Hooks const&);
	void operator=(Hooks const&);

protected:
	std::vector<IHookableModule*> modules;

	_LoadLibraryA   ORIG_LoadLibraryA;
	_LoadLibraryW   ORIG_LoadLibraryW;
	_LoadLibraryExA   ORIG_LoadLibraryExA;
	_LoadLibraryExW   ORIG_LoadLibraryExW;
	_FreeLibrary    ORIG_FreeLibrary;
};
