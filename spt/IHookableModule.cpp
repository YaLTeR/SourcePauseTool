#include "stdafx.h"

#include "Hooks.hpp"
#include "IHookableModule.hpp"

HMODULE IHookableModule::GetModule()
{
	return hModule;
}

std::wstring IHookableModule::GetHookedModuleName()
{
	return moduleName;
}

void IHookableModule::Clear()
{
	hModule = nullptr;
	moduleStart = 0;
	moduleLength = 0;
}
