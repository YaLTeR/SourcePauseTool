#include "stdafx.hpp"
#include "..\feature.hpp"

namespace patterns
{
	PATTERNS(Host_Error,
		"BMS-0.9",
		"55 8B EC 81 EC 04 04 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 ?? FF 05",
		"5135",
		"83 05 ?? ?? ?? ?? 01 81 EC 00 04 00 00 80 3D ?? ?? ?? ?? 00 74 ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 C4 04",
	    "SDK2013", // Also works for current(8491853) steampipe
	    "55 8B EC FF 05 ?? ?? ?? ?? 81 EC 00 04 00 00 80 3D ?? ?? ?? ?? 00 74")

}

// ""Fix"" crashing and hardlocking if issuing too many commands too fast during a load
class HardlockFix : public FeatureWrapper<HardlockFix>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;

private:
	DECL_HOOK_CDECL(void, Host_Error, const char* error, ...);
};

static HardlockFix hardlock_fix;

bool HardlockFix::ShouldLoadFeature()
{
	return true;
}

void HardlockFix::InitHooks()
{
	HOOK_FUNCTION(engine, Host_Error);
}

IMPL_HOOK_CDECL(HardlockFix, void, Host_Error, const char* error, ...) {

	if (strncmp(error, "Reliable message (type ", 23) == 0) // I'm not mid function hooking for this, easier to just prevent the specfic call...
	{
		return;
	}
	else
	{
		va_list args;
		va_start(args, error);
		hardlock_fix.ORIG_Host_Error(error, args);
		va_end(args);
	}
}
