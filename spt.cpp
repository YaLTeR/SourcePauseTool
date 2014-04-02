#include <limits>
#include <string>
#include <vector>

#include "memutils.h"
#include <detours.h>

#include "engine/iserverplugin.h"
#include "convar.h"
#include "tier2/tier2.h"

#pragma comment( lib, "detours.lib" )

#include "tier0/memdbgoff.h" // YaLTeR - switch off the memory debugging.

#undef max // This thing is defined somewhere in tier0 includes and I don't need it at all.
#define SPT_VERSION "0.3-beta"

const unsigned int uiMax = std::numeric_limits<unsigned int>::max();

// useful helper func
inline bool FStrEq(const char *sz1, const char *sz2)
{
    return(Q_stricmp(sz1, sz2) == 0);
}
//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class CSourcePauseTool: public IServerPluginCallbacks
{
public:
    CSourcePauseTool();
    ~CSourcePauseTool();

    // IServerPluginCallbacks methods
    virtual bool            Load(   CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
    virtual void            Unload( void );
    virtual void            Pause( void );
    virtual void            UnPause( void );
    virtual const char     *GetPluginDescription( void );
    virtual void            LevelInit( char const *pMapName );
    virtual void            ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
    virtual void            GameFrame( bool simulating );
    virtual void            LevelShutdown( void );
    virtual void            ClientActive( edict_t *pEntity );
    virtual void            ClientDisconnect( edict_t *pEntity );
    virtual void            ClientPutInServer( edict_t *pEntity, char const *playername );
    virtual void            SetCommandClient( int index );
    virtual void            ClientSettingsChanged( edict_t *pEdict );
    virtual PLUGIN_RESULT   ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
    virtual PLUGIN_RESULT   ClientCommand( edict_t *pEntity, const CCommand &args );
    virtual PLUGIN_RESULT   NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
    virtual void            OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

    // added with version 3 of the interface.
    virtual void            OnEdictAllocated( edict_t *edict );
    virtual void            OnEdictFreed( const edict_t *edict  );
};


//
// The plugin is a static singleton that is exported as an interface
//
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSourcePauseTool, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_SourcePauseTool);

ConVar y_spt_pause( "y_spt_pause", "1", FCVAR_ARCHIVE );
ConVar y_spt_motion_blur_fix( "y_spt_motion_blur_fix", "0" );

struct
{
    // engine.dll
    bool bFoundSV_ActivateServer;
    bool bFoundFinishRestore;
    bool bFoundSetPaused;
    bool bFoundm_bLoadgame;
    bool bFoundGameServerPtr;

    // client.dll
    bool bFoundDoImageSpaceMotionBlur;
    bool bFoundgpGlobals;
} hookState;

HMODULE hEngineDll;
size_t dwEngineDllStart, dwEngineDllSize;
DWORD_PTR dwGameServerPtr;
bool *pM_bLoadgame;
bool bShouldPreventNextUnpause = false;

HMODULE hClientDll;
size_t dwClientDllStart, dwClientDllSize;
DWORD_PTR *pgpGlobals;

