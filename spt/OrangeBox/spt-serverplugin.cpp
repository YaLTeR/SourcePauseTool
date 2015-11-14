#include <chrono>
#include <sstream>

#include "spt-serverplugin.hpp"
#include "modules.hpp"
#include "cvars.hpp"
#include "..\sptlib-wrapper.hpp"
#include <SPTLib\Hooks.hpp>

#include "cdll_int.h"
#include "engine\iserverplugin.h"
#include "tier2\tier2.h"

#include "tier0\memdbgoff.h" // YaLTeR - switch off the memory debugging.

// useful helper func
inline bool FStrEq( const char *sz1, const char *sz2 )
{
	return(Q_stricmp( sz1, sz2 ) == 0);
}

// Interfaces from the engine
IVEngineClient *engine = nullptr;
ICvar *icvar = nullptr;

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
		icvar->RegisterConCommandBase(pCommand);
		return true;
	}

};

CPluginConVarAccessor g_ConVarAccessor;
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

//
// The plugin is a static singleton that is exported as an interface
//
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CSourcePauseTool, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_SourcePauseTool );

bool CSourcePauseTool::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	auto startTime = std::chrono::high_resolution_clock::now();

	ConnectTier1Libraries(&interfaceFactory, 1);

#if !defined( OE )
	ConVar_Register(0);
#endif

	engine = (IVEngineClient*)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	if (!engine)
	{
		DevWarning("SPT: Failed to get the IVEngineClient interface.\n");
		Warning("SPT: y_spt_afterframes has no effect.\n");
		Warning("SPT: _y_spt_setpitch and _y_spt_setyaw have no effect.\n");
		Warning("SPT: _y_spt_pitchspeed and _y_spt_yawspeed have no effect.\n");
	}

#if defined( OE )
	icvar = (ICvar*)interfaceFactory(VENGINE_CVAR_INTERFACE_VERSION, NULL);
#else
	icvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, NULL);
#endif

	if (!icvar)
	{
		DevWarning("SPT: Failed to get the ICvar interface.\n");
		Warning("SPT: Could not register any CVars and ConCommands.\n");
		Warning("SPT: y_spt_cvar has no effect.\n");
	}
#if defined( OE )
	else
	{
		ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);

		//auto c = icvar->FindVar("default_fov");
		//if (!c)
		//{
		//	Warning("SPT: Could not find default_fov.\n");
		//}
		//else
		//{
		//	c->InstallChangeCallback(DefaultFOVChangeCallback);
		//}

		_viewmodel_fov = icvar->FindVar("viewmodel_fov");
		if (!_viewmodel_fov)
		{
			DevWarning("SPT: Could not find viewmodel_fov.\n");
			Warning("SPT: _y_spt_force_90fov has no effect.\n");
		}
	}
#endif

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
	out << "SourcePauseTool v" SPT_VERSION " was loaded in " << loadTime << "ms.\n";

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

CON_COMMAND(_y_spt_afterframes_reset, "Reset the afterframes queue.")
{
	clientDLL.ResetAfterframesQueue();
}

CON_COMMAND(y_spt_cvar, "CVar manipulation.")
{
	if (!engine || !icvar)
		return;

#if defined( OE )
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar *cvar = icvar->FindVar(args.Arg(1));
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
	if (!engine || !icvar)
		return;

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar *cvar = icvar->FindVar(args.Arg(1));
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
	if (!engine)
		return;
	if (engine)
		serverDLL.StartTimer();
		ConColorMsg( Color(0, 255, 0, 255), "[SPT] Starting timer." );
}

CON_COMMAND(y_spt_timer_stop, "Stops the SPT timer and prints the current time.")
{
	if (!engine)
		return;
	if (engine)
		serverDLL.StopTimer();
		ConColorMsg( Color(0, 255, 0, 255), "[SPT] Timer has been stopped" );
		ConColorMsg( Color(0, 255, 0, 255), "[SPT] Current time (in ticks): %u\n", serverDLL.GetTicksPassed() );
}

CON_COMMAND(y_spt_timer_reset, "Stops and resets the SPT timer.")
{
	if (!engine)
		return;
	if (engine)
		serverDLL.ResetTimer();
		ConColorMsg( Color(0, 255, 0, 255), "[SPT] Timer has been reset." );
}

CON_COMMAND(y_spt_timer_print, "Prints the current time of the SPT timer.")
{
	if (!engine)
		return;
	if (engine)
		ConColorMsg( Color(0, 255, 0, 255), "[SPT] Current time (in ticks): %u\n", serverDLL.GetTicksPassed() );
}

CON_COMMAND(ps_help_abh, "Prints a short explanation of ABH.")
{	
	if (!engine)
		return;
	if (engine)
		ConColorMsg( Color(128, 255,128, 255), "Accelerated Back Hopping (ABH for short) is a Glitch, and it's the main method of transportation on OrangeBox Engine-based games.\n");
		ConColorMsg( Color(255, 255, 255, 255), "ABH replaces Bunnyhopping, which was fixed in the OrangeBox Engine.\n");
		ConColorMsg( Color(255, 255, 255, 255), "It is also significantly faster than bunnyhopping.\n");
		ConColorMsg( Color(255, 255, 255, 255), "\n");
		ConColorMsg( Color(255, 255, 255, 255), "To perform this trick, jump forward, turn around in the air and jump right as you land.\n");
		ConColorMsg( Color(255, 255, 255, 255), "It's generally recommended not to hold S or W(or your respective keys for moving forwards and back) while ABHing. Binding jump to mouse wheel or using a jumping script is recommended as it makes timing jumps a lot easier.\n");
		ConColorMsg( Color(128, 255, 128, 255), "See Variants : AFH or ASH");
}

CON_COMMAND(ps_help_sg, "Prints a short explanation of Save Glitch.")
{
	if (!engine)
		return;
	if (engine)
		ConColorMsg( Color(128, 255, 128, 255), "The Save Glitch is a Glitch in Portal which moves your collision and the point from where you shoot portals to a different location than your view. Simplified you can say that the Save Glitch is a permanent form of the Edge Glitch.\n");
		ConColorMsg( Color(255, 255, 255, 255), "\n");
		ConColorMsg( Color(255, 255, 255, 255), "It is performed by saving and loading while being Edge Glitched. Then you can walk away from the portal, you are now in the Save Glitched state.\n");
		ConColorMsg( Color(255, 255, 255, 255), "Your camera will only collide with certain objects, it will not collide with the ground. Instead the collision is detected from a location relative to your current position and the portals.\n");
		ConColorMsg( Color(255, 255, 255, 255), "You will have to follow a certain path to not fall through the floor. Also, you have to follow some rules if you want the Save Glitch to not be cancelled.\n");
		ConColorMsg( Color(255, 255, 255, 255), "\n");
		ConColorMsg( Color(255, 255, 255, 255), "After the load your camera still remains at the point where you were before. However, the point from where your portalgun shoots the portals (your ghost body) is at the other portal. If you move your camera, your ghost body will move relative to the other portal. Also it will collide with the ground, while your camera will not. This means that you can walk through walls and doors with your camera, but you will also fall through the floor if your ghost body will loose contact with the ground. However, your camera will still collide with some objects like door frames or cube dispensers.\n");
}
