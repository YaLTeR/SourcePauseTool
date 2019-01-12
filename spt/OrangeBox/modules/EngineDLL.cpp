#include "stdafx.h"
#include "..\cvars.hpp"
#include "..\modules.hpp"

#include "..\..\sptlib-wrapper.hpp"
#include <SPTLib\memutils.hpp>
#include <SPTLib\hooks.hpp>
#include "EngineDLL.hpp"
#include "vguimatsurfaceDLL.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\patterns.hpp"

using std::uintptr_t;
using std::size_t;

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer()
{
	return engineDLL.HOOKED_SV_ActivateServer_Func();
}

void __fastcall EngineDLL::HOOKED_FinishRestore(void* thisptr, int edx)
{
	return engineDLL.HOOKED_FinishRestore_Func(thisptr, edx);
}

void __fastcall EngineDLL::HOOKED_SetPaused(void* thisptr, int edx, bool paused)
{
	return engineDLL.HOOKED_SetPaused_Func(thisptr, edx, paused);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame(float time)
{
	return engineDLL.HOOKED__Host_RunFrame_Func(time);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Input(float accumulated_extra_samples, int bFinalTick)
{
	return engineDLL.HOOKED__Host_RunFrame_Input_Func(accumulated_extra_samples, bFinalTick);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Server(int bFinalTick)
{
	return engineDLL.HOOKED__Host_RunFrame_Server_Func(bFinalTick);
}

void __cdecl EngineDLL::HOOKED_Cbuf_Execute()
{
	return engineDLL.HOOKED_Cbuf_Execute_Func();
}

void __fastcall EngineDLL::HOOKED_VGui_Paint(void * thisptr, int edx, int mode)
{
	engineDLL.HOOKED_VGui_Paint_Func(thisptr, edx, mode);
}

void EngineDLL::Hook(const std::wstring& moduleName, void* moduleHandle, void* moduleBase, size_t moduleLength, bool needToIntercept)
{
	Clear(); // Just in case.

	patternContainer.Init(moduleName, (int)moduleBase, moduleLength);

	uintptr_t pSpawnPlayer = NULL,
		pMiddleOfSV_InitGameDLL = NULL,
		pRecord = NULL;

	/*
	patternContainer.AddEntry(HOOKED_SV_ActivateServer, (PVOID*)&ORIG_SV_ActivateServer, Patterns::ptnsSV_ActivateServer, "SV_ActivateServer");
	patternContainer.AddEntry(HOOKED_FinishRestore, (PVOID*)&ORIG_FinishRestore, Patterns::ptnsFinishRestore, "FinishRestore");
	patternContainer.AddEntry(HOOKED_SetPaused, (PVOID*)&ORIG_SetPaused, Patterns::ptnsSetPaused, "SetPaused");
	patternContainer.AddEntry(nullptr, (PVOID*)&pMiddleOfSV_InitGameDLL, Patterns::ptnsMiddleOfSV_InitGameDLL, "MiddleOfSV_InitGameDLL");
	patternContainer.AddEntry(nullptr, (PVOID*)&pRecord, Patterns::ptnsRecord, "Record");
	patternContainer.AddEntry(HOOKED_VGui_Paint, (PVOID*)&ORIG_VGui_Paint, Patterns::ptnsVGui_Paint, "VGui_Paint");
	patternContainer.AddEntry(nullptr, (PVOID*)&pSpawnPlayer, Patterns::ptnsSpawnPlayer, "SpawnPlayer");


	// m_bLoadgame and pGameServer (&sv)
	if (pSpawnPlayer)
	{
		MemUtils::ptnvec_size ptnNumber = patternContainer.FindPatternIndex((PVOID*)&pSpawnPlayer);

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

		case 4:
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 26));
			//pGameServer = (*(void **)(pSpawnPlayer + 21)); - We get this one from SV_ActivateServer in OE.
			break;

		case 5: // 6879 is the same as 5339 here.
			pM_bLoadgame = (*(bool **)(pSpawnPlayer + 8));
			pGameServer = (*(void **)(pSpawnPlayer + 21));
			break;
		}

		EngineDevMsg("m_bLoadGame is situated at %p.\n", pM_bLoadgame);

#if !defined( OE )
		EngineDevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}
	else
	{
		EngineWarning("y_spt_pause 2 has no effect.\n");
	}

	// SV_ActivateServer
	if (ORIG_SV_ActivateServer)
	{
		MemUtils::ptnvec_size ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_SV_ActivateServer);
		switch (ptnNumber)
		{
		case 3:
			pGameServer = (*(void **)((int)ORIG_SV_ActivateServer + 223));
			break;
		}

#if defined( OE )
		EngineDevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}
	else
	{
		EngineWarning("y_spt_pause 2 has no effect.\n");
		EngineWarning("_y_spt_afterframes_reset_on_server_activate has no effect.\n");
	}

	// FinishRestore
	if (!ORIG_FinishRestore)
	{
		EngineWarning("y_spt_pause 1 has no effect.\n");
	}

	// SetPaused
	if (!ORIG_FinishRestore)
	{
		EngineWarning("y_spt_pause has no effect.\n");
	}

	// interval_per_tick
	if (pMiddleOfSV_InitGameDLL)
	{
		MemUtils::ptnvec_size ptnNumber = patternContainer.FindPatternIndex((PVOID*)&pMiddleOfSV_InitGameDLL);

		switch (ptnNumber)
		{
		case 0:
			pIntervalPerTick = *reinterpret_cast<float**>(pMiddleOfSV_InitGameDLL + 18);
			break;

		case 1:
			pIntervalPerTick = *reinterpret_cast<float**>(pMiddleOfSV_InitGameDLL + 16);
			break;
		}
		
		EngineDevMsg("Found interval_per_tick at %p.\n", pIntervalPerTick);
	}
	else
	{
		EngineWarning("_y_spt_tickrate has no effect.\n");
	}

	if (pRecord)
	{
		pDemoplayer = *reinterpret_cast<void***>(pRecord + 132);
		EngineDevMsg("Found demoplayer at %p.\n", pDemoplayer);
	}
	else
	{
		EngineWarning("y_spt_pause_demo_on_tick is not available.\n");
	}

	if (!ORIG_VGui_Paint)
	{
		EngineWarning("Speedrun hud is not available.\n");
	}
	*/
	patternContainer.Hook();
}

void EngineDLL::Unhook()
{
	patternContainer.Unhook();
	Clear();
}

void EngineDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_SV_ActivateServer = nullptr;
	ORIG_FinishRestore = nullptr;
	ORIG_SetPaused = nullptr;
	ORIG__Host_RunFrame = nullptr;
	ORIG__Host_RunFrame_Input = nullptr;
	ORIG__Host_RunFrame_Server = nullptr;
	ORIG_Cbuf_Execute = nullptr;
	ORIG_VGui_Paint = nullptr;
	pGameServer = nullptr;
	pM_bLoadgame = nullptr;
	shouldPreventNextUnpause = false;
	pIntervalPerTick = nullptr;
	pHost_Frametime = nullptr;
	pM_State = nullptr;
	pM_nSignonState = nullptr;
	pDemoplayer = nullptr;
}

float EngineDLL::GetTickrate() const
{
	if (pIntervalPerTick)
		return *pIntervalPerTick;

	return 0;
}

void EngineDLL::SetTickrate(float value)
{
	if (pIntervalPerTick)
		*pIntervalPerTick = value;
}

int EngineDLL::Demo_GetPlaybackTick() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall ***)(void*)>(demoplayer))[2](demoplayer);
}

