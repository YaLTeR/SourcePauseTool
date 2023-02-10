#include "stdafx.hpp"
#include "ent_utils.hpp"
#include "..\feature.hpp"

namespace patterns
{
	PATTERNS(SetCollisionGroup,
		"4044",
		"8B 54 24 ?? 8B 81 ?? ?? ?? ?? 3B C2",
		"5135",
		"56 8B F1 8B 86 ?? ?? ?? ?? 3B 44 24 08 8D",
		"7467727",
		"55 8B EC 53 8B 5D ?? 56 57 8B F9 39 9F ?? ?? ?? ?? 74 ?? 8B ?? ?? ?? ?? ?? 8D",
		"BMS-Retail",
		"55 8B EC 53 8B D9 56 57 8B 7D ?? 39 BB ?? ?? ?? ?? 74 ?? 80 79 ?? 00");
}

class CollisionGroup : public FeatureWrapper<CollisionGroup>
{
public:
	DECL_MEMBER_THISCALL(void, SetCollisionGroup, void*, int collisionGroup);
protected:
	virtual bool ShouldLoadFeature() override
	{
		return true;
	}

	virtual void InitHooks() override
	{
		FIND_PATTERN(server, SetCollisionGroup);
	}

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

CollisionGroup spt_collisiongroup;

CON_COMMAND_F(y_spt_set_collision_group, "Set player's collision group\nUsually:\n- 5 is normal collisions\n- 10 is quickclip\n", FCVAR_CHEAT)
{
	if (!spt_collisiongroup.ORIG_SetCollisionGroup)
		return;

	if (args.ArgC() < 2)
	{
		Warning("Format: y_spt_set_collision_group <collision group index>\nUsually:\n- 5 is normal collisions\n- 10 is quickclip\n");
		return;
	}

	auto playerPtr = utils::GetServerPlayer();
	int collide = atoi(args.Arg(1));

	spt_collisiongroup.ORIG_SetCollisionGroup(playerPtr, collide);
}

void CollisionGroup::LoadFeature()
{
	if (ORIG_SetCollisionGroup)
		InitCommand(y_spt_set_collision_group);
}

void CollisionGroup::UnloadFeature() {}