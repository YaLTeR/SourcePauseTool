#pragma once
#include "convar.h"
#include <filesystem>
#include <vector>
#include <string>

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
	std::filesystem::path prevPath;
	bool prevHasDoubleQuote = false;
};

#ifdef OE
#define _CON_COMMAND_WRAPPER(name) \
	static void name(ArgsWrapper args); \
	static void name##_wrapper() \
	{ \
		if (!interfaces::engine) \
			return; \
		ArgsWrapper args; \
		name(args); \
	}
#endif

#ifdef OE
#define CON_COMMAND_DOWN(name, description) \
	_CON_COMMAND_WRAPPER(name##_down); \
	static ConCommand name##_down##_command("+" #name, name##_down##_wrapper, description); \
	static void name##_down(ArgsWrapper args)

#define CON_COMMAND_UP(name, description) \
	_CON_COMMAND_WRAPPER(name##_up); \
	static ConCommand name##_up##_command("-" #name, name##_up##_wrapper, description); \
	static void name##_up(ArgsWrapper args)
#else
#define CON_COMMAND_DOWN(name, description) \
	static void name##_down(const CCommand& args); \
	static ConCommand name##_down##_command("+" #name, name##_down, description); \
	static void name##_down(const CCommand& args)

#define CON_COMMAND_UP(name, description) \
	static void name##_up(const CCommand& args); \
	static ConCommand name##_up##_command("-" #name, name##_up, description); \
	static void name##_up(const CCommand& args)
#endif

#ifdef OE
#define CON_COMMAND_F_COMPLETION(name, description, flags, completion) \
	_CON_COMMAND_WRAPPER(name); \
	static ConCommand name##_command(#name, name##_wrapper, description, flags, completion); \
	static void name(ArgsWrapper args)
#endif

#define AUTOCOMPLETION_FUNCTION(command) command##_CompletionFunc

#define DECLARE_AUTOCOMPLETIONFILE_FUNCTION(command, subdirectory, extension) \
	static FileAutoCompleteList command##Complete(#command, subdirectory, extension); \
	static int \
	    command##_CompletionFunc(const char* partial, \
	                             char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]) \
	{ \
		return command##Complete.AutoCompletionFunc(partial, commands); \
	}

#ifndef SSDK2013 // autocomplete crashes on stemapipe :((
#define CON_COMMAND_AUTOCOMPLETEFILE(name, description, flags, subdirectory, extension) \
	DECLARE_AUTOCOMPLETIONFILE_FUNCTION(name, subdirectory, extension) \
	CON_COMMAND_F_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name))

#define DECLARE_AUTOCOMPLETION_FUNCTION(command, completion) \
	static AutoCompleteList command##Complete(#command, std::vector<std::string> completion); \
	static int \
	    command##_CompletionFunc(const char* partial, \
	                             char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]) \
	{ \
		return command##Complete.AutoCompletionFunc(partial, commands); \
	}

#define CON_COMMAND_AUTOCOMPLETE(name, description, flags, completion) \
	DECLARE_AUTOCOMPLETION_FUNCTION(name, completion) \
	CON_COMMAND_F_COMPLETION(name, description, flags, AUTOCOMPLETION_FUNCTION(name))

#else
#define CON_COMMAND_AUTOCOMPLETEFILE(name, description, flags, subdirectory, extension) \
	CON_COMMAND_F(name, description, flags)

#define CON_COMMAND_AUTOCOMPLETE(name, description, flags, completion) CON_COMMAND_F(name, description, flags)

#endif

#ifndef SSDK2007
typedef void (*FnCommandCallbackV1_t)(void);
#endif

struct ConCommand_guts
{
	void* vfptr;
	ConCommandBase* m_pNext;
	bool m_bRegistered;

	// Static data
	const char* m_pszName;
	const char* m_pszHelpString;

	// ConVar flags
	int m_nFlags;
#ifndef OE
	union
	{
		FnCommandCallbackV1_t m_fnCommandCallbackV1;
		FnCommandCallback_t m_fnCommandCallback;
		ICommandCallback* m_pCommandCallback;
	};

	union
	{
		FnCommandCompletionCallback m_fnCompletionCallback;
		ICommandCompletionCallback* m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
#else
	FnCommandCallbackV1_t m_fnCommandCallbackV1;
	FnCommandCompletionCallback m_fnCompletionCallback;
	bool m_bHasCompletionCallback;
#endif
};

#ifdef OE
ConCommand* FindCommand(const char* name);
#endif
