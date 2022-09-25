#include "stdafx.h"
#include "game_detection.hpp"
#include "rng.hpp"
#include "tier1/checksum_md5.h"
#include "cmodel.h"

#ifdef OE
#include "..\game_shared\usercmd.h"
#else
#include "usercmd.h"
#endif

ConVar y_spt_set_ivp_seed_on_load(
    "y_spt_set_ivp_seed_on_load",
    "",
    FCVAR_CHEAT,
    "Sets the ivp seed once during the next load, can prevent some physics rng when running a tas.\n");

RNGStuff spt_rng;

namespace patterns
{
	PATTERNS(SetPredictionRandomSeed,
	         "5135",
	         "8B 44 24 ?? 85 C0 75 ?? C7 05 ?? ?? ?? ?? FF FF FF FF",
	         "hl1movement",
	         "55 8B EC 8B 45 ?? 85 C0 75 ?? C7 05 ?? ?? ?? ?? FF FF FF FF");
	PATTERNS(ivp_srand,
	         "5135",
	         "8B 44 24 04 85 C0 75 05 B8 01 00 00 00 A3 ?? ?? ?? ?? C3",
	         "7122284",
	         "55 8B EC 8B 45 08 B9 01 00 00 00 85 C0 0F 44 C1 A3 ?? ?? ?? ?? 5D C3");
	PATTERNS(CBasePlayer__InitVCollision,
	         "5135",
	         "57 8B F9 8B 07 8B 90 ?? ?? ?? ?? FF D2 A1 ?? ?? ?? ?? 83 78 30 00",
	         "7122284",
	         "55 8B EC 83 EC 34 57 8B F9 8B 07 FF 90 ?? ?? ?? ?? A1 ?? ?? ?? ?? 83 78 30 00");
} // namespace patterns

void RNGStuff::InitHooks()
{
	HOOK_FUNCTION(server, SetPredictionRandomSeed);
	if (!utils::DoesGameLookLikeBMSMod())
	{
		HOOK_FUNCTION(server, CBasePlayer__InitVCollision);
		FIND_PATTERN(vphysics, ivp_srand);
	}
}

int RNGStuff::GetPredictionRandomSeed(int commandOffset)
{
	int command_number = spt_rng.commandNumber + commandOffset;
	return MD5_PseudoRandom(command_number) & 0x7fffffff;
}

bool RNGStuff::ShouldLoadFeature()
{
	return true;
}

void RNGStuff::PreHook()
{
	if (ORIG_ivp_srand)
	{
		uint32_t offs[] = {14, 17};
		int idx = GetPatternIndex((void**)&ORIG_ivp_srand);
		IVP_RAND_SEED = *(uint32_t**)((uintptr_t)ORIG_ivp_srand + offs[idx]);
	}
}

void RNGStuff::LoadFeature()
{
	if (ORIG_ivp_srand && ORIG_CBasePlayer__InitVCollision)
		InitConcommandBase(y_spt_set_ivp_seed_on_load);
}

void RNGStuff::UnloadFeature() {}

void __cdecl RNGStuff::HOOKED_SetPredictionRandomSeed(void* usercmd)
{
	CUserCmd* ptr = reinterpret_cast<CUserCmd*>(usercmd);
	if (ptr)
	{
		spt_rng.commandNumber = ptr->command_number;
	}

	spt_rng.ORIG_SetPredictionRandomSeed(usercmd);
}

void __fastcall RNGStuff::HOOKED_CBasePlayer__InitVCollision(void* thisptr,
                                                             int edx,
                                                             const Vector& vecAbsOrigin,
                                                             const Vector& vecAbsVelocity)
{
	spt_rng.ORIG_CBasePlayer__InitVCollision(thisptr, edx, vecAbsOrigin, vecAbsVelocity);
	// set the seed before any vphys sim happens, don't use GetInt() since that's casted from a float
	if (y_spt_set_ivp_seed_on_load.GetString()[0] != '\0')
	{
		spt_rng.ORIG_ivp_srand((uint32_t)strtoul(y_spt_set_ivp_seed_on_load.GetString(), nullptr, 10));
		y_spt_set_ivp_seed_on_load.SetValue("");
	}
	DevWarning("spt: ivp seed is %u\n", *spt_rng.IVP_RAND_SEED);
}
