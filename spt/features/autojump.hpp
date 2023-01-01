#pragma once

#include "..\feature.hpp"
#include "ent_props.hpp"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

namespace patterns
{
	PATTERNS(
	    CheckJumpButton,
	    "5135",
	    "83 EC 1C 56 8B F1 8B 4E 04 80 B9 04 0A 00 00 00 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 70 0D 00 00",
	    "4104",
	    "83 EC 1C 56 8B F1 8B 4E 08 80 B9 C4 09 00 00 00 74 0E 8B 76 04 83 4E 28 02 32 C0 5E 83 C4 1C C3 D9 EE D8 91 30 0D 00 00",
	    "5339",
	    "55 8B EC 83 EC 20 56 8B F1 8B ?? 04 80 ?? 40 ?? 00 00 00 74 0E 8B ?? 08 83 ?? 28 02 32 C0 5E 8B E5 5D C3",
	    "2257546",
	    "55 8B EC 83 EC 18 56 8B F1 8B ?? 04 80 ?? ?? ?? 00 00 00 74 0E 8B ?? 08 83 ?? 28 02 32 C0 5E 8B E5 5D C3",
	    "2257546-hl1",
	    "55 8B EC 51 56 8B F1 57 8B 7E 04 85 FF 74 10 8B 07 8B CF 8B 80 ?? ?? ?? ?? FF D0 84 C0 75 02 33 FF",
	    "2707",
	    "83 EC 10 53 56 8B F1 8B 4E 08 8A 81 ?? ?? ?? ?? 84 C0 74 0F 8B 76 04 83 4E ?? 02 5E 32 C0 5B 83 C4 10 C3 D9 81 30 0C 00 00 D8 1D",
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
	    "55 8B EC 83 EC 20 56 8B F1 8B ?? 04 80 ?? 48 ?? 00 00 00 74 0E 8B ?? 08 83 ?? 28 02 32 C0 5E 8B E5 5D C3",
	    "BMS-Retail-3",
	    "55 8B EC 83 EC 0C 56 8B F1 8B ?? 04 80 ?? ?? ?? 00 00 00 74 0E 8B ?? 08 83 ?? 28 02 32 C0 5E 8B E5 5D C3",
	    "missinginfo1_6",
	    "55 8B EC 83 EC 1C 56 8B F1 8B 4E 04 80 B9 ?? ?? ?? ?? ?? 74 0E 8B 76 08 83 4E 28 02 32 C0 5E 8B E5");
}

// y_spt_autojump
class AutojumpFeature : public FeatureWrapper<AutojumpFeature>
{
public:
	ptrdiff_t off_mv_ptr = 0;
	ptrdiff_t off_player_ptr = 0;
	bool cantJumpNextTime = false;
	bool insideCheckJumpButton = false;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	DECL_HOOK_THISCALL(bool, CheckJumpButton);
	DECL_HOOK_THISCALL(bool, CheckJumpButton_client);
	DECL_HOOK_THISCALL(void, FinishGravity);
	DECL_HOOK_THISCALL(void, CPortalGameMovement__AirMove);
	DECL_MEMBER_THISCALL(void, CGameMovement__AirMove);

	bool client_cantJumpNextTime = false;
	bool client_insideCheckJumpButton = false;

	PlayerField<bool> m_bDucked;
};

extern AutojumpFeature spt_autojump;
