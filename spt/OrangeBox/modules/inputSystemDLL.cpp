#include "stdafx.h"

#include "inputsystemDLL.hpp"

#include "convar.h"

#include "..\modules.hpp"
#include "..\patterns.hpp"

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::inputsystem::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[inputsystem dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::inputsystem::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[inputsystem dll] Could not find " #future_name ".\n"); \
		} \
	}

ConVar y_spt_focus_nosleep("y_spt_focus_nosleep", "0", 0, "Improves FPS while alt-tabbed.");

void InputSystemDLL::Hook(const std::wstring& moduleName,
                          void* moduleHandle,
                          void* moduleBase,
                          size_t moduleLength,
                          bool needToIntercept)
{
	auto startTime = std::chrono::high_resolution_clock::now();

	Clear();
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	patternContainer.Init(moduleName);

	DEF_FUTURE(CInputSystem__SleepUntilInput);
	GET_HOOKEDFUTURE(CInputSystem__SleepUntilInput);

	if (!ORIG_CInputSystem__SleepUntilInput)
	{
		Warning("y_spt_focus_nosleep has no effect.\n");
	}

	patternContainer.Hook();

	auto loadTime =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	DevMsg("[inputsystem dll] Done hooking in %dms\n", loadTime);
}

void InputSystemDLL::Unhook()
{
	patternContainer.Unhook();
}

void InputSystemDLL::Clear() {}

void InputSystemDLL::HOOKED_CInputSystem__SleepUntilInput(void* thisptr, int edx, int nMaxSleepTimeMS)
{
	if (y_spt_focus_nosleep.GetBool())
		nMaxSleepTimeMS = 0;
	inputSystemDLL.ORIG_CInputSystem__SleepUntilInput(thisptr, edx, nMaxSleepTimeMS);
}
