#include "stdafx.h"

#include "vguimatsurfaceDLL.hpp"

#include <SPTLib\Windows\detoursutils.hpp>
#include <SPTLib\hooks.hpp>
#include <SPTLib\memutils.hpp>

#include "..\..\utils\ent_utils.hpp"
#include "..\..\utils\property_getter.hpp"
#include "..\..\utils\string_parsing.hpp"
#include "..\..\vgui\vgui_utils.hpp"
#include "..\cvars.hpp"
#include "..\module_hooks.hpp"
#include "..\modules.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\overlay\portal_camera.hpp"
#include "..\patterns.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "Color.h"
#include "const.h"
#include "vgui_controls\controls.h"

#undef max
#undef min

#ifndef OE
ConVar y_spt_hud_hops("y_spt_hud_hops", "0", FCVAR_CHEAT, "When set to 1, displays the hop practice HUD.");
ConVar y_spt_hud_hops_x("y_spt_hud_hops_x", "-85", FCVAR_CHEAT, "Hops HUD x offset");
ConVar y_spt_hud_hops_y("y_spt_hud_hops_y", "100", FCVAR_CHEAT, "Hops HUD y offset");
ConVar y_spt_hud_velocity_angles("y_spt_hud_velocity_angles", "0", FCVAR_CHEAT, "Display velocity Euler angles.");
ConVar _y_spt_overlay_crosshair_size("_y_spt_overlay_crosshair_size", "10", FCVAR_CHEAT, "Overlay crosshair size.");
ConVar _y_spt_overlay_crosshair_thickness("_y_spt_overlay_crosshair_thickness",
                                          "1",
                                          FCVAR_CHEAT,
                                          "Overlay crosshair thickness.");
ConVar _y_spt_overlay_crosshair_color("_y_spt_overlay_crosshair_color",
                                      "0 255 0 255",
                                      FCVAR_CHEAT,
                                      "Overlay crosshair RGBA color.");
const int INDEX_MASK = MAX_EDICTS - 1;

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

void VGui_MatSurfaceDLL::Hook(const std::wstring& moduleName,
                              void* moduleHandle,
                              void* moduleBase,
                              size_t moduleLength,
                              bool needToIntercept)
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
	hopsFont = scheme->GetFont("Trebuchet24", false);

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
	ORIG_StartDrawing = nullptr;
	ORIG_FinishDrawing = nullptr;
	sinceLanded = 0;
	displayHop = 0;
	loss = 0;
	percentage = 0;
}

void VGui_MatSurfaceDLL::Jump()
{
	if (!y_spt_hud_hops.GetBool())
		return;

	if (sinceLanded == 0)
		CalculateAbhVel();

	velNotCalced = true;
	lastHop = sinceLanded;
}

void VGui_MatSurfaceDLL::OnGround(bool onground)
{
	if (!y_spt_hud_hops.GetBool())
		return;

	if (!onground)
	{
		sinceLanded = 0;

		if (velNotCalced)
		{
			velNotCalced = false;

			// Don't count very delayed hops
			if (lastHop > 15)
				return;

			auto vel = clientDLL.GetPlayerVelocity().Length2D();
			loss = maxVel - vel;
			percentage = (vel / maxVel) * 100;
			displayHop = lastHop - 1;
			displayHop = std::max(0, displayHop);
		}
	}
	else
	{
		if (sinceLanded == 0)
		{
			CalculateAbhVel();
		}
		++sinceLanded;
	}
}

void VGui_MatSurfaceDLL::NewTick()
{
	previousVel = currentVel;
}

