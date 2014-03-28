#ifndef __MEMUTILS_H_
#define __MEMUTILS_H_

#include <Windows.h>

namespace MemUtils
{
	bool GetModuleInfo(const WCHAR *szModuleName, size_t &moduleBase, size_t &moduleSize);
	bool GetModuleInfo(HMODULE hModule, size_t &moduleBase, size_t &moduleSize);

	inline bool DataCompare(const BYTE *pData, const BYTE *pSig, const char *szPattern);
	DWORD_PTR FindPattern(size_t dwStart, size_t dwLength, const BYTE *pSig, const char *szMask);

	void ReplaceBytes(const DWORD_PTR dwAddr, const size_t length, const BYTE *pNewBytes);
}

#endif