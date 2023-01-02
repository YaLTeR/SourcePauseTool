#include "stdafx.hpp"
#include "file.hpp"
#include "interfaces.hpp"
#include <fstream>

#ifdef OE
extern ConVar y_spt_gamedir;
#endif

bool FileExists(const std::string& fileName)
{
	std::string dir = fileName;
	std::ifstream is;
	is.open(dir);
	return is.is_open();
}

std::string GetGameDir()
{
#ifdef OE
	return y_spt_gamedir.GetString();
#else
	char BUFFER[256];
	if (!interfaces::engine_server)
		return std::string();
	else
	{
		interfaces::engine_server->GetGameDir(BUFFER, ARRAYSIZE(BUFFER));
		return BUFFER;
	}
#endif
}
