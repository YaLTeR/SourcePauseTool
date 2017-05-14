#include <chrono>
#include <sstream>

#include "spt-serverplugin.hpp"
#include "modules.hpp"
#include "cvars.hpp"
#include "..\sptlib-wrapper.hpp"
#include <SPTLib\Hooks.hpp>

#include "cdll_int.h"
#include "engine\iserverplugin.h"
#include "eiface.h"
#include "tier2\tier2.h"

#if !OE && !SSDK2013
#include "mathlib\vmatrix.h"
#endif

#include "tier0\memdbgoff.h" // YaLTeR - switch off the memory debugging.

using namespace std::literals;

// useful helper func
inline bool FStrEq( const char *sz1, const char *sz2 )
{
	return(Q_stricmp( sz1, sz2 ) == 0);
}

// Interfaces from the engine
IVEngineClient *engine = nullptr;
IVEngineServer *engine_server = nullptr;
void *gm = nullptr;

// For OE CVar and ConCommand registering.
#if defined( OE )
class CPluginConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase(ConCommandBase *pCommand)
	{
		pCommand->AddFlags(FCVAR_PLUGIN);

		// Unlink from plugin only list
		pCommand->SetNext(0);

		// Link to engine's list instead
		g_pCVar->RegisterConCommandBase(pCommand);
		return true;
	}

};

CPluginConVarAccessor g_ConVarAccessor;

// OE: For correct linking in VS 2015.
int (WINAPIV * __vsnprintf) (char*, size_t, const char*, va_list) = _vsnprintf;
#endif

void CallServerCommand(const char* cmd)
{
	if (engine)
		engine->ClientCmd(cmd);
}

void GetViewAngles(float viewangles[3])
{
	if (engine)
	{
		QAngle va;
		engine->GetViewAngles(va);

		viewangles[0] = va.x;
		viewangles[1] = va.y;
		viewangles[2] = va.z;
	}
}

void SetViewAngles(const float viewangles[3])
{
	if (engine)
	{
		QAngle va(viewangles[0], viewangles[1], viewangles[2]);
		engine->SetViewAngles(va);
	}
}

void DefaultFOVChangeCallback(ConVar *var, char const *pOldString)
{
	if (FStrEq(var->GetString(), "75") && FStrEq(pOldString, "90"))
	{
		//Msg("Attempted to change default_fov from 90 to 75. Preventing.\n");
		var->SetValue("90");
	}
}

IServerUnknown* GetServerPlayer()
{
	if (!engine_server)
		return nullptr;

	auto edict = engine_server->PEntityOfEntIndex(1);
	if (!edict)
		return nullptr;

	return edict->GetUnknown();
}

bool DoesGameLookLikePortal()
{
#ifndef OE
	if (g_pCVar) {
		if (g_pCVar->FindCommand("upgrade_portalgun"))
			return true;

		return false;
	}

	if (engine) {
		auto game_dir = engine->GetGameDirectory();
		return (GetFileName(string_converter.from_bytes(game_dir)) == L"portal"s);
	}
#endif

	return false;
}

bool FoundEngineServer()
{
	return (engine_server != nullptr);
}

//
// The plugin is a static singleton that is exported as an interface
//
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CSourcePauseTool, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_SourcePauseTool );

bool CSourcePauseTool::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	auto startTime = std::chrono::high_resolution_clock::now();

	ConnectTier1Libraries(&interfaceFactory, 1);

	gm = gameServerFactory(INTERFACENAME_GAMEMOVEMENT, NULL);
	if (gm) {
		DevMsg("SPT: Found IGameMovement at %p.\n", gm);
	} else {
		DevWarning("SPT: Could not find IGameMovement.\n");
		DevWarning("SPT: ProcessMovement logging with tas_log is unavaliable.\n");
	}

	if (!g_pCVar)
	{
		DevWarning("SPT: Failed to get the ICvar interface.\n");
		Warning("SPT: Could not register any CVars and ConCommands.\n");
		Warning("SPT: y_spt_cvar has no effect.\n");
	}
#if defined( OE )
	else
	{
		ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);

		_viewmodel_fov = g_pCVar->FindVar("viewmodel_fov");
		if (!_viewmodel_fov)
		{
			DevWarning("SPT: Could not find viewmodel_fov.\n");
			Warning("SPT: _y_spt_force_90fov has no effect.\n");
		}
	}
