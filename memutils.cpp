#include <Windows.h>
#include <Psapi.h>
#include "memutils.h"

#pragma comment( lib, "psapi.lib" )

namespace MemUtils
{
    bool GetModuleInfo(const WCHAR *szModuleName, size_t &moduleBase, size_t &moduleSize)
    {
        HANDLE hProcess = GetCurrentProcess();
        HMODULE hModule = GetModuleHandleW(szModuleName);

        if (!hProcess || !hModule)
        {
            return false;
        }

        MODULEINFO moduleInfo;
        GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(moduleInfo));

        moduleBase = (size_t) moduleInfo.lpBaseOfDll;
        moduleSize = (size_t) moduleInfo.SizeOfImage;

        return true;
    }

    bool GetModuleInfo(HMODULE hModule, size_t &moduleBase, size_t &moduleSize)
    {
        HANDLE hProcess = GetCurrentProcess();

        if (!hProcess || !hModule)
        {
            return false;
        }

        MODULEINFO moduleInfo;
        GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(moduleInfo));

        moduleBase = (size_t) moduleInfo.lpBaseOfDll;
        moduleSize = (size_t) moduleInfo.SizeOfImage;

        return true;
    }

    inline bool DataCompare(const BYTE *pData, const BYTE *pSig, const char *szPattern)
    {
        for ( ; *szPattern != NULL; ++pData, ++pSig, ++szPattern)
        {
            if (*szPattern == 'x' && *pData != *pSig)
            {
                return false;
            }
        }

        return (*szPattern == NULL);
    }

    DWORD FindPattern(DWORD dwStart, DWORD dwLength, BYTE *pSig, const char *szMask)
    {
        for (DWORD i = NULL; i < dwLength; i++)
        {
            if ( DataCompare( (BYTE *) (dwStart + i), pSig, szMask) )
            {
                return (DWORD) (dwStart + i);
            }
        }

        return NULL;
    }

    void ReplaceBytes(const DWORD dwAddr, const DWORD dwLength, const BYTE *pNewBytes)
    {
        DWORD dwOldProtect;

        VirtualProtect((void *) dwAddr, dwLength, PAGE_EXECUTE_READWRITE,&dwOldProtect);

        for (DWORD i = NULL; i < dwLength; i++)
        {
            *(BYTE *) (dwAddr + i) = pNewBytes[i];
        }

        VirtualProtect((void *) dwAddr, dwLength, dwOldProtect, NULL);
    }
}