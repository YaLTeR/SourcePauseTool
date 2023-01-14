#include "stdafx.hpp"
#include "..\feature.hpp"
#include "convar.hpp"
#include "..\utils\game_detection.hpp"

typedef void(__cdecl* _Host_AccumulateTime)(float dt);
ConVar tas_pause("tas_pause", "0", 0, "Does a pause where you can look around when the game is paused.\n");

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

namespace patterns
{
	PATTERNS(_Host_RunFrame,
	         "5135",
	         "55 8B EC 83 EC 1C 53 56 57 33 FF 80 3D ?? ?? ?? ?? 00 74 17 E8 ?? ?? ?? ?? 83 F8 FE 74 0D 68",
	         "dmomm",
	         "55 8B EC 83 EC 10 80 3D ?? ?? ?? ?? 00 53 56 57 74 ?? E8 ?? ?? ?? ??",
	         "BMS-Retail-0.9",
	         "56 57 74 17 E8 ?? ?? ?? ?? 83 F8 FE 74 0D 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04 8B 0D ?? ?? ?? ?? 81 F9 ?? ?? ?? ?? 75 0C");
	PATTERNS(Host_AccumulateTime,
	         "5135",
	         "51 F3 0F 10 ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? F3 0F 11 ?? ?? ?? ?? ?? 8B 01 8B 50",
	         "dmomm",
	         "D9 05 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? D8 44 24 ?? D9 1D ?? ?? ?? ?? 8B 01",
			 "BMS-Retail-0.9",
			 "55 8B EC D9 45 08 83 EC 10 DC 05 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 53 B3 01 DD 1D ?? ?? ?? ?? 8B 01 8B 40 20 FF D0 33 D2");
} // namespace patterns

void TASPause::InitHooks()
{
	FIND_PATTERN(engine, _Host_RunFrame);
	HOOK_FUNCTION(engine, Host_AccumulateTime);
}

void TASPause::LoadFeature()
{
	if (ORIG__Host_RunFrame)
	{
		int ptnNumber = GetPatternIndex((void**)&ORIG__Host_RunFrame);
		ptrdiff_t off_pHost_Frametime = 0;
		switch (ptnNumber)
		{
		case 0: // 5135
			off_pHost_Frametime = (utils::GetBuildNumber() <= 3420) ? 309 : 227;
			break;
		case 1: // dmomm
			off_pHost_Frametime = 217;
			break;
		case 2: //bms 0.9
			off_pHost_Frametime = 211;
			break;
		}
		pHost_Frametime = *reinterpret_cast<float**>((uintptr_t)ORIG__Host_RunFrame + off_pHost_Frametime);
	}
	else
	{
		pHost_Frametime = nullptr;
	}

	if (ORIG_Host_AccumulateTime)
	{
		int ptnNumber = GetPatternIndex((void**)&ORIG_Host_AccumulateTime);
		ptrdiff_t off_pHost_Realtime = 0;
		switch (ptnNumber)
		{
		case 0: // 5135
			off_pHost_Realtime = 5;
			break;
		case 1: // dmomm
			off_pHost_Realtime = 2;
			break;
		case 2:
			off_pHost_Realtime = 11;
			break;
		}
		pHost_Realtime = *reinterpret_cast<float**>((uintptr_t)ORIG_Host_AccumulateTime + off_pHost_Realtime);
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
