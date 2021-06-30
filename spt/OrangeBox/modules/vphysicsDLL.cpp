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

void __fastcall VPhysicsDLL::HOOKED_GetShadowVelocity(void* thisptr, int edx, Vector* velocity)
{
	TRACE_ENTER();
	Vector* s = velocity;
	vphysicsDLL.ORIG_GetShadowVelocity(thisptr, edx, velocity);
	vphysicsDLL.PlayerHavokVel.x = s->x;
	vphysicsDLL.PlayerHavokVel.y = s->y;
	vphysicsDLL.PlayerHavokVel.z = s->z;
	return;
}

void __fastcall VPhysicsDLL::HOOKED_GetPosition(void* thisptr, int edx, Vector* worldPosition, QAngle* angles)
{
	TRACE_ENTER();
	return vphysicsDLL.ORIG_GetPosition(thisptr, edx, worldPosition, angles);
}

int __fastcall VPhysicsDLL::HOOKED_GetShadowPosition(void* thisptr, int edx, Vector* worldPosition, QAngle* angles)
{
	TRACE_ENTER();
	Vector* s = worldPosition;
	int d = vphysicsDLL.ORIG_GetShadowPosition(thisptr, edx, worldPosition, angles);
	//DevMsg("%.3f %.3f %.3f\n", worldPosition->x, worldPosition->y, worldPosition->z);
	vphysicsDLL.PlayerHavokPos.x = s->x;
	vphysicsDLL.PlayerHavokPos.y = s->y;
	vphysicsDLL.PlayerHavokPos.z = s->z;
	return d;
}

void VPhysicsDLL::Hook(const std::wstring& moduleName,
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

	uint32_t ORIG_MiddleOfRecheck_ov_element = NULL;

	DEF_FUTURE(MiddleOfRecheck_ov_element);
	DEF_FUTURE(GetShadowVelocity);
	DEF_FUTURE(GetPosition);
	DEF_FUTURE(GetShadowPosition);
	GET_FUTURE(MiddleOfRecheck_ov_element);
	GET_HOOKEDFUTURE(GetShadowVelocity);
	GET_HOOKEDFUTURE(GetShadowPosition);


	if (ORIG_MiddleOfRecheck_ov_element)
		this->isgFlagPtr = *(bool**)(ORIG_MiddleOfRecheck_ov_element + 2);
	else
		Warning("y_spt_hud_isg 1 and y_spt_set_isg have no effect\n");

	patternContainer.Hook();

	auto loadTime =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	DevMsg("[vphysics dll] Done hooking in %dms\n", loadTime);
}

void VPhysicsDLL::Unhook()
{
	patternContainer.Unhook();
}

void VPhysicsDLL::Clear()
{
	IHookableNameFilter::Clear();
	this->isgFlagPtr = nullptr;
	ORIG_GetShadowVelocity = nullptr;
}