#else
	else
	{
		_sv_airaccelerate = g_pCVar->FindVar("sv_airaccelerate");
		_sv_accelerate = g_pCVar->FindVar("sv_accelerate");
		_sv_friction = g_pCVar->FindVar("sv_friction");
		_sv_maxspeed = g_pCVar->FindVar("sv_maxspeed");
		_sv_stopspeed = g_pCVar->FindVar("sv_stopspeed");
	}

	ConVar_Register(0);
#endif

	engine = (IVEngineClient*)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	if (!engine)
	{
		DevWarning("SPT: Failed to get the IVEngineClient interface.\n");
		Warning("SPT: y_spt_afterframes has no effect.\n");
		Warning("SPT: _y_spt_setpitch and _y_spt_setyaw have no effect.\n");
		Warning("SPT: _y_spt_pitchspeed and _y_spt_yawspeed have no effect.\n");
		Warning("SPT: y_spt_stucksave has no effect.\n");
	}

	if (DoesGameLookLikePortal())
	{
		DevMsg("SPT: This game looks like portal. Setting the tas_* cvars appropriately.\n");

		tas_force_airaccelerate.SetValue(15);
		tas_force_wishspeed_cap.SetValue(60);
		tas_reset_surface_friction.SetValue(0);
	}

	engine_server = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	if (!engine_server)
	{
		DevWarning("SPT: Failed to get the IVEngineServer interface.\n");
	}

	EngineConCmd = CallServerCommand;
	EngineGetViewAngles = GetViewAngles;
	EngineSetViewAngles = SetViewAngles;

	_EngineMsg = Msg;
	_EngineDevMsg = DevMsg;
	_EngineWarning = Warning;
	_EngineDevWarning = DevWarning;

	Hooks::getInstance().AddToHookedModules(&engineDLL);
	Hooks::getInstance().AddToHookedModules(&clientDLL);
	Hooks::getInstance().AddToHookedModules(&serverDLL);

	Hooks::getInstance().Init();

	auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
	std::ostringstream out;
	out << "SourcePauseTool version " SPT_VERSION " was loaded in " << loadTime << "ms.\n";

	Msg("%s", std::string(out.str()).c_str());

	return true;
}

void CSourcePauseTool::Unload( void )
{
	Hooks::getInstance().Free();

#if !defined( OE )
	ConVar_Unregister();
#endif

	DisconnectTier1Libraries();
}

const char *CSourcePauseTool::GetPluginDescription( void )
{
	return "SourcePauseTool v" SPT_VERSION ", Ivan \"YaLTeR\" Molodetskikh";
}

CON_COMMAND(_y_spt_afterframes_wait, "Delays the afterframes queue. Usage: _y_spt_afterframes_wait <delay>")
{
	if (!engine)
		return;

	if (args.ArgC() != 2)
	{
		Msg("Usage: _y_spt_afterframes_wait <delay>\n");
		return;
	}

	int delay = std::stoi(args.Arg(1));

	clientDLL.DelayAfterframesQueue(delay);
}

CON_COMMAND(_y_spt_afterframes, "Add a command into an afterframes queue. Usage: _y_spt_afterframes <count> <command>")
{
	if (!engine)
		return;

#if defined( OE )
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() != 3)
	{
		Msg("Usage: _y_spt_afterframes <count> <command>\n");
		return;
	}

	afterframes_entry_t entry;

	std::istringstream ss(args.Arg(1));
	ss >> entry.framesLeft;
	entry.command.assign(args.Arg(2));

	clientDLL.AddIntoAfterframesQueue(entry);
}

#if !defined( OE )
CON_COMMAND(_y_spt_afterframes2, "Add everything after count as a command into the queue. Do not insert the command in quotes. Usage: _y_spt_afterframes2 <count> <command>")
{
	if (!engine)
		return;

	if (args.ArgC() < 3)
	{
		Msg("Usage: _y_spt_afterframes2 <count> <command>\n");
		return;
	}

	afterframes_entry_t entry;

	std::istringstream ss(args.Arg(1));
	ss >> entry.framesLeft;
	const char *cmd = args.ArgS() + strlen(args.Arg(1)) + 1;
	entry.command.assign(cmd);

	clientDLL.AddIntoAfterframesQueue(entry);
}
#endif

CON_COMMAND(_y_spt_afterframes_await_load, "Pause reading from the afterframes queue until the next load or changelevel. Useful for writing scripts spanning multiple maps or save-load segments.")
{
	clientDLL.PauseAfterframesQueue();
}

CON_COMMAND(_y_spt_afterframes_reset, "Reset the afterframes queue.")
{
	clientDLL.ResetAfterframesQueue();
}

