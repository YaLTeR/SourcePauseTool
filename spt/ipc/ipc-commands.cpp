#include "stdafx.h"
#include "convar.h"
#include "spt\OrangeBox\spt-serverplugin.hpp"
#include "thirdparty\json.hpp"
#include "ipc-spt.hpp"
#include "spt\utils\string_parsing.hpp"

CON_COMMAND(y_spt_ipc_echo,
            "y_spt_ipc_echo <text> - Sends a text message to IPC client. Don't insert fancy characters into this.\n")
{
#if defined(OE)
	ArgsWrapper args;
#endif
	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_ipc_echo <text>\n");
		return;
	}
	else if (!ipc::IsActive())
	{
		Msg("No IPC client connected.\n");
		return;
	}

	int argc = args.ArgC();
	nlohmann::json msg;

	std::string text;
#if !defined(OE)
	auto length = std::strlen(args.ArgS());
	text.reserve(length);
#endif
	msg["type"] = "echo";

	for (int i = 1; i < argc; i++)
	{
		if (i > 1)
		{
			text.push_back(' ');
		}
		const char* arg = args.Arg(i);
		text += args.Arg(i);
	}

	// The json library just calls exit instead of throwing an exception on invalid UTF-8
	// so gotta check it's valid first.
	if (IsValidUTF8(text.c_str()))
	{
		msg["text"] = text;
		ipc::Send(msg);
	}
	else
	{
		Msg("Text contained non-UTF8 characters, cannot send message.\n");
	}
}