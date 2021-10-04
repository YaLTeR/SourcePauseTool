#include "stdafx.h"

#include "EngineDLL.hpp"

#include <SPTLib\hooks.hpp>
#include <SPTLib\memutils.hpp>

#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\ent_utils.hpp"
#include "..\cvars.hpp"
#include "..\modules.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\patterns.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "vguimatsurfaceDLL.hpp"

using std::size_t;
using std::uintptr_t;

#define TAG "[engine dll] "

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer()
{
	TRACE_ENTER();
	return engineDLL.HOOKED_SV_ActivateServer_Func();
}

void __fastcall EngineDLL::HOOKED_FinishRestore(void* thisptr, int edx)
{
	TRACE_ENTER();
	return engineDLL.HOOKED_FinishRestore_Func(thisptr, edx);
}

void __fastcall EngineDLL::HOOKED_SetPaused(void* thisptr, int edx, bool paused)
{
	TRACE_ENTER();
	return engineDLL.HOOKED_SetPaused_Func(thisptr, edx, paused);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame(float time)
{
	TRACE_ENTER();
	return engineDLL.HOOKED__Host_RunFrame_Func(time);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Input(float accumulated_extra_samples, int bFinalTick)
{
	TRACE_ENTER();
	return engineDLL.HOOKED__Host_RunFrame_Input_Func(accumulated_extra_samples, bFinalTick);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Server(int bFinalTick)
{
	TRACE_ENTER();
	return engineDLL.HOOKED__Host_RunFrame_Server_Func(bFinalTick);
}

void __cdecl EngineDLL::HOOKED_Host_AccumulateTime(float dt)
{
	if (tas_pause.GetBool())
	{
		*engineDLL.pHost_Realtime += dt;
		*engineDLL.pHost_Frametime = 0;
	}
	else
		engineDLL.ORIG_Host_AccumulateTime(dt);
}

void __cdecl EngineDLL::HOOKED_Cbuf_Execute()
{
	TRACE_ENTER();
	return engineDLL.HOOKED_Cbuf_Execute_Func();
}

void __fastcall EngineDLL::HOOKED_VGui_Paint(void* thisptr, int edx, int mode)
{
	TRACE_ENTER();
	engineDLL.HOOKED_VGui_Paint_Func(thisptr, edx, mode);
}

void __fastcall EngineDLL::HOOKED_StopRecording(void* thisptr, int edx)
{
	TRACE_ENTER();
	engineDLL.HOOKED_StopRecording_Func(thisptr, edx);
}

void __fastcall EngineDLL::HOOKED_SetSignonState(void* thisptr, int edx, int state)
{
	TRACE_ENTER();
	engineDLL.HOOKED_SetSignonState_Func(thisptr, edx, state);
}

void __cdecl EngineDLL::HOOKED_Stop()
{
	TRACE_ENTER();
	engineDLL.HOOKED_Stop_Func();
}

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::engine::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg(TAG "Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::engine::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning(TAG "Could not find " #future_name ".\n"); \
		} \
	}

#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg(TAG "Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::engine::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning(TAG "Could not find " #future_name ".\n"); \
		} \
	}

void EngineDLL::Hook(const std::wstring& moduleName,
                     void* moduleHandle,
                     void* moduleBase,
                     size_t moduleLength,
                     bool needToIntercept)
{
	auto startTime = std::chrono::high_resolution_clock::now();

	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	patternContainer.Init(moduleName);

	uintptr_t ORIG_SpawnPlayer = NULL, ORIG_MiddleOfSV_InitGameDLL = NULL, ORIG_Record = NULL;

	DEF_FUTURE(SV_ActivateServer);
	DEF_FUTURE(FinishRestore);
	DEF_FUTURE(Record);
	DEF_FUTURE(SetPaused);
	DEF_FUTURE(MiddleOfSV_InitGameDLL);
	DEF_FUTURE(VGui_Paint);
	DEF_FUTURE(SpawnPlayer);
	DEF_FUTURE(CEngineTrace__PointOutsideWorld);
	DEF_FUTURE(_Host_RunFrame);
	DEF_FUTURE(Host_AccumulateTime);
	DEF_FUTURE(StopRecording);
	DEF_FUTURE(SetSignonState);
	DEF_FUTURE(Stop);

	GET_HOOKEDFUTURE(SV_ActivateServer);
	GET_HOOKEDFUTURE(FinishRestore);
	GET_HOOKEDFUTURE(SetPaused);
	GET_FUTURE(MiddleOfSV_InitGameDLL);
	GET_HOOKEDFUTURE(VGui_Paint);
	GET_FUTURE(SpawnPlayer);
	GET_FUTURE(CEngineTrace__PointOutsideWorld);
	GET_FUTURE(_Host_RunFrame);
	GET_HOOKEDFUTURE(Host_AccumulateTime);
	GET_HOOKEDFUTURE(StopRecording);
	GET_HOOKEDFUTURE(SetSignonState);
	GET_HOOKEDFUTURE(Stop);

	pVGUI_Paint = (uint)(ORIG_VGui_Paint);

	// m_bLoadgame and pGameServer (&sv)
	if (ORIG_SpawnPlayer)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_SpawnPlayer);

		switch (ptnNumber)
		{
		case 0:
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 5));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 18));
			break;

		case 1:
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 8));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 21));
			break;

		case 2: // 4104 is the same as 5135 here.
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 5));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 18));
			break;

		case 3: // 2257546 is the same as 5339 here.
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 8));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 21));
			break;

		case 4:
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 26));
			//pGameServer = (*(void **)(pSpawnPlayer + 21)); - We get this one from SV_ActivateServer in OE.
			break;

		case 5: // 6879 is the same as 5339 here.
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 8));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 21));
			break;

		default:
			Warning("Spawnplayer did not have a matching switch-case statement!\n");
			break;
		}

		DevMsg("m_bLoadGame is situated at %p.\n", pM_bLoadgame);