void VGui_MatSurfaceDLL::DrawHUD(vrect_t* screen)
{
	if (!ORIG_StartDrawing || !ORIG_FinishDrawing)
		return;

	auto surface = (IMatSystemSurface*)vgui::surface();
	auto scheme = vgui::GetScheme();
	ORIG_StartDrawing(surface, 0);

	try
	{
		if (y_spt_hud_hops.GetBool())
		{
			DrawHopHud(screen, scheme, surface);
		}

		if (y_spt_hud_demo_curtime.GetBool() || y_spt_hud_timer_show.GetBool()
		    || y_spt_hud_demo_accumtime.GetBool() ||
			(y_spt_demo_legit_check.GetBool() &&  engineDLL.IsDemoRecording()) 
			|| y_spt_hud_velocity.GetBool()
		    || y_spt_hud_flags.GetBool()
		    || y_spt_hud_moveflags.GetBool()
		    || y_spt_hud_movecollideflags.GetBool() || y_spt_hud_collisionflags.GetBool()
		    || y_spt_hud_script_length.GetBool() || y_spt_hud_accel.GetBool() || y_spt_hud_vars.GetBool()
		    || y_spt_hud_portal_bubble.GetBool() || y_spt_hud_ag_sg_tester.GetBool()
		    || !whiteSpacesOnly(y_spt_hud_ent_info.GetString()) || y_spt_hud_oob.GetBool()
		    || y_spt_hud_velocity_angles.GetBool())
		{
			DrawTopHUD(screen, scheme, surface);
		}
	}
	catch (const std::exception& e)
	{
		Msg("Error drawing HUD: %s\n", e.what());
	}

	ORIG_FinishDrawing(surface, 0);
}

void VGui_MatSurfaceDLL::DrawCrosshair(vrect_t* screen)
{
	static std::string color = "";
	static int r = 0, g = 0, b = 0, a = 0;

	if (strcmp(color.c_str(), _y_spt_overlay_crosshair_color.GetString()) != 0)
	{
		color = _y_spt_overlay_crosshair_color.GetString();
		sscanf(color.c_str(), "%d %d %d %d", &r, &g, &b, &a);
	}

	auto surface = (IMatSystemSurface*)vgui::surface();
	surface->DrawSetColor(r, g, b, a);
	int x = screen->x + screen->width / 2;
	int y = screen->y + screen->height / 2;
	int width = _y_spt_overlay_crosshair_size.GetInt();
	int thickness = _y_spt_overlay_crosshair_thickness.GetInt();

	if (thickness > width)
		std::swap(thickness, width);

	surface->DrawFilledRect(x - thickness / 2, y - width / 2, x + thickness / 2 + 1, y + width / 2 + 1);
	surface->DrawFilledRect(x - width / 2, y - thickness / 2, x - thickness / 2, y + thickness / 2 + 1);
	surface->DrawFilledRect(x + thickness / 2 + 1, y - thickness / 2, x + width / 2 + 1, y + thickness / 2 + 1);
}

void VGui_MatSurfaceDLL::CalculateAbhVel()
{
	auto vel = clientDLL.GetPlayerVelocity().Length2D();
	auto ducked = clientDLL.GetFlagsDucking();
	auto sprinting = utils::GetProperty<bool>(0, "m_fIsSprinting");
	auto vars = clientDLL.GetMovementVars();

	float modifier;

	if (ducked)
		modifier = 0.1;
	else if (sprinting)
		modifier = 0.5;
	else
		modifier = 1;

	float jspeed = vars.Maxspeed + (vars.Maxspeed * modifier);

	maxVel = vel + (vel - jspeed);
	maxVel = std::max(maxVel, jspeed);
}

const wchar* FLAGS[] = {L"FL_ONGROUND",
                        L"FL_DUCKING",
                        L"FL_WATERJUMP",
                        L"FL_ONTRAIN",
                        L"FL_INRAIN",
                        L"FL_FROZEN",
                        L"FL_ATCONTROLS",
                        L"FL_CLIENT",
                        L"FL_FAKECLIENT",
                        L"FL_INWATER",
                        L"FL_FLY",
                        L"FL_SWIM",
                        L"FL_CONVEYOR",
                        L"FL_NPC",
                        L"FL_GODMODE",
                        L"FL_NOTARGET",
                        L"FL_AIMTARGET",
                        L"FL_PARTIALGROUND",
                        L"FL_STATICPROP",
                        L"FL_GRAPHED",
                        L"FL_GRENADE",
                        L"FL_STEPMOVEMENT",
                        L"FL_DONTTOUCH",
                        L"FL_BASEVELOCITY",
                        L"FL_WORLDBRUSH",
                        L"FL_OBJECT",
                        L"FL_KILLME",
                        L"FL_ONFIRE",
                        L"FL_DISSOLVING",
                        L"FL_TRANSRAGDOLL",
                        L"FL_UNBLOCKABLE_BY_PLAYER"};

