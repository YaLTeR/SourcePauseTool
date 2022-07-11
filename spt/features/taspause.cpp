#include "stdafx.h"
#include "..\feature.hpp"
#include "convar.hpp"

typedef void(__cdecl* _Host_AccumulateTime)(float dt);
ConVar tas_pause("tas_pause", "0", 0, "Does a pause where you can look around when the game is paused.\n");

namespace patterns::engine
{
	PATTERNS(_Host_RunFrame,
	         "5135",
	         "55 8B EC 83 EC 1C 53 56 57 33 FF 80 3D ?? ?? ?? ?? 00 74 17 E8 ?? ?? ?? ?? 83 F8 FE 74 0D 68");
	PATTERNS(Host_AccumulateTime,
	         "5135",
	         "51 F3 0F 10 ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? F3 0F 11 ?? ?? ?? ?? ?? 8B 01 8B 50");
} // namespace patterns::engine

class TASPause : public FeatureWrapper<TASPause>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	float* pHost_Frametime = nullptr;
	float* pHost_Realtime = nullptr;
	uintptr_t ORIG__Host_RunFrame = 0;
	_Host_AccumulateTime ORIG_Host_AccumulateTime = nullptr;

	static void __cdecl HOOKED_Host_AccumulateTime(float dt);
};

static TASPause spt_taspause;

bool TASPause::ShouldLoadFeature()
{
	return true;
}

void TASPause::InitHooks()
{
	FIND_PATTERN(engine, _Host_RunFrame);
	HOOK_FUNCTION(engine, Host_AccumulateTime);
}

void TASPause::LoadFeature()
{
	if (ORIG__Host_RunFrame)
	{
		pHost_Frametime = *reinterpret_cast<float**>((uintptr_t)ORIG__Host_RunFrame + 227);
	}
	else
	{
		pHost_Frametime = nullptr;
	}

	if (ORIG_Host_AccumulateTime)
	{
		pHost_Realtime = *reinterpret_cast<float**>((uintptr_t)ORIG_Host_AccumulateTime + 5);
	}
	else
	{
		pHost_Realtime = nullptr;
	}

	if (ORIG_Host_AccumulateTime && ORIG__Host_RunFrame)
	{
		InitConcommandBase(tas_pause);
	}
}

void TASPause::UnloadFeature() {}

void __cdecl TASPause::HOOKED_Host_AccumulateTime(float dt)
{
	if (tas_pause.GetBool())
	{
		*spt_taspause.pHost_Realtime += dt;
		*spt_taspause.pHost_Frametime = 0;
	}
	else
		spt_taspause.ORIG_Host_AccumulateTime(dt);
}
