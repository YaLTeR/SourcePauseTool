#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <detours.h>

#include "../utf8conv/utf8conv.hpp"

#include "detoursutils.hpp"
#include "hooks.hpp"
#include "patterns.hpp"
#include "spt.hpp"

#include "convar.h"

namespace Hooks
{
    std::unordered_map<std::wstring, hookFuncList_t> moduleHookList =
    {
        {
            L"engine.dll",
            {
                EngineDll::Hook,
                EngineDll::Unhook
            }
        },

        {
            L"client.dll",
            {
                ClientDll::Hook,
                ClientDll::Unhook
            }
        }
    };

    ConVar y_spt_pause( "y_spt_pause", "1", FCVAR_ARCHIVE );
    ConVar y_spt_motion_blur_fix( "y_spt_motion_blur_fix", "0" );

    namespace EngineDll
    {
        struct
        {
            module_info_t moduleInfo;

            _SV_ActivateServer ORIG_SV_ActivateServer;
            _FinishRestore ORIG_FinishRestore;
            _SetPaused ORIG_SetPaused;

            DWORD_PTR pGameServer;
            bool *pM_bLoadgame;
            bool shouldPreventNextUnpause;
        } hookState;

        namespace Internal
        {
            bool __cdecl HOOKED_SV_ActivateServer()
            {
                bool result = hookState.ORIG_SV_ActivateServer();

                EngineDevLog( "Engine call: SV_ActivateServer() => %s;\n", (result ? "true" : "false") );

                if (hookState.ORIG_SetPaused && hookState.pM_bLoadgame && hookState.pGameServer)
                {
                    if ((y_spt_pause.GetInt() == 2) && *hookState.pM_bLoadgame)
                    {
                        hookState.ORIG_SetPaused( (void *)hookState.pGameServer, 0, true );
                        EngineDevLog( "Pausing...\n" );

                        hookState.shouldPreventNextUnpause = true;
                    }
                }

                return result;
            }

            void __fastcall HOOKED_FinishRestore( void *thisptr, int edx )
            {
                EngineDevLog( "Engine call: FinishRestore();\n" );

                if (hookState.ORIG_SetPaused && (y_spt_pause.GetInt() == 1))
                {
                    hookState.ORIG_SetPaused( thisptr, 0, true );
                    EngineDevLog( "Pausing...\n" );

                    hookState.shouldPreventNextUnpause = true;
                }

                return hookState.ORIG_FinishRestore( thisptr, edx );
            }

            void __fastcall HOOKED_SetPaused( void *thisptr, int edx, bool paused )
            {
                if (hookState.pM_bLoadgame)
                {
                    EngineDevLog( "Engine call: SetPaused( %s ); m_bLoadgame = %s\n", (paused ? "true" : "false"), (*hookState.pM_bLoadgame ? "true" : "false") );
                }
                else
                {
                    EngineDevLog( "Engine call: SetPaused( %s );\n", (paused ? "true" : "false") );
                }

                if (paused == false)
                {
                    if (hookState.shouldPreventNextUnpause)
                    {
                        EngineDevLog( "Unpause prevented.\n" );
                        hookState.shouldPreventNextUnpause = false;
                        return;
                    }
                }

                hookState.shouldPreventNextUnpause = false;
                return hookState.ORIG_SetPaused( thisptr, edx, paused );
            }
        }

