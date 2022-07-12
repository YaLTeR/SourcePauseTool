#include "stdafx.h"
#include "autojump.hpp"

#include "basehandle.h"
#include "SDK\hl_movedata.h"
#include "convar.hpp"
#include "dbg.h"
#include "..\cvars.hpp"
#include "signals.hpp"

#ifdef OE
#include "mathlib.h"
#endif

AutojumpFeature spt_autojump;
ConVar y_spt_autojump("y_spt_autojump", "0", FCVAR_ARCHIVE);
ConVar _y_spt_autojump_ensure_legit("_y_spt_autojump_ensure_legit", "1", FCVAR_CHEAT);
ConVar y_spt_additional_jumpboost("y_spt_additional_jumpboost",
                                  "0",
                                  0,
                                  "1 = Add in ABH movement mechanic, 2 = Add in OE movement mechanic.\n");

namespace patterns
{
	PATTERNS(
	    CheckJumpButton,
	    "5135",
	    "83 EC 1C 56 8B F1 8B 4E 04 80 B9 04 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 70 0D 00 00",
	    "4104",
	    "83 EC 1C 56 8B F1 8B 4E 08 80 B9 C4 09 00 00 00 74 0E 8B 76 04 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 30 0D 00 00",
	    "5339",
	    "55 8B EC 83 EC 20 56 8B F1 8B 4E 04 80 B9 40 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 AC 0D 00 00",
	    "2257546",
	    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 40 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 ?? 0D 00 00",
	    "2257546-hl1",
	    "55 8B EC 51 56 8B F1 57 8B 7E 04 85 FF 74 10 8B 07 8B CF 8B 80 ?? ?? ?? ?? FF D0 84 C0 75 02 33 FF",
	    "1910503",
	    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 40 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B",
	    "bms",
	    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 10 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 E8 08 F9 FF FF 84 C0 75 F0",
	    "estranged",
	    "55 8B EC 83 EC ?? 56 8B F1 8B 4E 04 80 B9 44 0A 00 00 00 74 0E 8B ?? 08 83 ?? 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 B0 0D 00 00 0F 57 C0 0F 2E",
	    "2707",
	    "83 EC 10 53 56 8B F1 8B 4E 08 8A 81 ?? ?? ?? ?? 84 C0 74 0F 8B 76 04 83 4E ?? 02 5E 32 C0 5B 83 C4 10 C3 D9 81 30 0C 00 00 D8 1D",
	    "BMS-Retail-3",
	    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 9C 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0",
	    "BMS-Retail",
	    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 78 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5 5D C3 8B 06 8B 80 44 01 00 00 FF D0 84 C0 75",
	    "BMS-Retail-2",
	    "55 8B EC 83 EC 0C 56 8B F1 8B 46 04 80 B8 84 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B",
	    "2949",
	    "83 EC 14 56 8B F1 8B 4E 08 80 B9 30 09 00 00 00 0F 85 E1 00 00 00 D9 05 ?? ?? ?? ?? D9 81 70 0C 00 00",
	    "dmomm",
	    "81 EC ?? ?? ?? ?? 56 8B F1 8B 4E 10 80 B9 ?? ?? ?? ?? ?? 74 11 8B 76 0C 83 4E 28 02 32 C0 5E 81 C4",
	    "4044-episodic",
	    "83 EC 14 56 8B F1 8B 4E 08 80 B9 ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? D9 05 ?? ?? ?? ?? D9 81",
	    "6879",
	    "55 8B EC 83 EC 0C 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 07 32 C0 5E 8B E5 5D C3 53 BB",
	    "missinginfo1_4_7",
	    "55 8B EC 83 EC 44 56 89 4D D0 8B 45 D0 8B 48 08 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 C8 85 C9",
	    "te120",
	    "55 8B EC 83 EC 20 56 8B F1 8B 4E 04 80 B9 48 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 B4 0D 00 00",
	    "icemod2013",
	    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 50 0A 00 00 00 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 D8 0D 00 00",
	    "missinginfo1_6",
	    "55 8B EC 83 EC 1C 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5");
	PATTERNS(
	    FinishGravity,
	    "bms",
	    "8B 51 04 F3 0F 10 82 7C 0D 00 00 0F 57 C9 0F 2E C1 9F F6 C4 44 7A 51 F3 0F 10 82 28 02 00 00 0F 2E C1 9F F6 C4 44",
	    "2707",
	    "8B 51 08 D9 82 ?? ?? ?? ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 44 7A 4D D9 82 ?? ?? ?? ?? D8 1D",
	    "4044",
	    "8B 51 08 D9 05 ?? ?? ?? ?? D9 82 ?? ?? ?? ?? DA E9 DF E0 F6 C4 44 7A 4F D9 05 ?? ?? ?? ?? D9 82",
	    "4104",
	    "D9 EE 8B 51 08 D8 92 ?? ?? ?? ?? DF E0 F6 C4 44 7A 3F D8 9A ?? ?? ?? ?? DF E0 F6 C4 44 7B 08 D9 82",
	    "5135",
	    "D9 EE 8B 51 04 D8 92 ?? ?? ?? ?? DF E0 F6 C4 44 7A 3F D8 9A ?? ?? ?? ?? DF E0 F6 C4 44 7B 08 D9 82",
	    "2257546",
	    "55 8B EC 51 0F 57 C9 57 8B F9 8B 57 04 F3 0F 10 82 ?? ?? ?? ?? 0F 2E C1 9F F6 C4 44 7A 47 F3 0F 10 82",
	    "6879",
	    "8B 51 04 F3 0F 10 8A ?? ?? ?? ?? 0F 57 C0 0F 2E C8 9F F6 C4 44 0F 8A ?? ?? ?? ?? F3 0F 10 8A",
	    "missinginfo1_4_7",
	    "55 8B EC 83 EC 0C 89 4D F8 8B 45 F8 8B 48 08 D9 05 ?? ?? ?? ?? D9 81 ?? ?? ?? ?? DA E9 DF E0 F6 C4 44",
	    "missinginfo1_6",
	    "8B 51 04 D9 82 ?? ?? ?? ?? D9 EE D9 C0 DD EA DF E0 DD D9 F6 C4 44 7A 3F D9 82 ?? ?? ?? ?? DA E9");
	PATTERNS(
	    CheckJumpButton_client,
	    "5135",
	    "83 EC 18 56 8B F1 8B 4E 04 80 B9 74 0F 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 83 C4 18 C3 D9 EE D8 91 6C 10 00 00",
	    "4104",
	    "83 EC 18 56 8B F1 8B 4E 08 80 B9 3C 0F 00 00 00 74 0E 8B 76 04 83 4E 28 02 32 C0 5E 83 C4 18 C3 D9 EE D8 91 30 10 00 00",
	    "5339",
	    "55 8B EC 83 EC 1C 56 8B F1 8B 4E 04 80 B9 24 10 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5 5D C3 F3 0F 10 89 20 11 00 00",
	    "2257546",
	    "55 8B EC 83 EC 14 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 46 08 83 48 28 02 32 C0 5E 8B E5",
	    "2257546-hl1",
	    "55 8B EC 51 56 8B F1 57 8B 7E 04 85 FF 74 2B 8B 07 8B CF 8B 80 ?? ?? ?? ?? FF D0 84 C0 74 1B 6A 00",
	    "2707",
	    "83 EC 10 53 56 8B F1 8B 4E 08 8A 81 ?? ?? ?? ?? 84 C0 74 0F 8B 76 04 83 4E 28 02 5E 32 C0 5B 83 C4 10",
	    "4044-episodic",
	    "83 EC 10 56 8B F1 8B 4E 08 80 B9 ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? D9 05 ?? ?? ?? ?? D9 81",
	    "1910503",
	    "55 8B EC 83 EC 14 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5",
	    "6879",
	    "55 8B EC 83 EC 0C 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 07 32 C0 5E 8B E5 5D C3 53 BB",
	    "missinginfo1_4_7",
	    "55 8B EC 83 EC 3C 56 89 4D D8 8B 45 D8 8B 48 08 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F B6 C8 85 C9",
	    "missinginfo1_6",
	    "55 8B EC 83 EC 18 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5");

} // namespace patterns