// engine.dll patterns
MemUtils::ptnvec ptnsSpawnPlayer =
{
    {
        "5135",
        { 0x83, 0xEC, 0x14, 0x80, 0x3D, '?', '?', '?', '?', 0x00, 0x56, 0x8B, 0xF1, 0x74, '?', 0x6A, 0x00, 0xB9, '?', '?', '?', '?', 0xE8, '?', '?', '?', '?', 0xEB, 0x23, 0x8B, 0x0D, '?', '?', '?', '?', 0x8B, 0x01, 0x8B, 0x96, '?', '?', '?', '?', 0x8B, 0x40, 0x0C, 0x52, 0xFF, 0xD0, 0x8B, 0x8E, '?', '?', '?', '?', 0x51, 0xE8, '?', '?', '?', '?', 0x83, 0xC4, 0x04, 0x8B, 0x46, 0x0C, 0x8B, 0x56, 0x04, 0x8B, 0x52, 0x74, 0x83, 0xC0, 0x01 },
        "xxxxx????xxxxx?xxx????x????xxxx????xxxx????xxxxxxxx????xx????xxxxxxxxxxxxxxx"
    },

    {
        "5339",
        { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x14, 0x80, 0x3D, '?', '?', '?', '?', 0x00, 0x56, 0x8B, 0xF1, 0x74, '?', 0x6A, 0x00, 0xB9, '?', '?', '?', '?', 0xE8, '?', '?', '?', '?', 0xEB, 0x23, 0x8B, 0x0D, '?', '?', '?', '?', 0x8B, 0x01, 0x8B, 0x96, '?', '?', '?', '?', 0x8B, 0x40, 0x0C, 0x52, 0xFF, 0xD0, 0x8B, 0x8E, '?', '?', '?', '?', 0x51, 0xE8, '?', '?', '?', '?', 0x83, 0xC4, 0x04, 0x8B, 0x46, 0x0C, 0x8B, 0x56, 0x04, 0x8B, 0x52, 0x74, 0x40 },
        "xxxxxxxx????xxxxx?xxx????x????xxxx????xxxx????xxxxxxxx????xx????xxxxxxxxxxxxx"
    },

    {
        "4104",
        { 0x83, 0xEC, 0x14, 0x80, 0x3D, '?', '?', '?', '?', 0x00, 0x56, 0x8B, 0xF1, 0x74, '?', 0x6A, 0x00, 0xB9, '?', '?', '?', '?', 0xE8, '?', '?', '?', '?', 0xEB, 0x23, 0x8B, 0x0D, '?', '?', '?', '?', 0x8B, 0x01, 0x8B, 0x96, '?', '?', '?', '?', 0x8B, 0x40, 0x0C, 0x52, 0xFF, 0xD0, 0x8B, 0x8E, '?', '?', '?', '?', 0x51, 0xE8, '?', '?', '?', '?', 0x83, 0xC4, 0x04, 0x8B, 0x46, 0x0C, 0x8B, 0x56, 0x04, 0x8B, 0x52, 0x70, 0x83, 0xC0, 0x01 },
        "xxxxx????xxxxx?xxx????x????xxxx????xxxx????xxxxxxxx????xx????xxxxxxxxxxxxxxx"
    }
};

MemUtils::ptnvec ptnsSV_ActivateServer =
{
    {
        "5135",
        { 0x83, 0xEC, 0x08, 0x57, 0x8B, 0x3D, '?', '?', '?', '?', 0x68, '?', '?', '?', '?', 0xFF, 0xD7, 0x83, 0xC4, 0x04, 0xE8, '?', '?', '?', '?', 0x8B, 0x10 },
        "xxxxxx????x????xxxxxx????xx"
    },

    {
        "5339",
        { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x0C, 0x57, 0x8B, 0x3D, '?', '?', '?', '?', 0x68, '?', '?', '?', '?', 0xFF, 0xD7, 0x83, 0xC4, 0x04, 0xE8, '?', '?', '?', '?', 0x8B, 0x10 },
        "xxxxxxxxx????x????xxxxxx????xx"
    }
};

MemUtils::ptnvec ptnsFinishRestore =
{
    {
        "5135",
        { 0x81, 0xEC, 0xA4, 0x06, 0x00, 0x00, 0x33, 0xC0, 0x55, 0x8B, 0xE9, 0x8D, 0x8C, 0x24, 0x20, 0x01, 0x00, 0x00, 0x89, 0x84, 0x24, 0x08, 0x01, 0x00, 0x00 },
        "xxxxxxxxxxxxxxxxxxxxxxxxx"
    },

    {
        "5339",
        { 0x55, 0x8B, 0xEC, 0x81, 0xEC, 0xA4, 0x06, 0x00, 0x00, 0x33, 0xC0, 0x53, 0x8B, 0xD9, 0x8D, 0x8D, 0x74, 0xF9, 0xFF, 0xFF, 0x89, 0x85, 0x5C, 0xF9, 0xFF, 0xFF },
        "xxxxxxxxxxxxxxxxxxxxxxxxxx"
    }
};

