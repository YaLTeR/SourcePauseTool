#include "stdafx.h"
#include "modules.hpp"
#include "modules\EngineDLL.hpp"
#include "cvars.hpp"
#include "..\sptlib-wrapper.hpp"
#include "scripts\srctas_reader.hpp"
#include "scripts\tests\test.hpp"

namespace modulehooks
{
	void PauseOnDemoTick()
	{
#ifdef OE
		if (engineDLL.Demo_IsPlayingBack() && !engineDLL.Demo_IsPlaybackPaused())
		{
			auto tick = y_spt_pause_demo_on_tick.GetInt();
			if (tick != 0)
			{
				if (tick < 0)
					tick += engineDLL.Demo_GetTotalTicks();

				if (tick == engineDLL.Demo_GetPlaybackTick())
					EngineConCmd("demo_pause");
			}
		}
#endif
	}

	float jumpTime = 0;

	float GetJumpTime()
	{
		return jumpTime;
	}

	void TimeJump()
	{
		jumpTime = 510.0f;
	}

	void JumpTickElapsed()
	{
		jumpTime -= 1000.0f * engineDLL.GetTickrate();
	}

	void ConnectSignals()
	{
		clientDLL.AfterFramesSignal.Connect(&scripts::g_TASReader, &scripts::SourceTASReader::OnAfterFrames);
		clientDLL.AfterFramesSignal.Connect(&scripts::g_Tester, &scripts::Tester::OnAfterFrames);
		clientDLL.AfterFramesSignal.Connect(&PauseOnDemoTick);

		clientDLL.TickSignal.Connect(&scripts::g_Tester, &scripts::Tester::DataIteration);
		clientDLL.TickSignal.Connect(&JumpTickElapsed);
		serverDLL.JumpSignal.Connect(&TimeJump);

#ifndef OE
		clientDLL.TickSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::NewTick);
		clientDLL.OngroundSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::OnGround);

		serverDLL.JumpSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::Jump);
#endif
	}
}