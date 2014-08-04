#include "stdafx.h"
#pragma once

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
