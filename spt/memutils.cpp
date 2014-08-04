#include "stdafx.h"

#include <limits>

#include <Psapi.h>
#include "memutils.hpp"

#pragma comment( lib, "psapi.lib" )

using std::uintptr_t;
using std::size_t;

namespace MemUtils
{
	bool GetModuleInfo(const std::wstring& szModuleName, uintptr_t* moduleBase, size_t* moduleSize)
	{
		HANDLE hProcess = GetCurrentProcess();
		HMODULE hModule = GetModuleHandleW(szModuleName.c_str());

		if (!hProcess || !hModule)
		{
			return false;
		}

		MODULEINFO moduleInfo;
		GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(moduleInfo));

		if (moduleBase)
			*moduleBase = (uintptr_t)moduleInfo.lpBaseOfDll;

		if (moduleSize)
			*moduleSize = (size_t)moduleInfo.SizeOfImage;

		return true;
	}

	bool GetModuleInfo(const std::wstring& szModuleName, HMODULE* moduleHandle, uintptr_t* moduleBase, size_t* moduleSize)
	{
		HANDLE hProcess = GetCurrentProcess();
		HMODULE hModule = GetModuleHandleW(szModuleName.c_str());

		if (!hProcess || !hModule)
		{
			return false;
		}

		MODULEINFO moduleInfo;
		GetModuleInformation( hProcess, hModule, &moduleInfo, sizeof(moduleInfo) );

		if (moduleHandle)
			*moduleHandle = hModule;

		if (moduleBase)
			*moduleBase = (uintptr_t)moduleInfo.lpBaseOfDll;

		if (moduleSize)
			*moduleSize = (size_t)moduleInfo.SizeOfImage;

		return true;
	}

	bool GetModuleInfo(HMODULE hModule, uintptr_t* moduleBase, size_t* moduleSize)
	{
		HANDLE hProcess = GetCurrentProcess();

		if (!hProcess || !hModule)
		{
			return false;
		}

		MODULEINFO moduleInfo;
		GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(moduleInfo));

		if (moduleBase)
			*moduleBase = (uintptr_t)moduleInfo.lpBaseOfDll;

		if (moduleSize)
			*moduleSize = (size_t)moduleInfo.SizeOfImage;

		return true;
	}

	inline bool DataCompare(const byte* pData, const byte* pSig, const char* szPattern)
	{
		for (; *szPattern != 0; ++pData, ++pSig, ++szPattern)
		{
			if (*szPattern == 'x' && *pData != *pSig)
			{
				return false;
			}
		}

		return (*szPattern == 0);
	}

	uintptr_t FindPattern(uintptr_t start, size_t length, const byte* pSig, const char* szMask)
	{
		for (uintptr_t i = NULL; i < length; i++)
		{
			if (DataCompare((byte *)(start + i), pSig, szMask))
			{
				return (start + i);
			}
		}

		return NULL;
	}

	ptnvec_size FindUniqueSequence(uintptr_t start, size_t length, const ptnvec& patterns, uintptr_t* pdwAddress)
	{
		for (ptnvec_size i = 0; i < patterns.size(); i++)
		{
			uintptr_t address = FindPattern(start, length, patterns[i].pattern.data(), patterns[i].mask.c_str());
			if (address)
			{
				size_t newSize = length - (address - start + 1);
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

	void ReplaceBytes(const uintptr_t addr, const size_t length, const byte* pNewBytes)
	{
		DWORD dwOldProtect;

		VirtualProtect((void *)addr, length, PAGE_EXECUTE_READWRITE, &dwOldProtect);

		for (uintptr_t i = NULL; i < length; i++)
		{
			*(byte *)(addr + i) = pNewBytes[i];
		}

		VirtualProtect((void *)addr, length, dwOldProtect, NULL);
	}

	uintptr_t HookVTable(const uintptr_t* vtable, const size_t index, const uintptr_t function)
	{
		uintptr_t oldFunction = vtable[index];

		ReplaceBytes( (uintptr_t)( &(vtable[index]) ), sizeof(uintptr_t), (byte*)&function );

		return oldFunction;
	}
}
