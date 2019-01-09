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
#include "Color.h"
#include "const.h"
#include "..\scripts\srctas_reader.hpp"

const int INDEX_MASK = MAX_EDICTS - 1;

void VGui_MatSurfaceDLL::Hook(const std::wstring & moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	patternContainer.Init(moduleName, moduleStart, moduleLength);
	patternContainer.AddEntry(nullptr, (PVOID*)&ORIG_StartDrawing, Patterns::ptnsStartDrawing, "StartDrawing");
	patternContainer.AddEntry(nullptr, (PVOID*)&ORIG_FinishDrawing, Patterns::ptnsFinishDrawing, "FinishDrawing");

	if (!ORIG_FinishDrawing || !ORIG_StartDrawing)
		Msg("HUD drawing solutions are not available.\n");

	auto icvar = GetCvarInterface();
	cl_showpos = icvar->FindVar("cl_showpos");
	cl_showfps = icvar->FindVar("cl_showfps");
	auto scheme = GetIScheme();

	font = scheme->GetFont("DefaultFixedOutline", false);

	patternContainer.Hook();
}

void VGui_MatSurfaceDLL::Unhook()
{
	patternContainer.Unhook();
}

void VGui_MatSurfaceDLL::Clear()
{
}

void VGui_MatSurfaceDLL::NewTick()
{
	previousVel = currentVel;
}

void VGui_MatSurfaceDLL::DrawHUD(vrect_t* screen)
{
	if (!ORIG_StartDrawing || !ORIG_FinishDrawing)
		return;

	ORIG_StartDrawing();
	auto surface = GetSurface();
	auto scheme = GetIScheme();

	try
	{
		if (y_spt_hud_velocity.GetBool() || 
			y_spt_hud_flags.GetBool() || 
			y_spt_hud_script_length.GetBool() ||
			y_spt_hud_accel.GetBool() ||
			y_spt_hud_portal_bubble.GetBool())
		{
			DrawTopRightHUD(screen, scheme, surface);
		}

		if (_y_spt_overlay_crosshair.GetBool())
		{
			// WIP, haven't been able to solve how to draw stuff on the overlay without removing everything from the main screen
		}

	}
	catch (const std::exception& e)
	{
		Msg("Error drawing HUD: %s\n", e.what());
	}

	ORIG_FinishDrawing();
}

const wchar* FLAGS[] = { L"FL_ONGROUND", L"FL_DUCKING", L"FL_WATERJUMP", L"FL_ONTRAIN", NULL, L"FL_FROZEN", L"FL_ATCONTROLS", NULL, NULL
, L"FL_INWATER", L"FL_FLY", L"FL_SWIM", L"FL_CONVEYOR", NULL, L"FL_GODMODE", L"FL_NOTARGET", NULL, L"FL_PARTIALGROUND", NULL, NULL, NULL, NULL, NULL, L"FL_BASEVELOCITY" };

void VGui_MatSurfaceDLL::DrawTopRightHUD(vrect_t * screen, vgui::IScheme * scheme, IMatSystemSurface * surface)
{
#ifndef OE
	int fontTall = surface->GetFontTall(font);
	int x = screen->width - 300 + 2;
	int vertIndex = 0;

	surface->DrawSetTextFont(font);
	surface->DrawSetTextColor(255, 255, 255, 255);
	surface->DrawSetTexture(0);

	if (cl_showpos && cl_showpos->GetBool())
		vertIndex += 3;
	if (cl_showfps && cl_showfps->GetBool())
		++vertIndex;

	const int BUFFER_SIZE = 80;
	wchar_t buffer[BUFFER_SIZE];
	currentVel = clientDLL.GetPlayerVelocity();
	Vector accel = currentVel - previousVel;

	if (y_spt_hud_velocity.GetBool())
	{
		swprintf_s(buffer, BUFFER_SIZE, L"vel(xyz): %.5f %.5f %.5f", currentVel.x, currentVel.y, currentVel.z);
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;

		swprintf_s(buffer, BUFFER_SIZE, L"vel(xy): %.5f", currentVel.Length2D());
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;
	}

	if (y_spt_hud_accel.GetBool())
	{
		swprintf_s(buffer, BUFFER_SIZE, L"accel(xyz): %.5f %.5f %.5f", accel.x, accel.y, accel.z);
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;

		swprintf_s(buffer, BUFFER_SIZE, L"accel(xy): %.5f", accel.Length2D());
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;
	}

	if (y_spt_hud_script_length.GetBool())
	{
		swprintf_s(buffer, BUFFER_SIZE, L"frame: %d / %d", scripts::g_TASReader.GetCurrentTick(), scripts::g_TASReader.GetCurrentScriptLength());
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;
	}

	if (y_spt_hud_portal_bubble.GetBool())
	{
		int in_bubble = (serverDLL.GetEnviromentPortalHandle() & INDEX_MASK) != INDEX_MASK;
		swprintf_s(buffer, BUFFER_SIZE, L"portal bubble: %d", in_bubble);
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;
	}

	if (y_spt_hud_flags.GetBool())
	{
		int mask = 1;
		int flags = clientDLL.GetPlayerFlags();
		for (int u = 0; u < ARRAYSIZE(FLAGS); ++u)
		{
			if (FLAGS[u])
			{
				int mask = 1 << u;
				swprintf_s(buffer, BUFFER_SIZE, L"%s: %d\n", FLAGS[u], (flags & mask) != 0);
				surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
				surface->DrawPrintText(buffer, wcslen(buffer));
				++vertIndex;
			}

		}
	}

#endif
}

