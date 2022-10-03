#include "stdafx.h"
#include "command.hpp"
#include "file.hpp"

#ifdef OE
#include "interfaces.hpp"
#endif

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

	// hack: prevent accessing root dir
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

	bool shouldUpdate = (dir != prevPath || subStrings.second == "");
	prevPath = dir;
	bool doubleQuoteChanged = prevHasDoubleQuote != hasDoubleQuote;
	prevHasDoubleQuote = hasDoubleQuote;

	if (shouldUpdate)
	{
		completions.clear();
		std::string completionPath = currentDir.string();
		if (completionPath.length() > 0)
			completionPath += "/";

		for (auto& p : fs::directory_iterator(dir, ec))
		{
			if (fs::is_directory(p.status()))
				completions.push_back(completionPath + p.path().filename().string() + '/');
			else if (p.path().extension() == extension)
			{
				std::string completion(completionPath + p.path().stem().string());
				completions.push_back(completion);
			}
		}
	}
	if (shouldUpdate || doubleQuoteChanged)
	{
		for (auto& item : completions)
		{
			char end = item.back();
			if (hasDoubleQuote && end != '"' && end != '/')
				item += '"';
			else if (end == '"')
				item.pop_back();
		}
	}
	return AutoCompleteSuggest(partial, commands);
}

#ifdef OE
ConCommand* FindCommand(const char* name)
{
	if (!interfaces::g_pCVar)
		return NULL;
	const ConCommandBase* cmd = interfaces::g_pCVar->GetCommands();
	for (; cmd; cmd = cmd->GetNext())
	{
		if (!Q_stricmp(name, cmd->GetName()))
			break;
	}
	if (!cmd || !cmd->IsCommand())
		return NULL;

	return const_cast<ConCommand*>(static_cast<const ConCommand*>(cmd));
}

#endif