CON_COMMAND(y_spt_cvar, "CVar manipulation.")
{
	if (!engine || !g_pCVar)
		return;

#if defined( OE )
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar *cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	if (args.ArgC() == 2)
	{
		Msg("\"%s\" = \"%s\"\n", cvar->GetName(), cvar->GetString(), cvar->GetHelpText());
		Msg("Default: %s\n", cvar->GetDefault());

		float val;
		if (cvar->GetMin(val))
		{
			Msg("Min: %f\n", val);
		}

		if (cvar->GetMax(val))
		{
			Msg("Max: %f\n", val);
		}
		
		const char *helpText = cvar->GetHelpText();
		if (helpText[0] != '\0')
			Msg("- %s\n", cvar->GetHelpText());

		return;
	}

	const char *value = args.Arg(2);
	cvar->SetValue(value);
}

#if !defined( OE )
CON_COMMAND(y_spt_cvar2, "CVar manipulation, sets the CVar value to the rest of the argument string.")
{
	if (!engine || !g_pCVar)
		return;

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar *cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	if (args.ArgC() == 2)
	{
		Msg("\"%s\" = \"%s\"\n", cvar->GetName(), cvar->GetString(), cvar->GetHelpText());
		Msg("Default: %s\n", cvar->GetDefault());

		float val;
		if (cvar->GetMin(val))
		{
			Msg("Min: %f\n", val);
		}

		if (cvar->GetMax(val))
		{
			Msg("Max: %f\n", val);
		}
		
		const char *helpText = cvar->GetHelpText();
		if (helpText[0] != '\0')
			Msg("- %s\n", cvar->GetHelpText());

		return;
	}

	const char *value = args.ArgS() + strlen(args.Arg(1)) + 1;
	cvar->SetValue(value);
}
#endif

#if defined( OE )
static void DuckspamDown()
#else
static void DuckspamDown(const CCommand &args)
#endif
{
	clientDLL.EnableDuckspam();
}
static ConCommand DuckspamDown_Command("+y_spt_duckspam", DuckspamDown, "Enables the duckspam.");

#if defined( OE )
static void DuckspamUp()
#else
static void DuckspamUp(const CCommand &args)
#endif
{
	clientDLL.DisableDuckspam();
}
static ConCommand DuckspamUp_Command("-y_spt_duckspam", DuckspamUp, "Disables the duckspam.");

CON_COMMAND(_y_spt_setpitch, "Sets the pitch. Usage: _y_spt_setpitch <pitch>")
{
	if (!engine)
		return;

#if defined( OE )
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() != 2)
	{
		Msg("Usage: _y_spt_setpitch <pitch>\n");
		return;
	}

	clientDLL.SetPitch( atof(args.Arg(1)) );
}

CON_COMMAND(_y_spt_setyaw, "Sets the yaw. Usage: _y_spt_setyaw <yaw>")
{
	if (!engine)
		return;

#if defined( OE )
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() != 2)
	{
		Msg("Usage: _y_spt_setyaw <yaw>\n");
		return;
	}

	clientDLL.SetYaw( atof(args.Arg(1)) );
}

CON_COMMAND(_y_spt_setangles, "Sets the angles. Usage: _y_spt_setangles <pitch> <yaw>")
{
	if (!engine)
		return;

#if defined( OE )
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() != 3)
	{
		Msg("Usage: _y_spt_setangles <pitch> <yaw>\n");
		return;
	}
	
	clientDLL.SetPitch( atof(args.Arg(1)) );
	clientDLL.SetYaw( atof(args.Arg(2)) );
}

CON_COMMAND(_y_spt_getvel, "Gets the last velocity of the player.")
{
	const Vector vel = serverDLL.GetLastVelocity();

	Warning("Velocity (x, y, z): %f %f %f\n", vel.x, vel.y, vel.z);
	Warning("Velocity (xy): %f\n", vel.Length2D());
}

CON_COMMAND(_y_spt_getangles, "Gets the view angles of the player.")
{
	if (!engine)
		return;

	QAngle va;
	engine->GetViewAngles(va);
	
	Warning("View Angle (x): %f\n", va.x);
	Warning("View Angle (y): %f\n", va.y);
	Warning("View Angle (z): %f\n", va.z);
	Warning("View Angle (x, y, z): %f %f %f\n", va.x, va.y, va.z);
}

