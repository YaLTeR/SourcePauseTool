#ifndef __HOOKS_H__
#define __HOOKS_H__

#ifdef _WIN32
#pragma once
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>

namespace Hooks
{
    typedef void( *_HookFunc ) ( std::wstring &moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength );
    typedef void( *_UnHookFunc ) ( std::wstring &moduleName );

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
            bool __cdecl HOOKED_SV_ActivateServer();
            void __fastcall HOOKED_FinishRestore( void *thisptr, int edx );
            void __fastcall HOOKED_SetPaused( void *thisptr, int edx, bool paused );
        }

        typedef bool( __cdecl *_SV_ActivateServer ) ();
        typedef void( __fastcall *_FinishRestore ) ( void *thisptr, int edx );
        typedef void( __fastcall *_SetPaused ) ( void *thisptr, int edx, bool paused );

        void Hook( std::wstring &moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength );
        void Unhook( std::wstring &moduleName );
        void Clear();
    }

    namespace ClientDll
    {
        namespace Internal
        {
            void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h );
            bool __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx );
        }

        typedef void( __cdecl *_DoImageSpaceMotionBlur ) ( void *view, int x, int y, int w, int h );
        typedef bool( __fastcall *_CheckJumpButton ) ( void *thisptr, int edx );

        void Hook( std::wstring &moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength );
        void Unhook( std::wstring &moduleName );
        void Clear();
    }

    namespace ServerDll
    {
        namespace Internal
        {
            bool __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx );
        }

        typedef bool( __fastcall *_CheckJumpButton ) ( void *thisptr, int edx );

        void Hook( std::wstring &moduleName, HMODULE hModule, size_t moduleStart, size_t moduleLength );
        void Unhook( std::wstring &moduleName );
        void Clear();
    }

    namespace Internal
    {
        HMODULE WINAPI HOOKED_LoadLibraryA( LPCSTR lpFileName );
        HMODULE WINAPI HOOKED_LoadLibraryW( LPCWSTR lpFileName );
        HMODULE WINAPI HOOKED_LoadLibraryExA( LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags );
        HMODULE WINAPI HOOKED_LoadLibraryExW( LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags );
        BOOL WINAPI HOOKED_FreeLibrary( HMODULE hModule );
    }

    typedef HMODULE( WINAPI *_LoadLibraryA ) ( LPCSTR lpLFileName );
    typedef HMODULE( WINAPI *_LoadLibraryW ) ( LPCWSTR lpFileName );
    typedef HMODULE( WINAPI *_LoadLibraryExA ) ( LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags );
    typedef HMODULE( WINAPI *_LoadLibraryExW ) ( LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags );
    typedef BOOL( WINAPI *_FreeLibrary ) ( HMODULE hModule );

    void Init();
    void Free();
    void Clear();

    void HookModule( std::wstring moduleName );
    void UnhookModule( std::wstring moduleName );
}

#endif // __HOOKS_H__
