#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\..\utils\patterncontainer.hpp"
#include "tier0\basetypes.h"
#include "vguimatsurface\imatsystemsurface.h"
#include "vgui\ischeme.h"
#include "convar.h"
#ifdef OE
#include "vector.h"
#endif

using std::uintptr_t;
using std::size_t;

typedef int(__cdecl *_StartDrawing) ();
typedef int(__cdecl *_FinishDrawing) ();

class VGui_MatSurfaceDLL : public IHookableNameFilter
{
public:
	VGui_MatSurfaceDLL() : IHookableNameFilter({ L"vguimatsurface.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	void NewTick();
	void DrawHUD(vrect_t* screen);
	void DrawTopRightHUD(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface);
	void DrawFlagsHud(bool mutuallyExclusiveFlags, const wchar* hudName, int& vertIndex, int x, const wchar** nameArray, int count, IMatSystemSurface* surface, wchar* buffer, int bufferCount, int flags, int fontTall);
	void DrawSingleFloat(int& vertIndex, const wchar* name, float f, int fontTall, int bufferCount, int x, IMatSystemSurface* surface, wchar* buffer);
	void DrawSingleInt(int& vertIndex, const wchar* name, int i, int fontTall, int bufferCount, int x, IMatSystemSurface* surface, wchar* buffer);
	void DrawTripleFloat(int& vertIndex, const wchar* name, float f1, float f2, float f3, int fontTall, int bufferCount, int x, IMatSystemSurface* surface, wchar* buffer);

	_StartDrawing ORIG_StartDrawing;
	_FinishDrawing ORIG_FinishDrawing;
protected:
	PatternContainer patternContainer;
	ConVar* cl_showpos;
	ConVar* cl_showfps;
	vgui::HFont font;

	Vector currentVel;
	Vector previousVel;
};