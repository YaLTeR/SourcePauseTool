#include "stdafx.h"
#include "cvars.hpp"
#include "feature.hpp"
#include "interfaces.hpp"
#include "tier1\tier1.h"

static std::unordered_map<ConCommandBase*, void*> cmd_to_feature;

// For OE CVar and ConCommand registering.
#if defined(OE)
class CPluginConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* pCommand)
	{
		if (cmd_to_feature.find(pCommand) == cmd_to_feature.end())
		{
			DevWarning("Command %s was unloaded, because it was not initialized!\n", pCommand->GetName());
			return false;
		}
		pCommand->AddFlags(FCVAR_PLUGIN);

		// Unlink from plugin only list
		pCommand->SetNext(0);

		// Link to engine's list instead
		g_pCVar->RegisterConCommandBase(pCommand);
		return true;
	}
};

CPluginConVarAccessor g_ConVarAccessor;

// OE: For correct linking in VS 2015.
int(__cdecl* __vsnprintf)(char*, size_t, const char*, va_list) = _vsnprintf;
#endif

void Cvar_InitConCommandBase(ConCommandBase& concommand, void* owner)
{
	if (cmd_to_feature.find(&concommand) != cmd_to_feature.end())
	{
		Warning("Two commands trying to init concommand %s!\n", concommand.GetName());
	}
	else
	{
		cmd_to_feature[&concommand] = owner;
	}
}

#if !defined(OE)
void Cvar_RegisterSPTCvars()
{
	if (!g_pCVar)
		return;
	ConVar_Register(0);
	int identifier = y_spt_pause.GetDLLIdentifier();

	ConCommandBase* cmd = interfaces::g_pCVar->GetCommands();

	// Loops through the console variables and commands, delete any that are initialized
	while (cmd != NULL)
	{
		ConCommandBase* todelete = nullptr;
		if (cmd->GetDLLIdentifier() == identifier)
		{
			if (cmd_to_feature.find(cmd) == cmd_to_feature.end())
			{
				DevWarning("Command %s was unloaded, because it was not initialized!\n",
				           cmd->GetName());
				todelete = cmd;
			}
		}
		cmd = cmd->GetNext();
		if (todelete)
			g_pCVar->UnregisterConCommand(todelete);
	}
}
#endif

#ifdef OE
void Cvar_RegisterSPTCvars()
{
	if (g_pCVar)
	{
		ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);
	}
}
#endif
