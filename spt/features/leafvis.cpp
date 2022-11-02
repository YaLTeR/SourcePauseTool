#include "stdafx.h"
#include "..\feature.hpp"

namespace patterns
{
	PATTERNS(MiddleOfLeafVisBuild,
	         "3420", // also 5135, old steampipe and new steampipe
	         "74 ?? 39 35 ?? ?? ?? ?? 0F 84 ?? ?? ?? ??");
}

ConVar y_spt_draw_leaf("y_spt_leafvis_index",
                       "0",
                       FCVAR_CHEAT,
                       "Choose which BSP leaf mat_leafvis will use. Requires mat_leafvis to be set\n");

// Enhance mat_leafvis by allowing you to choose the BSP leaf by index
class LeafVisFeature : public FeatureWrapper<LeafVisFeature>
{
public:
protected:
	virtual void InitHooks() override;
	virtual void LoadFeature() override;

private:
	void* ORIG_MiddleOfLeafVisBuild = nullptr;
	static void HOOKED_MiddleOfLeafVisBuild();
};
static LeafVisFeature spt_leafvis;

void LeafVisFeature::InitHooks()
{
	HOOK_FUNCTION(engine, MiddleOfLeafVisBuild);
}

void LeafVisFeature::LoadFeature()
{
	if (ORIG_MiddleOfLeafVisBuild != nullptr)
		InitConcommandBase(y_spt_draw_leaf);
}

static __forceinline int GetLeafIndex()
{
	return y_spt_draw_leaf.GetInt();
}

/**
* The game calls CM_PointLeafnum, returning the leaf index of the current view
* point and loads ESI with it. This hooks is right after that, setting the leaf
* index to our cvar value
*/
__declspec(naked) void LeafVisFeature::HOOKED_MiddleOfLeafVisBuild()
{
	__asm {
		push eax;
		pushfd;
	}
	// this loads eax
	GetLeafIndex();
	__asm {
		cmp eax, 0;
		cmova esi, eax; // override leafIndex in LeafVisBuild if y_spt_draw_leaf > 0
		popfd;
		pop eax;
		jmp spt_leafvis.ORIG_MiddleOfLeafVisBuild;
	}
}