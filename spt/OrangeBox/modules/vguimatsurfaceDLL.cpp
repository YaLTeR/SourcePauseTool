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
#include "..\..\vgui\vgui_utils.hpp"
#include "vgui_controls\controls.h"

const int INDEX_MASK = MAX_EDICTS - 1;
ConVar y_spt_hud_vars("y_spt_hud_vars", "0", FCVAR_CHEAT, "Turns on the movement vars hud.\n");

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
	auto scheme = vgui::GetScheme();

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
	auto surface = (IMatSystemSurface*)vgui::surface();
	auto scheme = vgui::GetScheme();

	try
	{
		if (y_spt_hud_velocity.GetBool() || 
			y_spt_hud_flags.GetBool() || 
			y_spt_hud_moveflags.GetBool() ||
			y_spt_hud_movecollideflags.GetBool() ||
			y_spt_hud_collisionflags.GetBool() ||
			y_spt_hud_script_length.GetBool() ||
			y_spt_hud_accel.GetBool() ||
			y_spt_hud_vars.GetBool() ||
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

const wchar* FLAGS[] = { L"FL_ONGROUND", L"FL_DUCKING", L"FL_WATERJUMP", L"FL_ONTRAIN", L"FL_INRAIN", L"FL_FROZEN", L"FL_ATCONTROLS", L"FL_CLIENT", L"FL_FAKECLIENT"
, L"FL_INWATER", L"FL_FLY", L"FL_SWIM", L"FL_CONVEYOR", L"FL_NPC", L"FL_GODMODE", L"FL_NOTARGET", L"FL_AIMTARGET", L"FL_PARTIALGROUND", L"FL_STATICPROP", L"FL_GRAPHED", L"FL_GRENADE",
L"FL_STEPMOVEMENT", L"FL_DONTTOUCH",  L"FL_BASEVELOCITY", L"FL_WORLDBRUSH", L"FL_OBJECT", L"FL_KILLME", L"FL_ONFIRE", L"FL_DISSOLVING", L"FL_TRANSRAGDOLL", L"FL_UNBLOCKABLE_BY_PLAYER" };

const wchar* MOVETYPE_FLAGS[] = { L"MOVETYPE_NONE", L"MOVETYPE_ISOMETRIC", L"MOVETYPE_WALK", L"MOVETYPE_STEP", L"MOVETYPE_FLY", L"MOVETYPE_FLYGRAVITY", L"MOVETYPE_VPHYSICS",
L"MOVETYPE_PUSH", L"MOVETYPE_NOCLIP", L"MOVETYPE_LADDER", L"MOVETYPE_OBSERVER", L"MOVETYPE_CUSTOM"};

const wchar* MOVECOLLIDE_FLAGS[] = { L"MOVECOLLIDE_DEFAULT", L"MOVECOLLIDE_FLY_BOUNCE", L"MOVECOLLIDE_FLY_CUSTOM", L"MOVECOLLIDE_COUNT" };

const wchar* COLLISION_GROUPS[] = { L"COLLISION_GROUP_NONE", L"COLLISION_GROUP_DEBRIS", L"COLLISION_GROUP_DEBRIS_TRIGGER", L"COLLISION_GROUP_INTERACTIVE_DEBRIS", L"COLLISION_GROUP_INTERACTIVE"
, L"COLLISION_GROUP_PLAYER", NULL, L"COLLISION_GROUP_VEHICLE", L"COLLISION_GROUP_PLAYER_MOVEMENT", L"COLLISION_GROUP_NPC", L"COLLISION_GROUP_IN_VEHICLE", L"COLLISION_GROUP_WEAPON", 
L"COLLISION_GROUP_VEHICLE_CLIP", L"COLLISION_GROUP_PROJECTILE", L"COLLISION_GROUP_DOOR_BLOCKER", L"COLLISION_GROUP_PASSABLE_DOOR", L"COLLISION_GROUP_DISSOLVING", L"COLLISION_GROUP_PUSHAWAY",
L"COLLISION_GROUP_NPC_ACTOR", L"COLLISION_GROUP_NPC_SCRIPTED", L"LAST_SHARED_COLLISION_GROUP" };

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
	wchar_t format[BUFFER_SIZE];
	wchar_t buffer[BUFFER_SIZE];
	currentVel = clientDLL.GetPlayerVelocity();
	Vector accel = currentVel - previousVel;
	int width = y_spt_hud_decimals.GetInt();

	if (y_spt_hud_velocity.GetBool())
	{
		swprintf_s(buffer, BUFFER_SIZE, L"vel(xyz): %.*f %.*f %.*f", width, currentVel.x, width, currentVel.y, width, currentVel.z);
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;

		DrawSingleFloat(vertIndex, L"vel(xy)", currentVel.Length2D(), fontTall, BUFFER_SIZE, x, surface, buffer);
	}

	if (y_spt_hud_accel.GetBool())
	{
		swprintf_s(buffer, BUFFER_SIZE, L"accel(xyz): %.*f %.*f %.*f", width, accel.x, width, accel.y, width, accel.z);
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;

		DrawSingleFloat(vertIndex, L"accel(xy)", accel.Length2D(), fontTall, BUFFER_SIZE, x, surface, buffer);
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
		int flags = clientDLL.GetPlayerFlags();
		DrawFlagsHud(false, NULL, vertIndex, x, FLAGS, ARRAYSIZE(FLAGS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_moveflags.GetBool())
	{
		int flags = serverDLL.GetPlayerMoveType() & 0xF;
		DrawFlagsHud(true, L"Move type", vertIndex, x, MOVETYPE_FLAGS, ARRAYSIZE(MOVETYPE_FLAGS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_collisionflags.GetBool())
	{
		int flags = serverDLL.GetPlayerCollisionGroup();
		DrawFlagsHud(true, L"Collision group", vertIndex, x, COLLISION_GROUPS, ARRAYSIZE(COLLISION_GROUPS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_movecollideflags.GetBool())
	{
		int flags = serverDLL.GetPlayerMoveCollide() & 0x7;
		DrawFlagsHud(true, L"Move collide", vertIndex, x, MOVECOLLIDE_FLAGS, ARRAYSIZE(MOVECOLLIDE_FLAGS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_vars.GetBool())
	{
		auto vars = clientDLL.GetMovementVars();
		DrawSingleFloat(vertIndex, L"Accelerate", vars.Accelerate, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"Airaccelerate", vars.Airaccelerate, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"Ent friction", vars.EntFriction, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"Frametime", vars.Frametime, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"Friction", vars.Friction, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"Maxspeed", vars.Maxspeed, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"Stopspeed", vars.Stopspeed, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"Wishspeed cap", vars.WishspeedCap, fontTall, BUFFER_SIZE, x, surface, buffer);
	}

#endif
}

void VGui_MatSurfaceDLL::DrawFlagsHud(bool mutuallyExclusiveFlags, const wchar* hudName, int& vertIndex, int x, const wchar** nameArray, int count, IMatSystemSurface* surface, wchar* buffer, int bufferCount, int flags, int fontTall)
{
	bool drewSomething = false;
	for (int u = 0; u < count; ++u)
	{
		if (nameArray[u])
		{
			bool draw = false;
			if (mutuallyExclusiveFlags && flags == u)
			{
				swprintf_s(buffer, bufferCount, L"%s: %s\n", hudName, nameArray[u]);
				draw = true;
			}		
			else if(!mutuallyExclusiveFlags)
			{
				swprintf_s(buffer, bufferCount, L"%s: %d\n", nameArray[u], (flags & (1 << u)) != 0);
				draw = true;
			}
				
			if (draw)
			{
				surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
				surface->DrawPrintText(buffer, wcslen(buffer));
				++vertIndex;
				drewSomething = true;
			}
		}
	}

	if(!drewSomething)
		swprintf_s(buffer, bufferCount, L"%s: %s\n", hudName, L"unknown");
}

void VGui_MatSurfaceDLL::DrawSingleFloat(int& vertIndex, const wchar * name, float f, int fontTall, int bufferCount, int x, IMatSystemSurface* surface, wchar* buffer)
{
	int width = y_spt_hud_decimals.GetInt();
	swprintf_s(buffer, bufferCount, L"%s: %.*f", name, width, f);
	surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
	surface->DrawPrintText(buffer, wcslen(buffer));
	++vertIndex;
}