#if !defined(OE)
		DevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}
	else
	{
		Warning("y_spt_pause 2 has no effect.\n");
	}

	// SV_ActivateServer
	if (ORIG_SV_ActivateServer)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_SV_ActivateServer);
		switch (ptnNumber)
		{
		case 3:
			pGameServer = (*(void**)((int)ORIG_SV_ActivateServer + 223));
			break;
		}

#if defined(OE)
		DevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}
	else
	{
		Warning("y_spt_pause 2 has no effect.\n");
		Warning("_y_spt_afterframes_reset_on_server_activate has no effect.\n");
	}

	// FinishRestore
	if (!ORIG_FinishRestore)
	{
		Warning("y_spt_pause 1 has no effect.\n");
	}

	// SetPaused
	if (!ORIG_SetPaused)
	{
		Warning("y_spt_pause has no effect.\n");
	}

	// interval_per_tick
	if (ORIG_MiddleOfSV_InitGameDLL)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_MiddleOfSV_InitGameDLL);

		switch (ptnNumber)
		{
		case 0:
			pIntervalPerTick = *reinterpret_cast<float**>(ORIG_MiddleOfSV_InitGameDLL + 18);
			break;

		case 1:
			pIntervalPerTick = *reinterpret_cast<float**>(ORIG_MiddleOfSV_InitGameDLL + 16);
			break;
		}

		DevMsg("Found interval_per_tick at %p.\n", pIntervalPerTick);
	}
	else
	{
		Warning("_y_spt_tickrate has no effect.\n");
	}

	auto pRecord = fRecord.get();
	if (ORIG_Record)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_Record);

		if (ptnNumber == 0)
		{
			pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 132);
			pDemorecorder = **reinterpret_cast<void****>(ORIG_Record + 0x7A);

			// vftable offsets
			GetPlaybackTick_Offset = 2;
			GetTotalTicks_Offset = 3;
			IsPlaybackPaused_Offset = 5;
			IsPlayingBack_Offset = 6;
		}
		else if (ptnNumber == 1)
		{
			pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 0xA2);

			// vftable offsets
			GetPlaybackTick_Offset = 3;
			GetTotalTicks_Offset = 4;
			IsPlaybackPaused_Offset = 6;
			IsPlayingBack_Offset = 7;
		}
		else
			Warning(
			    "Record pattern had no matching clause for catching the demoplayer. y_spt_pause_demo_on_tick unavailable.\n");

		DevMsg("Found demoplayer at %p and demorecorder at %p, record is at %p.\n",
		       pDemoplayer,
		       pDemorecorder,
		       ORIG_Record);
	}
	else
	{
		Warning("y_spt_pause_demo_on_tick is not available.\n");
	}

	// CDemoRecorder::StopRecording
	if (ORIG_StopRecording)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_StopRecording);

		if (ptnNumber == 0)
		{
			m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 65);
			m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 72);
		}
		else if (ptnNumber == 1)
		{
			m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 70);
			m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 77);
		}

		DevMsg("Found CDemoRecorder offsets m_nDemoNumber %d, m_bRecording %d.\n",
		       m_nDemoNumber_Offset,
		       m_bRecording_Offset);
		if (!ORIG_Stop)
			Warning("Manually stopping a TAS demo recording won't stop autorecording.\n");
	}

	// Move 1 byte since the pattern starts a byte before the function
	if (ORIG_SetSignonState)
		ORIG_SetSignonState = (_SetSignonState)((uint32_t)ORIG_SetSignonState + 1);

	if (!ORIG_StopRecording || !ORIG_SetSignonState)
	{
		Warning(
		    "TAS demo recording may overwrite demos if level transitions and saveloads are present in the same script.\n");
	}

	if (!ORIG_VGui_Paint)
	{
		Warning("Speedrun hud is not available.\n");
	}

	if (ORIG__Host_RunFrame)
	{
		pHost_Frametime = *reinterpret_cast<float**>((uintptr_t)ORIG__Host_RunFrame + 227);
	}

	if (ORIG_Host_AccumulateTime)
	{
		pHost_Realtime = *reinterpret_cast<float**>((uintptr_t)ORIG_Host_AccumulateTime + 5);
	}

	patternContainer.Hook();

	auto loadTime =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	DevMsg(TAG "Done hooking in %dms\n", loadTime);
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
	ORIG_StopRecording = nullptr;
	curtime = 0x0;
	ORIG_curtime = 0x0;
	ORIG_FinishRestore = nullptr;
	ORIG_SetPaused = nullptr;
	ORIG__Host_RunFrame = nullptr;	
	ORIG__Host_RunFrame_Input = nullptr;
	ORIG__Host_RunFrame_Server = nullptr;
	ORIG_StopRecording = nullptr;
	ORIG_Cbuf_Execute = nullptr;
	ORIG_VGui_Paint = nullptr;
	ORIG_StopRecording = nullptr;
	ORIG_SetSignonState = nullptr;
	ORIG_Stop = nullptr;
	pGameServer = nullptr;
	pM_bLoadgame = nullptr;
	shouldPreventNextUnpause = false;
	pIntervalPerTick = nullptr;
	pHost_Frametime = nullptr;
	pM_State = nullptr;
	pM_nSignonState = nullptr;
	pDemoplayer = nullptr;
	currentAutoRecordDemoNumber = 1;
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
	TRACE_ENTER();
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetPlaybackTick_Offset](demoplayer);
}

