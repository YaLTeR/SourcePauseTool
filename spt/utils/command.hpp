#pragma once

#include "convar.h"

class AutoCompletList
{
public:
	AutoCompletList(const char* command, const char* subdirectory, const char* extension)
	{
		this->command = command;
		this->subdirectory = subdirectory;
		this->extension = extension;
	}

	AutoCompletList(const char* command, std::vector<std::string> completion)
	{
		this->command = command;
		this->completion = completion;
	}

	int AutoCompletionFunc(const char* partial,
	                       char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

	int AutoCompletionFileFunc(const char* partial,
	                           char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

private:
	const char* command;
	const char* subdirectory;
	const char* extension;
	std::vector<std::string> completion;
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
	AutoCompletList command##Complete(#command, subdirectory, extension); \
	int command##_CompletionFunc(const char* partial, \
	                             char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]) \
	{ \
		return command##Complete.AutoCompletionFileFunc(partial, commands); \
	}

#ifdef SSDK2013
// autocomplete crashes on stemapipe :((
#define CON_COMMAND_AUTOCOMPLETEFILE(name, description, flags, subdirectory, extension) \
	CON_COMMAND_F(name, description, flags)
#else
#define CON_COMMAND_AUTOCOMPLETEFILE(name, description, flags, subdirectory, extension) \
	DECLARE_AUTOCOMPLETIONFILE_FUNCTION(name, subdirectory, extension) \
	CON_COMMAND_F_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name))
#endif

#define DECLARE_AUTOCOMPLETION_FUNCTION(command, completion) \
	AutoCompletList command##Complete(#command, std::vector<std::string> completion); \
	int command##_CompletionFunc(const char* partial, \
	                             char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]) \
	{ \
		return command##Complete.AutoCompletionFunc(partial, commands); \
	}

#define CON_COMMAND_AUTOCOMPLETE(name, description, flags, completion) \
	DECLARE_AUTOCOMPLETION_FUNCTION(name, completion) \
	CON_COMMAND_F_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name))
