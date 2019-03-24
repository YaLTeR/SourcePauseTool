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
	}

	void ConnectSignals()
	{
		clientDLL.AfterFramesSignal.Connect(&scripts::g_TASReader, &scripts::SourceTASReader::OnAfterFrames);
		clientDLL.AfterFramesSignal.Connect(&scripts::g_Tester, &scripts::Tester::OnAfterFrames);
		clientDLL.AfterFramesSignal.Connect(&PauseOnDemoTick);

		clientDLL.TickSignal.Connect(&scripts::g_Tester, &scripts::Tester::DataIteration);
#ifndef OE
		clientDLL.TickSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::NewTick);
		clientDLL.OngroundSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::OnGround);

		serverDLL.JumpSignal.Connect(&vgui_matsurfaceDLL, &VGui_MatSurfaceDLL::Jump);
#endif
	}
}