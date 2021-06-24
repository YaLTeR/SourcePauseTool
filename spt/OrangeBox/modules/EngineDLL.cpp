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
#include "vguimatsurfaceDLL.hpp"

using std::size_t;
using std::uintptr_t;

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

bool SPT_command_listing_evaluate(const char* candidate)
{
	if (candidate == "")
		return false;

	const char* const values[] = {"spt_", "tas_", "sv_cheats"};
	for each (const char* check in values)
	{
		if (((std::string)candidate).find(check) != std::string::npos)
				return true;
	}
	return false;
}

void __fastcall EngineDLL::HOOKED_Record_Func(void* thisptr)
{
	TRACE_ENTER();

	engineDLL.ORIG_Record_Func(thisptr);

	if (y_spt_demo_write_timestamps.GetBool())
	{
		char time[100]{};
		sprintf(time, "! START RECORDING TIMESTAMP %.20f", Plat_FloatTime());
		engineDLL.WriteConsoleCommand(time, 0);
	}

	if (!y_spt_demo_legit_check.GetBool())
		return;

	ConMsg("SPT COMMAND LISTING !!! \n");

	engineDLL.WriteConsoleCommand("SPT COMMAND LISTING", 0);
	char text[500]{};

	auto icvar = GetCvarInterface();
	ConCommandBase* cmd = icvar->GetCommands();

	// Loops through the console variables and commands
	while (cmd != NULL)
	{
		const char* name = cmd->GetName();
		if (!cmd->IsCommand() && name != NULL && SPT_command_listing_evaluate(name))
		{
			auto convar = icvar->FindVar(name);
			// convar found
			if (convar != NULL)
			{
				sprintf(text, "\0 VAR %s      default [%s]      current s[%s] i[%d] f[%.8f] \n",
				    name,
					convar->GetDefault(), 
					convar->GetString(),
					convar->GetInt(),
					convar->GetFloat());

				ConMsg(text);

				engineDLL.WriteConsoleCommand(text, -9000);
			}
			else
				throw std::exception("Unable to find listed console variable!");
		}
		cmd = cmd->GetNext();
	}
}

bool EngineDLL::IsDemoRecording() 
{
	if (engineDLL.pDemorecorder != nullptr)
	{
		return *(bool*)((uint)pDemorecorder + 0x646);
	}

	return false;
}

void __fastcall EngineDLL::HOOKED_StopRecording(void* thisptr) 
{
	TRACE_ENTER();

	if (y_spt_demo_write_timestamps.GetBool())
	{
		char time[100]{};
		sprintf(time, "! STOP RECORDING TIMESTAMP %.20f", Plat_FloatTime());
		engineDLL.WriteConsoleCommand(time, engineDLL.GetDemoTickCount());
	}

	return engineDLL.ORIG_StopRecording(thisptr);
}

void __fastcall EngineDLL::HOOKED_WriteConsoleCommand(void* thisptr, int edx, char* cmdstring, int tick)
{
	TRACE_ENTER();
	return engineDLL.ORIG_WriteConsoleCommand(thisptr, edx, cmdstring, tick);
}

void __fastcall EngineDLL::WriteConsoleCommand(char* cmdstring, int tick)
{
	if (pDemorecorder == nullptr)
		return;
	engineDLL.ORIG_WriteConsoleCommand((void*)((uint)pDemorecorder + 0x4),	// m_demofile
										*(uint*)pDemorecorder,				// demorecorder vftable ptr
										cmdstring,
										tick);
}

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::engine::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[engine dll] Found " #future_name " at %p (using the %s pattern).\n", \
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
			DevWarning("[engine dll] Could not find " #future_name ".\n"); \
		} \
	}

#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[engine dll] Found " #future_name " at %p (using the %s pattern).\n", \
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
			DevWarning("[engine dll] Could not find " #future_name ".\n"); \
		} \
	}

int lastDemoTick = 0;
int lastDemoTickBase = 0;
int accumDemoTime = 0;

static void ResetAccumDemoTime(const CCommand& args)
{
	accumDemoTime = 0;
}

static ConCommand y_spt_hud_demo_accumtime_reset("y_spt_hud_demo_accumtime_reset", ResetAccumDemoTime);

int EngineDLL::GetDemoTickCount()
{
	if (ORIG_curtime && pDemorecorder)
	{
		int tick = 0;
		int curtime = *(int*)(ORIG_curtime + 12);
		if (IsDemoRecording())
		{
			int tickbase = *(int*)((int)pDemorecorder + 0x53c);
			// wait until the tickbase is set
			if (tickbase < lastDemoTickBase)
				return 0;

			tick = curtime - tickbase;
			if (y_spt_hud_demo_accumtime.GetBool())
				accumDemoTime += (tick - lastDemoTick) > 0 ? tick - lastDemoTick : 0;
		}
		else
			lastDemoTickBase = curtime; 

		lastDemoTick = tick;
		return lastDemoTick;
	}
	return -1;
}

float EngineDLL::GetAccumDemoTime() 
{
	if (ORIG_curtime && pDemorecorder)
	{
		return accumDemoTime * GetTickrate();
	}
	return -1;
}