MemUtils::ptnvec ptnsSetPaused =
{
    {
        "5135",
        { 0x83, 0xEC, 0x14, 0x56, 0x8B, 0xF1, 0x8B, 0x06, 0x8B, 0x50, '?', 0xFF, 0xD2, 0x84, 0xC0, 0x74, '?', 0x8B, 0x06, 0x8B, 0x50, '?', 0x8B, 0xCE, 0xFF, 0xD2, 0x84, 0xC0, 0x74, '?', 0x8A, 0x44, 0x24, 0x1C, 0x8B, 0x16, 0x8B, 0x92, 0x80, 0x00, 0x00, 0x00 },
        "xxxxxxxxxx?xxxxx?xxxx?xxxxxxx?xxxxxxxxxxxx"
    },

    {
        "5339",
        { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x14, 0x56, 0x8B, 0xF1, 0x8B, 0x06, 0x8B, 0x50, '?', 0xFF, 0xD2, 0x84, 0xC0, 0x74, '?', 0x8B, 0x06, 0x8B, 0x50, '?', 0x8B, 0xCE, 0xFF, 0xD2, 0x84, 0xC0, 0x74, '?', 0x8A, 0x45, 0x08, 0x8B, 0x16, 0x8B, 0x92, 0x80, 0x00, 0x00, 0x00 },
        "xxxxxxxxxxxxx?xxxxx?xxxx?xxxxxxx?xxxxxxxxxxx"
    },

    {
        "4104",
        { 0x83, 0xEC, 0x14, 0x56, 0x8B, 0xF1, 0x8B, 0x06, 0x8B, 0x50, '?', 0xFF, 0xD2, 0x84, 0xC0, 0x74, '?', 0x8B, 0x06, 0x8B, 0x50, '?', 0x8B, 0xCE, 0xFF, 0xD2, 0x84, 0xC0, 0x74, '?', 0x8A, 0x44, 0x24, 0x1C, 0x8B, 0x16, 0x8B, 0x52, 0x7C, 0x33, 0xC9, 0x84, 0xC0 },
        "xxxxxxxxxx?xxxxx?xxxx?xxxxxxx?xxxxxxxxxxxxx"
    }
};

// client.dll patterns
MemUtils::ptnvec ptnsDoImageSpaceMotionBlur =
{
    {
        "5135",
        { 0xA1, '?', '?', '?', '?', 0x81, 0xEC, 0x8C, 0x00, 0x00, 0x00, 0x83, 0x78, 0x30, 0x00, 0x0F, 0x84, 0xF3, 0x06, 0x00, 0x00, 0x8B, 0x0D, '?', '?', '?', '?', 0x8B, 0x11, 0x8B, 0x82, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xD0 },
        "x????xxxxxxxxxxxxxxxxxx????xxxxxxxxxx"
    },

    {
        "5339",
        { 0x55, 0x8B, 0xEC, 0xA1, '?', '?', '?', '?', 0x83, 0xEC, 0x7C, 0x83, 0x78, 0x30, 0x00, 0x0F, 0x84, 0x4A, 0x08, 0x00, 0x00, 0x8B, 0x0D, '?', '?', '?', '?', 0x8B, 0x11, 0x8B, 0x82, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xD0 },
        "xxxx????xxxxxxxxxxxxxxx????xxxxxxxxxx"
    },

    {
        "4104",
        { 0xA1, '?', '?', '?', '?', 0x81, 0xEC, 0x8C, 0x00, 0x00, 0x00, 0x83, 0x78, 0x30, 0x00, 0x0F, 0x84, 0xEE, 0x06, 0x00, 0x00, 0x8B, 0x0D, '?', '?', '?', '?', 0x8B, 0x11, 0x8B, 0x42, 0x7C, 0xFF, 0xD0 },
        "x????xxxxxxxxxxxxxxxxxx????xxxxxxx"
    }
};