const wchar* MOVETYPE_FLAGS[] = {L"MOVETYPE_NONE",
                                 L"MOVETYPE_ISOMETRIC",
                                 L"MOVETYPE_WALK",
                                 L"MOVETYPE_STEP",
                                 L"MOVETYPE_FLY",
                                 L"MOVETYPE_FLYGRAVITY",
                                 L"MOVETYPE_VPHYSICS",
                                 L"MOVETYPE_PUSH",
                                 L"MOVETYPE_NOCLIP",
                                 L"MOVETYPE_LADDER",
                                 L"MOVETYPE_OBSERVER",
                                 L"MOVETYPE_CUSTOM"};

const wchar* MOVECOLLIDE_FLAGS[] = {L"MOVECOLLIDE_DEFAULT",
                                    L"MOVECOLLIDE_FLY_BOUNCE",
                                    L"MOVECOLLIDE_FLY_CUSTOM",
                                    L"MOVECOLLIDE_COUNT"};

const wchar* COLLISION_GROUPS[] = {L"COLLISION_GROUP_NONE",
                                   L"COLLISION_GROUP_DEBRIS",
                                   L"COLLISION_GROUP_DEBRIS_TRIGGER",
                                   L"COLLISION_GROUP_INTERACTIVE_DEBRIS",
                                   L"COLLISION_GROUP_INTERACTIVE",
                                   L"COLLISION_GROUP_PLAYER",
                                   L"COLLISION_GROUP_BREAKABLE_GLASS,",
                                   L"COLLISION_GROUP_VEHICLE",
                                   L"COLLISION_GROUP_PLAYER_MOVEMENT",
                                   L"COLLISION_GROUP_NPC",
                                   L"COLLISION_GROUP_IN_VEHICLE",
                                   L"COLLISION_GROUP_WEAPON",
                                   L"COLLISION_GROUP_VEHICLE_CLIP",
                                   L"COLLISION_GROUP_PROJECTILE",
                                   L"COLLISION_GROUP_DOOR_BLOCKER",
                                   L"COLLISION_GROUP_PASSABLE_DOOR",
                                   L"COLLISION_GROUP_DISSOLVING",
                                   L"COLLISION_GROUP_PUSHAWAY",
                                   L"COLLISION_GROUP_NPC_ACTOR",
                                   L"COLLISION_GROUP_NPC_SCRIPTED",
                                   L"LAST_SHARED_COLLISION_GROUP"};

static const int MAX_ENTRIES = 128;
static const int INFO_BUFFER_SIZE = 256;
static wchar INFO_ARRAY[MAX_ENTRIES * INFO_BUFFER_SIZE];
static const char ENT_SEPARATOR = ';';
static const char PROP_SEPARATOR = ',';

#define DRAW() \
	{ \
		surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex); \
		surface->DrawPrintText(buffer, wcslen(buffer)); \
		++vertIndex; \
	}

#define DRAW_FLOAT(name, floatVal) \
	{ \
		DrawSingleFloat(vertIndex, name, floatVal, fontTall, BUFFER_SIZE, x, surface, buffer); \
	}

#define DRAW_INT(name, intVal) \
	{ \
		DrawSingleInt(vertIndex, name, intVal, fontTall, BUFFER_SIZE, x, surface, buffer); \
	}

#define DRAW_TRIPLEFLOAT(name, floatVal1, floatVal2, floatVal3) \
	{ \
		DrawTripleFloat( \
		    vertIndex, name, floatVal1, floatVal2, floatVal3, fontTall, BUFFER_SIZE, x, surface, buffer); \
	}

