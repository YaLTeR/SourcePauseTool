#include "stdafx.hpp"
#include "cvars.hpp"

#include <stdarg.h>
#include <algorithm>

#include "SPTLib\MemUtils.hpp"
#include "thirdparty/x86.h"
#include "tier1\tier1.h"

#include "spt\features\cvar.hpp"
#include "spt\feature.hpp"
#include "convar.hpp"
#include "interfaces.hpp"
#include "sptlib-wrapper.hpp"

struct FeatureCommand
{
	void* owner; // Feature*
	ConCommandBase* command;
	bool dynamicCommand; // dynamically allocated
	bool dynamicName;
	bool unhideOnUnregister;
};
static std::vector<FeatureCommand> cmd_to_feature;
static bool first_time_init = true;
static ConCommandBase** pluginCommandListHead = nullptr;

void Cvar_InitConCommandBase(ConCommandBase& concommand, void* owner)
{
	auto found = std::find_if(cmd_to_feature.begin(),
	                          cmd_to_feature.end(),
	                          [concommand](const FeatureCommand& fc) { return fc.command == &concommand; });
	if (found != cmd_to_feature.end())
	{
		Warning("Two commands trying to init concommand %s!\n", concommand.GetName());
		return;
	}

	FeatureCommand cmd = {owner, &concommand, false, false, false};
	cmd_to_feature.push_back(cmd);
	// Add command to command list since it got wiped after registering commands the first time
	if (!first_time_init)
	{
		if (pluginCommandListHead && !concommand.IsFlagSet(FCVAR_UNREGISTERED))
		{
			reinterpret_cast<ConCommandBase_guts&>(concommand).m_pNext = *pluginCommandListHead;
			*pluginCommandListHead = &concommand;
		}
	}
}

void FormatConCmd(const char* fmt, ...)
{
	static char BUFFER[8192];

	va_list args;
	va_start(args, fmt);
	vsprintf_s(BUFFER, ARRAYSIZE(BUFFER), fmt, args);
	va_end(args);

	EngineConCmd(BUFFER);
}

typedef ICvar*(__cdecl* _GetCvarIF)();

extern "C" ICvar* GetCVarIF()
{
	static ICvar* ptr = nullptr;

	if (ptr != nullptr)
	{
		return ptr;
	}
	else
	{
		void* moduleHandle;
		void* moduleBase;
		std::size_t moduleSize;

		if (MemUtils::GetModuleInfo(L"vstdlib.dll", &moduleHandle, &moduleBase, &moduleSize))
		{
			_GetCvarIF func = (_GetCvarIF)MemUtils::GetSymbolAddress(moduleHandle, "GetCVarIF");
			ptr = func();
		}

		return ptr;
	}
}

static void RemoveCommandFromList(ConCommandBase** head, ConCommandBase* command)
{
	if (head == NULL)
		return;

	ConCommandBase_guts* pPrev = NULL;
	for (ConCommandBase_guts* pCommand = *(ConCommandBase_guts**)head; pCommand;
	     pCommand = (ConCommandBase_guts*)pCommand->m_pNext)
	{
		if (pCommand != (ConCommandBase_guts*)command)
		{
			pPrev = pCommand;
			continue;
		}

		if (pPrev == NULL)
			*head = pCommand->m_pNext;
		else
			pPrev->m_pNext = pCommand->m_pNext;

		pCommand->m_pNext = NULL;
		break;
	}
}

#ifdef OE
static ConCommandBase** GetGlobalCommandListHead()
{
	// According to the SDK, ICvar::GetCommands is position 9 on the vtable
	void** icvarVtable = *(void***)interfaces::g_pCVar;
	uint8_t* addr = (uint8_t*)icvarVtable[9];
	// Follow along thunked function
	if (*addr == X86_JMPIW)
	{
		uint32_t offset = *(uint32_t*)(addr + 1);
		addr += x86_len(addr) + offset;
	}
	// Check if it's the right instruction
	if (*addr == X86_MOVEAXII)
		return *(ConCommandBase***)(addr + 1);
	return NULL;
}

