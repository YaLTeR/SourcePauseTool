#include "stdafx.hpp"
#include "string_utils.hpp"
#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <cstdint>

bool whiteSpacesOnly(const std::string& s)
{
	return std::all_of(s.begin(), s.end(), isspace);
}

void ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

const char* FormatTempString(const char* value, ...)
{
	static char FORMAT_BUFFER[256];

	va_list args;
	va_start(args, value);
	vsprintf_s(FORMAT_BUFFER, ARRAYSIZE(FORMAT_BUFFER), value, args);
	va_end(args);
	return FORMAT_BUFFER;
}

const wchar* FormatTempString(const wchar* format, ...)
{
	va_list args;
	va_start(args, format);
	const wchar* output = FormatTempString(format, args);
	va_end(args);

	return output;
}

const wchar* FormatTempString(const wchar* format, va_list args)
{
	static wchar buffer[1024];
	vswprintf_s(buffer, ARRAYSIZE(buffer), format, args);
	return buffer;
}

// UTF-8 detection code, with minor additions
// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

const int UTF8_ACCEPT = 0;
const int UTF8_REJECT = 1;

static const uint8_t utf8d[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 00..1f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20..3f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 40..5f
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 60..7f
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9, // 80..9f
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7, // a0..bf
    8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   // c0..df
    0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // e0..ef
    0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // f0..ff
    0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   0,   1,   1,   1,   1,   1,   0,   1,   0,   1,   1,   1,   1,   1,   1, // s1..s2
    1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1, // s3..s4
    1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1, // s5..s6
    1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1,
    1,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, // s7..s8
};

uint32_t inline decode(uint32_t* state, uint32_t* codep, uint32_t byte)
{
	uint32_t type = utf8d[byte];

	*codep = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);

	*state = utf8d[256 + *state * 16 + type];
	return *state;
}

uint32_t validate_utf8(uint32_t* state, const char* str, size_t len)
{
	size_t i;
	uint32_t type;

	for (i = 0; i < len; i++)
	{
		type = utf8d[(uint8_t)str[i]];
		*state = utf8d[256 + (*state) * 16 + type];

		if (*state == UTF8_REJECT)
			break;
	}

	return *state;
}

bool IsValidUTF8(const char* string)
{
	uint32_t state = UTF8_ACCEPT;
	validate_utf8(&state, string, std::strlen(string));
	return state == UTF8_ACCEPT;
}