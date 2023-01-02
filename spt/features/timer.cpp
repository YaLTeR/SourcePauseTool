#include "stdafx.hpp"
#include "..\feature.hpp"
#include "generic.hpp"
#include "signals.hpp"

#include "convar.h"

class Timer : public FeatureWrapper<Timer>
{
public:
	void StartTimer()
	{
		timerRunning = true;
	}
	void StopTimer()
	{
		timerRunning = false;
	}
	void ResetTimer()
	{
		ticksPassed = 0;
		timerRunning = false;
	}
	unsigned int GetTicksPassed() const
	{
		return ticksPassed;
	}

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void Tick();
	int ticksPassed = 0;
	bool timerRunning = false;
};

static Timer spt_timer;

bool Timer::ShouldLoadFeature()
{
	return true;
}

void Timer::InitHooks() {}

void Timer::UnloadFeature() {}

void Timer::Tick()
{
	if (timerRunning)
		++ticksPassed;
}

CON_COMMAND(y_spt_timer_start, "Starts the SPT timer.")
{
	spt_timer.StartTimer();
}

CON_COMMAND(y_spt_timer_stop, "Stops the SPT timer and prints the current time.")
{
	spt_timer.StopTimer();
	Warning("Current time (in ticks): %u\n", spt_timer.GetTicksPassed());
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
	}
}
