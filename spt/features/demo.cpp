#include "stdafx.hpp"
#include "demo.hpp"
#include "generic.hpp"
#include "signals.hpp"
#include "..\cvars.hpp"
#include "..\feature.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\utils\game_detection.hpp"
#include "visualizations\imgui\imgui_interface.hpp"

DemoStuff spt_demostuff;

ConVar y_spt_pause_demo_on_tick(
    "y_spt_pause_demo_on_tick",
    "0",
    0,
    "Invokes the demo_pause command on the specified demo tick.\n"
    "Zero means do nothing.\n"
    "x > 0 means pause the demo at tick number x.\n"
    "x < 0 means pause the demo at <demo length> + x, so for example -1 will pause the demo at the last tick.\n\n"
    "Demos ending with changelevels report incorrect length; you can obtain the correct demo length using listdemo and then set this CVar to <demo length> - 1 manually.");
ConVar y_spt_demo_block_cmd("y_spt_demo_block_cmd", "0", 0, "Stop all commands from being run by demo playback.");

namespace patterns
{
	PATTERNS(StopRecording,
	         "5135",
	         "56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 74 ?? 8B 86 ?? ?? ?? ?? 85 C0 57",
	         "1910503",
	         "55 8B EC 51 56 8B F1 8B 06 8B 50 ?? FF D2 84 C0 0F ?? ?? ?? ?? ?? 8B 86 ?? ?? ?? ?? 57");
	PATTERNS(Stop,
	         "5135",
	         "83 3D ?? ?? ?? ?? 01 75 ?? 8B 0D ?? ?? ?? ?? 8B 01 8B ?? ?? FF D2 84 C0 75 ?? C7",
	         "1910503",
	         "83 3D ?? ?? ?? ?? 01 75 ?? 8B 0D ?? ?? ?? ?? 8B 01 8B ?? ?? FF D2 84 C0 75 ?? 68");
	PATTERNS(
	    Record,
	    "2707",
	    "81 EC 08 01 00 00 E8 ?? ?? ?? ?? 83 F8 02 74 1E E8 ?? ?? ?? ?? 83 F8 03 74 14 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04 81 C4 08 01 00 00 C3 53 32 DB 88 5C 24 04 E8",
	    "5135",
	    "81 EC 08 01 00 00 83 ?? ?? ?? ?? ?? ?? 75 15 68 ?? ?? ?? ?? FF ?? ?? ?? ?? ?? 83 C4 04 81 C4 08 01 00 00 C3",
	    "2229355",
	    "55 8B EC 81 EC 08 01 00 00 83 3D ?? ?? ?? ?? 00 75 ?? 68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 83 C4 04 8B E5 5D C3 56 8B 75 ?? 8B 06");
	PATTERNS(
	    CDemoPlayer__StartPlayback,
	    "5135",
	    "53 55 56 57 8B F1 E8 ?? ?? ?? ?? 8B 3D ?? ?? ?? ??",
	    "7197370",
	    "55 8B EC 53 56 57 8B F9 C6 87 ?? ?? ?? ?? 01 E8 ?? ?? ?? ?? 8B 35 ?? ?? ?? ?? 68 ?? ?? ?? ?? 6A 00 C7 05 ?? ?? ?? ?? FF FF FF FF");
	PATTERNS(CDemoFile__ReadConsoleCommand, "5135", "68 00 04 00 00 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? B8 ?? ?? ?? ??");
} // namespace patterns

void DemoStuff::InitHooks()
{
	HOOK_FUNCTION(engine, StopRecording);
	HOOK_FUNCTION(engine, CDemoPlayer__StartPlayback);
	HOOK_FUNCTION(engine, Stop);
	HOOK_FUNCTION(engine, CDemoFile__ReadConsoleCommand);
	FIND_PATTERN(engine, Record);
}

bool DemoStuff::ShouldLoadFeature()
{
	return true;
}

