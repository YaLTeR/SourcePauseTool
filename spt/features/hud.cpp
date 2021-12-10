#include "stdafx.h"
#ifdef SSDK2007
#include <algorithm>
#include "..\feature.hpp"
#include "convar.hpp"
#include "interfaces.hpp"
#include "tier0\basetypes.h"
#include "vgui\ischeme.h"
#include "vguimatsurface\imatsystemsurface.h"
#include "..\utils\ent_utils.hpp"
#include "game_detection.hpp"
#include "..\utils\property_getter.hpp"
#include "..\vgui\vgui_utils.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "autojump.hpp"
#include "generic.hpp"
#include "isg.hpp"
#include "overlay.hpp"
#include "playerio.hpp"
#include "signals.hpp"
#include "..\overlay\portal_camera.hpp"
#include "..\cvars.hpp"
#include "tracing.hpp"

#undef max
#undef min
#include "string_utils.hpp"

typedef void(__fastcall* _StartDrawing)(void* thisptr, int edx);
typedef void(__fastcall* _FinishDrawing)(void* thisptr, int edx);
typedef void(__fastcall* _VGui_Paint)(void* thisptr, int edx, int mode);

ConVar y_spt_hud_velocity("y_spt_hud_velocity", "0", FCVAR_CHEAT, "Turns on the velocity hud.\n");
ConVar y_spt_hud_flags("y_spt_hud_flags", "0", FCVAR_CHEAT, "Turns on the flags hud.\n");
ConVar y_spt_hud_moveflags("y_spt_hud_moveflags", "0", FCVAR_CHEAT, "Turns on the move type hud.\n");
ConVar y_spt_hud_movecollideflags("y_spt_hud_movecollideflags", "0", FCVAR_CHEAT, "Turns on the move collide hud.\n");
ConVar y_spt_hud_collisionflags("y_spt_hud_collisionflags", "0", FCVAR_CHEAT, "Turns on the collision group hud.\n");
ConVar y_spt_hud_accel("y_spt_hud_accel", "0", FCVAR_CHEAT, "Turns on the acceleration hud.\n");
ConVar y_spt_hud_script_length("y_spt_hud_script_progress", "0", FCVAR_CHEAT, "Turns on the script progress hud.\n");
ConVar y_spt_hud_portal_bubble("y_spt_hud_portal_bubble", "0", FCVAR_CHEAT, "Turns on portal bubble index hud.\n");
ConVar y_spt_hud_decimals("y_spt_hud_decimals",
                          "2",
                          FCVAR_CHEAT,
                          "Determines the number of decimals in the SPT HUD.\n");
ConVar y_spt_hud_vars("y_spt_hud_vars", "0", FCVAR_CHEAT, "Turns on the movement vars HUD.\n");
ConVar y_spt_hud_ag_sg_tester("y_spt_hud_ag_sg_tester",
                              "0",
                              FCVAR_CHEAT,
                              "Tests if angle glitch will save glitch you.\n");
ConVar y_spt_hud_ent_info(
    "y_spt_hud_ent_info",
    "",
    FCVAR_CHEAT,
    "Display entity info on HUD. Format is \"[ent index],[prop regex],[prop regex],...,[prop regex];[ent index],...,[prop regex]\".\n");
ConVar y_spt_hud_left("y_spt_hud_left", "0", FCVAR_CHEAT, "When set to 1, displays SPT HUD on the left.\n");
ConVar y_spt_hud_isg("y_spt_hud_isg", "0", FCVAR_CHEAT, "Is the ISG flag set?\n");
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

