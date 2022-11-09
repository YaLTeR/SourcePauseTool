#pragma once

#include "convar.h"

#ifdef OE
#include "interfaces.hpp"

#define CON_COMMAND_CALLBACK_ARGS ConVar* var, char const* pOldString

#undef CON_COMMAND
#undef CON_COMMAND_F

struct ArgsWrapper
{
	ArgsWrapper(){};

	int ArgC()
	{
		return interfaces::engine->Cmd_Argc();
	};

	const char* Arg(int arg)
	{
		return interfaces::engine->Cmd_Argv(arg);
	};
};

#define CON_COMMAND(name, description) \
	static void name(ArgsWrapper args); \
	static void name##_wrapper() \
	{ \
		if (!interfaces::engine) \
			return; \
		ArgsWrapper args; \
		name(args); \
	} \
	static ConCommand name##_command(#name, name##_wrapper, description); \
	static void name(ArgsWrapper args)

#define CON_COMMAND_F(name, description, flags) \
	static void name(ArgsWrapper args); \
	static void name##_wrapper() \
	{ \
		if (!interfaces::engine) \
			return; \
		ArgsWrapper args; \
		name(args); \
	} \
	static ConCommand name##_command(#name, name##_wrapper, description, flags); \
	static void name(ArgsWrapper args)

#else
#define CON_COMMAND_CALLBACK_ARGS IConVar* var, const char* pOldValue, float flOldValue
#endif