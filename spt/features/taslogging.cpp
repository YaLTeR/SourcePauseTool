#include "stdafx.h"
#include "..\feature.hpp"
#include "cmodel.h"
#include "SDK\hl_movedata.h"
#include "convar.hpp"
#include "interfaces.hpp"

typedef void(__fastcall* _ProcessMovement)(void* thisptr, int edx, void* pPlayer, void* pMove);

ConVar tas_log("tas_log", "0", 0, "If enabled, dumps a whole bunch of different stuff into the console.");

// Some logging stuff that can be turned on with tas_log 1
class TASLogging : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

private:
	_ProcessMovement ORIG_ProcessMovement = nullptr;

	static void __fastcall HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove);
};

static TASLogging spt_taslogging;

bool TASLogging::ShouldLoadFeature()
{
	return true;
}

void TASLogging::InitHooks()
{
	if (interfaces::gm)
	{
		auto vftable = *reinterpret_cast<void***>(interfaces::gm);
		AddVFTableHook(VFTableHook(vftable, 1, (void*)HOOKED_ProcessMovement, (void**)&ORIG_ProcessMovement),
		               "server");
	}
	else
	{
		Warning("tas_log 1 has no effect.\n");
	}
}

void __fastcall TASLogging::HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove)
{
	CHLMoveData* mv = reinterpret_cast<CHLMoveData*>(pMove);
	if (tas_log.GetBool())
		DevMsg("[ProcessMovement PRE ] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);

	spt_taslogging.ORIG_ProcessMovement(thisptr, edx, pPlayer, pMove);

	if (tas_log.GetBool())
		DevMsg("[ProcessMovement POST] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);
}
