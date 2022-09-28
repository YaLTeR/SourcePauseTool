#pragma once

#ifdef SSDK2013
// autocomplete crashes on stemapipe :((
#define CON_COMMAND_AUTOCOMPLETEFILE(name, description, flags, subdirectory, extension) \
	CON_COMMAND_F(name, description, flags)

#define CON_COMMAND_AUTOCOMPLETE(name, description, flags, completion) CON_COMMAND_F(name, description, flags)

#else

#include <vector>
#include <string>
#include "convar.h"

class AutoCompleteList
{
public:
	AutoCompleteList(const char* command);
	AutoCompleteList(const char* command, std::vector<std::string> completions);
	virtual int AutoCompletionFunc(const char* partial,
	                               char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

protected:
	int AutoCompleteSuggest(const char* partial,
	                        char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
	std::pair<std::string, std::string> SplitPartial(const char* partial, bool& hasDoubleQuote);
	bool shouldCompleteDoubleQuote = true;
	const char* command;
	std::vector<std::string> completions;
};

class FileAutoCompleteList : public AutoCompleteList
{
public:
	FileAutoCompleteList(const char* command, const char* subDirectory, const char* extension);
	int AutoCompletionFunc(const char* partial,
	                       char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

private:
	const char* subDirectory;
	const char* extension;
};

#ifdef OE
#define CON_COMMAND_F_COMPLETION(name, description, flags, completion) \
	static void name(ArgsWrapper args); \
	static void name##_wrapper() \
	{ \
		if (!interfaces::engine) \
			return; \
		ArgsWrapper args; \
		name(args); \
	} \
	static ConCommand name##_command(#name, name##_wrapper, description, flags, completion); \
	static void name(ArgsWrapper args)
#endif

#define AUTOCOMPLETION_FUNCTION(command) command##_CompletionFunc

#define DECLARE_AUTOCOMPLETIONFILE_FUNCTION(command, subdirectory, extension) \
	FileAutoCompleteList command##Complete(#command, subdirectory, extension); \
	int command##_CompletionFunc(const char* partial, \
	                             char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]) \
	{ \
		return command##Complete.AutoCompletionFunc(partial, commands); \
	}

#define CON_COMMAND_AUTOCOMPLETEFILE(name, description, flags, subdirectory, extension) \
	DECLARE_AUTOCOMPLETIONFILE_FUNCTION(name, subdirectory, extension) \
	CON_COMMAND_F_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name))

#define DECLARE_AUTOCOMPLETION_FUNCTION(command, completion) \
	AutoCompleteList command##Complete(#command, std::vector<std::string> completion); \
	int command##_CompletionFunc(const char* partial, \
	                             char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]) \
	{ \
		return command##Complete.AutoCompletionFunc(partial, commands); \
	}

#define CON_COMMAND_AUTOCOMPLETE(name, description, flags, completion) \
	DECLARE_AUTOCOMPLETION_FUNCTION(name, completion) \
	CON_COMMAND_F_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name))

#endif
