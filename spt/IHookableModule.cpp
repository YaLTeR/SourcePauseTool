#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "Hooks.hpp"
#include "IHookableModule.hpp"

IHookableModule::IHookableModule()
{
	//Hooks::getInstance().AddToHookedModules(this);
}

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