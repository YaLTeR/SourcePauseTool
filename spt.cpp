//===== Copyright © 1996-2008, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>

#include "interface.h"
#include "engine/iserverplugin.h"
#include "eiface.h"
#include "convar.h"
#include "tier2/tier2.h"

#include "memutils.h"
#include <detours.h>

#pragma comment( lib, "detours.lib" )

// Uncomment this to compile the sample TF2 plugin code, note: most of this is duplicated in serverplugin_tony, but kept here for reference!
//#define SAMPLE_TF2_PLUGIN
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Interfaces from the engine
IVEngineServer	*engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)

// useful helper func
inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}
//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class CEmptyServerPlugin: public IServerPluginCallbacks
{
public:
	CEmptyServerPlugin();
	~CEmptyServerPlugin();

	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

	// added with version 3 of the interface.
	virtual void			OnEdictAllocated( edict_t *edict );
	virtual void			OnEdictFreed( const edict_t *edict  );
};


// 
// The plugin is a static singleton that is exported as an interface
//
CEmptyServerPlugin g_EmtpyServerPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEmptyServerPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_EmtpyServerPlugin);

ConVar y_spt_pause{ "y_spt_pause", "1", FCVAR_ARCHIVE };

HMODULE hEngineDll;
size_t dwEngineDllStart, dwEngineDllSize;
DWORD_PTR dwGameServerPtr;
bool *pM_bLoadgame;
bool bShouldPreventNextUnpause = false;

typedef void(__fastcall *SetPause_t) (void *thisptr, int edx, byte paused);
SetPause_t ORIG_SetPause;

typedef void(__fastcall *FinishRestore_t) (void *thisptr, int edx);
FinishRestore_t ORIG_FinishRestore;

typedef bool(__cdecl *SV_ActivateServer_t) ();
SV_ActivateServer_t ORIG_SV_ActivateServer;

void __fastcall HOOKED_SetPause(void *thisptr, int edx, byte paused)
{
	Log("SPT: Engine call: SetPause(%d); m_bLoadgame = %d\n", paused, *pM_bLoadgame);

	if (paused == false)
	{
		if (bShouldPreventNextUnpause)
		{
			Log("SPT: Unpause prevented.\n");
			bShouldPreventNextUnpause = false;
			return;
		}
	}

	bShouldPreventNextUnpause = false;
	return ORIG_SetPause(thisptr, edx, paused);
}

bool __cdecl HOOKED_SV_ActivateServer()
{
	bool result = ORIG_SV_ActivateServer();

	Log("SPT: Engine call: SV_ActivateServer() => %d;\n", result);

	if ((y_spt_pause.GetInt() == 2) && *pM_bLoadgame)
	{
		ORIG_SetPause((void *)dwGameServerPtr, 0, true);
		Log("SPT: Pausing...\n");

		bShouldPreventNextUnpause = true;
	}

	return result;
}

void __fastcall HOOKED_FinishRestore(void *thisptr, int edx)
{
	Log("SPT: Engine call: FinishRestore();\n");

	if (y_spt_pause.GetInt() == 1)
	{
		ORIG_SetPause(thisptr, 0, true);
		Log("SPT: Pausing...\n");

		bShouldPreventNextUnpause = true;
	}

	return ORIG_FinishRestore(thisptr, edx);
}

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CEmptyServerPlugin::CEmptyServerPlugin() {};

