#include "stdafx.hpp"

#include "patterns_new.hpp"
#include "convar.h"
#include "OrangeBox/modules.hpp"

#define INSIDE(modulebase, modulesize, addr) ((uint)modulebase <= addr && ((uint)modulebase + modulesize) >= addr)
#define MAX_DT_SIZE 0x700
#define MAX_SIGNATURE_STRING_LEN 256
#define MAX_CHECK_CALL_BOUNDARY 0x7FFFFFFF
#define BOUND(val, min, max)\
	val = ((min) >= (val)) ? (min) : (val);\
	val = ((max) <= (val)) ? (max) : (val);
#define IS_WITHIN(val, min, max) (((val) >= (min)) && ((val) <= (max)))

namespace PatternsExt
{
	// find entity offset using provided data table function
	// ARGS:
	// moduleBase:		base address of the module the data table sits in
	// moduleSize:		size of the module the data table sits in
	// base:			the end of the data table
	// name:			name of entity offset
	// outOffset:		offset to add onto output
	static int FindEntityOffsetThroughDT(void* moduleBase,
	                                     size_t moduleSize,
	                                     uintptr_t base,
	                                     const char* name,
	                                     int outOffset = 0)
	{
		int foundOffset = 0;

		if ((uint)moduleBase <= 0 || base <= 0)
			return 0;

		PatternScanner scanner((void*)(base - MAX_DT_SIZE), MAX_DT_SIZE);
		scanner.onMatchEvaluate = [&](bool* done, uintptr_t* ptr) {
			uintptr_t string = *reinterpret_cast<uintptr_t*>(*ptr);
			if (!INSIDE(moduleBase, moduleSize, string)) 
				*done = false;
			else 
				*done = (strcmp((char*)string, name) == 0);
		};

		Pattern first("6A ?? 68", 3);
		first.onFound = [&](uintptr_t ptr) { 
			foundOffset = (int)*reinterpret_cast<char*>(ptr - 2); 
		};
		Pattern second("68 ?? ?? ?? ?? 68", 6);
		second.onFound = [&](uintptr_t ptr) {
			foundOffset = *reinterpret_cast<int*>(ptr - 5);
		};

		PatternCollection collection(first);
		collection.AddPattern(second);

		scanner.Scan(collection);

		if (foundOffset == 0)
			DevWarning(1, "%s offset couldn't be found!\n", name);
		else
			DevMsg("%s offset is 0x%X\n", name, foundOffset + outOffset);

		return foundOffset + outOffset;
	}

	// find string's address
	// ARGS:
	// scanner:		PatternScanner to use
	// string:		string to scan for
	static uintptr_t FindStringAddress(PatternScanner scanner, const char* string)
	{
		char sig[MAX_SIGNATURE_STRING_LEN];
		pUtils.toHexArray(string, sig);
		Pattern target(sig, 0);

		return scanner.Scan(target);
	}

	// find var's reference (direct pointer reference)
	// ARGS:
	// scanner:		PatternScanner to use
	// addr:		address of the target variable
	// prefix:		bytes before those that directly references the trget var
	// suffix:		bytes after those that directly references the trget var
	static uintptr_t FindVarReference(PatternScanner scanner,
	                                  uintptr_t addr,
	                                  const char* prefix = "",
	                                  const char* suffix = "",
	                                  int offset = 0)
	{
		if (addr <= 0)
			return 0;

		char sig[MAX_SIGNATURE_STRING_LEN] = "";
		char byteString[15];
		pUtils.toHexArray(addr, byteString);

		sprintf(sig, "%s %s %s", prefix, byteString, suffix);

		Pattern p(sig, offset);
		return scanner.Scan(p);
	}

	static Pattern GeneratePatternFromVar(uintptr_t addr,
	                                      const char* prefix = "",
	                                      const char* suffix = "",
	                                      int offset = 0)
	{
		char sig[MAX_SIGNATURE_STRING_LEN] = "";
		char byteString[15];
		pUtils.toHexArray(addr, byteString);
		sprintf(sig, "%s %s %s", prefix, byteString, suffix);
		Pattern p(sig, offset);
		return p;
	}

