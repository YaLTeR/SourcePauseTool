#include "stdafx.h"
#include "vguimatsurfaceDLL.hpp"
#include <SPTLib\memutils.hpp>
#include <SPTLib\Windows\detoursutils.hpp>
#include <SPTLib\hooks.hpp>
#include "..\modules.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\cvars.hpp"
#include "..\..\utils\string_parsing.hpp"
#include "Color.h"
#include "const.h"
#include "..\scripts\srctas_reader.hpp"
#include "..\..\vgui\vgui_utils.hpp"
#include "vgui_controls\controls.h"
#include "..\patterns.hpp"
#include "..\overlay\portal_camera.hpp"

const int INDEX_MASK = MAX_EDICTS - 1;
ConVar y_spt_hud_vars("y_spt_hud_vars", "0", FCVAR_CHEAT, "Turns on the movement vars hud.\n"); // Putting this in cvars.cpp crashes the game xdddd
ConVar y_spt_hud_ag_sg_tester("y_spt_hud_ag_sg_tester", "0", FCVAR_CHEAT, "Tests if angle glitch will save glitch you.\n");

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::vguimatsurface::##name);
#define GET_FUTURE(future_name) \
    { \
        auto pattern = f##future_name.get(); \
    }

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::vguimatsurface::##name);
#define GET_FUTURE(future_name) \
    { \
        auto pattern = f##future_name.get(); \
    }

void VGui_MatSurfaceDLL::Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept)
{
	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	auto icvar = GetCvarInterface();
	cl_showpos = icvar->FindVar("cl_showpos");
	cl_showfps = icvar->FindVar("cl_showfps");
	auto scheme = vgui::GetScheme();
	font = scheme->GetFont("DefaultFixedOutline", false);

	patternContainer.Init(moduleName);

	DEF_FUTURE(StartDrawing);
	DEF_FUTURE(FinishDrawing);
	GET_FUTURE(StartDrawing);
	GET_FUTURE(FinishDrawing);

	if (!ORIG_FinishDrawing || !ORIG_StartDrawing)
		Warning("HUD drawing solutions are not available.\n");

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
			y_spt_hud_portal_bubble.GetBool() ||
			y_spt_hud_ag_sg_tester.GetBool())
		{
			DrawTopRightHUD(screen, scheme, surface);
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
, L"COLLISION_GROUP_PLAYER", L"COLLISION_GROUP_BREAKABLE_GLASS,", L"COLLISION_GROUP_VEHICLE", L"COLLISION_GROUP_PLAYER_MOVEMENT", L"COLLISION_GROUP_NPC", L"COLLISION_GROUP_IN_VEHICLE", L"COLLISION_GROUP_WEAPON", 
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
	wchar_t buffer[BUFFER_SIZE];
	currentVel = clientDLL.GetPlayerVelocity();
	Vector accel = currentVel - previousVel;

	if (y_spt_hud_velocity.GetBool())
	{
		DrawTripleFloat(vertIndex, L"vel(xyz)", currentVel.x, currentVel.y, currentVel.z, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"vel(xy)", currentVel.Length2D(), fontTall, BUFFER_SIZE, x, surface, buffer);
	}

	if (y_spt_hud_accel.GetBool())
	{
		DrawTripleFloat(vertIndex, L"accel(xyz)", accel.x, accel.y, accel.z, fontTall, BUFFER_SIZE, x, surface, buffer);
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
		DrawSingleInt(vertIndex, L"portal bubble", in_bubble, fontTall, BUFFER_SIZE, x, surface, buffer);
	}

	if (y_spt_hud_flags.GetBool())
	{
		int flags = clientDLL.GetPlayerFlags();
		DrawFlagsHud(false, NULL, vertIndex, x, FLAGS, ARRAYSIZE(FLAGS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_moveflags.GetBool())
	{
		int flags = serverDLL.GetPlayerMoveType();
		DrawFlagsHud(true, L"Move type", vertIndex, x, MOVETYPE_FLAGS, ARRAYSIZE(MOVETYPE_FLAGS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_collisionflags.GetBool())
	{
		int flags = serverDLL.GetPlayerCollisionGroup();
		DrawFlagsHud(true, L"Collision group", vertIndex, x, COLLISION_GROUPS, ARRAYSIZE(COLLISION_GROUPS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_movecollideflags.GetBool())
	{
		int flags = serverDLL.GetPlayerMoveCollide();
		DrawFlagsHud(true, L"Move collide", vertIndex, x, MOVECOLLIDE_FLAGS, ARRAYSIZE(MOVECOLLIDE_FLAGS), surface, buffer, BUFFER_SIZE, flags, fontTall);
	}

	if (y_spt_hud_vars.GetBool() && serverActive())
	{
		auto vars = clientDLL.GetMovementVars();
		DrawSingleFloat(vertIndex, L"accelerate", vars.Accelerate, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"airaccelerate", vars.Airaccelerate, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"ent friction", vars.EntFriction, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"frametime", vars.Frametime, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"friction", vars.Friction, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"maxspeed", vars.Maxspeed, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"stopspeed", vars.Stopspeed, fontTall, BUFFER_SIZE, x, surface, buffer);
		DrawSingleFloat(vertIndex, L"wishspeed cap", vars.WishspeedCap, fontTall, BUFFER_SIZE, x, surface, buffer);
	}

	if (y_spt_hud_ag_sg_tester.GetBool() && serverActive())
	{
		Vector v = clientDLL.GetPlayerEyePos();
		QAngle q;
		std::wstring result = calculateWillAGSG(v, q);
		swprintf_s(buffer, BUFFER_SIZE, L"ag sg: %s", result.c_str());
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
		surface->DrawPrintText(buffer, wcslen(buffer));
		++vertIndex;
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

void VGui_MatSurfaceDLL::DrawSingleInt(int & vertIndex, const wchar * name, int i, int fontTall, int bufferCount, int x, IMatSystemSurface * surface, wchar * buffer)
{
	swprintf_s(buffer, bufferCount, L"%s: %d", name, i);
	surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
	surface->DrawPrintText(buffer, wcslen(buffer));
	++vertIndex;
}

void VGui_MatSurfaceDLL::DrawTripleFloat(int & vertIndex, const wchar * name, float f1, float f2, float f3, int fontTall, int bufferCount, int x, IMatSystemSurface * surface, wchar * buffer)
{
	int width = y_spt_hud_decimals.GetInt();
	swprintf_s(buffer, bufferCount, L"%s: %.*f %.*f %.*f", name, width, f1, width, f2, width, f3);
	surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex);
	surface->DrawPrintText(buffer, wcslen(buffer));
	++vertIndex;
}