void DemoStuff::PreHook()
{
	// CDemoRecorder::StopRecording
	if (ORIG_StopRecording)
	{
		int index = GetPatternIndex((void**)&ORIG_StopRecording);
		if (index == 0) // 5135
		{
			m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 65);
			m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 72);
		}
		else if (index == 1) // 1910503
		{
			m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 70);
			m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 77);
		}
		else
		{
			Warning("StopRecording had no matching offset clause.\n");
		}

		DevMsg("Found CDemoRecorder offsets m_nDemoNumber %d, m_bRecording %d.\n",
		       m_nDemoNumber_Offset,
		       m_bRecording_Offset);
	}

	if (ORIG_Record)
	{
		int index = GetPatternIndex((void**)&ORIG_Record);
		if (index == 0) // 2707
		{
			pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 132);

			// vftable offsets
			GetPlaybackTick_Offset = 2;
			GetTotalTicks_Offset = 3;
			IsPlaybackPaused_Offset = 5;
			IsPlayingBack_Offset = 6;
		}
		else if (index == 1) // 5135
		{
			pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 0xA2);
			// vftable offsets
			if (utils::GetBuildNumber() <= 3420)
			{
				// 3420 offsets
				GetPlaybackTick_Offset = 2;
				GetTotalTicks_Offset = 3;
				IsPlayingBack_Offset = 5;
				IsPlaybackPaused_Offset = 6;
			}
			else
			{
				// 5135 offsets
				GetPlaybackTick_Offset = 3;
				GetTotalTicks_Offset = 4;
				IsPlayingBack_Offset = 6;
				IsPlaybackPaused_Offset = 7;
			}
		}
		else if (index == 2) // 2229355
		{
			// new steampipe hl2
			if (utils::GetBuildNumber() >= 2229355 || utils::GetBuildNumber() == 0)
			{
				// 2229355 offset
				pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 0x9C);
			}
			else
			{
				// 1910503 offset
				pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 0x96);
			}
			// vftable offsets
			GetPlaybackTick_Offset = 3;
			GetTotalTicks_Offset = 4;
			IsPlayingBack_Offset = 6;
			IsPlaybackPaused_Offset = 7;
		}
		else
			Warning(
			    "Record pattern had no matching clause for catching the demoplayer. spt_pause_demo_on_tick unavailable.\n");

		DevMsg("Found demoplayer at %p, record is at %p.\n", pDemoplayer, (void*)ORIG_Record);

		if (ORIG_CDemoPlayer__StartPlayback)
			DemoStartPlaybackSignal.Works = true;
	}
}

void DemoStuff::UnloadFeature() {}

void DemoStuff::Demo_StopRecording()
{
	spt_demostuff.HOOKED_Stop();
}

int DemoStuff::Demo_GetPlaybackTick() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetPlaybackTick_Offset](demoplayer);
}

int DemoStuff::Demo_GetTotalTicks() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetTotalTicks_Offset](demoplayer);
}

bool DemoStuff::Demo_IsPlayingBack() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlayingBack_Offset](demoplayer);
}

bool DemoStuff::Demo_IsPlaybackPaused() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlaybackPaused_Offset](demoplayer);
}

bool DemoStuff::Demo_IsAutoRecordingAvailable() const
{
	return (ORIG_StopRecording && SetSignonStateSignal.Works);
}

bool DemoStuff::Demo_IsDemoPlayerAvailable() const
{
	return !!pDemoplayer;
}

void DemoStuff::StartAutorecord()
{
	spt_demostuff.isAutoRecordingDemo = true;
	spt_demostuff.currentAutoRecordDemoNumber = 0;
}

void DemoStuff::StopAutorecord()
{
	spt_demostuff.isAutoRecordingDemo = false;
	spt_demostuff.currentAutoRecordDemoNumber = 1;
}

IMPL_HOOK_THISCALL(DemoStuff, void, StopRecording, void*)
{ // This hook will get called twice per loaded save (in most games/versions, at least, according to SAR people), once with m_bLoadgame being false and the next one being true
	if (!spt_demostuff.isAutoRecordingDemo)
	{
		spt_demostuff.ORIG_StopRecording(thisptr);
		spt_demostuff.StopAutorecord();
		return;
	}

	bool* pM_bRecording = (bool*)((uint32_t)thisptr + spt_demostuff.m_bRecording_Offset);
	int* pM_nDemoNumber = (int*)((uint32_t)thisptr + spt_demostuff.m_nDemoNumber_Offset);

	// This will set m_nDemoNumber to 0 and m_bRecording to false
	spt_demostuff.ORIG_StopRecording(thisptr);

	if (spt_demostuff.isAutoRecordingDemo)
	{
		*pM_nDemoNumber = spt_demostuff.currentAutoRecordDemoNumber;
		*pM_bRecording = true;
	}
}

