#include <chrono>
#include <sstream>

#include "spt-serverplugin.hpp"
#include "../spt.hpp"
#include "../hooks.hpp"

#include "cdll_int.h"
#include "engine/iserverplugin.h"
#include "tier2/tier2.h"

#include "tier0/memdbgoff.h" // YaLTeR - switch off the memory debugging.

// useful helper func
inline bool FStrEq( const char *sz1, const char *sz2 )
{
	return(Q_stricmp( sz1, sz2 ) == 0);
}

// Interfaces from the engine
IVEngineClient *engine = nullptr;
ICvar *icvar = nullptr;

void CallServerCommand(const char* cmd)
{
	if (engine)
		engine->ClientCmd(cmd);
}

//
// The plugin is a static singleton that is exported as an interface
//
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CSourcePauseTool, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_SourcePauseTool );

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CSourcePauseTool::CSourcePauseTool() {};
CSourcePauseTool::~CSourcePauseTool() {};

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CSourcePauseTool::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	auto startTime = std::chrono::high_resolution_clock::now();

	ConnectTier1Libraries(&interfaceFactory, 1);
	ConVar_Register(0);

	engine = (IVEngineClient*)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	if (!engine)
	{
		DevWarning("SPT: Failed to get the IVEngineClient interface.\n");
		Warning("SPT: y_spt_afterframes has no effect.\n");
	}

	icvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, NULL);
	if (!icvar)
	{
		DevWarning("SPT: Failed to get the ICvar interface.\n");
		Warning("SPT: y_spt_cvar has no effect.\n");
	}

	EngineMsg = Msg;
	EngineDevMsg = DevMsg;
	EngineWarning = Warning;
	EngineDevWarning = DevWarning;
	EngineConCmd = CallServerCommand;

	Hooks::getInstance().Init();

	auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
	std::ostringstream out;
	out << "SourcePauseTool v" SPT_VERSION " was loaded in " << loadTime << "ms.\n";

	Msg("%s", std::string(out.str()).c_str());

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CSourcePauseTool::Unload( void )
{
	Hooks::getInstance().Free();

	ConVar_Unregister();
	DisconnectTier1Libraries();
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CSourcePauseTool::Pause( void ) {};

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CSourcePauseTool::UnPause( void ) {};

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CSourcePauseTool::GetPluginDescription( void )
{
	return "SourcePauseTool v" SPT_VERSION ", Ivan \"YaLTeR\" Molodetskikh";
}

CON_COMMAND(y_spt_afterframes, "Add a command into an afterframes queue. Usage: y_spt_afterframes <count> <command>")
{
	if (!icvar)
		return;

	if (args.ArgC() != 3)
	{
		Msg("Usage: y_spt_afterframes <count> <command>\n");
		return;
	}

	afterframes_entry_t entry;

	std::istringstream ss(args.Arg(1));
	ss >> entry.framesLeft;
	entry.command.assign(args.Arg(2));

	Hooks::getInstance().clientDLL.AddIntoAfterframesQueue(entry);
}

CON_COMMAND(y_spt_afterframes_reset, "Reset the afterframes queue.")
{
	Hooks::getInstance().clientDLL.ResetAfterframesQueue();
}

CON_COMMAND(y_spt_cvar, "CVar manipulation.")
{
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

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CSourcePauseTool::LevelInit( char const *pMapName ) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//      edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CSourcePauseTool::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) {};

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CSourcePauseTool::GameFrame( bool simulating ) {};

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CSourcePauseTool::LevelShutdown( void ) {}; // !!!!this can get called multiple times per map change

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientActive( edict_t *pEntity ) {};

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientDisconnect( edict_t *pEntity ) {};

//---------------------------------------------------------------------------------
// Purpose: called on
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientPutInServer( edict_t *pEntity, char const *playername ) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CSourcePauseTool::SetCommandClient( int index ) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientSettingsChanged( edict_t *pEdict ) {};

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CSourcePauseTool::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CSourcePauseTool::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CSourcePauseTool::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CSourcePauseTool::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) {};
void CSourcePauseTool::OnEdictAllocated( edict_t *edict ) {};
void CSourcePauseTool::OnEdictFreed( const edict_t *edict ) {};
