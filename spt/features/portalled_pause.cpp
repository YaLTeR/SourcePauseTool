#include "stdafx.hpp"

#if defined(SSDK2007) || defined(SSDK2013)

#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"
#include "afterticks.hpp"
#include "ent_utils.hpp"
#include "game_detection.hpp"

ConVar spt_on_portalled_pause_for("spt_on_portalled_pause_for",
                                  "0",
                                  0,
                                  "Whenever player portalled, pause for this many ticks.");

// Pause on player portalled
class PortalledPause : public FeatureWrapper<PortalledPause>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	DECL_HOOK_THISCALL(void, TeleportTouchingEntity, void*, void* pOther);
};

static PortalledPause spt_portalled_pause_feat;

namespace patterns
{
	PATTERNS(TeleportTouchingEntity,
	         "5135",
	         "81 EC 1C 01 00 00 55 8B E9 89 6C 24 ?? E8 ?? ?? ?? ?? 85 C0",
	         "7197370",
	         "55 8B EC 81 EC 28 01 00 00 56 8B F1");
}

bool PortalledPause::ShouldLoadFeature()
{
	return utils::DoesGameLookLikePortal();
}

void PortalledPause::InitHooks()
{
	HOOK_FUNCTION(server, TeleportTouchingEntity);
}

void PortalledPause::LoadFeature()
{
	if (ORIG_TeleportTouchingEntity)
	{
		InitConcommandBase(spt_on_portalled_pause_for);
	}
}

void PortalledPause::UnloadFeature() {}

IMPL_HOOK_THISCALL(PortalledPause, void, TeleportTouchingEntity, void*, void* pOther)
{
	spt_portalled_pause_feat.ORIG_TeleportTouchingEntity(thisptr, pOther);

	if (pOther != utils::GetServerPlayer())
		return;

	const auto pauseFor = spt_on_portalled_pause_for.GetInt();
	if (pauseFor > 0)
	{
		EngineConCmd("setpause");

		afterticks_entry_t entry;
		entry.ticksLeft = pauseFor;
		entry.command = "unpause";
		spt_afterticks.AddAfterticksEntry(entry);
	}
}

#endif
