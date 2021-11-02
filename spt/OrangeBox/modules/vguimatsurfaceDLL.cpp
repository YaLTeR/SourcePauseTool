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
#include "..\..\patterns_new.cpp"

#undef max
#undef min

#ifndef OE
ConVar y_spt_hud_hops("y_spt_hud_hops",
                      "0",
                      FCVAR_CHEAT | FCVAR_SPT_HUD,
                      "When set to 1, displays the hop practice HUD.");
ConVar y_spt_hud_hops_x("y_spt_hud_hops_x", "-85", FCVAR_CHEAT, "Hops HUD x offset");
ConVar y_spt_hud_hops_y("y_spt_hud_hops_y", "100", FCVAR_CHEAT, "Hops HUD y offset");
ConVar y_spt_hud_velocity_angles("y_spt_hud_velocity_angles",
                                 "0",
                                 FCVAR_CHEAT | FCVAR_SPT_HUD,
                                 "Display velocity Euler angles.");
ConVar _y_spt_overlay_crosshair_size("_y_spt_overlay_crosshair_size", "10", FCVAR_CHEAT, "Overlay crosshair size.");
ConVar _y_spt_overlay_crosshair_thickness("_y_spt_overlay_crosshair_thickness",
                                          "1",
                                          FCVAR_CHEAT,
                                          "Overlay crosshair thickness.");
ConVar _y_spt_overlay_crosshair_color("_y_spt_overlay_crosshair_color",
                                      "0 255 0 255",
                                      FCVAR_CHEAT,
                                      "Overlay crosshair RGBA color.");
ConVar y_spt_hud_drawing_function("y_spt_hud_drawing_function",
                                  "0",
                                  0,
                                  "Switches between options for hud drawing functions:\n \
    0: Use functions inferred from SDK files\n \
    1: Use scanned functions (if hud drawing routines crash the game)\n \
    2: Use scanned functions (modified for 1.0+ BMS Retail)");
const int INDEX_MASK = MAX_EDICTS - 1;

#define TAG "[vguimatsurface dll] "
#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::vguimatsurface::##name);
#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[vguimatsurface.dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::vguimatsurface::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[vguimatsurface.dll] Could not find " #future_name ".\n"); \
		} \
	}

using namespace PatternsExt;