float EngineDLL::GetDemoTime()
{
	if (ORIG_curtime && pDemorecorder)
	{
		return GetDemoTickCount() * GetTickrate();
	}
	return -1;
}

const char* EngineDLL::GetDemoName()
{
	if (pDemorecorder)
	{
		return (const char*)((int)pDemorecorder + 0x540);
	}
	return "";
}

server_state_t EngineDLL::GetServerState()
{
	if (ORIG_ServerState)
		return *(server_state_t*)ORIG_ServerState;
	return ss_active;
}


int lastCurTick = 0;
double lastRealTime = 0;
float curRunTime = 0;

static void ResetRunTime()
{
	curRunTime = 0;
}

static ConCommand y_spt_hud_timer_reset("y_spt_hud_timer_reset", ResetRunTime);

float EngineDLL::GetCurrentRunTime() 
{
	if (!ORIG_curtime || !ORIG_ServerState)
		return 0;

	float delta =
	    (GetServerState() == ss_paused)  
		? Plat_FloatTime() - lastRealTime 
		: (*(int*)(ORIG_curtime + 12) - lastCurTick) * GetTickrate();

	if (lastCurTick == 0 || lastRealTime == 0)
		delta = 0;

	lastCurTick = *(int*)(ORIG_curtime + 12);
	lastRealTime = Plat_FloatTime();

	if (delta > 0)
		curRunTime += delta;

	return curRunTime;
}

void EngineDLL::Hook(const std::wstring& moduleName,
                     void* moduleHandle,
                     void* moduleBase,
                     size_t moduleLength,
                     bool needToIntercept)
{
	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	patternContainer.Init(moduleName);

	uintptr_t ORIG_SpawnPlayer = NULL, ORIG_MiddleOfSV_InitGameDLL = NULL, ORIG_Record = NULL;

	DEF_FUTURE(SV_ActivateServer);
	DEF_FUTURE(FinishRestore);
	DEF_FUTURE(Record);
	DEF_FUTURE(Record_Func);
	DEF_FUTURE(SetPaused);
	DEF_FUTURE(MiddleOfSV_InitGameDLL);
	DEF_FUTURE(VGui_Paint);
	DEF_FUTURE(SpawnPlayer);
	DEF_FUTURE(CEngineTrace__PointOutsideWorld);
	DEF_FUTURE(_Host_RunFrame);
	DEF_FUTURE(Host_AccumulateTime);
	DEF_FUTURE(WriteConsoleCommand);
	DEF_FUTURE(curtime);
	DEF_FUTURE(StopRecording);
	DEF_FUTURE(ServerState);

	GET_HOOKEDFUTURE(SV_ActivateServer);
	GET_HOOKEDFUTURE(FinishRestore);
	GET_HOOKEDFUTURE(SetPaused);
	GET_FUTURE(MiddleOfSV_InitGameDLL);
	GET_HOOKEDFUTURE(VGui_Paint);
	GET_FUTURE(SpawnPlayer);
	GET_FUTURE(CEngineTrace__PointOutsideWorld);
	GET_FUTURE(_Host_RunFrame);
	GET_HOOKEDFUTURE(Host_AccumulateTime);
	GET_HOOKEDFUTURE(Record_Func);
	GET_HOOKEDFUTURE(WriteConsoleCommand);
	GET_FUTURE(curtime);
	GET_HOOKEDFUTURE(StopRecording);
	GET_FUTURE(ServerState);

	if (ORIG_curtime)
	{
		int add = 0;
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_curtime);
		switch (ptnNumber)
		{
		case 0:
			add = 22;
			break;

		case 1:
			add = 26;
			break; 

		case 2:
			add = 27;
			break; 
		}

		ORIG_curtime = *(uintptr_t*)(ORIG_curtime + add);
		DevMsg("cur time found at %p\n", ORIG_curtime);
	}

	if (ORIG_ServerState)
	{
		ORIG_ServerState = *(uintptr_t*)(ORIG_ServerState + 22);
		DevMsg("server state found at %p\n", ORIG_ServerState);
	}

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
	if (!ORIG_FinishRestore)
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
			    "Record pattern had no matching clause for catching the demoplayer. y_spt_pause_demo_on_tick unavailable.");

		DevMsg("Found demoplayer at %p and demorecorder at %p, record is at %p.\n",
		       pDemoplayer,
		       pDemorecorder,
		       ORIG_Record);
	}
	else
	{
		Warning("y_spt_pause_demo_on_tick is not available.\n");
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
	ORIG_WriteConsoleCommand = nullptr;
	ORIG_StopRecording = nullptr;
	curtime = 0x0;
	ORIG_curtime = 0x0;
	ORIG_FinishRestore = nullptr;
	ORIG_SetPaused = nullptr;
	ORIG_Record_Func = nullptr;
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
	if (mode == 2 && !clientDLL.renderingOverlay)
	{
		vgui_matsurfaceDLL.DrawHUD((vrect_t*)clientDLL.screenRect);
	}

	if (clientDLL.renderingOverlay)
		vgui_matsurfaceDLL.DrawCrosshair((vrect_t*)clientDLL.screenRect);

#endif

	ORIG_VGui_Paint(thisptr, edx, mode);
}
