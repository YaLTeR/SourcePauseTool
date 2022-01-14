#include "stdafx.h"
#include "..\feature.hpp"
#include "physvis.hpp"
#include "generic.hpp"
#include "game_detection.hpp"
#include "..\overlay\portal_camera.hpp"
#include "property_getter.hpp"
#include "interfaces.hpp"
#include "..\cvars.hpp"

ConVar y_spt_draw_portal_env("y_spt_draw_portal_env",
                             "0",
                             FCVAR_CHEAT | FCVAR_DONTRECORD,
                             "Draw the geometry in a portal's physics environment");

ConVar y_spt_draw_portal_env_type(
    "y_spt_draw_portal_env_type",
    "auto",
    FCVAR_CHEAT | FCVAR_DONTRECORD,
    "Usage: y_spt_draw_portal_env_type collide|auto|blue|orange|<index>; draw world collision and static props in a portal environment\n"
    "   - collide: draw what the player has collision with\n"
    "   - auto: prioritize what the player has collision with, otherwise use last drawn portal index\n"
    "   - blue/orange: look for specific portal color\n"
    "   - index: specify portal entity index");

ConVar y_spt_draw_portal_env_ents("y_spt_draw_portal_env_ents",
                                  "0",
                                  FCVAR_CHEAT | FCVAR_DONTRECORD,
                                  "Draw entities owned by portal and shadow clones from remote portal");

ConVar y_spt_draw_portal_env_remote("y_spt_draw_portal_env_remote",
                                    "0",
                                    FCVAR_CHEAT | FCVAR_DONTRECORD,
                                    "Draw geometry from remote portal");

ConVar y_spt_draw_portal_env_wireframe("y_spt_draw_portal_env_wireframe",
                                       "1",
                                       FCVAR_CHEAT | FCVAR_DONTRECORD,
                                       "Draw wireframe for static geometry");

#if defined(SSDK2007) || defined(SSDK2013) // :)

/* 
* This is an _EditMeshFunc. Since we're drawing directly on top of world geo there's a lot of
* z-fighting. This mostly prevents that by pushing each face outwards by some epsilon that depends
* on the camera's distance to the mesh (the further away we are, the more the mesh should be extruded).
*/
int ExtrudeMesh(Vector*& verts, int numVerts, const matrix3x4_t* mat)
{
	const Vector camPos = spt_generic.GetCameraOrigin();
	// determine furthest vert from camera
	float maxDistSqr = 0;
	for (int i = 0; i < numVerts; i++)
	{
		float distSqr;
		if (mat)
		{
			// outVerts doesn't have the transform applied yet
			Vector out;
			VectorTransform(verts[i].Base(), *mat, out.Base());
			distSqr = camPos.DistToSqr(out);
		}
		else
		{
			distSqr = camPos.DistToSqr(verts[i]);
		}
		maxDistSqr = MAX(maxDistSqr, distSqr);
	}
	// extrude the mesh by this epsilon - seems to work alright in most cases
	float normEps = MAX(pow(maxDistSqr, 0.6f) / 50000, 0.0001f);
	for (int i = 0; i < numVerts; i += 3)
	{
		Vector norm = (verts[i] - verts[i + 1]).Cross(verts[i + 2] - verts[i]);
		norm.NormalizeInPlace();
		norm *= normEps;
		verts[i] += norm;
		verts[i + 1] += norm;
		verts[i + 2] += norm;
	}
	return numVerts;
}

static int lastPortalIndex = -1;

IServerEntity* GetSgDrawPortal()
{
	if (!y_spt_draw_portal_env.GetBool() || !interfaces::engine_server->PEntityOfEntIndex(0))
		return nullptr;

	const char* typeStr = y_spt_draw_portal_env_type.GetString();
	bool want_auto = !strcmp(typeStr, "auto");
	bool want_collide = !strcmp(typeStr, "collide");
	typeStr = want_collide ? "auto" : typeStr; // 'collide' here is what 'auto' means in 'getPortal'

	// 'auto' here should prioritize player's env
	IClientEntity* pEnt = getPortal(typeStr, false); // blue/orange/index/collide/auto in bubble
	int portalIdx = pEnt ? pEnt->entindex() : -1;

	if (portalIdx == -1 && want_auto)
	{
		// try the last index we used
		if (invalidPortal(utils::GetClientEntity(lastPortalIndex - 1)))
			lastPortalIndex = -1;
		else
			portalIdx = lastPortalIndex;
		// let's try blue and orange
		if (portalIdx == -1)
		{
			pEnt = getPortal("blue", false);
			if (!pEnt)
				pEnt = getPortal("orange", false);
			if (pEnt)
				portalIdx = pEnt->entindex();
		}
	}
	lastPortalIndex = portalIdx;
	auto serverPortal = interfaces::engine_server->PEntityOfEntIndex(portalIdx);
	return serverPortal ? serverPortal->GetIServerEntity() : nullptr;
}

