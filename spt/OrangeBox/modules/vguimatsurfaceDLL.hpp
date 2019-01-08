#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>
#include "SPTLib\detoursutils.hpp"

using std::uintptr_t;
using std::size_t;

typedef int(__cdecl *_StartDrawing) ();
typedef int(__cdecl *_FinishDrawing) ();

class VGui_MatSurfaceDLL : public IHookableNameFilter
{
public:
	VGui_MatSurfaceDLL() : IHookableNameFilter({ L"vguimatsurface.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	void DrawCrosshair();
	static void __cdecl HOOKED_StartDrawing();
	static void __cdecl HOOKED_FinishDrawing();

	_StartDrawing ORIG_StartDrawing;
	_FinishDrawing ORIG_FinishDrawing;
protected:
	DetoursUtils::PatternContainer patternContainer;
};