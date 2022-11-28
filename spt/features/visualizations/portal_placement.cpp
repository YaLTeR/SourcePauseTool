#include "stdafx.h"

#include "renderer\mesh_renderer.hpp"
#include "spt\features\tracing.hpp"

#if defined(SPT_MESH_RENDERING_ENABLED) && defined(SPT_TRACE_PORTAL_ENABLED)

#include "spt\feature.hpp"
#include "spt\features\hud.hpp"
#include "spt\utils\ent_utils.hpp"
#include "spt\utils\portal_utils.hpp"
#include "spt\utils\game_detection.hpp"
#include "spt\utils\signals.hpp"

#define PORTAL_PLACEMENT_FAIL_NO_SERVER -1.0f
#define PORTAL_PLACEMENT_FAIL_NO_WEAPON -2.0f

#define PORTAL_PLACEMENT_SUCCESS_NO_BUMP 1.0f
#define PORTAL_PLACEMENT_SUCCESS_CANT_FIT 0.1f
#define PORTAL_PLACEMENT_SUCCESS_CLEANSER 0.028f
#define PORTAL_PLACEMENT_SUCCESS_OVERLAP_LINKED 0.027f
#define PORTAL_PLACEMENT_SUCCESS_NEAR 0.0265f
#define PORTAL_PLACEMENT_SUCCESS_INVALID_VOLUME 0.026f
#define PORTAL_PLACEMENT_SUCCESS_INVALID_SURFACE 0.025f
#define PORTAL_PLACEMENT_SUCCESS_PASSTHROUGH_SURFACE 0.0f

ConVar y_spt_hud_portal_placement("y_spt_hud_portal_placement",
                                  "0",
                                  FCVAR_CHEAT,
                                  "Show portal placement info\n"
                                  "1 = Boolean result\n"
                                  "2 = String result\n"
                                  "3 = Float result");
ConVar y_spt_draw_pp("y_spt_draw_pp", "0", FCVAR_CHEAT, "Draw portal placement.");
ConVar y_spt_draw_pp_blue("y_spt_draw_pp_blue", "1", FCVAR_CHEAT, "Draw blue portal placement.");
ConVar y_spt_draw_pp_orange("y_spt_draw_pp_orange", "1", FCVAR_CHEAT, "Draw orange portal placement.");
ConVar y_spt_draw_pp_failed("y_spt_draw_pp_failed", "0", FCVAR_CHEAT, "Draw failed portal placement.");
ConVar y_spt_draw_pp_bbox("y_spt_draw_pp_bbox", "0", FCVAR_CHEAT, "Draw the bounding box of the portal placement.");

// Portal placement related features
class PortalPlacement : public FeatureWrapper<PortalPlacement>
{
public:
	struct PlacementInfo
	{
		float placementResult;
		Vector finalPos;
		QAngle finalAngles;
		trace_t tr;
	};

	void UpdatePlacementInfo();

	PlacementInfo p1;
	PlacementInfo p2;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	void OnMeshRenderSignal(MeshRendererDelegate& mr);
};

static PortalPlacement spt_pp;

void PortalPlacement::UpdatePlacementInfo()
{
	auto player = utils::GetServerPlayer();
	if (!player)
	{
		p1.placementResult = PORTAL_PLACEMENT_FAIL_NO_SERVER;
		return;
	}

	if (!spt_tracing.ORIG_GetActiveWeapon(player))
	{
		p1.placementResult = PORTAL_PLACEMENT_FAIL_NO_WEAPON;
		return;
	}
	Vector camPos = utils::GetPlayerEyePosition();
	QAngle angles = utils::GetPlayerEyeAngles();

	p1.placementResult =
	    spt_tracing.TraceTransformFirePortal(p1.tr, camPos, angles, p1.finalPos, p1.finalAngles, false);
	p2.placementResult =
	    spt_tracing.TraceTransformFirePortal(p2.tr, camPos, angles, p2.finalPos, p2.finalAngles, true);
}

static void DrawPortal(MeshBuilderDelegate& mb, const PortalPlacement::PlacementInfo& info, color32 col)
{
	const float PORTAL_HALF_WIDTH = 32.0f;
	const float PORTAL_HALF_HEIGHT = 54.0f;

	const color32 failedColor = {255, 0, 0, 127};
	const color32 noDrawColor = {0, 0, 0, 0};

	color32 portalColor;
	if (info.placementResult <= 0.5f)
	{
		if (info.placementResult <= 0.5f && y_spt_draw_pp_failed.GetBool()
		    && info.placementResult != PORTAL_PLACEMENT_SUCCESS_PASSTHROUGH_SURFACE
		    && info.placementResult != PORTAL_PLACEMENT_SUCCESS_CLEANSER)
		{
			portalColor = failedColor;
		}
		else
		{
			portalColor = noDrawColor;
		}
	}
	else
	{
		portalColor = col;
	}

	MeshColor facePortalColor = MeshColor::Face(portalColor);
	mb.AddEllipse(info.finalPos, info.finalAngles, PORTAL_HALF_WIDTH, PORTAL_HALF_HEIGHT, 32, facePortalColor);

	if (y_spt_draw_pp_bbox.GetBool())
	{
		const Vector portalMaxs(1, PORTAL_HALF_WIDTH, PORTAL_HALF_HEIGHT);
		MeshColor outlinePortalColor(noDrawColor, portalColor);
		mb.AddBox(info.finalPos, -portalMaxs, portalMaxs, info.finalAngles, outlinePortalColor);
	}
}

