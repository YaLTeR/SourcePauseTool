#include "stdafx.h"

#include "command.hpp"
#include "file.hpp"
#include <filesystem>
namespace fs = std::filesystem;

int AutoCompletList::AutoCompletionFunc(const char* partial,
                                        char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char* substring = (char*)partial;
	if (std::strstr(partial, command))
	{
		substring = (char*)partial + strlen(command) + 1;
	}
	std::vector<std::string> items;
	for (auto& item : completion)
	{
		if (items.size() == COMMAND_COMPLETION_MAXITEMS)
			break;
		auto name = item.c_str();
		if (std::strstr(name, substring) == name)
		{
			items.push_back(item);
		}
	}
	int count = 0;
	for (auto& item : items)
	{
		std::strncpy(commands[count],
		             (std::string(command) + " " + item).c_str(),
		             COMMAND_COMPLETION_ITEM_LENGTH);
		commands[count][COMMAND_COMPLETION_ITEM_LENGTH - 1] = '\0';
		count++;
	}
	return count;
}

int AutoCompletList::AutoCompletionFileFunc(const char* partial,
                                            char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char* substring = (char*)partial;
	if (std::strstr(partial, command))
	{
		substring = (char*)partial + strlen(command) + 1;
	}
	char* pch = std::strrchr(substring, '/');
	char path[255];
	std::string dir = GetGameDir() + subdirectory;
	if (pch)
	{
		std::strncpy(path, substring, pch - substring + 1);
		path[pch - substring + 1] = '\0';
		dir = dir + "/" + path;
	}
	std::string ext(extension);
	completion.clear();
	for (auto& p : fs::directory_iterator(dir))
	{
		if (fs::is_directory(p.status()))
		{
			if (pch)
			{
				completion.push_back(path + p.path().filename().string() + "/");
			}
			else
			{
				completion.push_back(p.path().filename().string() + "/");
			}
		}
		else if (p.path().extension() == ext)
		{
			if (pch)
			{
				completion.push_back(path + p.path().stem().string());
			}
			else
			{
				completion.push_back(p.path().stem().string());
			}
		}
	}
	return AutoCompletionFunc(partial, commands);
}