CEmptyServerPlugin::~CEmptyServerPlugin() {};

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CEmptyServerPlugin::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	ConnectTier1Libraries(&interfaceFactory, 1);
	ConVar_Register(0);

	hEngineDll = GetModuleHandleA("engine.dll");
	if (!MemUtils::GetModuleInfo(hEngineDll, dwEngineDllStart, dwEngineDllSize))
	{
		Warning("SPT: Could not obtain the engine.dll module info!\n");
		return false;
	}

	Log("SPT: Obtained the engine.dll module info. Start: %p; Size: %x;\n", dwEngineDllStart, dwEngineDllSize);

	// m_bLoadgame and dwGameServerPtr (&sv)
	Log("SPT: Searching for SpawnPlayer (first pattern - 5135)...\n");

	DWORD_PTR dwSpawnPlayer = MemUtils::FindPattern(dwEngineDllStart, dwEngineDllSize, (PBYTE) "\x83\xEC\x14\x80\x3D\x00\x00\x00\x00\x00\x56\x8B\xF1\x74\x00\x6A\x00\xB9\x00\x00\x00\x00\xE8", "xxxxx????xxxxx?xxx????x");
	if (!dwSpawnPlayer)
	{
		Warning("SPT: Could not find SpawnPlayer!\n");
		return false;
	}

	Log("SPT: Found SpawnPlayer at %p.\n", dwSpawnPlayer);

	pM_bLoadgame = (bool *)(*(DWORD *)(dwSpawnPlayer + 5));
	Log("SPT: m_bLoadGame is situated at %p.\n", pM_bLoadgame);

	dwGameServerPtr = (DWORD_PTR)(*(DWORD *)(dwSpawnPlayer + 18));
	Log("SPT: dwGameServerPtr is %p.\n", dwGameServerPtr);

	// SV_ActivateServer
	Log("SPT: Searching for SV_ActivateServer (first pattern - 5135)...\n");

	DWORD_PTR dwSV_ActivateServer = MemUtils::FindPattern(dwEngineDllStart, dwEngineDllSize, (PBYTE) "\x83\xEC\x08\x57\x8B\x00\x00\x00\x00\x00\x68\x00\x00\x00\x00\xFF\xD7\x83\xC4\x04\xE8\x00\x00\x00\x00\x8B\x10", "xxxxx?????x????xxxxxx????xx");
	if (!dwSV_ActivateServer)
	{
		Warning("SPT: Could not find SV_ActivateServer!\n");
		return false;
	}

	ORIG_SV_ActivateServer = (SV_ActivateServer_t)dwSV_ActivateServer;

	Log("SPT: Found SV_ActivateServer at %p.\n", dwSV_ActivateServer);
	
	// FinishRestore
	Log("SPT: Searching for FinishRestore (first pattern - 5135)...\n");

	DWORD_PTR dwFinishRestore = MemUtils::FindPattern(dwEngineDllStart, dwEngineDllSize, (PBYTE) "\x81\xEC\xA4\x06\x00\x00\x33\xC0\x55\x8B\xE9\x8D\x8C\x24\x00\x00\x00\x00\x89\x84\x24", "xxxxxxxxxxxxxx????xxx");
	if (!dwFinishRestore)
	{
		Warning("SPT: Could not find FinishRestore!\n");
		return false;
	}

	ORIG_FinishRestore = (FinishRestore_t)dwFinishRestore;

	Log("SPT: Found FinishRestore at %p.\n", dwFinishRestore);

	// SetPause
	Log("SPT: Searching for SetPause (first pattern - 5135)...\n");

	DWORD_PTR dwSetPause = MemUtils::FindPattern(dwEngineDllStart, dwEngineDllSize, (PBYTE) "\x83\xEC\x14\x56\x8B\xF1\x8B\x06\x8B\x50\x00\xFF\xD2\x84\xC0\x74\x00\x8B\x06\x8B\x50\x00\x8B\xCE\xFF\xD2\x84\xC0\x74", "xxxxxxxxxx?xxxxx?xxxx?xxxxxxx");
	if (!dwSetPause)
	{
		Warning("SPT: Could not find SetPause!\n");
		return false;
	}

	ORIG_SetPause = (SetPause_t)dwSetPause;

	Log("SPT: Found SetPause at %p.\n", dwSetPause);

	// Detours
	Log("SPT: Detouring SV_ActivateServer, FinishRestore and SetPause...\n");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID &)ORIG_FinishRestore, HOOKED_FinishRestore);
	DetourAttach(&(PVOID &)ORIG_SV_ActivateServer, HOOKED_SV_ActivateServer);
	DetourAttach(&(PVOID &)ORIG_SetPause, HOOKED_SetPause);
	LONG error = DetourTransactionCommit();
	if (error == NO_ERROR)
	{
		Log("SPT: Detoured SV_ActivateServer, FinishRestore and SetPause.\n");
	}
	else
	{
		Warning("SPT: Error detouring SV_ActivateServer, FinishRestore and SetPause: %d.\n", error);
		return false;
	}

	Msg("SourcePauseTool v0.1 was loaded successfully.\n");

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Unload( void )
{
	Log("SPT: Removing the SV_ActivateServer detour, the FinishRestore detour and the SetPause detour...\n");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID &)ORIG_FinishRestore, HOOKED_FinishRestore);
	DetourDetach(&(PVOID &)ORIG_SV_ActivateServer, HOOKED_SV_ActivateServer);
	DetourDetach(&(PVOID &)ORIG_SetPause, HOOKED_SetPause);
	LONG error = DetourTransactionCommit();
	if (error == NO_ERROR)
	{
		Log("SPT: Removed the SV_ActivateServer detour, the FinishRestore detour and the SetPause detour successfully.\n");
	}
	else
	{
		Warning("SPT: Error removing the SV_ActivateServer detour, the FinishRestore detour and the SetPause detour: %d.\n", error);
	}

	ConVar_Unregister();
	DisconnectTier1Libraries( );
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Pause(void) {};

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::UnPause(void) {};

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CEmptyServerPlugin::GetPluginDescription( void )
{
	return "SourcePauseTool v0.1, Ivan \"YaLTeR\" Molodetskikh";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelInit(char const *pMapName) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {};

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::GameFrame(bool simulating) {};

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelShutdown(void) {}; // !!!!this can get called multiple times per map change

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientActive(edict_t *pEntity) {};

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientDisconnect(edict_t *pEntity) {};

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::SetCommandClient(int index) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientSettingsChanged(edict_t *pEdict) {};

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) {};
void CEmptyServerPlugin::OnEdictAllocated(edict_t *edict) {};
void CEmptyServerPlugin::OnEdictFreed(const edict_t *edict) {};
