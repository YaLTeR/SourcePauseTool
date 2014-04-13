#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "spt.h"
#include "memutils.h"
#include "patterns.h"
#include "hooks.h"

#include <detours.h>

#include "engine/iserverplugin.h"
#include "tier2/tier2.h"

#pragma comment( lib, "detours.lib" )

#include "tier0/memdbgoff.h" // YaLTeR - switch off the memory debugging.

#define SPT_VERSION "0.3-beta"

// useful helper func
inline bool FStrEq(const char *sz1, const char *sz2)
{
    return(Q_stricmp(sz1, sz2) == 0);
}

// Simple conversion of wstrings that only consist of simple characters
std::string WStringToString( std::wstring wstr )
{
    return std::string( wstr.begin(), wstr.end() );
}

//
// The plugin is a static singleton that is exported as an interface
//
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSourcePauseTool, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_SourcePauseTool);

// client.dll functions
//void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h )
//{
//    DWORD_PTR origgpGlobals = *pgpGlobals;
//
//    /*
//        Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
//        so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
//        function does not use anything apart from gpGlobals->realtime from gpGlobals,
//        so we can do such a replace to make it use gpGlobals->curtime instead without
//        breaking anything else.
//    */
//    if (hookState.bFoundgpGlobals)
//    {
//        if (y_spt_motion_blur_fix.GetBool())
//        {
//            *pgpGlobals = *pgpGlobals + 12;
//        }
//    }
//
//    ORIG_DoImageSpaceMorionBlur( view, x, y, w, h );
//
//    if (hookState.bFoundgpGlobals)
//    {
//        if (y_spt_motion_blur_fix.GetBool())
//        {
//            *pgpGlobals = origgpGlobals;
//        }
//    }
//}

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
    ConnectTier1Libraries(&interfaceFactory, 1);
    ConVar_Register(0);

    Hooks::Init();

    //hClientDll = GetModuleHandleA( "client.dll" );
    //if (!MemUtils::GetModuleInfo( hClientDll, dwClientDllStart, dwClientDllSize ))
    //{
    //    Warning("SPT: Could not obtain the client.dll module info!\n");
    //    Warning("SPT: y_spt_motion_blur_fix has no effect.\n");

    //    hookState.bFoundDoImageSpaceMotionBlur = false;
    //    hookState.bFoundgpGlobals = false;
    //}
    //else
    //{
    //    Log("SPT: Obtained the client.dll module info. Start: %p; Size: %x;\n", dwClientDllStart, dwClientDllSize);

    //    DWORD_PTR pDoImageSpaceMotionBlur = NULL;
    //    ptnNumber = MemUtils::FindUniqueSequence( dwClientDllStart, dwClientDllSize, Patterns::ptnsDoImageSpaceMotionBlur, &pDoImageSpaceMotionBlur );
    //    if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
    //    {
    //        ORIG_DoImageSpaceMorionBlur = (DoImageSpaceMotionBlur_t)pDoImageSpaceMotionBlur;
    //        Log( "SPT: Found DoImageSpaceMotionBlur at %p (using the build %s pattern).\n", pDoImageSpaceMotionBlur, Patterns::ptnsDoImageSpaceMotionBlur[ptnNumber].build.c_str( ) );

    //        switch (ptnNumber)
    //        {
    //        case 0:
    //            pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 132);
    //            break;

    //        case 1:
    //            pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 153);
    //            break;

    //        case 2:
    //            pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 129);
    //            break;
    //        }

    //        Log("SPT: pgpGlobals is %p.\n", pgpGlobals);
    //    }
    //    else
    //    {
    //        Warning("SPT: Could not find DoImageSpaceMotionBlur!\n");
    //        Warning("SPT: y_spt_motion_blur_fix has no effect.\n");

    //        hookState.bFoundDoImageSpaceMotionBlur = false;
    //        hookState.bFoundgpGlobals = false;
    //    }
    //}

    // Detours
    /*if (hookState.bFoundSV_ActivateServer
        || hookState.bFoundFinishRestore
        || hookState.bFoundSetPaused
        || hookState.bFoundDoImageSpaceMotionBlur)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        if (hookState.bFoundSV_ActivateServer)
            DetourAttach(&(PVOID &)ORIG_SV_ActivateServer, HOOKED_SV_ActivateServer);

        if (hookState.bFoundFinishRestore)
            DetourAttach(&(PVOID &)ORIG_FinishRestore, HOOKED_FinishRestore);

        if (hookState.bFoundSetPaused)
            DetourAttach(&(PVOID &)ORIG_SetPaused, HOOKED_SetPaused);

        if (hookState.bFoundDoImageSpaceMotionBlur)
            DetourAttach( &(PVOID &)ORIG_DoImageSpaceMorionBlur, HOOKED_DoImageSpaceMotionBlur );

        LONG error = DetourTransactionCommit();
        if (error == NO_ERROR)
        {
            Log("SPT: Detoured functions.\n");
        }
        else
        {
            Warning("SPT: Error detouring functions: %d.\n", error);
            return false;
        }
    }*/

    Msg("SourcePauseTool v" SPT_VERSION " was loaded successfully.\n");

    return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CSourcePauseTool::Unload( void )
{
    Hooks::Free();

    /*if (hookState.bFoundSV_ActivateServer
        || hookState.bFoundFinishRestore
        || hookState.bFoundSetPaused
        || hookState.bFoundDoImageSpaceMotionBlur)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        if (hookState.bFoundSV_ActivateServer)
            DetourDetach(&(PVOID &)ORIG_SV_ActivateServer, HOOKED_SV_ActivateServer);

        if (hookState.bFoundFinishRestore)
            DetourDetach(&(PVOID &)ORIG_FinishRestore, HOOKED_FinishRestore);

        if (hookState.bFoundSetPaused)
            DetourDetach(&(PVOID &)ORIG_SetPaused, HOOKED_SetPaused);

        if (hookState.bFoundDoImageSpaceMotionBlur)
            DetourDetach( &(PVOID &)ORIG_DoImageSpaceMorionBlur, HOOKED_DoImageSpaceMotionBlur );

        LONG error = DetourTransactionCommit();
        if (error == NO_ERROR)
        {
            Log("SPT: Removed the function detours successfully.\n");
        }
        else
        {
            Warning("SPT: Error removing the function detours: %d.\n", error);
        }
    }*/

    ConVar_Unregister();
    DisconnectTier1Libraries( );
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CSourcePauseTool::Pause(void) {};

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CSourcePauseTool::UnPause(void) {};

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CSourcePauseTool::GetPluginDescription( void )
{
    return "SourcePauseTool v" SPT_VERSION ", Ivan \"YaLTeR\" Molodetskikh";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CSourcePauseTool::LevelInit(char const *pMapName) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//      edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CSourcePauseTool::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {};

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CSourcePauseTool::GameFrame(bool simulating) {};

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CSourcePauseTool::LevelShutdown(void) {}; // !!!!this can get called multiple times per map change

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientActive(edict_t *pEntity) {};

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientDisconnect(edict_t *pEntity) {};

//---------------------------------------------------------------------------------
// Purpose: called on
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientPutInServer(edict_t *pEntity, char const *playername) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CSourcePauseTool::SetCommandClient(int index) {};

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CSourcePauseTool::ClientSettingsChanged(edict_t *pEdict) {};

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
void CSourcePauseTool::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) {};
void CSourcePauseTool::OnEdictAllocated(edict_t *edict) {};
void CSourcePauseTool::OnEdictFreed(const edict_t *edict) {};
