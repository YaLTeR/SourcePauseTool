#include "stdafx.hpp"

#include "convar.hpp"
#include "file.hpp"

#ifdef OE
#include "interfaces.hpp"
#endif

namespace fs = std::filesystem;

AutoCompleteList::AutoCompleteList() {}
AutoCompleteList::AutoCompleteList(std::vector<std::string> completions) : completions(completions) {}

int AutoCompleteList::AutoCompletionFunc(AUTOCOMPLETION_FUNCTION_PARAMS)
{
	bool hasDoubleQuote;
	auto subStrings = SplitPartial(partial, hasDoubleQuote);
	return AutoCompleteSuggest(subStrings.first, subStrings.second, completions, hasDoubleQuote, commands);
}

int AutoCompleteList::AutoCompleteSuggest(const std::string& suggestionBase,
                                          const std::string& incompleteArgument,
                                          const std::vector<std::string>& _completions,
                                          bool hasDoubleQuote,
                                          char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	int count = 0;
	for (const auto& item : _completions)
	{
		if (count == COMMAND_COMPLETION_MAXITEMS)
			break;
		if (item.find(incompleteArgument) == 0)
		{
			std::string completion = suggestionBase + item;
			if (hasDoubleQuote)
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
	size_t pos = s.find(' ') + 1;
	hasDoubleQuote = partial[pos] == '"';
	if (hasDoubleQuote)
		pos++;
	return {s.substr(0, pos), s.substr(pos)};
}

FileAutoCompleteList::FileAutoCompleteList(const char* subDirectory, const char* extension)
    : AutoCompleteList(), subDirectory(subDirectory), extension(extension) {}

int FileAutoCompleteList::AutoCompletionFunc(AUTOCOMPLETION_FUNCTION_PARAMS)
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

	bool shouldUpdate = (dir != prevPath || subStrings.second.empty());
	prevPath = dir;

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
	return AutoCompleteSuggest(subStrings.first, subStrings.second, completions, hasDoubleQuote, commands);
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
