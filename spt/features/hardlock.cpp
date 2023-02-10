#include "stdafx.hpp"

#ifdef BMS
#include "..\feature.hpp"
#define GAME_DLL
#include "cbase.h"
#include "inetmessage.h"
#include "recipientfilter.h"

namespace patterns
{
	PATTERNS(BroadcastMessage, "BMS-0.9", "55 8B EC 51 53 8B 5D ?? 56 8B F1 8B CB 89 75 ??");
}

// ""Fix"" crashing and hardlocking if issuing too many commands too fast during a load
class HardlockFix : public FeatureWrapper<HardlockFix>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;

private:
	DECL_HOOK_THISCALL(void, BroadcastMessage, void*, INetMessage& msg, IRecipientFilter& filter) {}
};

static HardlockFix hardlock_fix;

bool HardlockFix::ShouldLoadFeature()
{
	return true;
}

void HardlockFix::InitHooks()
{
	HOOK_FUNCTION(engine, BroadcastMessage);
}

#endif
