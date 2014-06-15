#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

#ifdef _WIN32
#pragma once
#endif

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using std::uintptr_t;
using std::size_t;

namespace MemUtils
{
	typedef unsigned char byte;

	typedef struct
	{
		std::string build;
		std::vector<unsigned char> pattern;
		std::string mask;
	} pattern_def_t;

	typedef std::vector<pattern_def_t> ptnvec;
	typedef std::vector<pattern_def_t>::size_type ptnvec_size;

#undef max // Just in case
	const ptnvec_size INVALID_SEQUENCE_INDEX = std::numeric_limits<ptnvec_size>::max();

	bool GetModuleInfo(const std::wstring& szModuleName, uintptr_t* moduleBase, size_t* moduleSize);
	bool GetModuleInfo(const std::wstring& szModuleName, HMODULE* moduleHandle, uintptr_t* moduleBase, size_t* moduleSize);
	bool GetModuleInfo(HMODULE hModule, uintptr_t* moduleBase, size_t* moduleSize);

	inline bool DataCompare(const byte* pData, const byte* pSig, const char* szPattern);
	uintptr_t FindPattern(uintptr_t start, size_t length, const byte* pSig, const char* szMask);

	ptnvec_size FindUniqueSequence(uintptr_t start, size_t length, const ptnvec& patterns, uintptr_t* pdwAddress = nullptr);

	void ReplaceBytes(const uintptr_t addr, const size_t length, const byte* pNewBytes);
}

#endif // __MEMUTILS_H__
