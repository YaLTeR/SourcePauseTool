#include <cstddef>
#include <cstdint>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "..\cvars.hpp"
#include "..\..\spt.hpp"
#include "..\..\memutils.hpp"
#include "..\..\detoursutils.hpp"
#include "..\..\patterns.hpp"
#include "..\..\hooks.hpp"
#include "EngineDLL.hpp"

using std::uintptr_t;
using std::size_t;

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer()
{
	return Hooks::getInstance().engineDLL.HOOKED_SV_ActivateServer_Func();
}

void __fastcall EngineDLL::HOOKED_FinishRestore(void* thisptr, int edx)
{
	return Hooks::getInstance().engineDLL.HOOKED_FinishRestore_Func(thisptr, edx);
}

void __fastcall EngineDLL::HOOKED_SetPaused(void* thisptr, int edx, bool paused)
{
	return Hooks::getInstance().engineDLL.HOOKED_SetPaused_Func(thisptr, edx, paused);
}

void EngineDLL::Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength)
{
	Clear(); // Just in case.

	this->hModule = hModule;
	this->moduleStart = moduleStart;
	this->moduleLength = moduleLength;
	this->moduleName = moduleName;

	MemUtils::ptnvec_size ptnNumber;

	// m_bLoadgame and pGameServer (&sv)
	EngineDevMsg("SPT: Searching for SpawnPlayer...\n");

	uintptr_t pSpawnPlayer = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsSpawnPlayer, &pSpawnPlayer);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		EngineDevMsg("SPT: Found SpawnPlayer at %p (using the build %s pattern).\n", pSpawnPlayer, Patterns::ptnsSpawnPlayer[ptnNumber].build.c_str());

		switch (ptnNumber)
		{
		case 0:
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 5));
			pGameServer = (*(void **)(pSpawnPlayer + 18));
			break;

		case 1:
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 8));
			pGameServer = (*(void **)(pSpawnPlayer + 21));
			break;

		case 2: // 4104 is the same as 5135 here.
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 5));
			pGameServer = (*(void **)(pSpawnPlayer + 18));
			break;

		case 3: // 2257546 is the same as 5339 here.
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 8));
			pGameServer = (*(void **)(pSpawnPlayer + 21));
			break;
		}

		EngineDevMsg("SPT: m_bLoadGame is situated at %p.\n", pM_bLoadgame);
		EngineDevMsg("SPT: pGameServer is %p.\n", pGameServer);
	}
	else
	{
		EngineDevWarning("SPT: Could not find SpawnPlayer!\n");
		EngineWarning("SPT: y_spt_pause 2 has no effect.\n");
	}

	// SV_ActivateServer
	EngineDevMsg("SPT: Searching for SV_ActivateServer...\n");

	uintptr_t pSV_ActivateServer = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsSV_ActivateServer, &pSV_ActivateServer);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_SV_ActivateServer = (_SV_ActivateServer)pSV_ActivateServer;
		EngineDevMsg("SPT: Found SV_ActivateServer at %p (using the build %s pattern).\n", pSV_ActivateServer, Patterns::ptnsSV_ActivateServer[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("SPT: Could not find SV_ActivateServer!\n");
		EngineWarning("SPT: y_spt_pause 2 has no effect.\n");
		EngineWarning("SPT: y_spt_afterframes_reset_on_server_activate has no effect.\n");
	}

	// FinishRestore
	EngineDevMsg("SPT: Searching for FinishRestore...\n");

	uintptr_t pFinishRestore = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsFinishRestore, &pFinishRestore);
	if (ptnNumber != MemUtils::INVALID_SEQUENCE_INDEX)
	{
		ORIG_FinishRestore = (_FinishRestore)pFinishRestore;
		EngineDevMsg("SPT: Found FinishRestore at %p (using the build %s pattern).\n", pFinishRestore, Patterns::ptnsFinishRestore[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("SPT: Could not find FinishRestore!\n");
		EngineWarning("SPT: y_spt_pause 1 has no effect.\n");
	}

	// SetPaused
	EngineDevMsg("SPT: Searching for SetPaused...\n");

	uintptr_t pSetPaused = NULL;
	ptnNumber = MemUtils::FindUniqueSequence(moduleStart, moduleLength, Patterns::ptnsSetPaused, &pSetPaused);
	if (pSetPaused)
	{
		ORIG_SetPaused = (_SetPaused)pSetPaused;
		EngineDevMsg("SPT: Found SetPaused at %p (using the build %s pattern).\n", pSetPaused, Patterns::ptnsSetPaused[ptnNumber].build.c_str());
	}
	else
	{
		EngineDevWarning("SPT: Could not find SetPaused!\n");
		EngineWarning("SPT: y_spt_pause has no effect.\n");
	}

	AttachDetours(moduleName, 6,
		&ORIG_SV_ActivateServer, HOOKED_SV_ActivateServer,
		&ORIG_FinishRestore, HOOKED_FinishRestore,
		&ORIG_SetPaused, HOOKED_SetPaused);
}

void EngineDLL::Unhook()
{
	DetachDetours(moduleName, 6,
		&ORIG_SV_ActivateServer, HOOKED_SV_ActivateServer,
		&ORIG_FinishRestore, HOOKED_FinishRestore,
		&ORIG_SetPaused, HOOKED_SetPaused);

	Clear();
}

void EngineDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_SV_ActivateServer = nullptr;
	ORIG_FinishRestore = nullptr;
	ORIG_SetPaused = nullptr;
	pGameServer = nullptr;
	pM_bLoadgame = nullptr;
	shouldPreventNextUnpause = false;
}

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer_Func()
{
	bool result = ORIG_SV_ActivateServer();

	EngineDevMsg("SPT: Engine call: SV_ActivateServer() => %s;\n", (result ? "true" : "false"));

	if (ORIG_SetPaused && pM_bLoadgame && pGameServer)
	{
		if ((y_spt_pause.GetInt() == 2) && *pM_bLoadgame)
		{
			ORIG_SetPaused((void *)pGameServer, 0, true);
			EngineDevMsg("SPT: Pausing...\n");

			shouldPreventNextUnpause = true;
		}
	}

	if (y_spt_afterframes_reset_on_server_activate.GetBool())
		Hooks::getInstance().clientDLL.ResetAfterframesQueue();

	return result;
}

void __fastcall EngineDLL::HOOKED_FinishRestore_Func(void* thisptr, int edx)
{
	EngineDevMsg("SPT: Engine call: FinishRestore();\n");

	if (ORIG_SetPaused && (y_spt_pause.GetInt() == 1))
	{
		ORIG_SetPaused(thisptr, 0, true);
		EngineDevMsg("SPT: Pausing...\n");

		shouldPreventNextUnpause = true;
	}

	return ORIG_FinishRestore(thisptr, edx);
}

void __fastcall EngineDLL::HOOKED_SetPaused_Func(void* thisptr, int edx, bool paused)
{
	if (pM_bLoadgame)
	{
		EngineDevMsg("SPT: Engine call: SetPaused( %s ); m_bLoadgame = %s\n", (paused ? "true" : "false"), (*pM_bLoadgame ? "true" : "false"));
	}
	else
	{
		EngineDevMsg("SPT: Engine call: SetPaused( %s );\n", (paused ? "true" : "false"));
	}

	if (paused == false)
	{
		if (shouldPreventNextUnpause)
		{
			EngineDevMsg("SPT: Unpause prevented.\n");
			shouldPreventNextUnpause = false;
			return;
		}
	}

	shouldPreventNextUnpause = false;
	return ORIG_SetPaused(thisptr, edx, paused);
}