// engine.dll fucntion declarations
typedef bool(__cdecl *SV_ActivateServer_t) ();
SV_ActivateServer_t ORIG_SV_ActivateServer;
typedef void(__fastcall *FinishRestore_t) (void *thisptr, int edx);
FinishRestore_t ORIG_FinishRestore;
typedef void(__fastcall *SetPaused_t) (void *thisptr, int edx, byte paused);
SetPaused_t ORIG_SetPaused;

// client.dll function declarations
typedef void( __cdecl *DoImageSpaceMotionBlur_t ) (void *view, int x, int y, int w, int h);
DoImageSpaceMotionBlur_t ORIG_DoImageSpaceMorionBlur;

// engine.dll functions
bool __cdecl HOOKED_SV_ActivateServer()
{
    bool result = ORIG_SV_ActivateServer();

    DevLog("SPT: Engine call: SV_ActivateServer() => %d;\n", result);

    if (hookState.bFoundSetPaused && hookState.bFoundm_bLoadgame && hookState.bFoundGameServerPtr)
    {
        if ((y_spt_pause.GetInt() == 2) && *pM_bLoadgame)
        {
            ORIG_SetPaused((void *)dwGameServerPtr, 0, true);
            DevLog("SPT: Pausing...\n");

            bShouldPreventNextUnpause = true;
        }
    }

    return result;
}

void __fastcall HOOKED_FinishRestore(void *thisptr, int edx)
{
    DevLog("SPT: Engine call: FinishRestore();\n");

    if (hookState.bFoundSetPaused && (y_spt_pause.GetInt() == 1))
    {
        ORIG_SetPaused(thisptr, 0, true);
        DevLog("SPT: Pausing...\n");

        bShouldPreventNextUnpause = true;
    }

    return ORIG_FinishRestore(thisptr, edx);
}

void __fastcall HOOKED_SetPaused(void *thisptr, int edx, byte paused)
{
    if (hookState.bFoundm_bLoadgame)
    {
        DevLog("SPT: Engine call: SetPaused(%d); m_bLoadgame = %d\n", paused, *pM_bLoadgame);
    }
    else
    {
        DevLog("SPT: Engine call: SetPaused(%d);\n", paused);
    }

    if (paused == false)
    {
        if (bShouldPreventNextUnpause)
        {
            DevLog("SPT: Unpause prevented.\n");
            bShouldPreventNextUnpause = false;
            return;
        }
    }

    bShouldPreventNextUnpause = false;
    return ORIG_SetPaused(thisptr, edx, paused);
}

// client.dll functions
void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h )
{
    DWORD_PTR origgpGlobals = *pgpGlobals;

    /*
        Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
        so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
        function does not use anything apart from gpGlobals->realtime from gpGlobals,
        so we can do such a replace to make it use gpGlobals->curtime instead without
        breaking anything else.
    */
    if (hookState.bFoundgpGlobals)
    {
        if (y_spt_motion_blur_fix.GetBool())
        {
            *pgpGlobals = *pgpGlobals + 12;
        }
    }

    ORIG_DoImageSpaceMorionBlur( view, x, y, w, h );

    if (hookState.bFoundgpGlobals)
    {
        if (y_spt_motion_blur_fix.GetBool())
        {
            *pgpGlobals = origgpGlobals;
        }
    }
}

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CSourcePauseTool::CSourcePauseTool()
{
    // Everything is true by default, set to false on fail.
    hookState.bFoundSV_ActivateServer = true;
    hookState.bFoundFinishRestore = true;
    hookState.bFoundSetPaused = true;
    hookState.bFoundm_bLoadgame = true;
    hookState.bFoundGameServerPtr = true;

    hookState.bFoundDoImageSpaceMotionBlur = true;
    hookState.bFoundgpGlobals = true;
}

