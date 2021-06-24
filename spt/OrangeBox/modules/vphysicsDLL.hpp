#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\..\utils\patterncontainer.hpp"

class VPhysicsDLL : public IHookableNameFilter
{
public:
	VPhysicsDLL() : IHookableNameFilter({L"vphysics.dll"}){};
	virtual void Hook(const std::wstring& moduleName,
	                  void* moduleHandle,
	                  void* moduleBase,
	                  size_t moduleLength,
	                  bool needToIntercept);

	virtual void Unhook();
	virtual void Clear();

	bool* isgFlagPtr;

protected:
	PatternContainer patternContainer;
};