	// find data table
	// ARGS:
	// scanner:		PatternScanner to use
	// name:		name of the data table
	static uintptr_t FindDataTable(PatternScanner scanner, const char* name) 
	{
		uintptr_t ptr, ptr2 = 0;
		ptr = FindStringAddress(scanner, name);

		if (ptr == 0x0)
			goto eof;
		
		ptr2 = FindVarReference(scanner, ptr, "68");
		if (ptr2 == 0x0)
		{
			Pattern p = GeneratePatternFromVar(ptr);
			p.onMatchEvaluate = _oMEArgs(&)
			{
				*foundPtr = FindVarReference(scanner, *foundPtr, "83 ?? ?? ?? ??");
				*done = *foundPtr != 0x0;
			};
			
			ptr2 = scanner.Scan(p);
		}

		DevMsg("%s data table found at %p!\n", name, ptr2);
		return ptr2;

	eof:
		DevWarning(1, "%s data table couldn't be found!\n", name);
		return 0;
	}

	// find entity offset
	// ARGS:
	// scanner:		scanner to use
	// name:		name of the offset
	static uintptr_t FindEntityOffset(PatternScanner scanner, const char* name) 
	{
		uintptr_t tmp = 0;
		int offset = 0;

		char sig[MAX_SIGNATURE_STRING_LEN];
		pUtils.toHexArray(name, sig);
		Pattern target(sig, 0);
		target.onMatchEvaluate = [&](bool* done, uintptr_t* ptr) {
			uintptr_t ref = FindVarReference(scanner, *ptr, "C7 05 ?? ?? ?? ??");
			*done = ref != 0x0;
			*ptr = ref;
		};

		tmp = scanner.Scan(target);
		if (tmp != 0)
		{
			PatternScanner newScanner((void*)(tmp + 1), 0x50);
			Pattern p("C7 05 ?? ?? ?? ?? ?? ?? ?? 00", 6);
			p.onFound = [&](uintptr_t ptr) {
				offset = *(int*)ptr;
				return ptr;
			};
			tmp = newScanner.Scan(p);
		}
		if (offset == 0) DevWarning(1, "%s offset couldn't be found\n", name);
		else DevMsg("%s offset is 0x%X\n", name, offset);
		return offset;
	}

	static const char backTraceBytes[4] = {0xCC, 0x90, 0xC2, 0xC3};

