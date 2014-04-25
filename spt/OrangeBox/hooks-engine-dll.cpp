#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>

#include "../hooks.hpp"
#include "../detoursutils.hpp"
#include "../memutils.hpp"
#include "../patterns.hpp"
#include "../spt.hpp"
#include "cvars.hpp"

namespace Hooks
{
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

                EngineDevLog( "SPT: Engine call: SV_ActivateServer() => %s;\n", (result ? "true" : "false") );

                if (hookState.ORIG_SetPaused && hookState.pM_bLoadgame && hookState.pGameServer)
                {
                    if ((y_spt_pause.GetInt() == 2) && *hookState.pM_bLoadgame)
                    {
                        hookState.ORIG_SetPaused( (void *)hookState.pGameServer, 0, true );
                        EngineDevLog( "SPT: Pausing...\n" );

                        hookState.shouldPreventNextUnpause = true;
                    }
                }

                return result;
            }

            void __fastcall HOOKED_FinishRestore( void *thisptr, int edx )
            {
                EngineDevLog( "SPT: Engine call: FinishRestore();\n" );

                if (hookState.ORIG_SetPaused && (y_spt_pause.GetInt() == 1))
                {
                    hookState.ORIG_SetPaused( thisptr, 0, true );
                    EngineDevLog( "SPT: Pausing...\n" );

                    hookState.shouldPreventNextUnpause = true;
                }

                return hookState.ORIG_FinishRestore( thisptr, edx );
            }

            void __fastcall HOOKED_SetPaused( void *thisptr, int edx, bool paused )
            {
                if (hookState.pM_bLoadgame)
                {
                    EngineDevLog( "SPT: Engine call: SetPaused( %s ); m_bLoadgame = %s\n", (paused ? "true" : "false"), (*hookState.pM_bLoadgame ? "true" : "false") );
                }
                else
                {
                    EngineDevLog( "SPT: Engine call: SetPaused( %s );\n", (paused ? "true" : "false") );
                }

                if (paused == false)
                {
                    if (hookState.shouldPreventNextUnpause)
                    {
                        EngineDevLog( "SPT: Unpause prevented.\n" );
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
            EngineDevLog( "SPT: Searching for SpawnPlayer...\n" );

            DWORD_PTR pSpawnPlayer = NULL;
            ptnNumber = MemUtils::FindUniqueSequence(hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsSpawnPlayer, &pSpawnPlayer);
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                EngineLog( "SPT: Found SpawnPlayer at %p (using the build %s pattern).\n", pSpawnPlayer, Patterns::ptnsSpawnPlayer[ptnNumber].build.c_str() );

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

                EngineLog( "SPT: m_bLoadGame is situated at %p.\n", hookState.pM_bLoadgame );
                EngineLog( "SPT: pGameServer is %p.\n", hookState.pGameServer );
            }
            else
            {
                EngineWarning( "SPT: Could not find SpawnPlayer!\n" );
                EngineWarning( "SPT: y_spt_pause 2 has no effect.\n" );
            }

            // SV_ActivateServer
            EngineDevLog( "SPT: Searching for SV_ActivateServer...\n" );

            DWORD_PTR pSV_ActivateServer = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsSV_ActivateServer, &pSV_ActivateServer );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_SV_ActivateServer = (_SV_ActivateServer)pSV_ActivateServer;
                EngineLog( "SPT: Found SV_ActivateServer at %p (using the build %s pattern).\n", pSV_ActivateServer, Patterns::ptnsSV_ActivateServer[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "SPT: Could not find SV_ActivateServer!\n" );
                EngineWarning( "SPT: y_spt_pause 2 has no effect.\n" );
            }

            // FinishRestore
            EngineDevLog( "SPT: Searching for FinishRestore...\n" );

            DWORD_PTR pFinishRestore = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsFinishRestore, &pFinishRestore );
            if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
            {
                hookState.ORIG_FinishRestore = (_FinishRestore)pFinishRestore;
                EngineLog( "SPT: Found FinishRestore at %p (using the build %s pattern).\n", pFinishRestore, Patterns::ptnsFinishRestore[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "SPT: Could not find FinishRestore!\n" );
                EngineWarning( "SPT: y_spt_pause 1 has no effect.\n" );
            }

            // SetPaused
            EngineDevLog( "SPT: Searching for SetPaused...\n" );

            DWORD_PTR pSetPaused = NULL;
            ptnNumber = MemUtils::FindUniqueSequence( hookState.moduleInfo.moduleStart, hookState.moduleInfo.moduleLength, Patterns::ptnsSetPaused, &pSetPaused );
            if (pSetPaused)
            {
                hookState.ORIG_SetPaused = (_SetPaused)pSetPaused;
                EngineLog( "SPT: Found SetPaused at %p (using the build %s pattern).\n", pSetPaused, Patterns::ptnsSetPaused[ptnNumber].build.c_str() );
            }
            else
            {
                EngineWarning( "SPT: Could not find SetPaused!\n" );
                EngineWarning( "SPT: y_spt_pause has no effect.\n" );
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
}