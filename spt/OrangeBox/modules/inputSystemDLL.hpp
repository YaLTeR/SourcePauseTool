#pragma once
#include <SPTLib\IHookableNameFilter.hpp>
#include "..\..\utils\patterncontainer.hpp"
#include "inputsystem\iinputsystem.h"

typedef void(__fastcall* _CInputSystem__SleepUntilInput)(void* thisptr, int edx, int nMaxSleepTimeMS);

class InputSystemDLL : public IHookableNameFilter
{
public:
	InputSystemDLL() : IHookableNameFilter({L"inputsystem.dll"}){};
	virtual void Hook(const std::wstring& moduleName,
	                  void* moduleHandle,
	                  void* moduleBase,
	                  size_t moduleLength,
	                  bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();
	static void __fastcall HOOKED_CInputSystem__SleepUntilInput(void* thisptr, int edx, int nMaxSleepTimeMS);

private:
	_CInputSystem__SleepUntilInput ORIG_CInputSystem__SleepUntilInput;
	PatternContainer patternContainer;
};