CON_COMMAND(_y_spt_tickrate, "Get or set the tickrate. Usage: _y_spt_tickrate [tickrate]")
{
	if (!engine)
		return;

#if defined( OE )
	ArgsWrapper args(engine);
#endif

	switch (args.ArgC())
	{
	case 1:
		Msg("Current tickrate: %f\n", engineDLL.GetTickrate());
		break;

	case 2:
		engineDLL.SetTickrate( atof(args.Arg(1)) );
		break;

	default:
		Msg("Usage: _y_spt_tickrate [tickrate]\n");
	}
}

CON_COMMAND(y_spt_timer_start, "Starts the SPT timer.")
{
	serverDLL.StartTimer();
}

CON_COMMAND(y_spt_timer_stop, "Stops the SPT timer and prints the current time.")
{
	serverDLL.StopTimer();
	Warning("Current time (in ticks): %u\n", serverDLL.GetTicksPassed());
}

CON_COMMAND(y_spt_timer_reset, "Stops and resets the SPT timer.")
{
	serverDLL.ResetTimer();
}

CON_COMMAND(y_spt_timer_print, "Prints the current time of the SPT timer.")
{
	Warning("Current time (in ticks): %u\n", serverDLL.GetTicksPassed());
}

#if !OE && !SSDK2013
// TODO: remove fixed offsets.

#undef GetClassName
CON_COMMAND(y_spt_find_portals, "Yes")
{
	if (clientDLL.offServerAbsOrigin == 0)
		return;

	for (int i = 0; i < MAX_EDICTS; ++i) {
		auto ent = engine_server->PEntityOfEntIndex(i);

		if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal")) {
			auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + clientDLL.offServerAbsOrigin);

			Msg("SPT: There's a portal with index %d at %.8f %.8f %.8f.\n", i, origin.x, origin.y, origin.z);
		}
	}
}

void calculate_offset_player_pos(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	// Here we make sure that the eye position and the eye angles match up.
	const Vector view_offset(0, 0, 64);
	auto& player_origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + clientDLL.offServerAbsOrigin);
	auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

	auto& matrix = *reinterpret_cast<VMatrix*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + 1072);

	auto eye_origin = player_origin + view_offset;
	auto new_eye_origin = matrix * eye_origin;
	new_player_origin = new_eye_origin - view_offset;

	new_player_angles = TransformAnglesToWorldSpace(player_angles, matrix.As3x4());
	new_player_angles.x = AngleNormalizePositive(new_player_angles.x);
	new_player_angles.y = AngleNormalizePositive(new_player_angles.y);
	new_player_angles.z = AngleNormalizePositive(new_player_angles.z);
}

CON_COMMAND(y_spt_calc_relative_position, "y_spt_calc_relative_position <index of the save glitch portal | \"blue\" | \"orange\"> [1 if you want to teleport there instead of just printing]")
{
	if (args.ArgC() != 2 && args.ArgC() != 3) {
		Msg("Usage: y_spt_calc_relative_position <index of the save glitch portal | \"blue\" | \"orange\"> [1 if you want to teleport there instead of just printing]\n");
		return;
	}

	if (clientDLL.offServerAbsOrigin == 0) {
		Warning("Could not find the required offset in the client DLL.\n");
		return;
	}

	int portal_index = atoi(args.Arg(1));

	if (!strcmp(args.Arg(1), "blue") || !strcmp(args.Arg(1), "orange")) {
		std::vector<int> indices;

		for (int i = 0; i < MAX_EDICTS; ++i) {
			auto ent = engine_server->PEntityOfEntIndex(i);

			if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal")) {
				auto is_orange_portal = *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + 1137);

				if (is_orange_portal == (args.Arg(1)[0] == 'o'))
					indices.push_back(i);
			}
		}

		if (indices.size() > 1) {
			Msg("There are multiple %s portals, please use the index:\n", args.Arg(1));

			for (auto i : indices) {
				auto ent = engine_server->PEntityOfEntIndex(i);
				auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + clientDLL.offServerAbsOrigin);

				Msg("%d located at %.8f %.8f %.8f\n", i, origin.x, origin.y, origin.z);
			}

			return;
		} else if (indices.size() == 0) {
			Msg("There are no %s portals.\n", args.Arg(1));
			return;
		} else {
			portal_index = indices[0];
		}
	}

	auto portal = engine_server->PEntityOfEntIndex(portal_index);
	if (!portal || portal->IsFree() || strcmp(portal->GetClassName(), "prop_portal") != 0) {
		Warning("The portal index is invalid.\n");
		return;
	}

	Vector new_player_origin;
	QAngle new_player_angles;
	calculate_offset_player_pos(portal, new_player_origin, new_player_angles);

	if (args.ArgC() == 2) {
		Msg(
			"setpos %.8f %.8f %.8f;setang %.8f %.8f %.8f\n",
			new_player_origin.x, new_player_origin.y, new_player_origin.z,
			new_player_angles.x, new_player_angles.y, new_player_angles.z
		);
	} else {
		char buf[256];
		snprintf(buf, ARRAYSIZE(buf), "setpos %.8f %.8f %.8f;setang %.8f %.8f %.8f\n",
			new_player_origin.x, new_player_origin.y, new_player_origin.z,
			new_player_angles.x, new_player_angles.y, new_player_angles.z
		);

		engine->ClientCmd(buf);
	}
}