#define DRAW_FLAGS(name, namesArray, flags, mutuallyExclusive) \
	{ \
		DrawFlagsHud(mutuallyExclusive, \
		             name, \
		             vertIndex, \
		             x, \
		             namesArray, \
		             ARRAYSIZE(namesArray), \
		             surface, \
		             buffer, \
		             BUFFER_SIZE, \
		             flags, \
		             fontTall); \
	}

void VGui_MatSurfaceDLL::DrawHopHud(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface)
{
	surface->DrawSetTextFont(hopsFont);
	surface->DrawSetTextColor(255, 255, 255, 255);
	surface->DrawSetTexture(0);
	int fontTall = surface->GetFontTall(hopsFont);

	const int MARGIN = 2;
	const int BUFFER_SIZE = 256;
	wchar_t buffer[BUFFER_SIZE];

	int width = y_spt_hud_decimals.GetInt();
	swprintf_s(buffer, BUFFER_SIZE, L"Timing: %d", displayHop);
	surface->DrawSetTextPos(screen->width / 2 + y_spt_hud_hops_x.GetFloat(),
	                        screen->height / 2 + y_spt_hud_hops_y.GetFloat());
	surface->DrawPrintText(buffer, wcslen(buffer));

	swprintf_s(buffer, BUFFER_SIZE, L"Speed loss: %.*f", width, loss);
	surface->DrawSetTextPos(screen->width / 2 + y_spt_hud_hops_x.GetFloat(),
	                        screen->height / 2 + y_spt_hud_hops_y.GetFloat() + (fontTall + MARGIN));
	surface->DrawPrintText(buffer, wcslen(buffer));

	swprintf_s(buffer, BUFFER_SIZE, L"Percentage: %.*f", width, percentage);
	surface->DrawSetTextPos(screen->width / 2 + y_spt_hud_hops_x.GetFloat(),
	                        screen->height / 2 + y_spt_hud_hops_y.GetFloat() + (fontTall + MARGIN) * 2);
	surface->DrawPrintText(buffer, wcslen(buffer));
}