void PortalPlacement::OnMeshRenderSignal(MeshRendererDelegate& mr)
{
	if (!y_spt_draw_pp.GetBool())
		return;

	// HUD callback didn't update placement info
	if (!y_spt_hud_portal_placement.GetBool())
		UpdatePlacementInfo();

	// No portalgun
	if (p1.placementResult == PORTAL_PLACEMENT_FAIL_NO_SERVER
	    || p1.placementResult == PORTAL_PLACEMENT_FAIL_NO_WEAPON)
		return;

	mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
	    [&](MeshBuilderDelegate& mb)
	    {
		    const color32 blueColor = {64, 160, 255, 127};
		    const color32 orangeColor = {255, 160, 32, 127};

		    // Draw in this order so the color will mix to purple when overlapping.
		    if (y_spt_draw_pp_orange.GetBool())
			    DrawPortal(mb, p2, orangeColor);

		    if (y_spt_draw_pp_blue.GetBool())
			    DrawPortal(mb, p1, blueColor);
	    },
	    {ZTEST_NONE}));
}

bool PortalPlacement::ShouldLoadFeature()
{
	return utils::DoesGameLookLikePortal();
}

static const wchar_t* PlacementResultToString(float placement)
{
	if (placement == PORTAL_PLACEMENT_SUCCESS_NO_BUMP)
		return L"Success with no bump";
	if (placement == PORTAL_PLACEMENT_SUCCESS_CANT_FIT)
		return L"Can't fit";
	if (placement == PORTAL_PLACEMENT_SUCCESS_CLEANSER)
		return L"Fizzler";
	if (placement == PORTAL_PLACEMENT_SUCCESS_OVERLAP_LINKED)
		return L"Overlaps existing portal";
	if (placement == PORTAL_PLACEMENT_SUCCESS_NEAR)
		return L"Near existing portal";
	if (placement == PORTAL_PLACEMENT_SUCCESS_INVALID_VOLUME)
		return L"Invalid volume";
	if (placement == PORTAL_PLACEMENT_SUCCESS_INVALID_SURFACE)
		return L"Invalid surface";
	if (placement == PORTAL_PLACEMENT_SUCCESS_PASSTHROUGH_SURFACE)
		return L"Passthrough surface";
	if (placement > 0.5f)
		return L"Success with bump";
	return L"Bump too far"; // Is this possible?
}

void PortalPlacement::LoadFeature()
{
	if (spt_tracing.ORIG_TraceFirePortal && spt_tracing.ORIG_GetActiveWeapon)
	{
#ifdef SPT_HUD_ENABLED
		AddHudCallback(
		    "pp",
		    []()
		    {
			    if (!y_spt_hud_portal_placement.GetBool())
				    return;

			    spt_pp.UpdatePlacementInfo();
			    float res1 = spt_pp.p1.placementResult;
			    float res2 = spt_pp.p2.placementResult;

			    const Color blue = {64, 160, 255, 255};
			    const Color orange = {255, 160, 32, 255};
			    const Color failed = {150, 150, 150, 255};

			    const Color& blueTextColor = res1 > 0.5f ? blue : failed;
			    const Color& orangeTextColor = res2 > 0.5f ? orange : failed;

			    if (res1 == PORTAL_PLACEMENT_FAIL_NO_SERVER)
			    {
				    spt_hud.DrawTopHudElement(L"Portal: No server player");
			    }
			    else if (res1 == PORTAL_PLACEMENT_FAIL_NO_WEAPON)
			    {
				    spt_hud.DrawTopHudElement(L"Portal: No portalgun");
			    }
			    else if (y_spt_hud_portal_placement.GetInt() == 1)
			    {
				    spt_hud.DrawColorTopHudElement(blueTextColor, L"Portal1: %d", res1 > 0.5f);
				    spt_hud.DrawColorTopHudElement(orangeTextColor, L"Portal2: %d", res2 > 0.5f);
			    }
			    else if (y_spt_hud_portal_placement.GetInt() == 2)
			    {
				    spt_hud.DrawColorTopHudElement(blueTextColor,
				                                   L"Portal1: %s",
				                                   PlacementResultToString(res1));
				    spt_hud.DrawColorTopHudElement(orangeTextColor,
				                                   L"Portal2: %s",
				                                   PlacementResultToString(res2));
			    }
			    else
			    {
				    spt_hud.DrawColorTopHudElement(blueTextColor, L"Portal1: %f", res1);
				    spt_hud.DrawColorTopHudElement(orangeTextColor, L"Portal2: %f", res2);
			    }
		    },
		    y_spt_hud_portal_placement);
#endif
#ifdef SPT_MESH_RENDERING_ENABLED
		if (spt_meshRenderer.signal.Works)
		{
			InitConcommandBase(y_spt_draw_pp);
			InitConcommandBase(y_spt_draw_pp_blue);
			InitConcommandBase(y_spt_draw_pp_orange);
			InitConcommandBase(y_spt_draw_pp_failed);
			InitConcommandBase(y_spt_draw_pp_bbox);
			spt_meshRenderer.signal.Connect(this, &PortalPlacement::OnMeshRenderSignal);
		}
#endif
	}
}

void PortalPlacement::UnloadFeature() {}

#endif
