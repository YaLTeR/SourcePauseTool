#pragma once

#include "stdafx.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>


using std::hex;
using std::stringstream;
using std::string;

#define MAX_INTERRUPT_SIZE 20

using namespace std;

class PatternUtils
{
public:
	unsigned char int90[MAX_INTERRUPT_SIZE];
	unsigned char intCC[MAX_INTERRUPT_SIZE];

	PatternUtils()
	{
		memset(int90, 0x90, MAX_INTERRUPT_SIZE);
		memset(intCC, 0xcc, MAX_INTERRUPT_SIZE);
	}

	constexpr uint8_t convertToHex(char c)
	{
		return (c >= '0' && c <= '9')   ? static_cast<uint8_t>(c - '0')
		       : (c >= 'a' && c <= 'f') ? static_cast<uint8_t>(c - 'a' + 10)
		       : (c >= 'A' && c <= 'F') ? static_cast<uint8_t>(c - 'A' + 10)
		                                : throw std::domain_error("not a hex digit");
	}

	constexpr bool ishex(char c)
	{
		return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}

	constexpr int count_bytes(const char* pattern)
	{
		int count = 0;

		for (; pattern[0]; ++pattern)
		{
			if (pattern[0] == ' ')
				continue;

			if (ishex(pattern[0]))
			{
				if (!ishex((++pattern)[0]))
					throw std::logic_error("the second hex digit is missing");

				++count;
				continue;
			}

			if (pattern[0] == '?')
			{
				if ((++pattern)[0] != '?')
					throw std::logic_error("the second question mark is missing");

				++count;
				continue;
			}
			else
				throw std::domain_error("only hex digits, spaces and question marks are allowed");
		}

		return count;
	}

	// CONVERT VAR TO SIGNATURE-FRIENDLY HEX BYTE ARRAY
	// ARGS:
	//		in:		input var
	//		out:	output var
	//		concat:	concatenate onto out var instead of overwriting
	void toHexArray(const char* in, char* out, bool concat = false)
	{
		string s1 = in;
		stringstream ss;

		for (const auto& item : s1)
			ss << " " << hex << int(item);
		ss << " " << setfill('0') << setw(2) << right << hex << 00;

		concat ? strcat(out, ss.str().c_str()) : strcpy(out, ss.str().c_str());
	}

	void toHexArray(unsigned int in, char* out, bool concat = false)
	{
		stringstream ss;
		for (int i = 0; i <= 3; ++i)
		{
			unsigned int b = (in >> 8 * i) & 0xFF;
			ss << " " << setfill('0') << setw(2) << right << hex << b;
		}
		concat ? strcat(out, ss.str().c_str()) : strcpy(out, ss.str().c_str());
	}

	void toHexArraySigned(int in, char* out, bool concat = false)
	{
		int temp = in;
		unsigned char* bytes = ((unsigned char*)&temp);
		char tmp[20] = "";
		sprintf(tmp, " %02X %02X %02X %02X ", bytes[0], bytes[1], bytes[2], bytes[3]);
		concat ? strcat(out, tmp) : strcpy(out, tmp);
	}

	bool charArrayContains(const char* in, char candidate) const
	{
		for (int i = 0; i < strlen(in); i++)
		{
			if (in[i] == candidate)
				return true;
		}
		return false;
	}
};

static PatternUtils pUtils;

// alternate / non-const implementation of patterns
struct Pattern
{
	uint8_t* bytes;
	char* mask;
	int offset;
	size_t count;

	// function ran when a suitable signature is found
	// ARGS:
	//	(bool*) done: decides whether the address found is good and sigscanning should finish, defaults to true
	//	(uintptr_t*) foundPtr: the pointer that was found using the signature (including offset)
	std::function<void(bool*, uintptr_t*)> onMatchEvaluate;

	// function ran when a suitable signature is found and the found pointer is ok
	//	ARGS:
	//	(uintptr_t) foundPtr: the pointer that was found using the signature (including offset)
	std::function<void(uintptr_t)> onFound;