void VGui_MatSurfaceDLL::Hook(const std::wstring& moduleName,
                              void* moduleHandle,
                              void* moduleBase,
                              size_t moduleLength,
                              bool needToIntercept)
{
	auto startTime = std::chrono::high_resolution_clock::now();

	const PatternScanner mScanner(moduleBase, moduleLength);

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

	if (!ORIG_StartDrawing)
	{
		GENERIC_BACKTRACE_NOTE(StartDrawing);
		uintptr_t tmp = FindStringAddress(mScanner, "-pixel_offset_y");
		tmp = FindVarReference(mScanner, tmp, "68");

		if (tmp != 0)
		{
			uintptr_t tmp2 = BackTraceToFuncStart(mScanner, tmp, 200, 3, false, 0x10000);
			if (tmp2 != 0)
			{
				DevMsg(TAG "Found StartDrawing at %p (through function backtracing)\n", tmp2);
				ORIG_StartDrawing = (_StartDrawing)tmp2;
			}

			if (!ORIG_FinishDrawing)
			{
				GENERIC_BACKTRACE_NOTE(FinishDrawing);

				Pattern p("C6 05 ?? ?? ?? ?? 01", 2);
				p.onMatchEvaluate = _oMEArgs()
				{
					*foundPtr = **(uintptr_t**)foundPtr;
					DevMsg(TAG "Found g_bInDrawing at %p\n", *foundPtr);
				};
				PatternScanner scanner((void*)tmp, 200);
				tmp = scanner.Scan(p);
				if (tmp != 0)
				{
					tmp = FindVarReference(mScanner, tmp, "C6 05", "00");
					tmp = BackTraceToFuncStart(mScanner, tmp, 200, 3, false, 0x10000);
					if (tmp != 0)
					{
						DevMsg(TAG "Found FinishDrawing at %p (through function backtracing)\n",
						       tmp);
						ORIG_FinishDrawing = (_FinishDrawing)tmp;
					}
				}
			}
		}
	}

	if (!ORIG_FinishDrawing || !ORIG_StartDrawing)
		Warning("HUD drawing solutions are not available.\n");

	uintptr_t tmp;
	Pattern p("c7 45 ?? ff ff ff ff", 0);
	PatternCollection p3("89 ?? ?? 89 ?? ??", 0);
	p3.AddPattern("8B ?? ?? 89 ?? ??", 0);

	p.onMatchEvaluate = _oMEArgs(&)
	{
		uintptr_t ptr1 = BackTraceToFuncStart(mScanner, *foundPtr, 0x100, 2, true);
		if (ptr1 != 0)
		{
			Pattern p2 = GeneratePatternFromVar(ptr1);
			p2.onMatchEvaluate = _oMEArgs(&)
			{
				uintptr_t found = *foundPtr;
				if (found % 4 == 0 && mScanner.CheckWithin(*(uintptr_t*)(found - 4)))
				{
					found = *(uintptr_t*)(found - 8);
					PatternScanner scanner((void*)found, 50);
					uintptr_t found2 = scanner.Scan(p3);

					if (found2 == 0)
						goto eof;

					unsigned char* bytes = (unsigned char*)found;
					for (int i = 0; i < 40; i++)
					{
						if (pUtils.checkInt(bytes + i, 2))
						{
							ORIG_DrawSetTextPos = (_DrawSetTextPos)found;
							DevMsg(TAG "DrawSetTextPos backup found at %p\n",
							       ORIG_DrawSetTextPos);
							*done = true;
							return;
						}
					}
				}
			eof:
				*done = false;
				*foundPtr = 0;
			};
			uintptr_t tmp2 = mScanner.Scan(p2);
			if (tmp2 != 0)
			{
				ORIG_DrawPrintText = (void*)ptr1;
				DevMsg(TAG "DrawPrintText backup found at %p\n", ORIG_DrawPrintText);
				*done = true;
				return;
			}
		}
		*done = false;
	};

	mScanner.Scan(p);
	patternContainer.Hook();

	auto loadTime =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	DevMsg(TAG "Done hooking in %dms\n", loadTime);
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

bool CheckCVars()
{
#ifndef OE
	auto icvar = GetCvarInterface();

#ifndef P2
	ConCommandBase* cmd = icvar->GetCommands();

	// Loops through the console variables and commands
	while (cmd != NULL)
	{
#else
	ICvar::Iterator iter(icvar);

	for (iter.SetFirst(); iter.IsValid(); iter.Next())
	{
		auto cmd = iter.Get();
#endif

		const char* name = cmd->GetName();
		// Reset any variables that have been marked to be reset for TASes
		if (!cmd->IsCommand() && name != NULL && cmd->IsFlagSet(FCVAR_SPT_HUD) && ((ConVar*)cmd)->GetBool())
			return true;

#ifndef P2
		cmd = cmd->GetNext();
#endif
	}
#endif
	return false;
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

		if (CheckCVars() || !whiteSpacesOnly(y_spt_hud_ent_info.GetString()))
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
		switch (y_spt_hud_drawing_function.GetInt()) \
		{ \
		case 0: \
		{ \
			surface->DrawSetTextPos(x, 2 + (fontTall + 2) * vertIndex); \
			surface->DrawPrintText(buffer, wcslen(buffer)); \
			++vertIndex; \
			break; \
		} \
		case 1: \
			if (ORIG_DrawSetTextPos && ORIG_DrawPrintText) \
			{ \
				ORIG_DrawSetTextPos(surface, 0, x, 2 + (fontTall + 2) * vertIndex); \
				((_DrawPrintText)ORIG_DrawPrintText)(surface, 0, buffer, wcslen(buffer), 0); \
				++vertIndex; \
				break; \
			} \
		case 2: \
			if (ORIG_DrawSetTextPos && ORIG_DrawPrintText) \
			{ \
				ORIG_DrawSetTextPos(surface, 0, x, 2 + (fontTall + 2) * vertIndex); \
				((_DrawPrintText2)ORIG_DrawPrintText)(surface, 0, buffer, wcslen(buffer), 0, 0); \
				++vertIndex; \
				break; \
			} \
		} \
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
	else if (screen)
		x = screen->width - 300 + 2;
	else
		// default to 300 if we dont have a screen
		x = 300;

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

	if (y_spt_hud_havok_velocity.GetBool())
	{
		switch (y_spt_hud_havok_velocity.GetInt())
		{
		case 3:
		{
			Vector* s = &(vphysicsDLL.PlayerHavokVel);
			DRAW_TRIPLEFLOAT(L"havok vel", s->x, s->y, s->z);
			break;
		}

		case 1:
		{
			float speed = vphysicsDLL.PlayerHavokVel.Length();
			DRAW_FLOAT(L"havok vel (xyz)", speed);
			break;
		}

		case 2:
		{
			float speed = vphysicsDLL.PlayerHavokVel.Length2D();
			DRAW_FLOAT(L"havok vel (xy)", speed);
			break;
		}
		}

		Vector* s = &(vphysicsDLL.PlayerHavokPos);
		DRAW_TRIPLEFLOAT(L"havok pos", s->x, s->y, s->z);
	}

	if (y_spt_hud_vec_velocity.GetBool())
	{
		Vector s = clientDLL.GetPlayerVecVelocity();
		DRAW_TRIPLEFLOAT(L"vec vel", s.x, s.y, s.z);
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

#ifdef SSDK2007
	if (y_spt_hud_isg.GetBool() && vphysicsDLL.isgFlagPtr)
	{
		DRAW_INT(L"isg", *(vphysicsDLL.isgFlagPtr));
	}
#endif
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