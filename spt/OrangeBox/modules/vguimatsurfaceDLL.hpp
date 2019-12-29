#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\..\utils\patterncontainer.hpp"
#include "convar.h"
#include "tier0\basetypes.h"
#include "vgui\ischeme.h"
#include "vguimatsurface\imatsystemsurface.h"

#ifndef OE

using std::size_t;
using std::uintptr_t;

typedef void(__fastcall* _StartDrawing)(void* thisptr, int edx);
typedef void(__fastcall* _FinishDrawing)(void* thisptr, int edx);

class VGui_MatSurfaceDLL : public IHookableNameFilter
{
public:
	VGui_MatSurfaceDLL() : IHookableNameFilter({L"vguimatsurface.dll"}){};
	virtual void Hook(const std::wstring& moduleName,
	                  void* moduleHandle,
	                  void* moduleBase,
	                  size_t moduleLength,
	                  bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	void Jump();
	void OnGround(bool ground);
	void NewTick();
	void DrawHUD(vrect_t* screen);
	void CalculateAbhVel();

	_StartDrawing ORIG_StartDrawing;
	_FinishDrawing ORIG_FinishDrawing;

protected:
	int displayHop;
	float loss;
	float percentage;

	bool velNotCalced;
	int lastHop;
	float maxVel;

	int sinceLanded;
	PatternContainer patternContainer;
	ConVar* cl_showpos;
	ConVar* cl_showfps;
	vgui::HFont font;
	vgui::HFont hopsFont;

	Vector currentVel;
	Vector previousVel;

	void DrawHopHud(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface);
	void DrawTopHUD(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface);
	void DrawFlagsHud(bool mutuallyExclusiveFlags,
	                  const wchar* hudName,
	                  int& vertIndex,
	                  int x,
	                  const wchar** nameArray,
	                  int count,
	                  IMatSystemSurface* surface,
	                  wchar* buffer,
	                  int bufferCount,
	                  int flags,
	                  int fontTall);
	void DrawSingleFloat(int& vertIndex,
	                     const wchar* name,
	                     float f,
	                     int fontTall,
	                     int bufferCount,
	                     int x,
	                     IMatSystemSurface* surface,
	                     wchar* buffer);
	void DrawSingleInt(int& vertIndex,
	                   const wchar* name,
	                   int i,
	                   int fontTall,
	                   int bufferCount,
	                   int x,
	                   IMatSystemSurface* surface,
	                   wchar* buffer);
	void DrawTripleFloat(int& vertIndex,
	                     const wchar* name,
	                     float f1,
	                     float f2,
	                     float f3,
	                     int fontTall,
	                     int bufferCount,
	                     int x,
	                     IMatSystemSurface* surface,
	                     wchar* buffer);
	void DrawSingleString(int& vertIndex,
	                      const wchar* str,
	                      int fontTall,
	                      int bufferCount,
	                      int x,
	                      IMatSystemSurface* surface,
	                      wchar* buffer);
};

#endif