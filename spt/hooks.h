#ifndef __HOOKS_H__
#define __HOOKS_H__

#ifdef _WIN32
#pragma once
#endif

#include <string>
#include <vector>

#include "memutils.h"

namespace Hooks
{
    typedef void( *_HookFunc ) ( std::wstring moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength );
    typedef void( *_UnHookFunc ) ( std::wstring moduleName );

    typedef struct
    {
        _HookFunc Hook;
        _UnHookFunc Unhook;
    } hookFuncList_t;

    typedef struct
    {
        HMODULE hModule;
        size_t moduleStart;
        size_t moduleLength;
    } module_info_t;

    // Hooked functions
    namespace EngineDll
    {
        namespace Internal
        {
            bool __cdecl HOOKED_SV_ActivateServer( );
            void __fastcall HOOKED_FinishRestore( void *thisptr, int edx );
            void __fastcall HOOKED_SetPaused( void *thisptr, int edx, bool paused );
        }

        typedef bool( __cdecl *_SV_ActivateServer ) ();
        typedef void( __fastcall *_FinishRestore ) ( void *thisptr, int edx );
        typedef void( __fastcall *_SetPaused ) ( void *thisptr, int edx, bool paused );

        void Hook( std::wstring moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength );
        void Unhook( std::wstring moduleName );
        void Clear();
    }

    namespace ClientDll
    {
        namespace Internal
        {
            void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h );
        }

        typedef void( __cdecl *_DoImageSpaceMotionBlur ) ( void *view, int x, int y, int w, int h );
        
        void Hook( std::wstring moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength );
        void Unhook( std::wstring moduleName );
        void Clear();
    }

    namespace Internal
    {
        HMODULE WINAPI HOOKED_LoadLibraryA( LPCSTR lpLibFileName );
        HMODULE WINAPI HOOKED_LoadLibraryW( LPCWSTR lpLibFileName );
        BOOL WINAPI HOOKED_FreeLibrary( HMODULE hModule );
    }

    typedef HMODULE( WINAPI *_LoadLibraryA ) ( LPCSTR lpLibFileName );
    typedef HMODULE( WINAPI *_LoadLibraryW ) ( LPCWSTR lpLibFileName );
    typedef BOOL( WINAPI *_FreeLibrary ) ( HMODULE hModule );

    void Init();
    void Free();
    void Clear();

    void HookModule( std::wstring moduleName );
    void UnhookModule( std::wstring moduleName );
}

#endif // __HOOKS_H__