#undef max
void setang_exact(const QAngle& angles)
{
	auto player = GetServerPlayer();
	auto teleport = reinterpret_cast<void (__fastcall *)(void*, int, const Vector*, const QAngle*, const Vector*)>((*reinterpret_cast<uintptr_t**>(player))[105]);

	teleport(player, 0, nullptr, &angles, nullptr);
	serverDLL.SnapEyeAngles(player, 0, angles);
}

// Trace as if we were firing a Portal with the given viewangles, return the squared distance to the resulting point and the normal.
double trace_fire_portal(QAngle angles, Vector& normal)
{
	setang_exact(angles);

	serverDLL.FirePortal(serverDLL.GetActiveWeapon(GetServerPlayer()), 0, false, nullptr, true);

	normal = serverDLL.lastTraceFirePortalNormal;
	return serverDLL.lastTraceFirePortalDistanceSq;
}

QAngle firstAngle;
bool firstInvocation = true;

CON_COMMAND(y_spt_find_seam_shot, "y_spt_find_seam_shot [<pitch1> <yaw1> <pitch2> <yaw2> <epsilon>] - tries to find a seam shot on a \"line\" between viewangles (pitch1; yaw1) and (pitch2; yaw2) with binary search. Decreasing epsilon will result in more viewangles checked. A default value is 0.00001. If no arguments are given, first invocation selects the first point, second invocation selects the second point and searches between them.")
{
	QAngle a, b;
	double eps = 0.00001 * 0.00001;

	if (args.ArgC() == 1) {
		if (firstInvocation) {
			engine->GetViewAngles(firstAngle);
			firstInvocation = !firstInvocation;

			Msg("First point set.\n");
			return;
		} else {
			firstInvocation = !firstInvocation;

			a = firstAngle;
			engine->GetViewAngles(b);
		}
	} else {
		if (args.ArgC() != 5 && args.ArgC() != 6) {
			Msg("Usage: y_spt_find_seam_shot <pitch1> <yaw1> <pitch2> <yaw2> <epsilon> - tries to find a seam shot on a \"line\" between viewangles (pitch1; yaw1) and (pitch2; yaw2) with binary search. Decreasing epsilon will result in more viewangles checked. A default value is 0.00001. If no arguments are given, first invocation selects the first point, second invocation selects the second point and searches between them.\n");
			return;
		}

		a = QAngle(atof(args.Arg(1)), atof(args.Arg(2)), 0);
		b = QAngle(atof(args.Arg(3)), atof(args.Arg(4)), 0);
		eps = (args.ArgC() == 5) ? eps : std::pow(atof(args.Arg(5)), 2);
	}

	if (!serverDLL.GetActiveWeapon(GetServerPlayer())) {
		Msg("You need to be holding a portal gun.\n");
		return;
	}

	Vector a_normal;
	const auto distance = std::max(trace_fire_portal(a, a_normal), trace_fire_portal(b, Vector()));

	// If our trace had a distance greater than the a or b distance by this amount, treat it as a seam shot.
	constexpr double GOOD_DISTANCE_DIFFERENCE = 50.0 * 50.0;

	QAngle test = a + (b - a) / 2;
	double difference;

	do {
		Vector test_normal;

		if (trace_fire_portal(test, test_normal) - distance > GOOD_DISTANCE_DIFFERENCE) {
			Msg("Found a seam shot at setang %.8f %.8f 0\n", test.x, test.y);
			return;
		}

		if (test_normal == a_normal) {
			a = test;
			test += (b - a) / 2;

			difference = (test - a).LengthSqr();
		} else {
			b = test;
			test += (a - b) / 2;

			difference = (test - b).LengthSqr();
		}

		Msg("Difference: %.8f\n", std::sqrt(difference));
	} while (difference > eps);

	Msg("Could not find a seam shot. Best guess: setang %.8f %.8f 0\n", test.x, test.y);
}
#endif
