#ifndef __HOOKS_H__
#define __HOOKS_H__

#ifdef _WIN32
#pragma once
#endif

#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "OrangeBox\modules\EngineDLL.hpp"
#include "OrangeBox\modules\ClientDLL.hpp"
#include "OrangeBox\modules\ServerDLL.hpp"

//// Hooked functions
//namespace ClientDll
//{
//	namespace Internal
//	{
//		void __cdecl HOOKED_DoImageSpaceMotionBlur( void *view, int x, int y, int w, int h );
//		bool __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx );
//	}
//
//	typedef void( __cdecl *_DoImageSpaceMotionBlur ) ( void *view, int x, int y, int w, int h );
//	typedef bool( __fastcall *_CheckJumpButton ) ( void *thisptr, int edx );
//}
//
//namespace ServerDll
//{
//	namespace Internal
//	{
//		bool __fastcall HOOKED_CheckJumpButton( void *thisptr, int edx );
//	}
//
//	typedef bool( __fastcall *_CheckJumpButton ) ( void *thisptr, int edx );
//}

typedef HMODULE(WINAPI *_LoadLibraryA) (LPCSTR lpLFileName);
typedef HMODULE(WINAPI *_LoadLibraryW) (LPCWSTR lpFileName);
typedef HMODULE(WINAPI *_LoadLibraryExA) (LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI *_LoadLibraryExW) (LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);
typedef BOOL(WINAPI *_FreeLibrary) (HMODULE hModule);

class Hooks
{
public:
	EngineDLL engineDLL;
	ClientDLL clientDLL;
	ServerDLL serverDLL;

	static Hooks& getInstance()
	{
		static Hooks instance;
		return instance;
	}

	void Init();
	void Free();
	void Clear();

	void HookModule(std::wstring moduleName);
	void UnhookModule(std::wstring moduleName);

	void AddToHookedModules(IHookableModule* module);

	HMODULE WINAPI HOOKED_LoadLibraryA_Func(LPCSTR lpFileName);
	HMODULE WINAPI HOOKED_LoadLibraryW_Func(LPCWSTR lpFileName);
	HMODULE WINAPI HOOKED_LoadLibraryExA_Func(LPCSTR lpFileName, HANDLE hFile, DWORD dwFlags);
	HMODULE WINAPI HOOKED_LoadLibraryExW_Func(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags);
	BOOL WINAPI HOOKED_FreeLibrary_Func(HMODULE hModule);

private:
	Hooks() {}
	Hooks(Hooks const&);
	void operator=(Hooks const&);

protected:
	std::vector<IHookableModule*> modules;

	_LoadLibraryA   ORIG_LoadLibraryA;
	_LoadLibraryW   ORIG_LoadLibraryW;
	_LoadLibraryExA   ORIG_LoadLibraryExA;
	_LoadLibraryExW   ORIG_LoadLibraryExW;
	_FreeLibrary    ORIG_FreeLibrary;
};

#endif // __HOOKS_H__
