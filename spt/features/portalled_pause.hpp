#pragma once

#include "spt/feature.hpp"
#include "thirdparty/Signal.h"

class IServerEntity;

class PortalledPause : public FeatureWrapper<PortalledPause>
{
public:
	/*
	* Params: (IServerEntity* portal, IServerEntity* teleportingEntity)
	* - check if .Works in LoadFeature() or later
	* - called right *after* a teleport
	*/
	Gallant::Signal2<IServerEntity*, IServerEntity*> portalTeleportedEntitySignal;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void LoadFeature() override;

private:
	DECL_STATIC_HOOK_THISCALL(void, TeleportTouchingEntity, IServerEntity*, IServerEntity* pOther);
};

inline PortalledPause spt_portalled_pause_feat;
