#include "stdafx.h"
#include "cvars.hpp"
#include "feature.hpp"
#include "interfaces.hpp"
#include "tier1\tier1.h"

static std::unordered_map<ConCommandBase*, void*> cmd_to_feature;

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

#if defined(OE)
void Cvar_RegisterSPTCvars()
{
	if (!interfaces::g_pCVar)
		return;

	// Was unable implement similar diagnostics with which commands get unloaded in OE
	// Using the OneTimeInit function completely broke the commands from being loaded the second time
	for (auto command_pair : cmd_to_feature)
	{
		command_pair.first->AddFlags(FCVAR_PLUGIN);
		// Unlink from plugin only list
		command_pair.first->SetNext(0);
		interfaces::g_pCVar->RegisterConCommandBase(command_pair.first);
	}
}

void Cvar_UnregisterSPTCvars()
{
	// The OE interface for ConVars leaves a lot to be desired...
	// First deletes all plugin ConVars and then readds the ConVars outside of SPT
	std::vector<ConCommandBase*> commandsToAdd;

	ConCommandBase* command = interfaces::g_pCVar->GetCommands();

	while (command)
	{
		// Detect which non-spt commands have this flag set, and add them back afterwards
		if (command->IsBitSet(FCVAR_PLUGIN) && cmd_to_feature.find(command) == cmd_to_feature.end())
		{
			commandsToAdd.push_back(command);
		}

		command = const_cast<ConCommandBase*>(command->GetNext());
	}

	interfaces::g_pCVar->UnlinkVariables(FCVAR_PLUGIN);

	for (ConCommandBase* command : commandsToAdd)
	{
		interfaces::g_pCVar->RegisterConCommandBase(command);
	}

	cmd_to_feature.clear();
}

#else
static bool first_time_init = true;

void Cvar_RegisterSPTCvars()
{
	if (!interfaces::g_pCVar)
		return;

	// First time init is more complicated but reports which cvars were unloaded
	// Useful for development
	if (first_time_init)
	{
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
	else
	{
		// The above implementation no longer works after restarts
		// Adding the concommands breaks the linked list structure there is for the plugin convars
		// And thus they don't get registered if you call ConVar_Register a second time
		for (auto command_pair : cmd_to_feature)
		{
			interfaces::g_pCVar->RegisterConCommand(command_pair.first);
		}
	}

	first_time_init = false;
}

void Cvar_UnregisterSPTCvars()
{
	for (auto command_pair : cmd_to_feature)
	{
		interfaces::g_pCVar->UnregisterConCommand(command_pair.first);
	}

	cmd_to_feature.clear();
}
#endif
