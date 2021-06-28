#pragma once

#include "stdafx.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <iomanip>

using std::hex;
using std::stringstream;
using std::string;

// VARIABLE, SINGLE SIGNATURE SCANNING FUNCTION
// ARGS:
//		base:	where to start sigscanning
//		size:	how far to sigscan
//		sig:	signature
//		len:	number of bytes
//		out:	output var
#define BASIC_SIGSCAN(base, size, sig, len, out) \
	{ \
		auto pattern = patterns::Pattern<len>(sig); \
		auto wrapper = patterns::PatternWrapper{"", pattern}; \
\
		out = MemUtils::find_pattern(base, size, wrapper); \
	}

// STRING ADDRESS SEARCH FUNCTION
// ARGS:
//		base:	where to start sigscanning
//		size:	how far to sigscan
//		string:	string to search
//		len:	length of string
//		out:	output var
#define FIND_STRING_ADDR(base, size, string, len, out) \
	{ \
		char sig[256];\
		PatternsExt::toHexArray(string, sig, false);\
		BASIC_SIGSCAN(base, size, sig, len, out);\
	}

// DIRECT POINTER REFEENCE SEARCH FUNCTION
// ARGS:
//		base:	where to start sigscanning
//		size:	how far to sigscan
//		addr:	address of pointer to search references of
//		prefix:	bytes before pointer reference bytes in signatrue
//		suffix:	bytes after pointer reference bytes in signatrue
//		len:	length of evalualted signature
//		out:	output var
#define FIND_VAR_REF(base, size, addr, prefix, suffix, len, out) \
	{ \
		char sig[256]; \
		sig[0] = 0;\
		strcat(sig, prefix); strcat(sig, " ");\
		PatternsExt::toHexArray(addr, sig, true); \
		strcat(sig, " "); strcat(sig, suffix);\
		BASIC_SIGSCAN(base, size, sig, len, out); \
	}

using namespace std;

class PatternsExt
{
public:

	// CONVERT VAR TO SIGNATURE-FRIENDLY HEX BYTE ARRAY
	// ARGS:
	//		in:		input var
	//		out:	output var
	//		concat:	concatenate onto out var instead of overwriting
	static void toHexArray(const char* in, char* out, bool concat = false)
	{
		string s1 = in;
		stringstream ss;

		for (const auto& item : s1)
			ss << " " << hex << int(item);
		ss << " " << setfill('0') << setw(2) << right << hex << 00;

		concat ? strcat(out, ss.str().c_str()) : strcpy(out, ss.str().c_str());
	}

	// CONVERT VAR TO SIGNATURE-FRIENDLY HEX BYTE ARRAY
	// ARGS:
	//		in:		input var
	//		out:	output var
	//		concat:	concatenate onto out var instead of overwriting
	static void toHexArray(unsigned int in, char* out, bool concat = false)
	{
		stringstream ss;
		for (int i = 0; i <= 3; ++i)
		{
			unsigned int b = (in >> 8 * i) & 0xFF;
			ss << " " << setfill('0') << setw(2) << right << hex << b;
		}

		concat ? strcat(out, ss.str().c_str()) : strcpy(out, ss.str().c_str());
	}

	// BACKTRACE FUNCTION
	// ARGS:
	//		addr:	address within function to begin backtracing
	//		out:	output var
	static uintptr_t BackTraceToFuncStart_Naive(uintptr_t addr)
	{
		int limit = 0x5000;
		const char bytes[3] = {0xCC, 0xCC, 0xCC};
		const char bytes2[3] = {0x90, 0x90, 0x90};
		uintptr_t ptr = addr;
		while (limit > 0)
		{
			void* ptr2 = (void*)(ptr - 3);
			if ((*(unsigned char*)ptr != 0xCC || *(unsigned char*)ptr != 0x90)
			    && (memcmp(ptr2, bytes, 3) == 0 || memcmp(ptr2, bytes2, 3) == 0))
				return ptr;
			ptr--;
			limit--;
		}
		return 0x0;
	}
};