static void UnregisterConCommand(ConCommandBase* pCommandToRemove)
{
	static bool searchedForList = false;
	static ConCommandBase** globalCommandListHead = nullptr;

	// Look for the global command list head once
	if (!searchedForList)
	{
		globalCommandListHead = GetGlobalCommandListHead();
		searchedForList = true;
	}

	if (!globalCommandListHead)
		return;

	// Not registered? Don't bother
	if (!pCommandToRemove->IsRegistered())
		return;

	reinterpret_cast<ConCommandBase_guts*>(pCommandToRemove)->m_bRegistered = false;
	RemoveCommandFromList(globalCommandListHead, pCommandToRemove);
}

class CPluginConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* pCommand)
	{
		pCommand->AddFlags(FCVAR_PLUGIN); // just because

		// Unlink from plugin only list
		// Necessary because RegisterConCommandBase skips the command if it's next isn't null
		pCommand->SetNext(0);

		// Link to engine's list instead
		interfaces::g_pCVar->RegisterConCommandBase(pCommand);
		return true;
	}
};

static CPluginConVarAccessor g_ConVarAccessor;
#endif // OE

// Returns the address of ConCommandBase::s_pConCommandBases
// This is where commands are stored before being registered
static ConCommandBase** GetPluginCommandListHead()
{
#ifdef OE
	// First thing that happens in this function is loading a register with the list head
	uint8_t* registerAddr = (uint8_t*)ConCommandBaseMgr::OneTimeInit;
#else
	// Close to the end of ConVar_Register, the list head is set to null
	// There's a convenient jump close to the start of the function that takes
	// us a couple instructions past that point
	uint8_t* registerAddr = (uint8_t*)ConVar_Register;
#endif
	// Not sure if it's only a debug build thing, but the function address might be
	// in a jmp table. Follow the jmp if that's the case
	if (*registerAddr == X86_JMPIW)
	{
		uint32_t offset = *(uint32_t*)(registerAddr + 1);
		registerAddr += x86_len(registerAddr) + offset;
	}
#ifdef OE
	return *(ConCommandBase***)(registerAddr + 2);
#else
	int jzOffset = 0, _len = 0;
	for (uint8_t* addr = registerAddr; _len != -1 && addr - registerAddr < 32; _len = x86_len(addr), addr += _len)
	{
		// Check for a JZ (both short jump and near jump opcodes)
		if (addr[0] == X86_JZ)
			jzOffset = addr[1];
		else if (addr[0] == X86_2BYTE && addr[1] == X86_2B_JZII)
			jzOffset = *(uint32_t*)(addr + 2);

		if (jzOffset)
		{
			// Take the ride down the jump and go back a couple instructions
			addr += x86_len(addr) + jzOffset - 11;
			// Make sure that we landed on the instruction we wanted
			if (addr[0] != X86_MOVMIW)
				return NULL;
			return *(ConCommandBase***)(addr + 2);
		}
	}
	return NULL;
#endif
}

// Check if spt_ prefix is at the start. If not, create a new command and hide the old one with
// the legacy name. Put prefix at the start if we don't find it at all
static void HandleBackwardsCompatibility(FeatureCommand& featCmd, const char* cmdName)
{
	ConCommandBase* cmd = featCmd.command;
	bool plusMinus = (cmdName[0] == '+' || cmdName[0] == '-');
	char* newName = const_cast<char*>(strstr(cmdName, "spt_"));
	if (newName == (cmdName + plusMinus)) // no legacy prefix
		return;

	bool allocatedName = false;
	// ex: tas_pause, +myprefix_something
	if (!newName)
	{
		int len = strlen(cmdName) + 5;
		newName = new char[len];
		newName[0] = cmdName[0];
		strcpy(newName + plusMinus, "spt_");
		strcat(newName, cmdName + plusMinus);
		allocatedName = true;
	}
	// ex: +y_spt_duckspam
	else if (plusMinus)
	{
		int len = strlen(newName) + 2;
		char* tmpName = new char[len];
		tmpName[0] = cmdName[0];
		strcpy(tmpName + 1, newName);
		newName = tmpName;
		allocatedName = true;
	}

	// New commands are added to the beginning of the linked list, so it's safe to do this
	// while iterating through the list. s_pAccessor needs to be NULL here or else the
	// constructor calls below will break the local command list
	ConCommandBase* newCommand;
	if (cmd->IsCommand())
	{
		ConCommand_guts* cmdGuts = reinterpret_cast<ConCommand_guts*>(cmd);
		newCommand = new ConCommand(newName,
		                            cmdGuts->m_fnCommandCallback,
		                            cmdGuts->m_pszHelpString,
		                            cmdGuts->m_nFlags,
		                            cmdGuts->m_fnCompletionCallback);
	}
	else
	{
		newCommand = new ConVarProxy((ConVar*)cmd, newName, ((ConVar_guts*)cmd)->m_nFlags);
	}

	FeatureCommand newCmd = {featCmd.owner, newCommand, true, allocatedName, false};
	cmd_to_feature.push_back(newCmd);

	// If a legacy command didn't originally have FCVAR_HIDDEN set, we need to hide it now and
	// unhide it when unregistering because if the commands are initialized again, they would all be hidden
	if (!cmd->IsFlagSet(FCVAR_HIDDEN))
	{
		cmd->AddFlags(FCVAR_HIDDEN);
		featCmd.unhideOnUnregister = true;
	}
}