int EngineDLL::Demo_GetTotalTicks() const
{
	TRACE_ENTER();
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetTotalTicks_Offset](demoplayer);
}

bool EngineDLL::Demo_IsPlayingBack() const
{
	TRACE_ENTER();
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlayingBack_Offset](demoplayer);
}

bool EngineDLL::Demo_IsPlaybackPaused() const
{
	TRACE_ENTER();
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlaybackPaused_Offset](demoplayer);
}

// Basically a wrapper for the stop command hooked function to expose it to the rest of the code with a nicer name
void EngineDLL::Demo_StopRecording()
{
	HOOKED_Stop_Func();
}

bool EngineDLL::Demo_IsAutoRecordingAvailable() const
{
	return (ORIG_StopRecording && ORIG_SetSignonState);
}

bool __cdecl EngineDLL::HOOKED_SV_ActivateServer_Func()
{
	bool result = ORIG_SV_ActivateServer();

	DevMsg("Engine call: SV_ActivateServer() => %s;\n", (result ? "true" : "false"));

	if (ORIG_SetPaused && pM_bLoadgame && pGameServer)
	{
		if ((y_spt_pause.GetInt() == 2) && *pM_bLoadgame)
		{
			ORIG_SetPaused((void*)pGameServer, 0, true);
			DevMsg("Pausing...\n");

			shouldPreventNextUnpause = true;
		}
	}

	if (_y_spt_afterframes_reset_on_server_activate.GetBool())
		clientDLL.ResetAfterframesQueue();

	return result;
}

