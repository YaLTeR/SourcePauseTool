#include "memutils.h"
#include <limits>
#include <Psapi.h>

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

        moduleBase = (size_t)moduleInfo.lpBaseOfDll;
        moduleSize = (size_t)moduleInfo.SizeOfImage;

        return true;
    }

    bool GetModuleInfo( const WCHAR *szModuleName, HMODULE &moduleHandle, size_t &moduleBase, size_t &moduleSize )
    {
        HANDLE hProcess = GetCurrentProcess( );
        moduleHandle = GetModuleHandleW( szModuleName );

        if (!hProcess || !moduleHandle)
        {
            return false;
        }

        MODULEINFO moduleInfo;
        GetModuleInformation( hProcess, moduleHandle, &moduleInfo, sizeof(moduleInfo) );

        moduleBase = (size_t)moduleInfo.lpBaseOfDll;
        moduleSize = (size_t)moduleInfo.SizeOfImage;

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

        moduleBase = (size_t)moduleInfo.lpBaseOfDll;
        moduleSize = (size_t)moduleInfo.SizeOfImage;

        return true;
    }

    inline bool DataCompare(const BYTE *pData, const BYTE *pSig, const char *szPattern)
    {
        for (; *szPattern != NULL; ++pData, ++pSig, ++szPattern)
        {
            if (*szPattern == 'x' && *pData != *pSig)
            {
                return false;
            }
        }

        return (*szPattern == NULL);
    }

    DWORD_PTR FindPattern(size_t dwStart, size_t dwLength, const BYTE *pSig, const char *szMask)
    {
        for (DWORD_PTR i = NULL; i < dwLength; i++)
        {
            if (DataCompare((BYTE *)(dwStart + i), pSig, szMask))
            {
                return (DWORD_PTR)(dwStart + i);
            }
        }

        return NULL;
    }

    ptnvec_size FindUniqueSequence(size_t dwStart, size_t dwLength, const ptnvec &patterns, DWORD_PTR *pdwAddress)
    {
        for (ptnvec_size i = 0; i < patterns.size(); i++)
        {
            DWORD_PTR address = FindPattern(dwStart, dwLength, patterns[i].pattern.data(), patterns[i].mask.c_str());
            if (address)
            {
                size_t newSize = dwLength - (address - dwStart + 1);
                if ( NULL == FindPattern(address + 1, newSize, patterns[i].pattern.data(), patterns[i].mask.c_str()) )
                {
                    if (pdwAddress)
                    {
                        *pdwAddress = address;
                    }

                    return i; // Return the number of the pattern.
                }
                else
                {
                    if (pdwAddress)
                    {
                        *pdwAddress = NULL;
                    }

                    return INVALID_SEQUENCE_INDEX; // Bogus sequence.
                }
            }
        }

        if (pdwAddress)
        {
            *pdwAddress = NULL;
        }

        return INVALID_SEQUENCE_INDEX; // Didn't find anything.
    }

    void ReplaceBytes(const DWORD_PTR dwAddr, const size_t dwLength, const BYTE *pNewBytes)
    {
        DWORD dwOldProtect;

        VirtualProtect((void *)dwAddr, dwLength, PAGE_EXECUTE_READWRITE, &dwOldProtect);

        for (DWORD_PTR i = NULL; i < dwLength; i++)
        {
            *(BYTE *)(dwAddr + i) = pNewBytes[i];
        }

        VirtualProtect((void *)dwAddr, dwLength, dwOldProtect, NULL);
    }
}