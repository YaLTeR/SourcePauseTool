#include "stdafx.h"

#include "vphysicsDLL.hpp"

#include "convar.h"

#include "..\modules.hpp"
#include "..\patterns.hpp"

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::vphysics::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[vphysics dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::vphysics::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[vphysics dll] Could not find " #future_name ".\n"); \
		} \
	}

#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[vphysics dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::vphysics::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[vphysics dll] Could not find " #future_name ".\n"); \
		} \
	}

void VPhysicsDLL::Hook(const std::wstring& moduleName,
                       void* moduleHandle,
                       void* moduleBase,
                       size_t moduleLength,
                       bool needToIntercept)
{
	Clear();
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	patternContainer.Init(moduleName);

	patternContainer.Hook();
}

void VPhysicsDLL::Unhook()
{
	patternContainer.Unhook();
}

void VPhysicsDLL::Clear() {}