ConVar _y_spt_afterframes_await_legacy("_y_spt_afterframes_await_legacy",
                                       "0",
                                       FCVAR_TAS_RESET,
                                       "Set to 1 for backwards compatibility with old scripts.");

void __fastcall EngineDLL::HOOKED_FinishRestore_Func(void* thisptr, int edx)
{
	DevMsg("Engine call: FinishRestore();\n");

	if (ORIG_SetPaused && (y_spt_pause.GetInt() == 1))
	{
		ORIG_SetPaused(thisptr, 0, true);
		DevMsg("Pausing...\n");

		shouldPreventNextUnpause = true;
	}

	ORIG_FinishRestore(thisptr, edx);

	if (_y_spt_afterframes_await_legacy.GetBool())
		clientDLL.ResumeAfterframesQueue();
}

void __fastcall EngineDLL::HOOKED_SetPaused_Func(void* thisptr, int edx, bool paused)
{
	if (!paused && !_y_spt_afterframes_await_legacy.GetBool())
		clientDLL.ResumeAfterframesQueue();

	if (pM_bLoadgame)
	{
		DevMsg("Engine call: SetPaused( %s ); m_bLoadgame = %s\n",
		       (paused ? "true" : "false"),
		       (*pM_bLoadgame ? "true" : "false"));
	}
	else
	{
		DevMsg("Engine call: SetPaused( %s );\n", (paused ? "true" : "false"));
	}

	if (paused == false)
	{
		if (shouldPreventNextUnpause)
		{
			DevMsg("Unpause prevented.\n");
			shouldPreventNextUnpause = false;
			return;
		}
	}

	shouldPreventNextUnpause = false;
	return ORIG_SetPaused(thisptr, edx, paused);
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Func(float time)
{
	DevMsg("_Host_RunFrame( %.8f ); m_nSignonState = %d;", time, *pM_nSignonState);
	if (pM_State)
		DevMsg(" m_State = %d;", *pM_State);
	DevMsg("\n");

	ORIG__Host_RunFrame(time);

	DevMsg("_Host_RunFrame end.\n");
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Input_Func(float accumulated_extra_samples, int bFinalTick)
{
	DevMsg("_Host_RunFrame_Input( %.8f, %d ); m_nSignonState = %d;", time, bFinalTick, *pM_nSignonState);
	if (pM_State)
		DevMsg(" m_State = %d;", *pM_State);
	DevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG__Host_RunFrame_Input(accumulated_extra_samples, bFinalTick);

	DevMsg("_Host_RunFrame_Input end.\n");
}

void __cdecl EngineDLL::HOOKED__Host_RunFrame_Server_Func(int bFinalTick)
{
	DevMsg("_Host_RunFrame_Server( %d ); m_nSignonState = %d;", bFinalTick, *pM_nSignonState);
	if (pM_State)
		DevMsg(" m_State = %d;", *pM_State);
	DevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG__Host_RunFrame_Server(bFinalTick);

	DevMsg("_Host_RunFrame_Server end.\n");
}

void __cdecl EngineDLL::HOOKED_Cbuf_Execute_Func()
{
	DevMsg("Cbuf_Execute(); m_nSignonState = %d;", *pM_nSignonState);
	if (pM_State)
		DevMsg(" m_State = %d;", *pM_State);
	DevMsg(" host_frametime = %.8f\n", *pHost_Frametime);

	ORIG_Cbuf_Execute();

	DevMsg("Cbuf_Execute() end.\n");
}

void __fastcall EngineDLL::HOOKED_VGui_Paint_Func(void* thisptr, int edx, int mode)
{
#ifndef OE
	if (y_spt_hud_enable.GetBool() && mode == 2 && !clientDLL.renderingOverlay)
		vgui_matsurfaceDLL.DrawHUD((vrect_t*)clientDLL.screenRect);

	if (clientDLL.renderingOverlay)
		vgui_matsurfaceDLL.DrawCrosshair((vrect_t*)clientDLL.screenRect);

#endif
	ORIG_VGui_Paint(thisptr, edx, mode);
}

void __fastcall EngineDLL::HOOKED_StopRecording_Func(void* thisptr, int edx)
{
	// This hook will get called twice per loaded save (in most games/versions, at least, according to SAR people), once with m_bLoadgame being false and the next one being true
	if (!scripts::g_TASReader.IsExecutingScript())
	{
		ORIG_StopRecording(thisptr, edx);
		isAutoRecordingDemo = false;
		currentAutoRecordDemoNumber = 1;
		return;
	}

	bool* pM_bRecording = (bool*)((uint32_t)thisptr + m_bRecording_Offset);
	int* pM_nDemoNumber = (int*)((uint32_t)thisptr + m_nDemoNumber_Offset);

	// This will set m_nDemoNumber to 0 and m_bRecording to false
	ORIG_StopRecording(thisptr, edx);

	if (isAutoRecordingDemo)
	{
		*pM_nDemoNumber = currentAutoRecordDemoNumber;
		*pM_bRecording = true;
	}
	else
	{
		currentAutoRecordDemoNumber = 1;
	}
}

void __fastcall EngineDLL::HOOKED_SetSignonState_Func(void* thisptr, int edx, int state)
{
	// This hook only makes sense if StopRecording is also properly hooked
	if (ORIG_StopRecording && scripts::g_TASReader.IsExecutingScript())
	{
		bool* pM_bRecording = (bool*)((uint32_t)thisptr + m_bRecording_Offset);
		int* pM_nDemoNumber = (int*)((uint32_t)thisptr + m_nDemoNumber_Offset);

		// SIGNONSTATE_SPAWN (5): ready to receive entity packets
		// SIGNONSTATE_FULL may be called twice on a load depending on the game and on specific situations. Using SIGNONSTATE_SPAWN for demo number increase instead
		if (state == 5 && isAutoRecordingDemo)
		{
			currentAutoRecordDemoNumber++;
		}
		// SIGNONSTATE_FULL (6): we are fully connected, first non-delta packet received
		// Starting a demo recording will call this function with SIGNONSTATE_FULL
		// After a load, the engine's demo recorder will only start recording when it reaches this state, so this is a good time to set the flag if needed
		else if (state == 6)
		{
			// Changing sessions may put the recording flag down
			// Start recording again
			if (isAutoRecordingDemo)
			{
				*pM_bRecording = true;
			}

			// We may have just started the first recording, so set our autorecording flag and take control over the demo number
			if (*pM_bRecording)
			{
				isAutoRecordingDemo = true;
				*pM_nDemoNumber = currentAutoRecordDemoNumber;
			}
		}
	}
	ORIG_SetSignonState(thisptr, edx, state);
}

void __cdecl EngineDLL::HOOKED_Stop_Func()
{
	isAutoRecordingDemo = false;
	ORIG_Stop();
}