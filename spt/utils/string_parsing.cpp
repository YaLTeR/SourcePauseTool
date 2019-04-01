#include "stdafx.h"
#include <Windows.h>
#include "string_parsing.hpp"
#include <algorithm>


bool whiteSpacesOnly(const std::string & s)
{
	return std::all_of(s.begin(), s.end(), isspace);
}

void ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}
