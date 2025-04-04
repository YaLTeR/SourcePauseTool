#include "stdafx.hpp"

#include <chrono>

#include "..\feature.hpp"
#include "generic.hpp"
#include "signals.hpp"
#include "tickrate.hpp"
#include "visualizations\imgui\imgui_interface.hpp"

#include "convar.h"

using chrono_timer = std::chrono::steady_clock;
using chrono_precision = std::chrono::milliseconds;

class Timer : public FeatureWrapper<Timer>
{
public:
	void StartTimer()
	{
		if (timerRunning)
			return;
		timerRunning = true;
		lastResumeTimeRta = chrono_timer::now();
	}
	void StopTimer()
	{
		if (!timerRunning)
			return;
		timerRunning = false;
		partialAccumulatedTimeRta +=
		    std::chrono::duration_cast<chrono_precision>(chrono_timer::now() - lastResumeTimeRta);
	}
	void ResetTimer()
	{
		ticksPassed = 0;
		partialAccumulatedTimeRta = chrono_precision::zero();
		timerRunning = false;
	}
	unsigned int GetTicksPassed() const
	{
		return ticksPassed;
	}
	double GetTimePassedRta() const
	{
		auto extra = timerRunning
		                 ? std::chrono::duration_cast<chrono_precision>(chrono_timer::now() - lastResumeTimeRta)
		                 : chrono_precision::zero();
		constexpr double ratio = (double)chrono_precision::period::num / (double)chrono_precision::period::den;
		return (partialAccumulatedTimeRta + extra).count() * ratio;
	}
	bool Running() const
	{
		return timerRunning;
	}

protected:
	virtual void LoadFeature() override;

private:
	void Tick(bool simulating);
	int ticksPassed = 0;
	chrono_precision partialAccumulatedTimeRta;
	std::chrono::time_point<chrono_timer> lastResumeTimeRta;
	bool timerRunning = false;
};

static Timer spt_timer;

void Timer::Tick(bool simulating)
{
	if (simulating && timerRunning)
		++ticksPassed;
}

CON_COMMAND(y_spt_timer_start, "Starts the SPT timer.")
{
	spt_timer.StartTimer();
}

CON_COMMAND(y_spt_timer_stop, "Stops the SPT timer and prints the current time.")
{
	spt_timer.StopTimer();
	Warning("Elapsed game time: %u ticks (%.3fs)\nElapsed real time: %.3fs\n",
	        spt_timer.GetTicksPassed(),
	        spt_timer.GetTicksPassed() * spt_tickrate.GetTickrate(),
	        spt_timer.GetTimePassedRta());
}

CON_COMMAND(y_spt_timer_reset, "Stops and resets the SPT timer.")
{
	spt_timer.ResetTimer();
}

CON_COMMAND(y_spt_timer_print, "Prints the current time of the SPT timer.")
{
	Warning("Current time (in ticks): %u\n", spt_timer.GetTicksPassed());
}

void Timer::LoadFeature()
{
	ticksPassed = 0;
	timerRunning = false;
	if (TickSignal.Works)
	{
		TickSignal.Connect(this, &Timer::Tick);
		InitCommand(y_spt_timer_start);
		InitCommand(y_spt_timer_stop);
		InitCommand(y_spt_timer_reset);
		InitCommand(y_spt_timer_print);

		SptImGuiGroup::QoL_Timer.RegisterUserCallback(
		    []()
		    {
			    unsigned int elapsedTicks = spt_timer.GetTicksPassed();
			    double elapsedSecsRta = spt_timer.GetTimePassedRta();

			    ImGui::BeginDisabled(spt_timer.Running());
			    if (ImGui::Button(elapsedSecsRta > 0 ? "resume###start" : "start###start"))
				    spt_timer.StartTimer();
			    ImGui::EndDisabled();
			    ImGui::BeginDisabled(!spt_timer.Running());
			    ImGui::SameLine();
			    if (ImGui::Button("pause"))
				    spt_timer.StopTimer();
			    ImGui::EndDisabled();
			    ImGui::SameLine();
			    if (ImGui::Button("reset"))
				    spt_timer.ResetTimer();
			    ImGui::SameLine();
			    SptImGui::HelpMarker(
			        "Can be triggered with spt_timer_start, spt_timer_stop, spt_timer_reset");

			    ImGui::Text("Elapsed game time: %u ticks (%.3fs)",
			                elapsedTicks,
			                elapsedTicks * spt_tickrate.GetTickrate());
			    ImGui::Text("Elapsed real time: %.3fs", elapsedSecsRta);
		    });
	}
}
