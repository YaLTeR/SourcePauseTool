#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <detours.h>

#include "../utf8conv/utf8conv.hpp"

#include "hooks.hpp"
#include "detoursutils.hpp"
#include "memutils.hpp"
#include "patterns.hpp"
#include "spt.hpp"

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

    struct
    {
        _LoadLibraryA   ORIG_LoadLibraryA;
        _LoadLibraryW   ORIG_LoadLibraryW;
        _LoadLibraryExA   ORIG_LoadLibraryExA;
        _LoadLibraryExW   ORIG_LoadLibraryExW;
        _FreeLibrary    ORIG_FreeLibrary;

        std::unordered_map<std::wstring, HMODULE> hookedModules;
    } hookState;

    namespace Internal
    {
        HMODULE WINAPI HOOKED_LoadLibraryA( LPCSTR lpFileName )
        {
            HMODULE rv = hookState.ORIG_LoadLibraryA( lpFileName );

            EngineDevLog( "SPT: Engine call: LoadLibraryA( \"%s\" ) => %p\n", lpFileName, rv );

            if (rv != NULL)
            {
                HookModule( utf8util::UTF16FromUTF8( lpFileName ) );
            }

            return rv;
        }

        HMODULE WINAPI HOOKED_LoadLibraryW( LPCWSTR lpFileName )
        {
            HMODULE rv = hookState.ORIG_LoadLibraryW( lpFileName );

            EngineDevLog( "SPT: Engine call: LoadLibraryW( \"%s\" ) => %p\n", utf8util::UTF8FromUTF16( lpFileName ), rv );

            if (rv != NULL)
            {
                HookModule( std::wstring( lpFileName ) );
            }

            return rv;
        }

        HMODULE WINAPI HOOKED_LoadLibraryExA( LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags )
        {
            HMODULE rv = hookState.ORIG_LoadLibraryExA( lpFileName, hFile, dwFlags );

            EngineDevLog( "SPT: Engine call: LoadLibraryExA( \"%s\" ) => %p\n", lpFileName, rv );

            if (rv != NULL)
            {
                HookModule( utf8util::UTF16FromUTF8( lpFileName ) );
            }

            return rv;
        }

        HMODULE WINAPI HOOKED_LoadLibraryExW( LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags )
        {
            HMODULE rv = hookState.ORIG_LoadLibraryExW( lpFileName, hFile, dwFlags );

            EngineDevLog( "SPT: Engine call: LoadLibraryExW( \"%s\" ) => %p\n", utf8util::UTF8FromUTF16( lpFileName ), rv );

            if (rv != NULL)
            {
                HookModule( std::wstring( lpFileName ) );
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

            EngineDevLog( "SPT: Engine call: FreeLibrary( %p ) => %s\n", hModule, (rv ? "true" : "false") );

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
        hookState.ORIG_LoadLibraryExA = LoadLibraryExA;
        hookState.ORIG_LoadLibraryExW = LoadLibraryExW;
        hookState.ORIG_FreeLibrary = FreeLibrary;

        AttachDetours( L"WinApi", 10,
            &hookState.ORIG_LoadLibraryA, Internal::HOOKED_LoadLibraryA,
            &hookState.ORIG_LoadLibraryW, Internal::HOOKED_LoadLibraryW,
            &hookState.ORIG_LoadLibraryExA, Internal::HOOKED_LoadLibraryExA,
            &hookState.ORIG_LoadLibraryExW, Internal::HOOKED_LoadLibraryExW,
            &hookState.ORIG_FreeLibrary, Internal::HOOKED_FreeLibrary );
    }

    void Free()
    {
        // Unhook everything
        for (auto it : moduleHookList)
        {
            UnhookModule( it.first );
        }

        DetachDetours( L"WinApi", 10,
            &hookState.ORIG_LoadLibraryA, Internal::HOOKED_LoadLibraryA,
            &hookState.ORIG_LoadLibraryW, Internal::HOOKED_LoadLibraryW,
            &hookState.ORIG_LoadLibraryExA, Internal::HOOKED_LoadLibraryExA,
            &hookState.ORIG_LoadLibraryExW, Internal::HOOKED_LoadLibraryExW,
            &hookState.ORIG_FreeLibrary, Internal::HOOKED_FreeLibrary );

        Clear();
    }

    void Clear()
    {
        hookState.ORIG_LoadLibraryA = NULL;
        hookState.ORIG_LoadLibraryW = NULL;
        hookState.ORIG_LoadLibraryExA = NULL;
        hookState.ORIG_LoadLibraryExW = NULL;
        hookState.ORIG_FreeLibrary = NULL;
        hookState.hookedModules.clear();
    }

    void HookModule( std::wstring moduleName )
    {
        moduleName = GetFileName( moduleName );

        auto module = moduleHookList.find( moduleName );
        if (module != moduleHookList.end())
        {
            HMODULE targetModule;
            size_t targetModuleStart, targetModuleLength;
            if (MemUtils::GetModuleInfo( moduleName.c_str(), targetModule, targetModuleStart, targetModuleLength ))
            {
                EngineLog( "SPT: Hooking %s (start: %p; size: %x)...\n", utf8util::UTF8FromUTF16( moduleName ).c_str(), targetModuleStart, targetModuleLength );
                module->second.Hook( moduleName, targetModule, targetModuleStart, targetModuleLength );
                hookState.hookedModules.insert( std::pair<std::wstring, HMODULE> ( moduleName, targetModule ) );
            }
            else
            {
                EngineWarning( "SPT: Unable to obtain the %s module info!\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
            }
        }
        else
        {
            EngineDevLog( "SPT: Tried to hook an unlisted module: %s\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
        }
    }

    void UnhookModule( std::wstring moduleName )
    {
        moduleName = GetFileName( moduleName );

        auto module = moduleHookList.find( moduleName );
        if (module != moduleHookList.end())
        {
            EngineLog( "SPT: Unhooking %s...\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
            module->second.Unhook( moduleName );
            hookState.hookedModules.erase( moduleName );
        }
        else
        {
            EngineDevLog( "SPT: Tried to unhook an unlisted module: %s\n", utf8util::UTF8FromUTF16( moduleName ).c_str() );
        }
    }
}
