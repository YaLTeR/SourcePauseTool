#include "stdafx.h"
#pragma once

#include <limits>
#include <vector>

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
	uintptr_t HookVTable(const uintptr_t* vtable, const size_t index, const uintptr_t function);
}
