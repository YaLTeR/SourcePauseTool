#include "stdafx.h"
#include "vguimatsurfaceDLL.hpp"
#include <SPTLib\memutils.hpp>
#include <SPTLib\detoursutils.hpp>
#include <SPTLib\hooks.hpp>
#include "..\modules.hpp"
#include "..\patterns.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\cvars.hpp"
#include "..\..\utils\string_parsing.hpp"

void VGui_MatSurfaceDLL::Hook(const std::wstring & moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	patternContainer.Init(moduleName, moduleStart, moduleLength);
	patternContainer.AddEntry(HOOKED_StartDrawing, (PVOID*)&ORIG_StartDrawing, Patterns::ptnsStartDrawing, "StartDrawing");
	patternContainer.AddEntry(HOOKED_FinishDrawing, (PVOID*)&ORIG_FinishDrawing, Patterns::ptnsFinishDrawing, "FinishDrawing");

	if (!ORIG_FinishDrawing || !ORIG_StartDrawing)
		Msg("HUD drawing solutions are not available.\n");

	patternContainer.Hook();
}

void VGui_MatSurfaceDLL::Unhook()
{
	patternContainer.Unhook();
}

void VGui_MatSurfaceDLL::Clear()
{
}

void VGui_MatSurfaceDLL::DrawCrosshair()
{
	if (_y_spt_overlay_crosshair.GetBool())
	{
		try
		{
			auto surface = GetSurface();
			auto scheme = GetIScheme();

			vgui_matsurfaceDLL.HOOKED_StartDrawing();
			// Insert drawing function
			vgui_matsurfaceDLL.HOOKED_FinishDrawing();
		}
		catch (const std::exception& e)
		{
			Msg("Error drawing overlay crosshair: %s\n", e.what());
		}
	}
}

void __cdecl VGui_MatSurfaceDLL::HOOKED_StartDrawing()
{
	vgui_matsurfaceDLL.ORIG_StartDrawing();
}

void __cdecl VGui_MatSurfaceDLL::HOOKED_FinishDrawing()
{
	vgui_matsurfaceDLL.ORIG_FinishDrawing();
}
