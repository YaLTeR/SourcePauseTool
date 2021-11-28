#include "stdafx.h"
#include "demo.hpp"
#include "generic.hpp"
#include "signals.hpp"
#include "..\cvars.hpp"
#include "..\feature.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "..\sptlib-wrapper.hpp"
#include "dbg.h"

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
	return (ORIG_StopRecording && ORIG_SetSignonState);
}

bool DemoStuff::ShouldLoadFeature()
{
	return true;
}

void DemoStuff::PreHook()
{
	// Move 1 byte since the pattern starts a byte before the function
	if (ORIG_SetSignonState)
		ORIG_SetSignonState = (_SetSignonState)((uint32_t)ORIG_SetSignonState + 1);

	// CDemoRecorder::StopRecording
	if (ORIG_StopRecording)
	{
		int index = GetPatternIndex((void**)&ORIG_StopRecording);
		if (index == 0)
		{
			m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 65);
			m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 72);
		}
		else if (index == 1)
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
		if (index == 0)
		{
			pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 132);

			// vftable offsets
			GetPlaybackTick_Offset = 2;
			GetTotalTicks_Offset = 3;
			IsPlaybackPaused_Offset = 5;
			IsPlayingBack_Offset = 6;
		}
		else if (index == 1)
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

		DevMsg("Found demoplayer at %p, record is at %p.\n", pDemoplayer, ORIG_Record);
	}
}

void DemoStuff::InitHooks()
{
	HOOK_FUNCTION(engine, StopRecording);
	HOOK_FUNCTION(engine, SetSignonState);
	HOOK_FUNCTION(engine, Stop);
}

void DemoStuff::LoadFeature()
{
	currentAutoRecordDemoNumber = 1;
	isAutoRecordingDemo = false;

	if (ORIG_Record)
	{
		TickSignal.Connect(this, &DemoStuff::OnTick);
	}

	if (!ORIG_StopRecording || !ORIG_SetSignonState)
	{
		Warning(
		    "TAS demo recording may overwrite demos if level transitions and saveloads are present in the same script.\n");
	}
	else if (!ORIG_Stop)
	{
		Warning("Manually stopping a TAS demo recording won't stop autorecording.\n");
	}
}

void DemoStuff::UnloadFeature() {}

void __fastcall DemoStuff::HOOKED_StopRecording(void* thisptr, int edx)
{ // This hook will get called twice per loaded save (in most games/versions, at least, according to SAR people), once with m_bLoadgame being false and the next one being true
	if (!scripts::g_TASReader.IsExecutingScript())
	{
		spt_demostuff.ORIG_StopRecording(thisptr, edx);
		spt_demostuff.isAutoRecordingDemo = false;
		spt_demostuff.currentAutoRecordDemoNumber = 1;
		return;
	}

	bool* pM_bRecording = (bool*)((uint32_t)thisptr + spt_demostuff.m_bRecording_Offset);
	int* pM_nDemoNumber = (int*)((uint32_t)thisptr + spt_demostuff.m_nDemoNumber_Offset);

	// This will set m_nDemoNumber to 0 and m_bRecording to false
	spt_demostuff.ORIG_StopRecording(thisptr, edx);

	if (spt_demostuff.isAutoRecordingDemo)
	{
		*pM_nDemoNumber = spt_demostuff.currentAutoRecordDemoNumber;
		*pM_bRecording = true;
	}
	else
	{
		spt_demostuff.currentAutoRecordDemoNumber = 1;
	}
}

void __fastcall DemoStuff::HOOKED_SetSignonState(void* thisptr, int edx, int state)
{
	// This hook only makes sense if StopRecording is also properly hooked
	if (spt_demostuff.ORIG_StopRecording && scripts::g_TASReader.IsExecutingScript())
	{
		bool* pM_bRecording = (bool*)((uint32_t)thisptr + spt_demostuff.m_bRecording_Offset);
		int* pM_nDemoNumber = (int*)((uint32_t)thisptr + spt_demostuff.m_nDemoNumber_Offset);

		// SIGNONSTATE_SPAWN (5): ready to receive entity packets
		// SIGNONSTATE_FULL may be called twice on a load depending on the game and on specific situations. Using SIGNONSTATE_SPAWN for demo number increase instead
		if (state == 5 && spt_demostuff.isAutoRecordingDemo)
		{
			spt_demostuff.currentAutoRecordDemoNumber++;
		}
		// SIGNONSTATE_FULL (6): we are fully connected, first non-delta packet received
		// Starting a demo recording will call this function with SIGNONSTATE_FULL
		// After a load, the engine's demo recorder will only start recording when it reaches this state, so this is a good time to set the flag if needed
		else if (state == 6)
		{
			// Changing sessions may put the recording flag down
			// Start recording again
			if (spt_demostuff.isAutoRecordingDemo)
			{
				*pM_bRecording = true;
			}

			// We may have just started the first recording, so set our autorecording flag and take control over the demo number
			if (*pM_bRecording)
			{
				spt_demostuff.isAutoRecordingDemo = true;
				*pM_nDemoNumber = spt_demostuff.currentAutoRecordDemoNumber;
			}
		}
	}
	spt_demostuff.ORIG_SetSignonState(thisptr, edx, state);
}

void __cdecl DemoStuff::HOOKED_Stop()
{
	spt_demostuff.isAutoRecordingDemo = false;
	if (spt_demostuff.ORIG_Stop)
		spt_demostuff.ORIG_Stop();
}

void DemoStuff::OnTick()
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
