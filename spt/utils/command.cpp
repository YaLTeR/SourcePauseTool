#include "stdafx.h"

#ifndef SSDK2013

#include "command.hpp"
#include "file.hpp"
#include <filesystem>
namespace fs = std::filesystem;

AutoCompleteList::AutoCompleteList(const char* command) : command(command) {}
AutoCompleteList::AutoCompleteList(const char* command, std::vector<std::string> completions)
    : command(command), completions(completions)
{
}

int AutoCompleteList::AutoCompletionFunc(const char* partial,
                                         char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	return AutoCompleteSuggest(partial, commands);
}

int AutoCompleteList::AutoCompleteSuggest(const char* partial,
                                          char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	bool hasDoubleQuote;
	auto subStrings = SplitPartial(partial, hasDoubleQuote);
	int count = 0;
	for (auto& item : completions)
	{
		if (count == COMMAND_COMPLETION_MAXITEMS)
			break;
		if (item.find(subStrings.second) == 0)
		{
			std::string completion = subStrings.first + item;
			if (shouldCompleteDoubleQuote && hasDoubleQuote)
				completion += '"';
			std::strncpy(commands[count], completion.c_str(), COMMAND_COMPLETION_ITEM_LENGTH);
			commands[count][COMMAND_COMPLETION_ITEM_LENGTH - 1] = '\0';
			count++;
		}
	}
	return count;
}

std::pair<std::string, std::string> AutoCompleteList::SplitPartial(const char* partial, bool& hasDoubleQuote)
{
	std::string s(partial);
	size_t pos = strlen(command) + 1;
	hasDoubleQuote = partial[pos] == '"';
	if (hasDoubleQuote)
		pos++;
	return {s.substr(0, pos), s.substr(pos)};
}

FileAutoCompleteList::FileAutoCompleteList(const char* command, const char* subDirectory, const char* extension)
    : AutoCompleteList(command), subDirectory(subDirectory), extension(extension)
{
	shouldCompleteDoubleQuote = false;
}

int FileAutoCompleteList::AutoCompletionFunc(const char* partial,
                                             char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	bool hasDoubleQuote;
	auto subStrings = SplitPartial(partial, hasDoubleQuote);

	// hack: accessing root dir like this will crash
	if (subStrings.second[0] == '/' || subStrings.second[0] == '\\')
		return 0;

	fs::path currentDir(subStrings.second);
	currentDir = currentDir.parent_path();

	fs::path dir(GetGameDir());
	dir /= subDirectory;

	dir /= currentDir;

	std::error_code ec;
	if (!fs::is_directory(dir, ec))
		return 0;

	completions.clear();

	std::string completionPath = currentDir.string();
	if (completionPath.length() > 0)
		completionPath += "/";

	for (auto& p : fs::directory_iterator(dir, ec))
	{
		if (fs::is_directory(p.status()))
		{
			completions.push_back(completionPath + p.path().filename().string() + "/");
		}
		else if (p.path().extension() == extension)
		{
			std::string completion(completionPath + p.path().stem().string());
			if (hasDoubleQuote)
				completion += '"';
			completions.push_back(completion);
		}
	}

	return AutoCompleteSuggest(partial, commands);
}

#endif