void VGui_MatSurfaceDLL::DrawTopHUD(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface)
{
	int fontTall = surface->GetFontTall(font);
	int x;

	if (y_spt_hud_left.GetBool())
		x = 6;
	else
		x = screen->width - 300 + 2;

	int vertIndex = 0;

	surface->DrawSetTextFont(font);
	surface->DrawSetTextColor(255, 255, 255, 255);
	surface->DrawSetTexture(0);

	if (!y_spt_hud_left.GetBool())
	{
		if (cl_showpos && cl_showpos->GetBool())
			vertIndex += 3;
		if (cl_showfps && cl_showfps->GetBool())
			++vertIndex;
	}

	const int BUFFER_SIZE = 256;
	wchar_t buffer[BUFFER_SIZE];
	currentVel = clientDLL.GetPlayerVelocity();
	Vector accel = currentVel - previousVel;
	std::string info(y_spt_hud_ent_info.GetString());

	if (!whiteSpacesOnly(info))
	{
		int entries = utils::FillInfoArray(info,
		                                   INFO_ARRAY,
		                                   MAX_ENTRIES,
		                                   INFO_BUFFER_SIZE,
		                                   PROP_SEPARATOR,
		                                   ENT_SEPARATOR);
		for (int i = 0; i < entries; ++i)
		{
			DrawSingleString(vertIndex,
			                 INFO_ARRAY + INFO_BUFFER_SIZE * i,
			                 fontTall,
			                 BUFFER_SIZE,
			                 x,
			                 surface,
			                 buffer);
		}
	}

	if (y_spt_hud_demo_curtime.GetBool() || y_spt_hud_demo_accumtime.GetBool())
	{
		int tick = engineDLL.GetDemoTickCount();

		if (y_spt_hud_demo_curtime.GetBool())
		{
			const char* ch = engineDLL.GetDemoName();
			const size_t len = strlen(ch) + 1;
			wchar_t* wch = new wchar_t[len];
			mbstowcs(wch, ch, len);

			swprintf_s(buffer, BUFFER_SIZE, L"Tick: %d", tick);
			DRAW();
			swprintf_s(buffer, BUFFER_SIZE, L"Time: %.4f", engineDLL.GetDemoTime());
			DRAW();
			swprintf_s(buffer, BUFFER_SIZE, L"Name: %s", wch);
			DRAW();
		}

		if (y_spt_hud_demo_accumtime.GetBool())
		{
			swprintf_s(buffer, BUFFER_SIZE, L"Accumulated demo time : %.4f", engineDLL.GetAccumDemoTime());
			DRAW();
		}
	}

	float curRunTime = engineDLL.GetCurrentRunTime();
	if (y_spt_hud_timer_show.GetBool())
	{
		int hours = (int)(curRunTime / 3600.0f);
		int minutes = (int)(((curRunTime / 3600.0f) - hours) * 60.0f);
		int seconds = (int)(((((curRunTime / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f);
		int millis =
		    (int)(((((((curRunTime / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f) - seconds) * 10000.0f);
		
		char printout[15];
		swprintf_s(buffer,
		           BUFFER_SIZE,
		           L"%02d:%02d:%02d.%04d",
		           hours,   //hours
		           minutes, //minutes
		           seconds, //seconds
		           millis);
		DRAW();
	}

	if (y_spt_demo_legit_check.GetBool() && engineDLL.IsDemoRecording())
	{
		swprintf_s(buffer, BUFFER_SIZE, L"SPT loaded while recording Demo");
		DRAW();
	}

	if (y_spt_hud_velocity.GetBool())
	{
		DRAW_TRIPLEFLOAT(L"vel(xyz)", currentVel.x, currentVel.y, currentVel.z);
		DRAW_FLOAT(L"vel(xy)", currentVel.Length2D());
	}

	if (y_spt_hud_velocity_angles.GetBool())
	{
		QAngle angles;
		VectorAngles(currentVel, Vector(0, 0, 1), angles);
		DRAW_TRIPLEFLOAT(L"vel(p/y/r)", angles.x, angles.y, angles.z);
	}

	if (y_spt_hud_accel.GetBool())
	{
		DRAW_TRIPLEFLOAT(L"accel(xyz)", accel.x, accel.y, accel.z);
		DRAW_FLOAT(L"accel(xy)", accel.Length2D());
	}

	if (y_spt_hud_script_length.GetBool())
	{
		swprintf_s(buffer,
		           BUFFER_SIZE,
		           L"frame: %d / %d",
		           scripts::g_TASReader.GetCurrentTick(),
		           scripts::g_TASReader.GetCurrentScriptLength());
		DRAW();
	}

#if SSDK2007
	if (y_spt_hud_portal_bubble.GetBool())
	{
		int in_bubble = GetEnvironmentPortal() != NULL;
		DRAW_INT(L"portal bubble", in_bubble);
	}
#endif

	if (y_spt_hud_flags.GetBool())
	{
		int flags = clientDLL.GetPlayerFlags();
		DRAW_FLAGS(NULL, FLAGS, flags, false);
	}

	if (y_spt_hud_moveflags.GetBool())
	{
		int flags = serverDLL.GetPlayerMoveType();
		DRAW_FLAGS(L"Move type", MOVETYPE_FLAGS, flags, true);
	}

	if (y_spt_hud_collisionflags.GetBool())
	{
		int flags = serverDLL.GetPlayerCollisionGroup();
		DRAW_FLAGS(L"Collision group", COLLISION_GROUPS, flags, true);
	}

	if (y_spt_hud_movecollideflags.GetBool())
	{
		int flags = serverDLL.GetPlayerMoveCollide();
		DRAW_FLAGS(L"Move collide", MOVECOLLIDE_FLAGS, flags, true);
	}

	if (y_spt_hud_vars.GetBool() && utils::playerEntityAvailable())
	{
		auto vars = clientDLL.GetMovementVars();
		DRAW_FLOAT(L"accelerate", vars.Accelerate);
		DRAW_FLOAT(L"airaccelerate", vars.Airaccelerate);
		DRAW_FLOAT(L"ent friction", vars.EntFriction);
		DRAW_FLOAT(L"frametime", vars.Frametime);
		DRAW_FLOAT(L"friction", vars.Friction);
		DRAW_FLOAT(L"maxspeed", vars.Maxspeed);
		DRAW_FLOAT(L"stopspeed", vars.Stopspeed);
		DRAW_FLOAT(L"wishspeed cap", vars.WishspeedCap);
		DRAW_INT(L"onground", (int)vars.OnGround);
	}

	if (y_spt_hud_ag_sg_tester.GetBool() && utils::playerEntityAvailable())
	{
		Vector v = clientDLL.GetPlayerEyePos();
		QAngle q;

		std::wstring result = calculateWillAGSG(v, q);
		swprintf_s(buffer, BUFFER_SIZE, L"ag sg: %s", result.c_str());
		DRAW();
	}

	if (y_spt_hud_oob.GetBool())
	{
		Vector v = clientDLL.GetCameraOrigin();
		trace_t tr;
		Strafe::Trace(tr, v, v + Vector(1, 1, 1));

		bool oob = engineDLL.ORIG_CEngineTrace__PointOutsideWorld(nullptr, 0, v) && !tr.startsolid;
		swprintf_s(buffer, BUFFER_SIZE, L"oob: %d", oob);
		DRAW();
	}
}

void VGui_MatSurfaceDLL::DrawFlagsHud(bool mutuallyExclusiveFlags,
                                      const wchar* hudName,
                                      int& vertIndex,
                                      int x,
                                      const wchar** nameArray,
                                      int count,
                                      IMatSystemSurface* surface,
                                      wchar* buffer,
                                      int bufferCount,
                                      int flags,
                                      int fontTall)
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
			else if (!mutuallyExclusiveFlags)
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

	if (!drewSomething)
		swprintf_s(buffer, bufferCount, L"%s: %s\n", hudName, L"unknown");
}

void VGui_MatSurfaceDLL::DrawSingleFloat(int& vertIndex,
                                         const wchar* name,
                                         float f,
                                         int fontTall,
                                         int bufferCount,
                                         int x,
                                         IMatSystemSurface* surface,
                                         wchar* buffer)
{
	int width = y_spt_hud_decimals.GetInt();
	swprintf_s(buffer, bufferCount, L"%s: %.*f", name, width, f);
	DRAW();
}

void VGui_MatSurfaceDLL::DrawSingleInt(int& vertIndex,
                                       const wchar* name,
                                       int i,
                                       int fontTall,
                                       int bufferCount,
                                       int x,
                                       IMatSystemSurface* surface,
                                       wchar* buffer)
{
	swprintf_s(buffer, bufferCount, L"%s: %d", name, i);
	DRAW();
}

void VGui_MatSurfaceDLL::DrawTripleFloat(int& vertIndex,
                                         const wchar* name,
                                         float f1,
                                         float f2,
                                         float f3,
                                         int fontTall,
                                         int bufferCount,
                                         int x,
                                         IMatSystemSurface* surface,
                                         wchar* buffer)
{
	int width = y_spt_hud_decimals.GetInt();
	swprintf_s(buffer, bufferCount, L"%s: %.*f %.*f %.*f", name, width, f1, width, f2, width, f3);
	DRAW();
}

void VGui_MatSurfaceDLL::DrawSingleString(int& vertIndex,
                                          const wchar* str,
                                          int fontTall,
                                          int bufferCount,
                                          int x,
                                          IMatSystemSurface* surface,
                                          wchar* buffer)
{
	swprintf_s(buffer, bufferCount, L"%s", str);
	DRAW();
}

#endif