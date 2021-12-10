#include "stdafx.h"
#include "game_detection.hpp"
#include "tracing.hpp"
#include "signals.hpp"
#include "interfaces.hpp"
#include "..\feature.hpp"
#include "..\vgui\lines.hpp"

ConVar y_spt_drawseams("y_spt_drawseams", "0", FCVAR_CHEAT, "Draws seamshot stuff.\n");

class GraphicsFeature : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void LoadFeature() override;
};

static GraphicsFeature spt_graphics;

bool GraphicsFeature::ShouldLoadFeature()
{
#ifdef SSDK2007
	return utils::DoesGameLookLikePortal() && interfaces::debugOverlay;
#else
	return false;
#endif
}

void GraphicsFeature::LoadFeature()
{
	if (spt_tracing.ORIG_FirePortal && interfaces::debugOverlay && AdjustAngles.Works)
	{
		InitConcommandBase(y_spt_drawseams);
		AdjustAngles.Connect(vgui::DrawLines);
	}
}
