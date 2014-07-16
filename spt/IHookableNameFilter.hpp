#ifndef __IHOOKABLENAMEFILTER_H__
#define __IHOOKABLENAMEFILTER_H__

#ifdef _WIN32
#pragma once
#endif // _WIN32

#include <set>
#include <string>
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

#endif // __IHOOKABLENAMEFILTER_H__