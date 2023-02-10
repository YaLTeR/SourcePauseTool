#pragma once

#include "..\feature.hpp"
#include "ent_props.hpp"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif


// y_spt_autojump
class AutojumpFeature : public FeatureWrapper<AutojumpFeature>
{
public:
	ptrdiff_t off_mv_ptr = 0;
	ptrdiff_t off_player_ptr = 0;
	bool cantJumpNextTime = false;
	bool insideCheckJumpButton = false;

	uintptr_t ptrCheckJumpButton = NULL;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void PreHook() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	DECL_HOOK_THISCALL(bool, CheckJumpButton, void*);
	DECL_HOOK_THISCALL(bool, CheckJumpButton_client, void*);
	DECL_HOOK_THISCALL(void, FinishGravity, void*);
	DECL_HOOK_THISCALL(void, CPortalGameMovement__AirMove, void*);
	DECL_MEMBER_THISCALL(void, CGameMovement__AirMove, void*);

	bool client_cantJumpNextTime = false;
	bool client_insideCheckJumpButton = false;

	PlayerField<bool> m_bDucked;
};

extern AutojumpFeature spt_autojump;
