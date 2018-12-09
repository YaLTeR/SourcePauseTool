#include "stdafx.h"
#include "file.hpp"
#include <fstream>

bool FileExists(const std::string& fileName)
{
	std::string dir = fileName;
	std::ifstream is;
	is.open(dir);
	return is.is_open();
}
