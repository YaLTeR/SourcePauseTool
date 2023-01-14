#pragma once

#include "..\feature.hpp"
#include "thirdparty\Signal.h"
#include <string>

struct afterticks_entry_t
{
	afterticks_entry_t(long long int ticksLeft, std::string command)
		: ticksLeft(ticksLeft), command(std::move(command))
	{
	}
	afterticks_entry_t() {}
	long long int ticksLeft;
	std::string command;
};

// Tick-based afterframes
class AfterticksFeature : public FeatureWrapper<AfterticksFeature>
{
public:
	void AddAfterticksEntry(afterticks_entry_t entry);
	void ResetAfterticksQueue();
	void PauseAfterticksQueue();
	void ResumeAfterticksQueue();
	void DelayAfterticksQueue(int delay);

	bool Works = false;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void LoadFeature() override;
	virtual void InitHooks() override;
	virtual void UnloadFeature() override;

private:
	void OnFrame();
	void SV_ActivateServer(bool result);
	void FinishRestore(void* thisptr);
	void SetPaused(void* thisptr, bool paused);
	int GetTickCount();

	int lastTickCount;
	// we rely on this as it is never reset and is purely driven by the engine
	uintptr_t ptrHostTickCount;
	uintptr_t ORIG_HostRunframe__TargetString;
	std::vector<patterns::MatchedPattern> MATCHES_Engine__StringReferences;
};

extern AfterticksFeature spt_afterticks;