int EngineDLL::Demo_GetTotalTicks() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall ***)(void*)>(demoplayer))[3](demoplayer);
}

bool EngineDLL::Demo_IsPlayingBack() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall ***)(void*)>(demoplayer))[5](demoplayer);
}

bool EngineDLL::Demo_IsPlaybackPaused() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall ***)(void*)>(demoplayer))[6](demoplayer);
}

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer_Func()
{
	bool result = ORIG_SV_ActivateServer();

	EngineDevMsg("Engine call: SV_ActivateServer() => %s;\n", (result ? "true" : "false"));

	if (ORIG_SetPaused && pM_bLoadgame && pGameServer)
	{
		if ((y_spt_pause.GetInt() == 2) && *pM_bLoadgame)
		{
			ORIG_SetPaused((void *)pGameServer, 0, true);
			EngineDevMsg("Pausing...\n");

			shouldPreventNextUnpause = true;
		}
	}

	if (_y_spt_afterframes_reset_on_server_activate.GetBool())
		clientDLL.ResetAfterframesQueue();

	return result;
}

void __fastcall EngineDLL::HOOKED_FinishRestore_Func(void* thisptr, int edx)
{
	EngineDevMsg("Engine call: FinishRestore();\n");

	if (ORIG_SetPaused && (y_spt_pause.GetInt() == 1))
	{
		ORIG_SetPaused(thisptr, 0, true);
		EngineDevMsg("Pausing...\n");

		shouldPreventNextUnpause = true;
	}

	ORIG_FinishRestore(thisptr, edx);

	clientDLL.ResumeAfterframesQueue();
}