CSourcePauseTool::~CSourcePauseTool() {};

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CSourcePauseTool::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
    ConnectTier1Libraries(&interfaceFactory, 1);
    ConVar_Register(0);

    unsigned int ptnNumber;

    hEngineDll = GetModuleHandleA("engine.dll");
    if (!MemUtils::GetModuleInfo(hEngineDll, dwEngineDllStart, dwEngineDllSize))
    {
        Warning("SPT: Could not obtain the engine.dll module info!\n");
        Warning("SPT: y_spt_pause has no effect.\n");

        hookState.bFoundSV_ActivateServer = false;
        hookState.bFoundFinishRestore = false;
        hookState.bFoundSetPaused = false;
        hookState.bFoundm_bLoadgame = false;
        hookState.bFoundGameServerPtr = false;
    }
    else
    {
        Log("SPT: Obtained the engine.dll module info. Start: %p; Size: %x;\n", dwEngineDllStart, dwEngineDllSize);

        // m_bLoadgame and dwGameServerPtr (&sv)
        Log("SPT: Searching for SpawnPlayer...\n");

        DWORD_PTR pSpawnPlayer = NULL;
        ptnNumber = MemUtils::FindUniqueSequence( dwEngineDllStart, dwEngineDllSize, ptnsSpawnPlayer, &pSpawnPlayer );
        if (ptnNumber != uiMax)
        {
            Log( "SPT: Found SpawnPlayer at %p (using the build %s pattern).\n", pSpawnPlayer, ptnsSpawnPlayer[ptnNumber].build.c_str() );

            switch (ptnNumber)
            {
            case 0:
                pM_bLoadgame = (bool *)(*(DWORD *)(pSpawnPlayer + 5));
                dwGameServerPtr = (DWORD_PTR)(*(DWORD *)(pSpawnPlayer + 18));
                break;

            case 1:
                pM_bLoadgame = (bool *)(*(DWORD *)(pSpawnPlayer + 8));
                dwGameServerPtr = (DWORD_PTR)(*(DWORD *)(pSpawnPlayer + 21));
                break;

            case 2: // 4104 is the same as 5135 here.
                pM_bLoadgame = (bool *)(*(DWORD *)(pSpawnPlayer + 5));
                dwGameServerPtr = (DWORD_PTR)(*(DWORD *)(pSpawnPlayer + 18));
                break;
            }

            Log("SPT: m_bLoadGame is situated at %p.\n", pM_bLoadgame);
            Log("SPT: dwGameServerPtr is %p.\n", dwGameServerPtr);
        }
        else
        {
            Warning("SPT: Could not find SpawnPlayer!\n");
            Warning("SPT: y_spt_pause 2 has no effect.\n");

            hookState.bFoundm_bLoadgame = false;
            hookState.bFoundGameServerPtr = false;
        }

        // SV_ActivateServer
        Log("SPT: Searching for SV_ActivateServer...\n");

        DWORD_PTR pSV_ActivateServer = NULL;
        ptnNumber = MemUtils::FindUniqueSequence( dwEngineDllStart, dwEngineDllSize, ptnsSV_ActivateServer, &pSV_ActivateServer );
        if (ptnNumber != uiMax)
        {
            ORIG_SV_ActivateServer = (SV_ActivateServer_t)pSV_ActivateServer;
            Log("SPT: Found SV_ActivateServer at %p (using the build %s pattern).\n", pSV_ActivateServer, ptnsSV_ActivateServer[ptnNumber].build.c_str());
        }
        else
        {
            Warning("SPT: Could not find SV_ActivateServer!\n");
            Warning("SPT: y_spt_pause 2 has no effect.\n");

            hookState.bFoundSV_ActivateServer = false;
        }

        // FinishRestore
        Log("SPT: Searching for FinishRestore...\n");

        DWORD_PTR pFinishRestore = NULL;
        ptnNumber = MemUtils::FindUniqueSequence( dwEngineDllStart, dwEngineDllSize, ptnsFinishRestore, &pFinishRestore );
        if (ptnNumber != uiMax)
        {
            ORIG_FinishRestore = (FinishRestore_t)pFinishRestore;
            Log("SPT: Found FinishRestore at %p (using the build %s pattern).\n", pFinishRestore, ptnsFinishRestore[ptnNumber].build.c_str());
        }
        else
        {
            Warning("SPT: Could not find FinishRestore!\n");
            Warning("SPT: y_spt_pause 1 has no effect.\n");

            hookState.bFoundFinishRestore = false;
        }

        // SetPaused
        Log("SPT: Searching for SetPaused...\n");

        DWORD_PTR pSetPaused = NULL;
        ptnNumber = MemUtils::FindUniqueSequence( dwEngineDllStart, dwEngineDllSize, ptnsSetPaused, &pSetPaused );
        if (pSetPaused)
        {
            ORIG_SetPaused = (SetPaused_t)pSetPaused;
            Log("SPT: Found SetPaused at %p (using the build %s pattern).\n", pSetPaused, ptnsSetPaused[ptnNumber].build.c_str());
        }
        else
        {
            Warning("SPT: Could not find SetPaused!\n");
            Warning("SPT: y_spt_pause has no effect.\n");

            hookState.bFoundSetPaused = false;
        }
    }

    hClientDll = GetModuleHandleA( "client.dll" );
    if (!MemUtils::GetModuleInfo( hClientDll, dwClientDllStart, dwClientDllSize ))
    {
        Warning( "SPT: Could not obtain the client.dll module info!\n" );
        Warning( "SPT: y_spt_motion_blur_fix has no effect.\n" );

        hookState.bFoundDoImageSpaceMotionBlur = false;
        hookState.bFoundgpGlobals = false;
    }
    else
    {
        Log( "SPT: Obtained the client.dll module info. Start: %p; Size: %x;\n", dwClientDllStart, dwClientDllSize );

        DWORD_PTR pDoImageSpaceMotionBlur = NULL;
        ptnNumber = MemUtils::FindUniqueSequence( dwClientDllStart, dwClientDllSize, ptnsDoImageSpaceMotionBlur, &pDoImageSpaceMotionBlur );
        if (ptnNumber != uiMax)
        {
            ORIG_DoImageSpaceMorionBlur = (DoImageSpaceMotionBlur_t)pDoImageSpaceMotionBlur;
            Log( "SPT: Found DoImageSpaceMotionBlur at %p (using the build %s pattern).\n", pDoImageSpaceMotionBlur, ptnsDoImageSpaceMotionBlur[ptnNumber].build.c_str() );

            switch (ptnNumber)
            {
            case 0:
                pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 132);
                break;

            case 1:
                pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 153);
                break;

            case 2:
                pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 129);
                break;
            }

            Log( "SPT: pgpGlobals is %p.\n", pgpGlobals );
        }
        else
        {
            Warning( "SPT: Could not find DoImageSpaceMotionBlur!\n" );
            Warning( "SPT: y_spt_motion_blur_fix has no effect.\n" );

            hookState.bFoundDoImageSpaceMotionBlur = false;
            hookState.bFoundgpGlobals = false;
        }
    }

    // Detours
    if (hookState.bFoundSV_ActivateServer
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
    }

    Msg("SourcePauseTool v" SPT_VERSION " was loaded successfully.\n");

    return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CSourcePauseTool::Unload( void )
{
    if (hookState.bFoundSV_ActivateServer
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
    }

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
