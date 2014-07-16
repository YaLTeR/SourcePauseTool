#ifndef __IHOOKABLEMODULE_H__
#define __IHOOKABLEMODULE_H__

#ifdef _WIN32
#pragma once
#endif // _WIN32

#include <cstddef>
#include <cstdint>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using std::uintptr_t;
using std::size_t;

class IHookableModule
{
public:
	virtual ~IHookableModule() {}
	virtual bool CanHook(const std::wstring& moduleFullName) = 0;
	virtual HMODULE GetModule();
	virtual std::wstring GetHookedModuleName();

	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength) = 0;
	virtual void Unhook() = 0;
	virtual void Clear();

	virtual void TryHookAll() = 0;

protected:
	HMODULE hModule;
	uintptr_t moduleStart;
	size_t moduleLength;
	std::wstring moduleName;
};

#endif // __IHOOKABLEMODULE_H__