// HUD stuff
class HUDFeature : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void Jump();
	void OnGround(bool ground);
	void NewTick();
	void DrawHUD(vrect_t* screen);
	void DrawCrosshair(vrect_t* screen);
	void CalculateAbhVel();

	static void __fastcall HOOKED_VGui_Paint(void* thisptr, int edx, int mode);

	_VGui_Paint ORIG_VGui_Paint = nullptr;
	_StartDrawing ORIG_StartDrawing = nullptr;
	_FinishDrawing ORIG_FinishDrawing = nullptr;

	int displayHop = 0;
	float loss = 0;
	float percentage = 0;

	bool velNotCalced;
	int lastHop = 0;
	float maxVel = 0;

	int sinceLanded = 0;
	ConVar* cl_showpos = nullptr;
	ConVar* cl_showfps = nullptr;
	vgui::HFont font = 0;
	vgui::HFont hopsFont = 0;

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

static HUDFeature spt_hud;

const int INDEX_MASK = MAX_EDICTS - 1;

bool HUDFeature::ShouldLoadFeature()
{
	return true;
}

void HUDFeature::InitHooks()
{
	FIND_PATTERN(vguimatsurface, StartDrawing);
	FIND_PATTERN(vguimatsurface, FinishDrawing);
	HOOK_FUNCTION(engine, VGui_Paint);
}

void HUDFeature::LoadFeature()
{
	currentVel.Init(0, 0, 0);
	previousVel.Init(0, 0, 0);
	cl_showpos = interfaces::g_pCVar->FindVar("cl_showpos");
	cl_showfps = interfaces::g_pCVar->FindVar("cl_showfps");
	auto scheme = vgui::GetScheme();
	font = scheme->GetFont("DefaultFixedOutline", false);
	hopsFont = scheme->GetFont("Trebuchet24", false);

	if (ORIG_VGui_Paint && ORIG_FinishDrawing && ORIG_StartDrawing)
	{
		// cba to make this detection more granular, this feature is a garbage fire that should be refactored anyway
		InitConcommandBase(y_spt_hud_velocity);
		InitConcommandBase(y_spt_hud_flags);
		InitConcommandBase(y_spt_hud_moveflags);
		InitConcommandBase(y_spt_hud_movecollideflags);
		InitConcommandBase(y_spt_hud_collisionflags);
		InitConcommandBase(y_spt_hud_accel);
		InitConcommandBase(y_spt_hud_script_length);
		InitConcommandBase(y_spt_hud_portal_bubble);
		InitConcommandBase(y_spt_hud_decimals);
		InitConcommandBase(y_spt_hud_vars);

		if (utils::DoesGameLookLikePortal())
		{
			InitConcommandBase(y_spt_hud_ag_sg_tester);
			InitConcommandBase(y_spt_hud_isg);
		}

		InitConcommandBase(y_spt_hud_ent_info);
		InitConcommandBase(y_spt_hud_left);
		InitConcommandBase(y_spt_hud_hops);
		InitConcommandBase(y_spt_hud_hops_x);
		InitConcommandBase(y_spt_hud_hops_y);
		InitConcommandBase(y_spt_hud_velocity_angles);
		InitConcommandBase(_y_spt_overlay_crosshair_size);
		InitConcommandBase(_y_spt_overlay_crosshair_thickness);
		InitConcommandBase(_y_spt_overlay_crosshair_color);

		AdjustAngles.Connect(this, &HUDFeature::NewTick);
		OngroundSignal.Connect(this, &HUDFeature::OnGround);
		JumpSignal.Connect(this, &HUDFeature::Jump);
	}
}

void HUDFeature::UnloadFeature() {}

void HUDFeature::Jump()
{
	if (!y_spt_hud_hops.GetBool())
		return;

	if (sinceLanded == 0)
		CalculateAbhVel();

	velNotCalced = true;
	lastHop = sinceLanded;
}

void HUDFeature::OnGround(bool onground)
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

			auto vel = spt_playerio.GetPlayerVelocity().Length2D();
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

void HUDFeature::NewTick()
{
	previousVel = currentVel;
}

