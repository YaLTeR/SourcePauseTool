#include "stdafx.h"
#include "convar.h"
#include "spt\OrangeBox\spt-serverplugin.hpp"
#include "thirdparty\json.hpp"
#include "ipc-spt.hpp"
#include "spt\utils\ent_utils.hpp"
#include "spt\utils\property_getter.hpp"
#include "spt\utils\string_parsing.hpp"

void MapPropToJson(RecvProp* prop, void* ptr, nlohmann::json& msg)
{
	void* value = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + prop->GetOffset());
	int i;
	float f;
	Vector v;

	switch (prop->m_RecvType)
	{
	case DPT_Int:
		i = *reinterpret_cast<int*>(value);
		msg[prop->GetName()] = i;
		break;
	case DPT_Float:
		f = *reinterpret_cast<float*>(value);
		msg[prop->GetName()] = f;
		break;
	case DPT_Vector:
		v = *reinterpret_cast<Vector*>(value);
		msg[FormatTempString("%s[0]", prop->GetName())] = v[0];
		msg[FormatTempString("%s[1]", prop->GetName())] = v[1];
		msg[FormatTempString("%s[2]", prop->GetName())] = v[2];
		break;
#ifdef SSDK2007
	case DPT_String:
		msg[prop->GetName()] = *reinterpret_cast<const char**>(value);
		break;
#endif
	default:
		break;
	}
}

void MapEntToJson(RecvTable* table, void* ptr, nlohmann::json& json)
{
	int numProps = table->m_nProps;

	for (int i = 0; i < numProps; ++i)
	{
		auto prop = table->GetProp(i);

		if (strcmp(prop->GetName(), "baseclass") == 0)
		{
			RecvTable* base = prop->GetDataTable();

			if (base)
				MapEntToJson(base, ptr, json);
		}
		else if (prop->GetOffset() != 0)
		{
			MapPropToJson(prop, ptr, json);
		}
	}
}

void MapEntToJson(IClientEntity* ent, nlohmann::json& json)
{
	auto table = ent->GetClientClass()->m_pRecvTable;
	MapEntToJson(table, ent, json);
}

#if !defined(OE)
CON_COMMAND(y_spt_ipc_ent, "y_spt_ipc_ent <entity index> - Outputs entity data to IPC client.\n")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_ipc_ent <entity index>\n");
		return;
	}
	else if (!ipc::IsActive())
	{
		Msg("No IPC client connected.\n");
		return;
	}

	nlohmann::json msg;
	msg["type"] = "ent";
	int index = std::atoi(args.Arg(1));
	auto ent = utils::GetClientEntity(index);

	if (!ent)
	{
		msg["exists"] = false;
	}
	else
	{
		msg["exists"] = true;
		msg["entity"] = nlohmann::json();
		MapEntToJson(ent, msg["entity"]);
	}
	ipc::Send(msg);
}
#endif

#if !defined(OE)
CON_COMMAND(
    y_spt_ipc_properties,
    "y_spt_ipc_properties <entity index> <property1> <property2> ... - Outputs entity properties to IPC client.\n")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_ipc_properties <entity index> <property1> <property2> ...\n");
		return;
	}
	else if (!ipc::IsActive())
	{
		Msg("No IPC client connected.\n");
		return;
	}

	nlohmann::json msg;
	msg["type"] = "ent";
	int index = std::atoi(args.Arg(1));
	auto ent = utils::GetClientEntity(index);

	if (!ent)
	{
		msg["exists"] = false;
	}
	else
	{
		msg["exists"] = true;
		msg["entity"] = nlohmann::json();
		int argc = args.ArgC();
		for (int i = 2; i < argc; ++i)
		{
			const char* propName = args.Arg(i);
			auto prop = utils::GetRecvProp(index, propName);

			if (prop)
			{
				MapPropToJson(prop, ent, msg["entity"]);
			}
			else
			{
				Msg("Could not find prop %s\n", propName);
			}
		}
	}
	ipc::Send(msg);
}
#endif

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