void __fastcall EngineDLL::HOOKED_SetPaused_Func(void* thisptr, int edx, bool paused)
{
	if (pM_bLoadgame)
	{
		EngineDevMsg("Engine call: SetPaused( %s ); m_bLoadgame = %s\n", (paused ? "true" : "false"), (*pM_bLoadgame ? "true" : "false"));
	}
	else
	{
		EngineDevMsg("Engine call: SetPaused( %s );\n", (paused ? "true" : "false"));
	}

	if (paused == false)
	{
		if (shouldPreventNextUnpause)
		{
			EngineDevMsg("Unpause prevented.\n");
			shouldPreventNextUnpause = false;
			return;
		}
	}

	shouldPreventNextUnpause = false;
	return ORIG_SetPaused(thisptr, edx, paused);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Func(float time)
{
	EngineDevMsg("_Host_RunFrame( %.8f ); m_nSignonState = %d;", time, *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg("\n");

	ORIG__Host_RunFrame(time);

	EngineDevMsg("_Host_RunFrame end.\n");
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Input_Func(float accumulated_extra_samples, int bFinalTick)
{
	EngineDevMsg("_Host_RunFrame_Input( %.8f, %d ); m_nSignonState = %d;", time, bFinalTick, *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG__Host_RunFrame_Input(accumulated_extra_samples, bFinalTick);

	EngineDevMsg("_Host_RunFrame_Input end.\n");
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Server_Func(int bFinalTick)
{
	EngineDevMsg("_Host_RunFrame_Server( %d ); m_nSignonState = %d;", bFinalTick, *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG__Host_RunFrame_Server(bFinalTick);

	EngineDevMsg("_Host_RunFrame_Server end.\n");
}

void __cdecl EngineDLL::HOOKED_Cbuf_Execute_Func()
{
	EngineDevMsg("Cbuf_Execute(); m_nSignonState = %d;", *pM_nSignonState);
	if (pM_State)
		_EngineDevMsg(" m_State = %d;", *pM_State);
	_EngineDevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG_Cbuf_Execute();

	EngineDevMsg("Cbuf_Execute() end.\n");
}

void __fastcall EngineDLL::HOOKED_VGui_Paint_Func(void * thisptr, int edx, int mode)
{
	if (mode == 2 && !clientDLL.renderingOverlay)
		vgui_matsurfaceDLL.DrawHUD((vrect_t*)clientDLL.screenRect);

	ORIG_VGui_Paint(thisptr, edx, mode);
}