void DrawSgCollision()
{
	auto iServerEnt = GetSgDrawPortal();
	if (!iServerEnt)
		return;
	auto portal = iServerEnt->GetBaseEntity();

	// portal simulator
#ifdef SSDK2007
	uint32_t* sim = (uint32_t*)portal + 327;
#else
	uint32_t* sim = (uint32_t*)portal + 341;
#endif

	WireframeMode mode = y_spt_draw_portal_env_wireframe.GetBool() ? FacesAndWire : OnlyFaces;

	// portal hole - not directly used for collision (green wireframe)
	PhysVisFeature::DrawCPhysCollide(*(CPhysCollide**)(sim + 70), {OnlyWire, {20, 255, 20, 255}});
	// world brushes - wall geo in front of portal (red)
	PhysVisFeature::DrawCPhysCollide(*(CPhysCollide**)(sim + 76), {mode, {255, 20, 20, 70}, ExtrudeMesh});
	// local wall tube - edge of portal (green)
	PhysVisFeature::DrawCPhysCollide(*(CPhysCollide**)(sim + 94), {mode, {0, 255, 0, 200}});
	// local wall brushes - wall geo behind the portal (blue)
	PhysVisFeature::DrawCPhysCollide(*(CPhysCollide**)(sim + 101), {mode, {40, 40, 255, 60}, ExtrudeMesh});
	// static props (piss colored)
	const CUtlVector<char[28]>& staticProps = *(CUtlVector<char[28]>*)(sim + 83);
	for (int i = 0; i < staticProps.Count(); i++)
	{
		const color32 c = {255, 255, 40, 50};
		PhysVisFeature::DrawCPhysCollide(*((CPhysCollide**)staticProps[i] + 2), {mode, c, ExtrudeMesh});
	}
	if (y_spt_draw_portal_env_remote.GetBool())
	{
		// remote brushes (light pink); extruding here since this frequently overlaps with local world geo
		PhysVisFeature::DrawCPhysicsObject(*(void**)(sim + 103), {mode, {255, 150, 150, 15}, ExtrudeMesh});
		// remote static props (light yellow)
		const CUtlVector<void*>& remoteStaticProps = *(CUtlVector<void*>*)(sim + 104);
		for (int i = 0; i < remoteStaticProps.Count(); i++)
			PhysVisFeature::DrawCPhysicsObject(remoteStaticProps[i], {mode, {255, 255, 150, 15}});
	}
	if (y_spt_draw_portal_env_ents.GetBool())
	{
		// owned entities (pink wireframe)
		const CUtlVector<CBaseEntity*>& ownedEnts = *(CUtlVector<CBaseEntity*>*)(sim + 2171);
		for (int i = 0; i < ownedEnts.Count(); i++)
			PhysVisFeature::DrawCBaseEntity(ownedEnts[i], {OnlyWire, {255, 100, 255, 255}});
		// shadow clones (white wireframe); ownedEnts also has shadow clones so these will draw over them
		const CUtlVector<CBaseEntity*>& shadowClones = *(CUtlVector<CBaseEntity*>*)(sim + 2166);
		for (int i = 0; i < shadowClones.Count(); i++)
			PhysVisFeature::DrawCBaseEntity(shadowClones[i], {OnlyWire, {255, 255, 255, 255}});
	}
}

class SgCollideVisFeature : public FeatureWrapper<SgCollideVisFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override
	{
		return spt_physvis.ShouldLoadFeature() && spt_generic.ShouldLoadFeature()
		       && utils::DoesGameLookLikePortal();
	}

	virtual void InitHooks() override{};

	virtual void LoadFeature() override
	{
		if (spt_physvis.AddPhysVisCallback({DrawSgCollision, "f"}))
		{
			InitConcommandBase(y_spt_draw_portal_env);
			InitConcommandBase(y_spt_draw_portal_env_type);
			InitConcommandBase(y_spt_draw_portal_env_ents);
			InitConcommandBase(y_spt_draw_portal_env_remote);
			InitConcommandBase(y_spt_draw_portal_env_wireframe);
		}
	};

	virtual void UnloadFeature() override{};
};

static SgCollideVisFeature spt_sgCollision;

#endif
