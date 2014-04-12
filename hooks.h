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
    typedef void( *_HookFunc ) ();

    typedef struct
    {
        _HookFunc Hook;
        _HookFunc Unhook;
    } hookFuncList_t;

    typedef struct
    {
        HANDLE hDll;
        size_t dllStart;
        size_t dllLength;
    } dll_info_t;

    // Hooked functions
    namespace Internal
    {
        HMODULE WINAPI HOOKED_LoadLibraryA( LPCSTR lpLibFileName );
        HMODULE WINAPI HOOKED_LoadLibraryW( LPCWSTR lpLibFileName );
        BOOL WINAPI HOOKED_FreeLibrary( HMODULE hModule );
    }

    namespace EngineDll
    {
        namespace Internal
        {
            bool __cdecl HOOKED_SV_ActivateServer( );
            void __fastcall HOOKED_FinishRestore( void *thisptr, int edx );
            void __fastcall HOOKED_SetPaused( void *thisptr, int edx, bool paused );
        }

        typedef bool( __cdecl *_SV_ActivateServer ) ();
        typedef void( __fastcall *_FinishRestore ) (void *thisptr, int edx);
        typedef void( __fastcall *_SetPaused ) (void *thisptr, int edx, bool paused);

        void Hook();
        void Unhook();
    }

    namespace ClientDll
    {
        namespace Internal
        {
            void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h );
        }

        typedef void( __cdecl *_DoImageSpaceMotionBlur ) (void *view, int x, int y, int w, int h);
        
        void Hook();
        void Unhook();
    }

    void Init();
    void Free();
    void HookModule( std::string moduleName );
    void UnhookModule( std::string moduleName );
}

#endif // __HOOKS_H__