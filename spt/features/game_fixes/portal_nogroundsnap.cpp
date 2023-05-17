#include "stdafx.hpp"

#include "..\feature.hpp"
#include "convar.hpp"
#include "..\autojump.hpp"
#include "interfaces.hpp"
#include "game_detection.hpp"

#ifndef OE

static void PortalNoGroundSnapCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue);

ConVar y_spt_portal_no_ground_snap("y_spt_portal_no_ground_snap",
								  "0",
								   0,
								  "Disables an additional check in CategorizePosition which causes sticky noclip",
								   PortalNoGroundSnapCVarCallback);

// Disables an extra check in CategorizePosition which causes sticky noclip
class PortalNoGroundSnapFeature : public FeatureWrapper<PortalNoGroundSnapFeature>
{
public:
	void Toggle(bool enabled);

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	
	// value replacements and single instruction overrides
	// we'll just write the bytes ourselves...
private:
	uint8 jumpCompare[3] = {0x85, 0xC0, 0x74};
	DECL_BYTE_REPLACE(Jump, 1, 0xEB);

	uintptr_t ORIG_CPortalGameMovement__MiddleOfCategorizePosition;
};

static PortalNoGroundSnapFeature spt_portal_nogroundsnap;
static void PortalNoGroundSnapCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue)
{
	spt_portal_nogroundsnap.Toggle(((ConVar*)pConVar)->GetBool());
}

namespace patterns
{
	PATTERNS(CPortalGameMovement__MiddleOfCategorizePosition, "1", "80 ?? ?? ?? 00 00 02 73 ?? 80");
}

bool PortalNoGroundSnapFeature::ShouldLoadFeature()
{
	return utils::DoesGameLookLikePortal();
}

void PortalNoGroundSnapFeature::InitHooks()
{
	FIND_PATTERN(server, CPortalGameMovement__MiddleOfCategorizePosition);
}

void PortalNoGroundSnapFeature::LoadFeature()
{
	if (!ORIG_CPortalGameMovement__MiddleOfCategorizePosition)
		return;

	/*
	* The check we're looking for is one inserted in the middle of CategorizePosition:
	* "if (player->GetGroundEntity() != NULL)".
	* 
	* We actually scan for another check a but below this, inside the if condition in the code,
	* as it is more distinctive in assembly...
	* 
	* E8 xx xx xx xx		CALL [GetGroundEntity]
	* 85 C0					TEST EAX, EAX
	* 74 xx					JE [after if statement]
	* ~ noise ~
	* 80 B9 xx xx xx xx 02	CMP byte ptr [player->m_nWaterLevel],02
	* 73 xx					JAE [loc]
	*/
	auto start = (uintptr_t)ORIG_CPortalGameMovement__MiddleOfCategorizePosition;
	for (int i = 0; i <= 100; i++)
	{
		if (memcmp((void*)(start - i), jumpCompare, 3))
			continue;

		INIT_BYTE_REPLACE(Jump, start - i + 2);
		InitConcommandBase(y_spt_portal_no_ground_snap);
		return;
	}
}

void PortalNoGroundSnapFeature::UnloadFeature()
{
	DESTROY_BYTE_REPLACE(Jump);
}


void PortalNoGroundSnapFeature::Toggle(bool enabled)
{
	if (enabled)
	{
		DO_BYTE_REPLACE(Jump);
	}
	else
	{
		UNDO_BYTE_REPLACE(Jump);
	}
}

#endif