void Cvar_RegisterSPTCvars()
{
	static bool searchedForList = false;
	if (!interfaces::g_pCVar)
		return;

	if (!searchedForList)
	{
		pluginCommandListHead = GetPluginCommandListHead();
		searchedForList = true;
	}
	if (!pluginCommandListHead)
		return;
	ConCommandBase* cmd = *pluginCommandListHead;
	while (cmd != NULL)
	{
		const char* cmdName = cmd->GetName();
		if (strlen(cmdName) == 0) // There's a CEmptyConVar in the list for some reason
		{
			cmd = (ConCommandBase*)cmd->GetNext();
			continue;
		}

		auto inittedCmd = std::find_if(cmd_to_feature.begin(),
		                               cmd_to_feature.end(),
		                               [cmd](const FeatureCommand& fc) { return fc.command == cmd; });
		// This will only ever happen on first time init
		if (inittedCmd == cmd_to_feature.end())
		{
			DevWarning("Command %s was unloaded, because it was not initialized!\n", cmdName);
			ConCommandBase* todelete = cmd;
			cmd = (ConCommandBase*)cmd->GetNext();
			RemoveCommandFromList(pluginCommandListHead, todelete);
			continue;
		}
		HandleBackwardsCompatibility(*inittedCmd, cmdName);
#ifdef OE
		if (cmd->IsBitSet(FCVAR_HIDDEN))
			spt_cvarstuff.hidden_cvars.push_back(cmd);
#endif
		cmd = (ConCommandBase*)cmd->GetNext();
	}
#ifndef OE
	ConVar_Register(0);
#else
	ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);
	// Clear the list head like newer engines do and reset s_pAccessor so the list doesn't break
	// next time SPT commands are registered
	*pluginCommandListHead = NULL;
	ConCommandBaseMgr::OneTimeInit(NULL);
#endif

	first_time_init = false;
}

void Cvar_UnregisterSPTCvars()
{
	if (!interfaces::g_pCVar)
		return;

	for (auto& cmd : cmd_to_feature)
	{
#ifdef OE
		UnregisterConCommand(cmd.command);
#else
		interfaces::g_pCVar->UnregisterConCommand(cmd.command);
#endif
		ConCommandBase_guts* cmdGuts = reinterpret_cast<ConCommandBase_guts*>(cmd.command);
		if (cmd.dynamicCommand)
		{
			if (cmd.dynamicName)
				delete[] cmdGuts->m_pszName;
			delete cmd.command;
		}
		// Refer to comment in HandleBackwardsCompatibility
		if (cmd.unhideOnUnregister)
			cmdGuts->m_nFlags &= ~FCVAR_HIDDEN;
	}

	cmd_to_feature.clear();
}

#ifdef SSDK2013
CON_COMMAND(spt_dummy, "") {}

// Hack: ConCommand::AutoCompleteSuggest is broken in the SDK. Replacing it with the game's one.
void ReplaceAutoCompleteSuggest()
{
	if (!_record)
		return;
	ConCommand_guts* cmd = (ConCommand_guts*)_record;

	const int vtidx_AutoCompleteSuggest = 10;
	ConCommand_guts* dummy = (ConCommand_guts*)&spt_dummy_command;

	void* temp;
	Feature::AddVFTableHook(VFTableHook(dummy->vfptr,
	                                    vtidx_AutoCompleteSuggest,
	                                    cmd->vfptr[vtidx_AutoCompleteSuggest],
	                                    &temp),
	                        "spt-2013");
}

#endif
