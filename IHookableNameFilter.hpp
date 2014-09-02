#include "sptlib-stdafx.hpp"
#pragma once

#include <set>
#include "IHookableModule.hpp"

class IHookableNameFilter : public IHookableModule
{
public:
	IHookableNameFilter(const std::set<std::wstring>& moduleNames) : moduleNames(moduleNames) {};
	virtual bool CanHook(const std::wstring& moduleFullName);
	virtual void Clear();
	virtual void TryHookAll();

protected:
	std::set<std::wstring> moduleNames;
};