void AutojumpFeature::InitHooks()
{
	HOOK_FUNCTION(server, CheckJumpButton);
	HOOK_FUNCTION(client, CheckJumpButton_client);
	HOOK_FUNCTION(server, FinishGravity);
}

bool AutojumpFeature::ShouldLoadFeature()
{
	return true;
}

void AutojumpFeature::LoadFeature()
{
	// Server-side CheckJumpButton
	if (ORIG_CheckJumpButton)
	{
		InitConcommandBase(y_spt_autojump);
		InitConcommandBase(_y_spt_autojump_ensure_legit);
		JumpSignal.Works = true;
		int ptnNumber = GetPatternIndex((void**)&ORIG_CheckJumpButton);
		switch (ptnNumber)
		{
		case 0:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 1:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 2:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 3:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 4:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 5:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 6:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 7:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 8:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 9:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 10:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 11:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 12:
			off1M_nOldButtons = 3;
			off2M_nOldButtons = 40;
			break;

		case 13:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 14:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 15:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 16:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 17:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 18:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;
		}
	}
	else
	{
		Warning("y_spt_autojump has no effect.\n");
	}

	if (ORIG_FinishGravity)
	{
		InitConcommandBase(y_spt_additional_jumpboost);
		int ptnNumber = GetPatternIndex((void**)&ORIG_FinishGravity);
		switch (ptnNumber)
		{
		case 0:
			off1M_bDucked = 1;
			off2M_bDucked = 2128;
			break;

		case 1:
			off1M_bDucked = 2;
			off2M_bDucked = 3120;
			break;

		case 2:
			off1M_bDucked = 2;
			off2M_bDucked = 3184;
			break;

		case 3:
			off1M_bDucked = 2;
			off2M_bDucked = 3376;
			break;

		case 4:
			off1M_bDucked = 1;
			off2M_bDucked = 3440;
			break;

		case 5:
			off1M_bDucked = 1;
			off2M_bDucked = 3500;
			break;

		case 6:
			off1M_bDucked = 1;
			off2M_bDucked = 3724;
			break;

		case 7:
			off1M_bDucked = 2;
			off2M_bDucked = 3112;
			break;

		case 8:
			off1M_bDucked = 1;
			off2M_bDucked = 3416;
			break;
		}
	}
	else
	{
		Warning("y_spt_additional_jumpboost has no effect.\n");
		off1M_bDucked = 0;
		off2M_bDucked = 0;
	}
}