void DemoStuff::OnSignonStateSignal(void* thisptr, int state)
{
	// This hook only makes sense if StopRecording is also properly hooked
	if (spt_demostuff.ORIG_StopRecording && spt_demostuff.isAutoRecordingDemo)
	{
		bool* pM_bRecording = (bool*)((uint32_t)thisptr + spt_demostuff.m_bRecording_Offset);
		int* pM_nDemoNumber = (int*)((uint32_t)thisptr + spt_demostuff.m_nDemoNumber_Offset);

		// SIGNONSTATE_SPAWN (5): ready to receive entity packets
		// SIGNONSTATE_FULL may be called twice on a load depending on the game and on specific situations. Using SIGNONSTATE_SPAWN for demo number increase instead
		if (state == 5)
		{
			spt_demostuff.currentAutoRecordDemoNumber++;
		}
		// SIGNONSTATE_FULL (6): we are fully connected, first non-delta packet received
		// Starting a demo recording will call this function with SIGNONSTATE_FULL
		// After a load, the engine's demo recorder will only start recording when it reaches this state, so this is a good time to set the flag if needed
		else if (state == 6)
		{
			// We may have just started the first recording, so set our autorecording flag and take control over the demo number
			if (*pM_bRecording)
			{
				// When we start recording, we put demo number at 0, so that if we start recording before a map is loaded the first demo number is 1
				// if we have demo number at 0 here, it means we started recording when already in map (we did not receive SIGNONSTATE_SPAWN in between)
				if (spt_demostuff.currentAutoRecordDemoNumber == 0)
				{
					spt_demostuff.currentAutoRecordDemoNumber = 1;
				}

				*pM_nDemoNumber = spt_demostuff.currentAutoRecordDemoNumber;
			}
		}
	}
}

IMPL_HOOK_THISCALL(DemoStuff, bool, CDemoPlayer__StartPlayback, void*, const char* filename, bool as_time_demo)
{
	bool result = spt_demostuff.ORIG_CDemoPlayer__StartPlayback(thisptr, filename, as_time_demo);
	DemoStartPlaybackSignal();
	return result;
}

IMPL_HOOK_THISCALL(DemoStuff, const char*, CDemoFile__ReadConsoleCommand, void*)
{
	const char* cmd = spt_demostuff.ORIG_CDemoFile__ReadConsoleCommand(thisptr);
	if (y_spt_demo_block_cmd.GetBool())
		return "";
	return cmd;
}

IMPL_HOOK_CDECL(DemoStuff, void, Stop)
{
	if (spt_demostuff.ORIG_Stop)
	{
		spt_demostuff.isAutoRecordingDemo = false;
		spt_demostuff.currentAutoRecordDemoNumber = 1;
		spt_demostuff.ORIG_Stop();
	}
}

void DemoStuff::OnFrame()
{
	if (Demo_IsPlayingBack() && !Demo_IsPlaybackPaused())
	{
		auto tick = y_spt_pause_demo_on_tick.GetInt();
		if (tick != 0)
		{
			if (tick < 0)
				tick += Demo_GetTotalTicks();

			if (tick == Demo_GetPlaybackTick())
				EngineConCmd("demo_pause");
		}
	}
}

CON_COMMAND(y_spt_record, "Starts autorecording a demo.")
{
	if (args.ArgC() == 1)
	{
		Msg("Usage: spt_record <filepath> [incremental]\n");
		return;
	}

#ifdef OE
	_record->Dispatch();
#else
	_record->Dispatch(args);
#endif
	spt_demostuff.StartAutorecord();
}

CON_COMMAND(y_spt_record_stop, "Stops recording a demo.")
{
#ifdef OE
	_stop->Dispatch();
#else
	_stop->Dispatch(args);
#endif
	if (!spt_demostuff.ORIG_Stop)
		spt_demostuff.StopAutorecord();
}

void DemoStuff::LoadFeature()
{
	if (ORIG_Record && pDemoplayer && FrameSignal.Works)
	{
		InitConcommandBase(y_spt_pause_demo_on_tick);
		FrameSignal.Connect(this, &DemoStuff::OnFrame);
	}

	if (ORIG_StopRecording && SetSignonStateSignal.Works)
	{
		InitCommand(y_spt_record);
		InitCommand(y_spt_record_stop);
		SetSignonStateSignal.Connect(this, &DemoStuff::OnSignonStateSignal);
	}

	if (ORIG_CDemoFile__ReadConsoleCommand)
	{
		InitConcommandBase(y_spt_demo_block_cmd);
	}

	SptImGuiGroup::QoL_Demo.RegisterUserCallback(
	    []()
	    {
		    SptImGui::CvarCheckbox(y_spt_demo_block_cmd, "##checkbox");
		    SptImGui::CvarInputTextInteger(y_spt_pause_demo_on_tick, "pause demo on tick", "enter tick value");
	    });
}
