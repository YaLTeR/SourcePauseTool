#pragma once

#include "..\feature.hpp"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

// y_spt_autojump
class AutojumpFeature : public FeatureWrapper<AutojumpFeature>
{
public:
	ptrdiff_t off1M_nOldButtons = 0;
	ptrdiff_t off2M_nOldButtons = 0;
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

	bool client_cantJumpNextTime = false;
	bool client_insideCheckJumpButton = false;
};

extern AutojumpFeature spt_autojump;
