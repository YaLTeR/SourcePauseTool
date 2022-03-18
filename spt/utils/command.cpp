#include "stdafx.h"

#include "command.hpp"
#include "file.hpp"
#include <filesystem>
namespace fs = std::experimental::filesystem;

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
		count++;
	}
	return count;
}

int AutoCompletList::AutoCompletionFileFunc(const char* partial,
                                            char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	std::string ext(extension);
	completion.clear();
	for (auto& p : fs::directory_iterator(GetGameDir() + subdirectory))
	{
		if (p.path().extension() == ext)
		{
			completion.push_back(p.path().stem().string());
		}
	}
	return AutoCompletionFunc(partial, commands);
}