void AutojumpFeature::UnloadFeature() {}

bool __fastcall AutojumpFeature::HOOKED_CheckJumpButton(void* thisptr, int edx)
{
	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + spt_autojump.off1M_nOldButtons));
	if (tas_log.GetBool())
		DevMsg("[CheckJumpButton PRE ] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons =
		    (int*)(*((uintptr_t*)thisptr + spt_autojump.off1M_nOldButtons) + spt_autojump.off2M_nOldButtons);
		origM_nOldButtons = *pM_nOldButtons;

		if (!spt_autojump.cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
	}

	spt_autojump.cantJumpNextTime = false;

	spt_autojump.insideCheckJumpButton = true;
	bool rv = spt_autojump.ORIG_CheckJumpButton(thisptr, edx); // This function can only change the jump bit.
	spt_autojump.insideCheckJumpButton = false;

	if (y_spt_autojump.GetBool())
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		JumpSignal();
		// We jumped.
		if (_y_spt_autojump_ensure_legit.GetBool())
		{
			spt_autojump.cantJumpNextTime = true; // Prevent consecutive jumps.
		}
	}

	if (tas_log.GetBool())
		DevMsg("[CheckJumpButton POST] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);

	return rv;
}

bool __fastcall AutojumpFeature::HOOKED_CheckJumpButton_client(void* thisptr, int edx)
{
	/*
	Not sure if this gets called at all from the client dll, but
	I will just hook it in exactly the same way as the server one.
	*/
	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons =
		    (int*)(*((uintptr_t*)thisptr + spt_autojump.off1M_nOldButtons) + spt_autojump.off2M_nOldButtons);
		origM_nOldButtons = *pM_nOldButtons;

		if (!spt_autojump.client_cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
	}

	spt_autojump.client_cantJumpNextTime = false;

	spt_autojump.client_insideCheckJumpButton = true;
	bool rv = spt_autojump.ORIG_CheckJumpButton_client(thisptr, edx); // This function can only change the jump bit.
	spt_autojump.client_insideCheckJumpButton = false;

	if (y_spt_autojump.GetBool())
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		// We jumped.
		if (_y_spt_autojump_ensure_legit.GetBool())
		{
			spt_autojump.client_cantJumpNextTime = true; // Prevent consecutive jumps.
		}
	}

	DevMsg("Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false"));

	return rv;
}

void __fastcall AutojumpFeature::HOOKED_FinishGravity(void* thisptr, int edx)
{
	if (spt_autojump.insideCheckJumpButton && y_spt_additional_jumpboost.GetBool())
	{
		CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + spt_autojump.off1M_nOldButtons));
		bool ducked =
		    *(bool*)(*((uintptr_t*)thisptr + spt_autojump.off1M_bDucked) + spt_autojump.off2M_bDucked);

		// <stolen from gamemovement.cpp>
		{
			Vector vecForward;
			AngleVectors(mv->m_vecViewAngles, &vecForward);
			vecForward.z = 0;
			VectorNormalize(vecForward);

			// We give a certain percentage of the current forward movement as a bonus to the jump speed.  That bonus is clipped
			// to not accumulate over time.
			float flSpeedBoostPerc = (!mv->m_bIsSprinting && !ducked) ? 0.5f : 0.1f;
			float flSpeedAddition = fabs(mv->m_flForwardMove * flSpeedBoostPerc);
			float flMaxSpeed = mv->m_flMaxSpeed + (mv->m_flMaxSpeed * flSpeedBoostPerc);
			float flNewSpeed = (flSpeedAddition + mv->m_vecVelocity.Length2D());

			// If we're over the maximum, we want to only boost as much as will get us to the goal speed
			if (y_spt_additional_jumpboost.GetInt() == 1)
			{
				if (flNewSpeed > flMaxSpeed)
				{
					flSpeedAddition -= flNewSpeed - flMaxSpeed;
				}

				if (mv->m_flForwardMove < 0.0f)
					flSpeedAddition *= -1.0f;
			}

			// Add it on
			VectorAdd((vecForward * flSpeedAddition), mv->m_vecVelocity, mv->m_vecVelocity);
		}
		// </stolen from gamemovement.cpp>
	}

	return spt_autojump.ORIG_FinishGravity(thisptr, edx);
}
