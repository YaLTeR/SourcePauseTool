#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

#ifdef _WIN32
#pragma once
#endif

#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace MemUtils
{
    typedef struct
    {
        std::string build;
        std::vector<BYTE> pattern;
        std::string mask;
    } pattern_def_t;

    typedef std::vector<pattern_def_t> ptnvec;
    typedef std::vector<pattern_def_t>::size_type ptnvec_size;

#undef max // Just in case
    const ptnvec_size INVALID_SEQUENCE_INDEX = std::numeric_limits<ptnvec_size>::max();

    bool GetModuleInfo(const WCHAR *szModuleName, size_t &moduleBase, size_t &moduleSize);
    bool GetModuleInfo(HMODULE hModule, size_t &moduleBase, size_t &moduleSize);

    inline bool DataCompare(const BYTE *pData, const BYTE *pSig, const char *szPattern);
    DWORD_PTR FindPattern(size_t dwStart, size_t dwLength, const BYTE *pSig, const char *szMask);

    ptnvec_size FindUniqueSequence(size_t dwStart, size_t dwLength, const ptnvec &patterns, DWORD_PTR *pdwAddress = NULL);

    void ReplaceBytes(const DWORD_PTR dwAddr, const size_t length, const BYTE *pNewBytes);
}

#endif // __MEMUTILS_H__