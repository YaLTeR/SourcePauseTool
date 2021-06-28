#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\..\utils\patterncontainer.hpp"
#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

using std::size_t;
using std::uintptr_t;

typedef void(__fastcall* _GetShadowVelocity)(void* thisptr, int edx, Vector* velocity);
typedef void(__fastcall* _GetPosition)(void* thisptr, int edx, Vector* worldPosition, QAngle* angles);
typedef int(__fastcall* _GetShadowPosition)(void* thisptr, int edx, Vector* worldPosition, QAngle* angles);

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

	static void __fastcall HOOKED_GetShadowVelocity(void* thisptr, int edx, Vector* velocity);
	static void __fastcall HOOKED_GetPosition(void* thisptr, int edx, Vector* worldPosition, QAngle* angles);
	static int __fastcall HOOKED_GetShadowPosition(void* thisptr, int edx, Vector* worldPosition, QAngle* angles);

	Vector PlayerHavokVel;
	Vector PlayerHavokPos;

	bool* isgFlagPtr;

protected:
	_GetShadowVelocity ORIG_GetShadowVelocity;
	_GetPosition ORIG_GetPosition;
	_GetShadowPosition ORIG_GetShadowPosition;
	PatternContainer patternContainer;
};