void HUDFeature::DrawHUD(vrect_t* screen)
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

		if (y_spt_hud_velocity.GetBool() || y_spt_hud_flags.GetBool() || y_spt_hud_moveflags.GetBool()
		    || y_spt_hud_movecollideflags.GetBool() || y_spt_hud_collisionflags.GetBool()
		    || y_spt_hud_script_length.GetBool() || y_spt_hud_accel.GetBool() || y_spt_hud_vars.GetBool()
		    || y_spt_hud_portal_bubble.GetBool() || y_spt_hud_ag_sg_tester.GetBool()
		    || !whiteSpacesOnly(y_spt_hud_ent_info.GetString()) || y_spt_hud_oob.GetBool()
		    || y_spt_hud_velocity_angles.GetBool() || y_spt_hud_isg.GetBool())
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

void HUDFeature::DrawCrosshair(vrect_t* screen)
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

void HUDFeature::CalculateAbhVel()
{
	auto vel = spt_playerio.GetPlayerVelocity().Length2D();
	auto ducked = spt_playerio.GetFlagsDucking();
	auto sprinting = utils::GetProperty<bool>(0, "m_fIsSprinting");
	auto vars = spt_playerio.GetMovementVars();

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

void __fastcall HUDFeature::HOOKED_VGui_Paint(void* thisptr, int edx, int mode)
{
#ifndef OE
	if (mode == 2 && !spt_overlay.renderingOverlay)
	{
		spt_hud.DrawHUD((vrect_t*)spt_overlay.screenRect);
	}

	if (spt_overlay.renderingOverlay)
		spt_hud.DrawCrosshair((vrect_t*)spt_overlay.screenRect);

#endif

	spt_hud.ORIG_VGui_Paint(thisptr, edx, mode);
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

void HUDFeature::DrawHopHud(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface)
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

void HUDFeature::DrawTopHUD(vrect_t* screen, vgui::IScheme* scheme, IMatSystemSurface* surface)
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
	currentVel = spt_playerio.GetPlayerVelocity();
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
		int flags = spt_playerio.GetPlayerFlags();
		DRAW_FLAGS(NULL, FLAGS, flags, false);
	}

	if (y_spt_hud_moveflags.GetBool())
	{
		int flags = spt_playerio.GetPlayerMoveType();
		DRAW_FLAGS(L"Move type", MOVETYPE_FLAGS, flags, true);
	}

	if (y_spt_hud_collisionflags.GetBool())
	{
		int flags = spt_playerio.GetPlayerCollisionGroup();
		DRAW_FLAGS(L"Collision group", COLLISION_GROUPS, flags, true);
	}

	if (y_spt_hud_movecollideflags.GetBool())
	{
		int flags = spt_playerio.GetPlayerMoveCollide();
		DRAW_FLAGS(L"Move collide", MOVECOLLIDE_FLAGS, flags, true);
	}

	if (y_spt_hud_vars.GetBool() && utils::playerEntityAvailable())
	{
		auto vars = spt_playerio.GetMovementVars();
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
		Vector v = spt_playerio.GetPlayerEyePos();
		QAngle q;

		std::wstring result = calculateWillAGSG(v, q);
		swprintf_s(buffer, BUFFER_SIZE, L"ag sg: %s", result.c_str());
		DRAW();
	}

	if (y_spt_hud_oob.GetBool())
	{
		Vector v = spt_generic.GetCameraOrigin();
		trace_t tr;
		Strafe::Trace(tr, v, v + Vector(1, 1, 1));

		bool oob = spt_tracing.ORIG_CEngineTrace__PointOutsideWorld(nullptr, 0, v) && !tr.startsolid;
		swprintf_s(buffer, BUFFER_SIZE, L"oob: %d", oob);
		DRAW();
	}

#ifdef SSDK2007
	if (y_spt_hud_isg.GetBool())
	{
		DRAW_INT(L"isg", IsISGActive());
	}
#endif
}

void HUDFeature::DrawFlagsHud(bool mutuallyExclusiveFlags,
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

void HUDFeature::DrawSingleFloat(int& vertIndex,
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

void HUDFeature::DrawSingleInt(int& vertIndex,
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

void HUDFeature::DrawTripleFloat(int& vertIndex,
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

void HUDFeature::DrawSingleString(int& vertIndex,
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