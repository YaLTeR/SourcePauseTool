#include "stdafx.hpp"
#include "..\feature.hpp"

namespace patterns
{
	PATTERNS(MsgFunc_CloseCaption,
	         "5135",
	         "81 EC 10 04 00 00 53 55 56 8B B4 24 20 04 00 00",
	         "7122284",
	         "55 8B EC 81 EC 08 04 00 00 53 8B 5D ?? 8D 85 ?? ?? ?? ??")
}

// Fix crash when playing demo with captions
class CaptionFix : public FeatureWrapper<CaptionFix>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;

private:
	DECL_HOOK_THISCALL(void, MsgFunc_CloseCaption, void*, void* msg);

	ConVar* closecaption = nullptr;
};

static CaptionFix caption_fix;

bool CaptionFix::ShouldLoadFeature()
{
	// can't use a static ConVarRef cuz that'll be invalidated on every spt_tas_restart_game
	closecaption = g_pCVar->FindVar("closecaption");
	return !!closecaption;
}

void CaptionFix::InitHooks()
{
	HOOK_FUNCTION(client, MsgFunc_CloseCaption);
}

IMPL_HOOK_THISCALL(CaptionFix, void, MsgFunc_CloseCaption, void*, void* msg)
{
	if (!caption_fix.closecaption->GetBool())
		return;
	caption_fix.ORIG_MsgFunc_CloseCaption(thisptr, msg);
}
