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
	if (g_pCVar) {
		if (g_pCVar->FindCommand("upgrade_portalgun"))
			return true;

		return false;
	}

	if (engine) {
		auto game_dir = engine->GetGameDirectory();
		return (GetFileName(string_converter.from_bytes(game_dir)) == L"portal"s);
	}

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
	clientDLL.SetAfterframesPauseState(2);
}

CON_COMMAND(_y_spt_afterframes_await_activate, "Pause reading from the afterframes queue until the next server activation. Similar to await_load except occurs early during the load. May be useful for recording a demo, or for specific rare occasions where early control is required.")
{
	clientDLL.SetAfterframesPauseState(1);
}

CON_COMMAND(_y_spt_afterframes_reset, "Reset the afterframes queue.")
{
	clientDLL.ResetAfterframesQueue();
	clientDLL.SetAfterframesPauseState(0);
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