        void Hook( std::wstring &moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength )
        {
            Clear(); // Just in case.

            hookState.moduleInfo.hModule = hModule;
            hookState.moduleInfo.moduleStart = moduleStart;
            hookState.moduleInfo.moduleLength = moduleLength;

            MemUtils::ptnvec_size ptnNumber;

            // m_bLoadgame and pGameServer (&sv)
            EngineLog( "Searching for SpawnPlayer...\n" );

            DWORD_PTR pSpawnPlayer = NULL;
            ptnNumber = MemUtils::FindUniqueSequence(hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsSpawnPlayer, &pSpawnPlayer);
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                EngineLog( "Found SpawnPlayer at %p (using the build %s pattern).\n", pSpawnPlayer, Patterns::ptnsSpawnPlayer[ptnNumber].build.c_str() );

                switch (ptnNumber)
                {
                case 0:
                    hookState.pM_bLoadgame = (bool *)(*(DWORD_PTR *)(pSpawnPlayer + 5));
                    hookState.pGameServer = (*(DWORD_PTR *)(pSpawnPlayer + 18));
                    break;

                case 1:
                    hookState.pM_bLoadgame = (bool *)(*(DWORD_PTR *)(pSpawnPlayer + 8));
                    hookState.pGameServer = (*(DWORD_PTR *)(pSpawnPlayer + 21));
                    break;

                case 2: // 4104 is the same as 5135 here.
                    hookState.pM_bLoadgame = (bool *)(*(DWORD_PTR *)(pSpawnPlayer + 5));
                    hookState.pGameServer = (*(DWORD_PTR *)(pSpawnPlayer + 18));
                    break;
                }

                EngineLog( "m_bLoadGame is situated at %p.\n", hookState.pM_bLoadgame );
                EngineLog( "pGameServer is %p.\n", hookState.pGameServer );
            }
            else
            {
                EngineWarning( "Could not find SpawnPlayer!\n" );
                EngineWarning( "y_spt_pause 2 has no effect.\n" );
            }

            // SV_ActivateServer
            EngineLog( "Searching for SV_ActivateServer...\n" );

            DWORD_PTR pSV_ActivateServer = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsSV_ActivateServer, &pSV_ActivateServer );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_SV_ActivateServer = (_SV_ActivateServer)pSV_ActivateServer;
                EngineLog( "Found SV_ActivateServer at %p (using the build %s pattern).\n", pSV_ActivateServer, Patterns::ptnsSV_ActivateServer[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "Could not find SV_ActivateServer!\n" );
                EngineWarning( "y_spt_pause 2 has no effect.\n" );
            }

            // FinishRestore
            EngineLog( "Searching for FinishRestore...\n" );