	Pattern(const char* pattern, int _offset)
	{
		count = pUtils.count_bytes(pattern);

		bytes = new uint8_t[count];
		mask = new char[count];
		offset = _offset;

		int i = 0;
		for (; pattern[0]; ++pattern)
		{
			if (pattern[0] == ' ')
				continue;

			if (pUtils.ishex(pattern[0]))
			{
				bytes[i] = pUtils.convertToHex(pattern[0]) * 16 + pUtils.convertToHex(pattern[1]);
				mask[i++] = 'x';

				++pattern;
				continue;
			}

			if (pattern[0] == '?')
			{
				mask[i++] = '?';

				++pattern;
				continue;
			}

			throw std::domain_error("only hex digits, spaces and question marks are allowed");
		}

		onMatchEvaluate = [](bool*, uintptr_t*) {  };
		onFound = [](uintptr_t) {};
	};
	
	inline bool match(const uint8_t* memory) const
	{
		for (size_t i = 0; i < count; ++i)
			if (mask[i] == 'x' && memory[i] != bytes[i])
				return false;

		return true;
	}
};
class PatternCollection
{
public:
	std::vector<Pattern> Patterns;
	std::function<void(bool*, uintptr_t*)> onMatchEvaluate;

	PatternCollection(const char* pattern, int offset)
	{
		AddPattern(pattern, offset);
		onMatchEvaluate = [](bool*, uintptr_t*) {};
	}

	PatternCollection(Pattern pattern)
	{
		AddPattern(pattern);
		onMatchEvaluate = [](bool*, uintptr_t*) {};
	}

	PatternCollection()
	{
		onMatchEvaluate = [](bool*, uintptr_t*) {};
	}

	void AddPattern(const char* pattern, int offset)
	{
		Pattern p(pattern, offset);
		Patterns.push_back(p);
	}

	void AddPattern(Pattern pattern) 
	{
		Patterns.push_back(pattern);
	}
};

typedef std::function<void(bool*, uintptr_t*)> _onMatchEvaluate;
#define _oMEArgs(capture) [capture](bool* done, uintptr_t* foundPtr)

class PatternScanner
{
public:

	// function ran when a suitable signature is found
	// ARGS:
	//	(bool*) done: decides whether the address found is good and sigscanning should finish, defaults to true
	//	(uintptr_t*) foundPtr: the pointer that was found using the signature (including offset)
	std::function<void(bool*, uintptr_t*)> onMatchEvaluate;

	PatternScanner(void* base, size_t size)
	{
		_base = base;
		_size = size;
		onMatchEvaluate = [](bool*, uintptr_t*) {};
	}

	inline uintptr_t Scan(PatternCollection patterns) const
	{
		for (Pattern target : patterns.Patterns)
		{
			if (_size < target.count)
				continue;

			auto p = static_cast<const uint8_t*>(_base);
			for (auto end = p + _size - target.count; p <= end; ++p)
			{
				if (target.match(p))
				{
					auto actualPtr = reinterpret_cast<uintptr_t>(p) + target.offset;
					bool done = true;
					target.onMatchEvaluate(&done, &actualPtr);
					patterns.onMatchEvaluate(&done, &actualPtr);
					onMatchEvaluate(&done, &actualPtr);

					if (!done)
						continue;

					target.onFound(actualPtr);
					return actualPtr;
				}
			}
		}
		return 0;
	}

	inline uintptr_t Scan(Pattern target) const
	{
		if (_size < target.count)
			return 0;

		auto p = static_cast<const uint8_t*>(_base);
		for (auto end = p + _size - target.count; p <= end; ++p)
		{
			if (target.match(p))
			{
				auto actualPtr = reinterpret_cast<uintptr_t>(p) + target.offset;
				bool done = true;
				target.onMatchEvaluate(&done, &actualPtr);
				onMatchEvaluate(&done, &actualPtr);

				if (!done)
					continue;

				target.onFound(actualPtr);
				return actualPtr;
			}
		}
		return 0;
	}

	uintptr_t _end()
	{
		return (uintptr_t)_base + _size;
	}

	void* _base;
	size_t _size;
};


