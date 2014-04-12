#include <unordered_map>

#include "hooks.h"
#include "spt.h"

namespace Hooks
{
    std::unordered_map<std::string, hookFuncList_t> moduleHookList =
    {
        {
            "engine.dll",
            {
                EngineDll::Hook,
                EngineDll::Unhook
            }
        },

        {
            "client.dll",
            {
                ClientDll::Hook,
                ClientDll::Unhook
            }
        }
    };

    namespace Internal
    {

    }

    namespace EngineDll
    {
        struct
        {
            dll_info_t dllInfo;

            _SV_ActivateServer ORIG_SV_ActivateServer;
            _FinishRestore ORIG_FinishRestore;
            _SetPaused ORIG_SetPaused;

            DWORD_PTR pGameServer;
            bool *pM_bLoadgame;
            bool shouldPreventNextUnpause;
        } hookState;

        void Hook()
        {
            // TODO
        }

        void Unhook()
        {
            // TODO
        }
    }

    namespace ClientDll
    {
        struct
        {
            dll_info_t dllInfo;

            _DoImageSpaceMotionBlur ORIG_DoImageSpaceMorionBlur;

            DWORD_PTR *pgpGlobals;
        } hookState;

        void Hook()
        {
            // TODO
        }

        void Unhook()
        {
            // TODO
        }
    }

    void Init()
    {
        // Try hooking each module in case it is already loaded
        for (auto it : moduleHookList)
        {
            HookModule( it.first );
        }
    }

    void Free()
    {
        // Unhook everything
        for (auto it : moduleHookList)
        {
            UnhookModule( it.first );
        }
    }

    void HookModule( std::string moduleName )
    {
        auto module = moduleHookList.find( moduleName );
        if (module != moduleHookList.end())
        {
            EngineLog( "Trying to hook a module: %s\n", moduleName.c_str() );
        }
        else
        {
            EngineDevLog( "Tried to hook an unlisted module: %s\n", moduleName.c_str() );
        }
    }

    void UnhookModule( std::string moduleName )
    {
        auto module = moduleHookList.find( moduleName );
        if (module != moduleHookList.end())
        {
            EngineLog( "Trying to unhook a module: %s\n", moduleName.c_str() );
        }
        else
        {
            EngineDevLog( "Tried to unhook an unlisted module: %s\n", moduleName.c_str() );
        }
    }
}