            DWORD_PTR pFinishRestore = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsFinishRestore, &pFinishRestore );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_FinishRestore = (_FinishRestore)pFinishRestore;
                EngineLog( "Found FinishRestore at %p (using the build %s pattern).\n", pFinishRestore, Patterns::ptnsFinishRestore[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "Could not find FinishRestore!\n" );
                EngineWarning( "y_spt_pause 1 has no effect.\n" );
            }

            // SetPaused
            EngineLog( "Searching for SetPaused...\n" );

            DWORD_PTR pSetPaused = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsSetPaused, &pSetPaused );
            if (pSetPaused)
            {
                hookState.ORIG_SetPaused = (_SetPaused)pSetPaused;
                EngineLog( "Found SetPaused at %p (using the build %s pattern).\n", pSetPaused, Patterns::ptnsSetPaused[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "Could not find SetPaused!\n" );
                EngineWarning( "y_spt_pause has no effect.\n" );
            }

            AttachDetours( moduleName, 6,
                &hookState.ORIG_SV_ActivateServer, Internal::HOOKED_SV_ActivateServer,
                &hookState.ORIG_FinishRestore, Internal::HOOKED_FinishRestore,
                &hookState.ORIG_SetPaused, Internal::HOOKED_SetPaused );
        }

        void Unhook( std::wstring &moduleName )
        {
            DetachDetours( moduleName, 6,
                &hookState.ORIG_SV_ActivateServer, Internal::HOOKED_SV_ActivateServer,
                &hookState.ORIG_FinishRestore, Internal::HOOKED_FinishRestore,
                &hookState.ORIG_SetPaused, Internal::HOOKED_SetPaused );

            Clear();
        }

        void Clear()
        {
            hookState.moduleInfo.hModule = NULL;
            hookState.moduleInfo.moduleStart = NULL;
            hookState.moduleInfo.moduleLength = NULL;
            hookState.ORIG_SV_ActivateServer = NULL;
            hookState.ORIG_FinishRestore = NULL;
            hookState.ORIG_SetPaused = NULL;
            hookState.pGameServer = NULL;
            hookState.pM_bLoadgame = NULL;
            hookState.shouldPreventNextUnpause = false;
        }
    }

    namespace ClientDll
    {
        struct
        {
            module_info_t moduleInfo;

            _DoImageSpaceMotionBlur ORIG_DoImageSpaceMorionBlur;

            DWORD_PTR *pgpGlobals;
        } hookState;

        namespace Internal
        {
            void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h )
            {
                DWORD_PTR origgpGlobals = NULL;

                /*
                Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
                so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
                function does not use anything apart from gpGlobals->realtime from gpGlobals,
                so we can do such a replace to make it use gpGlobals->curtime instead without
                breaking anything else.
                */
                if (hookState.pgpGlobals)
                {
                    if (y_spt_motion_blur_fix.GetBool())
                    {
                        origgpGlobals = *hookState.pgpGlobals;
                        *hookState.pgpGlobals = *hookState.pgpGlobals + 12;
                    }
                }

                hookState.ORIG_DoImageSpaceMorionBlur( view, x, y, w, h );

                if (hookState.pgpGlobals)
                {
                    if (y_spt_motion_blur_fix.GetBool())
                    {
                        *hookState.pgpGlobals = origgpGlobals;
                    }
                }
            }
        }

        void Hook( std::wstring &moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength )
        {
            Clear(); // Just in case.

            hookState.moduleInfo.hModule = hModule;
            hookState.moduleInfo.moduleStart = moduleStart;
            hookState.moduleInfo.moduleLength = moduleLength;

            MemUtils::ptnvec_size ptnNumber;

            // DoImageSpaceMotionBlur
            EngineLog( "Searching for DoImageSpaceMotionBlur...\n" );

            DWORD_PTR pDoImageSpaceMotionBlur = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsDoImageSpaceMotionBlur, &pDoImageSpaceMotionBlur );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_DoImageSpaceMorionBlur = (_DoImageSpaceMotionBlur)pDoImageSpaceMotionBlur;
                Log( "SPT: Found DoImageSpaceMotionBlur at %p (using the build %s pattern).\n", pDoImageSpaceMotionBlur, Patterns::ptnsDoImageSpaceMotionBlur[ptnNumber].build.c_str() );

                switch (ptnNumber)
                {
                case 0:
                    hookState.pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 132);
                    break;

                case 1:
                    hookState.pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 153);
                    break;

                case 2:
                    hookState.pgpGlobals = *(DWORD_PTR **)(pDoImageSpaceMotionBlur + 129);
                    break;
                }

                Log( "SPT: pgpGlobals is %p.\n", hookState.pgpGlobals );
            }
            else
            {
                Warning( "SPT: Could not find DoImageSpaceMotionBlur!\n" );
                Warning( "SPT: y_spt_motion_blur_fix has no effect.\n" );
            }

            AttachDetours( moduleName, 2,
                &hookState.ORIG_DoImageSpaceMorionBlur, Internal::HOOKED_DoImageSpaceMotionBlur );
        }

        void Unhook( std::wstring &moduleName )
        {
            DetachDetours( moduleName, 2,
                &hookState.ORIG_DoImageSpaceMorionBlur, Internal::HOOKED_DoImageSpaceMotionBlur );

            Clear();
        }

        void Clear()
        {
            hookState.moduleInfo.hModule = NULL;
            hookState.moduleInfo.moduleStart = NULL;
            hookState.moduleInfo.moduleLength = NULL;
            hookState.ORIG_DoImageSpaceMorionBlur = NULL;
            hookState.pgpGlobals = NULL;
        }
    }

    struct
    {
        _LoadLibraryA   ORIG_LoadLibraryA;
        _LoadLibraryW   ORIG_LoadLibraryW;
        _FreeLibrary    ORIG_FreeLibrary;

        std::unordered_map<std::wstring, HMODULE> hookedModules;
    } hookState;

    namespace Internal
    {
        HMODULE WINAPI HOOKED_LoadLibraryA( LPCSTR lpLibFileName )
        {
            HMODULE rv = hookState.ORIG_LoadLibraryA( lpLibFileName );

            EngineDevLog( "Engine call: LoadLibraryA( \"%s\" ) => %p\n", lpLibFileName, rv );

            if (rv != NULL)
            {
                HookModule( utf8util::UTF16FromUTF8( lpLibFileName ) );
            }

            return rv;
        }

        HMODULE WINAPI HOOKED_LoadLibraryW( LPCWSTR lpLibFileName )
        {
            HMODULE rv = hookState.ORIG_LoadLibraryW( lpLibFileName );

            EngineDevLog( "Engine call: LoadLibraryW( \"%s\" ) => %p\n", utf8util::UTF8FromUTF16( lpLibFileName ), rv );

            if (rv != NULL)
            {
                HookModule( std::wstring( lpLibFileName ) );
            }

            return rv;
        }

        BOOL WINAPI HOOKED_FreeLibrary( HMODULE hModule )
        {
            for (auto it : hookState.hookedModules)
            {
                if (it.second == hModule)
                {
                    UnhookModule( it.first );
                    break;
                }
            }

            BOOL rv = hookState.ORIG_FreeLibrary( hModule );

            EngineDevLog( "Engine call: FreeLibrary( %p ) => %s\n", hModule, (rv ? "true" : "false") );

            return rv;
        }
    }

    void Init()
    {
        Clear();

        // Try hooking each module in case it is already loaded
        for (auto it : moduleHookList)
        {
            HookModule( it.first );
        }

        hookState.ORIG_LoadLibraryA = LoadLibraryA;
        hookState.ORIG_LoadLibraryW = LoadLibraryW;
        hookState.ORIG_FreeLibrary = FreeLibrary;

        AttachDetours( L"WinApi", 6,
            &hookState.ORIG_LoadLibraryA, Internal::HOOKED_LoadLibraryA,
            &hookState.ORIG_LoadLibraryW, Internal::HOOKED_LoadLibraryW,
            &hookState.ORIG_FreeLibrary, Internal::HOOKED_FreeLibrary );
    }

    void Free()
    {
        // Unhook everything
        for (auto it : moduleHookList)
        {
            UnhookModule( it.first );
        }

        DetachDetours( L"WinApi", 6,
            &hookState.ORIG_LoadLibraryA, Internal::HOOKED_LoadLibraryA,
            &hookState.ORIG_LoadLibraryW, Internal::HOOKED_LoadLibraryW,
            &hookState.ORIG_FreeLibrary, Internal::HOOKED_FreeLibrary );

        Clear();
    }

    void Clear()
    {
        hookState.ORIG_LoadLibraryA = NULL;
        hookState.ORIG_LoadLibraryW = NULL;
        hookState.ORIG_FreeLibrary = NULL;
        hookState.hookedModules.clear();
    }

    void HookModule( std::wstring moduleName )
    {
        auto module = moduleHookList.find( moduleName );
        if (module != moduleHookList.end())
        {
            HMODULE targetModule;
            size_t targetModuleStart, targetModuleLength;
            if (MemUtils::GetModuleInfo( moduleName.c_str(), targetModule, targetModuleStart, targetModuleLength ))
            {
                EngineLog( "Hooking %s (start: %p; size: %x)...\n", utf8util::UTF8FromUTF16( moduleName ).c_str(), targetModuleStart, targetModuleLength );
                module->second.Hook( moduleName, targetModule, targetModuleStart, targetModuleLength );
                hookState.hookedModules.insert( std::pair<std::wstring, HMODULE> ( moduleName, targetModule ) );
            }
            else
            {
                EngineWarning( "Unable to obtain the %s module info!\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
            }
        }
        else
        {
            EngineDevLog( "Tried to hook an unlisted module: %s\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
        }
    }

    void UnhookModule( std::wstring moduleName )
    {
        auto module = moduleHookList.find( moduleName );
        if (module != moduleHookList.end())
        {
            EngineLog( "Unhooking %s...\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
            module->second.Unhook( moduleName );
            hookState.hookedModules.erase( moduleName );
        }
        else
        {
            EngineDevLog( "Tried to unhook an unlisted module: %s\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
        }
    }
}