	// BACKTRACE FUNCTION
	// backtrace from middle of function to the beginning, employing checking for padding bits (0xCC, 0x90)
	// checking vftable entry and checking call / jump calls
	// ARGS:
	// scanner:		scanner to use
	// addr:		address in the middle of the func
	// limit:		how many bytes to look up (normally 0x100 to 0x200)
	// interruptLevel the number of padding bytes to encounter before passing check
	// checkVFTable	enable checking for vftable entries
	// checkCallAmount how many bytes before and after addr to check for calls to the target function (recommended value between 0x5000 to 0x10000)
	static uintptr_t BackTraceToFuncStart(PatternScanner scanner, uintptr_t addr,
		int limit = 0x600, 
		int interruptLevel = 3, 
		bool checkVFTable = false, 
		uint checkCallAmount = 0) 
	{
		using namespace std;

		if (addr <= 0)
			return 0;

		uintptr_t ptr = addr;
		BOUND(interruptLevel, 0, MAX_INTERRUPT_SIZE);
		vector<uintptr_t> calledAddresses;

		ptr++;
		limit++;

		if (checkCallAmount > 0)
		{
			checkCallAmount += limit;
			BOUND(checkCallAmount, 1, MAX_CHECK_CALL_BOUNDARY);
			char pos[20] = "";
			char neg[20] = "";
			int j = 0x1;
			for (int i = 0; i < 4; i++)
			{
				strcat(pos, (checkCallAmount / j > 1) ? " ??" : " 00");
				strcat(neg, (checkCallAmount / j > 1) ? " ??" : " FF");
				j *= 0x100;
			}

			// i hate this...
			char sig[20] = "";
			PatternCollection p;
			sprintf(sig, "E8 %s", pos); p.AddPattern(sig, 1);
			sprintf(sig, "E8 %s", neg); p.AddPattern(sig, 1);
			sprintf(sig, "E9 %s", pos); p.AddPattern(sig, 1);
			sprintf(sig, "E9 %s", neg); p.AddPattern(sig, 1);

			uintptr_t newStart = addr - checkCallAmount;
			BOUND(newStart, (uint)scanner._base, scanner._end());
			PatternScanner newScanner((void*)(newStart), checkCallAmount * 2);

			p.onMatchEvaluate = _oMEArgs(&) {
				*done = false;
				uintptr_t called = *foundPtr + 4 + *(uint*)(*foundPtr);
				if (IS_WITHIN(called, addr - limit, addr))
					calledAddresses.push_back(called);
			};

			newScanner.Scan(p);
		}

		while (limit >= 0 && ptr >= 0 && ptr >= (uint)scanner._base)
		{
			ptr--;
			limit--;

			uchar curByte = *(uchar*)ptr;
			uchar lastByte = *(uchar*)(ptr + 1);
			if (!pUtils.charArrayContains(backTraceBytes, curByte)) continue;
			if (pUtils.charArrayContains(backTraceBytes, lastByte)) continue;
				
			if (interruptLevel > 0 && (curByte == 0xCC || curByte == 0x90))
			{
				void* ptr2 = (void*)(ptr - interruptLevel);
				if (memcmp(ptr2, pUtils.intCC, interruptLevel) == 0
				    || memcmp(ptr2, pUtils.int90, interruptLevel) == 0)
					return ptr + 0x1;
			}

			if (checkVFTable)
			{
				uintptr_t tmp = (curByte == 0xC2) ? ptr + 0x3 : ptr + 0x1;
				char sig[20] = "";
				pUtils.toHexArray(tmp, sig);
				Pattern p(sig, 0);
				p.onMatchEvaluate = [&](bool* found, uintptr_t* foundPtr) { 
					*found = (
						*foundPtr % 4 == 0 && 
						( INSIDE(scanner._base, scanner._size, *foundPtr + 0x4) ||
						  INSIDE(scanner._base, scanner._size, *foundPtr - 0x4) )); 
				};

				if (scanner.Scan(p) != 0x0)
					return tmp;
			}

			if (checkCallAmount > 0 && !calledAddresses.empty())
			{
				uintptr_t tmp = (curByte == 0xC2) ? ptr + 0x3 : ptr + 0x1;
				if (find(calledAddresses.begin(), calledAddresses.end(), tmp) != end(calledAddresses))
					return tmp;
			}
		}

		return 0;
	}

} // namespace PatternsExt


// old macros utilizing the old compile-time pattern systems
/*
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
		char sig[MAX_SIGNATURE_STRING_LEN]; \
		pUtils.toHexArray(string, sig, false); \
		BASIC_SIGSCAN(base, size, sig, len, out); \
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
		if (addr > 0) \
		{ \
			char sig[MAX_SIGNATURE_STRING_LEN]; \
			sig[0] = 0; \
			strcat(sig, prefix); \
			strcat(sig, " "); \
			pUtils.toHexArray(addr, sig, true); \
			strcat(sig, " "); \
			strcat(sig, suffix); \
			BASIC_SIGSCAN(base, size, sig, len, out); \
		} \
	}

// FIND DATA TABLE
// ARGS:
//		modulebase:	where to start sigscanning
//		modulesize:	how far to sigscan
//		name:	name of datatable
//		len:	length of datatable's name
//		out:	output var
#define FIND_DT(modulebase, modulesize, name, len, out) \
	{ \
		uintptr_t stringAddr = 0x0; \
		out = 0x0; \
		FIND_STRING_ADDR(moduleBase, moduleLength, name, len, stringAddr); \
		if (stringAddr != 0) \
		{ \
			FIND_VAR_REF(moduleBase, moduleLength, stringAddr, "68 ", "", 5, out); \
			if (out == 0x0) \
			{ \
				FIND_VAR_REF(moduleBase, moduleLength, stringAddr, "", "", 4, out); \
				FIND_VAR_REF(moduleBase, moduleLength, out, "83 ?? ?? ?? ?? ", "", 9, out); \
			} \
		} \
		if (out == 0x0) \
			DevWarning(1, "End of " #name " DataTable offset couldn't be found\n"); \
		else \
			DevMsg("End of " #name " DataTable offset function found at %p\n", out); \
	}
	*/