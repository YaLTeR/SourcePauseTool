#include "stdafx.h"

#include "renderer\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "predictable_entity.h"
#include "dt_common.h"
#include "collisionproperty.h"
#include "vphysics_interface.h"

#include "interfaces.hpp"
#include "spt\feature.hpp"
#include "spt\utils\game_detection.hpp"
#include "spt\utils\signals.hpp"
#include "spt\features\ent_props.hpp"

using interfaces::engine_server;

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

#define MC_PORTAL_HOLE (MeshColor::Wire({255, 255, 255, 255}))
#define MC_LOCAL_WORLD_BRUSHES (MeshColor{{255, 20, 20, 70}, {0, 0, 0, 255}})
#define MC_REMOTE_WORLD_BRUSHES (MeshColor{{255, 150, 150, 15}, {0, 0, 0, 255}})
#define MC_LOCAL_WALL_TUBE (MeshColor{{0, 255, 0, 200}, {0, 0, 0, 255}})
#define MC_LOCAL_WALL_BRUSHES (MeshColor{{40, 40, 255, 60}, {0, 0, 0, 255}})
#define MC_LOCAL_STATIC_PROPS (MeshColor{{255, 255, 40, 50}, {0, 0, 0, 255}})
#define MC_REMOTE_STATIC_PROPS (MeshColor{{255, 255, 150, 15}, {0, 0, 0, 255}})

// wireframe means either this portal or the linked own owns the entity, green means 

// wire is owned ents, solid is what UTIL_PORTAL_TRACERAY collides with
#define MC_ENTS (MeshColor{{0, 255, 0, 15}, {40, 255, 40, 255}})
// wire is shadow clones, solid is shadow clones from other portals that aren't supposed to™ collide with UTIL_PORTAL_TRACERAY
#define MC_SHADOW_CLONES (MeshColor{{255, 0, 255, 15}, {255, 100, 255, 255}})

#define PORTAL_CLASS "CProp_Portal"

// Since we're rendering geometry directly on top of the world and other entities, we get a LOT of z-fighting by
// default. This fixes that by scaling everything towards the camera just a little bit.
static void RenderCallbackZFightFix(const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
{
	infoOut.mat[0][3] -= infoIn.cvs.origin.x;
	infoOut.mat[1][3] -= infoIn.cvs.origin.y;
	infoOut.mat[2][3] -= infoIn.cvs.origin.z;

	matrix3x4_t scaleMat;
	SetScaleMatrix(0.9995f, scaleMat);
	MatrixMultiply(scaleMat, infoOut.mat, infoOut.mat);

	infoOut.mat[0][3] += infoIn.cvs.origin.x;
	infoOut.mat[1][3] += infoIn.cvs.origin.y;
	infoOut.mat[2][3] += infoIn.cvs.origin.z;
}

class SgCollideVisFeature : public FeatureWrapper<SgCollideVisFeature>
{
public:
	static SgCollideVisFeature featureInstance;

	struct
	{
		int pos;
		int ang;
		int isOrange;
		int isActivated;
		int linked;
		int simulator;
		int m_Collision;
	} portalFieldOffs;

	PlayerField<int> envPortalField;

	struct PortalInfo
	{
		Vector pos;
		QAngle ang;
		CBaseHandle linked;
		bool isOrange, isActivated, isOpen;
	};

	struct
	{
		bool collide, auto_, blue, orange, idx;
		int idxVal;
	} userWishes;

	enum class CachedEntType
	{
		OWNED,
		SHADOW_CLONE,
		TRACE_TEST,
		COUNT
	};

	struct MeshCache
	{
		struct CachedRemoteGeo
		{
			matrix3x4_t mat;
			StaticMesh mesh;
		};

		struct CachedEnt
		{
			CBaseEntity* pEnt;
			IPhysicsObject* pPhysObj;
			StaticMesh mesh;
		};

		StaticMesh portalHole;
		std::vector<StaticMesh> localWorld;
		std::vector<CachedRemoteGeo> remoteWorld;
		// bit of an oof, color modulation is really slow so we'll just have 1 mesh per color xd
		CachedEnt ents[MAX_EDICTS][2];

		CBaseHandle lastPortal{INVALID_EHANDLE_INDEX};
		PortalInfo lastPortalInfo{vec3_invalid};
		PortalInfo lastLinkedInfo{vec3_invalid};

		void Clear()
		{
			portalHole.Destroy();
			localWorld.clear();
			remoteWorld.clear();
			for (size_t i = 0; i < ARRAYSIZE(ents); i++)
				for (size_t j = 0; j < ARRAYSIZE(*ents); j++)
					ents[i][j].mesh.Destroy();
			lastPortal.Term();
			lastPortalInfo.pos = vec3_invalid;
			lastLinkedInfo.pos = vec3_invalid;
		}
	} cache;

	virtual bool ShouldLoadFeature() override
	{
		return utils::DoesGameLookLikePortal();
	}

	virtual void LoadFeature() override
	{
		if (!spt_collideToMesh.Works() || !spt_meshRenderer.signal.Works)
			return;

		// cache all field stuff & check that spt_entprops works
		portalFieldOffs.pos = spt_entprops.GetFieldOffset(PORTAL_CLASS, "m_vecAbsOrigin", true);
		portalFieldOffs.ang = spt_entprops.GetFieldOffset(PORTAL_CLASS, "m_angAbsRotation", true);
		portalFieldOffs.isOrange = spt_entprops.GetFieldOffset(PORTAL_CLASS, "m_bIsPortal2", true);
		portalFieldOffs.linked = spt_entprops.GetFieldOffset(PORTAL_CLASS, "m_hLinkedPortal", true);
		portalFieldOffs.isActivated = spt_entprops.GetFieldOffset(PORTAL_CLASS, "m_bActivated", true);
		portalFieldOffs.simulator = spt_entprops.GetFieldOffset(PORTAL_CLASS, "m_vPortalCorners", true);
		portalFieldOffs.m_Collision = spt_entprops.GetFieldOffset(PORTAL_CLASS, "m_hMovePeer", true);

		for (int i = 0; i < sizeof(portalFieldOffs) / sizeof(int); i++)
			if (reinterpret_cast<int*>(&portalFieldOffs)[i] == utils::INVALID_DATAMAP_OFFSET)
				return;
		envPortalField = spt_entprops.GetPlayerField<int>("m_hPortalEnvironment", PropMode::Server);
		if (!envPortalField.ServerOffsetFound())
			return;

		portalFieldOffs.simulator += sizeof(Vector) * 4;
		portalFieldOffs.m_Collision += sizeof(EHANDLE);

		spt_meshRenderer.signal.Connect(this, &SgCollideVisFeature::OnMeshRenderSignal);
		InitConcommandBase(y_spt_draw_portal_env);
		InitConcommandBase(y_spt_draw_portal_env_type);
		InitConcommandBase(y_spt_draw_portal_env_ents);
		InitConcommandBase(y_spt_draw_portal_env_remote);

		y_spt_draw_portal_env_type.InstallChangeCallback(
		    [](IConVar* var, const char*, float)
		    {
			    auto& wish = featureInstance.userWishes;
			    const char* typeStr = ((ConVar*)var)->GetString();
			    wish.collide = !_stricmp(typeStr, "collide");
			    wish.auto_ = !_stricmp(typeStr, "auto");
			    wish.blue = !_stricmp(typeStr, "blue") || wish.auto_;
			    wish.orange = !_stricmp(typeStr, "orange") || wish.auto_;
			    if (!wish.collide && !wish.auto_ && !wish.blue && !wish.orange)
			    {
				    wish.idx = true;
				    wish.idxVal = ((ConVar*)var)->GetInt() + 1; // client -> server index
			    }
			    else
			    {
				    wish.idx = false;
			    }
		    });
		// trigger callback to initialize wish values
		y_spt_draw_portal_env_type.SetValue(y_spt_draw_portal_env_type.GetDefault());
	}

	virtual void UnloadFeature() override
	{
		cache.Clear();
	}

	bool GetPortalInfo(edict_t* edict, PortalInfo* outInfo) const
	{
		if (!edict || strcmp(edict->GetClassName(), "prop_portal"))
			return false;
		if (outInfo)
		{
			uintptr_t ent = reinterpret_cast<uintptr_t>(edict->GetIServerEntity()->GetBaseEntity());
			outInfo->pos = *reinterpret_cast<Vector*>(ent + portalFieldOffs.pos);
			outInfo->ang = *reinterpret_cast<QAngle*>(ent + portalFieldOffs.ang);
			outInfo->isOrange = *reinterpret_cast<bool*>(ent + portalFieldOffs.isOrange);
			outInfo->isActivated = *reinterpret_cast<bool*>(ent + portalFieldOffs.isActivated);
			outInfo->linked = *reinterpret_cast<CBaseHandle*>(ent + portalFieldOffs.linked);
			outInfo->isOpen = outInfo->linked.IsValid();
		}
		return true;
	}

	bool PortalMatchesUserWish(const PortalInfo& pInfo) const
	{
		if (userWishes.collide && pInfo.isOpen) // if collide is set, assume we're checking player portal env
			return true;
		if (userWishes.blue && !pInfo.isOrange)
			return true;
		if (userWishes.orange && pInfo.isOrange)
			return true;
		return false;
	}

	edict_t* GetSgDrawPortal() const
	{
		if (!engine_server->PEntityOfEntIndex(0)
		    || (!y_spt_draw_portal_env.GetBool() && !y_spt_draw_portal_env_ents.GetBool()
		        && !y_spt_draw_portal_env_remote.GetBool()))
		{
			return nullptr;
		}

		if (userWishes.idx)
		{
			edict_t* ed = engine_server->PEntityOfEntIndex(userWishes.idxVal);
			if (GetPortalInfo(ed, nullptr))
				return ed;
			return nullptr;
		}

		// user doesn't want index - always prioritize player's env portal
		int envIdx = envPortalField.GetValue();
		if (envIdx != INVALID_EHANDLE_INDEX)
		{
			PortalInfo envInfo;
			edict_t* envEd = engine_server->PEntityOfEntIndex(envIdx & INDEX_MASK);
			if (GetPortalInfo(envEd, &envInfo) && PortalMatchesUserWish(envInfo))
				return envEd;
			else if (userWishes.collide)
				return nullptr;
		}
		else if (userWishes.collide)
		{
			return nullptr;
		}

		// player env doesn't match wish, let's check the last used index
		int lastUsedPortalIndex = cache.lastPortal.GetEntryIndex();
		if (lastUsedPortalIndex > 0)
		{
			edict_t* ed = engine_server->PEntityOfEntIndex(lastUsedPortalIndex);
			PortalInfo tmpInfo;
			if (GetPortalInfo(ed, &tmpInfo) && PortalMatchesUserWish(tmpInfo))
			{
				// check if the last used index refers to a different colored portal, saveloads can mess with that
				if (!userWishes.auto_)
					return ed;
				if (userWishes.auto_ && tmpInfo.isOrange == cache.lastPortalInfo.isOrange)
					return ed;
			}
		}

		// fine, go through all ents
		edict_t* bestMatch = nullptr;
		bool bestIsActivated = false;
		bool bestIsOpen = false;
		for (int i = 2; i < MAX_EDICTS; i++)
		{
			edict_t* tmpEd = engine_server->PEntityOfEntIndex(i);
			PortalInfo tmpInfo;
			if (GetPortalInfo(tmpEd, &tmpInfo) && PortalMatchesUserWish(tmpInfo))
			{
				// same color, pos, & ang? this is exactly what we want
				if (tmpInfo.isOrange == cache.lastPortalInfo.isOrange
				    && tmpInfo.pos == cache.lastPortalInfo.pos
				    && tmpInfo.ang == cache.lastPortalInfo.ang)
				{
					return tmpEd;
				}
				// otherwise, prioritize open & activated portals
				if (!bestMatch || (!bestIsActivated && tmpInfo.isActivated)
				    || (!bestIsOpen && tmpInfo.isOpen))
				{
					bestMatch = tmpEd;
					bestIsActivated = tmpInfo.isActivated;
					bestIsOpen = tmpInfo.isOpen;
				}
			}
		}
		return bestMatch; // couldn't find the same portal as was previously used, but maybe found something else
	}

	void OnMeshRenderSignal(MeshRendererDelegate& mr)
	{
		edict_t* portalEd = GetSgDrawPortal();
		if (!portalEd)
			return;
		CBaseHandle curPortal = portalEd->GetNetworkable()->GetEntityHandle()->GetRefEHandle();
		CBaseEntity* portalEnt = portalEd->GetIServerEntity()->GetBaseEntity();
		PortalInfo curInfo;
		GetPortalInfo(portalEd, &curInfo);

		if (cache.lastPortal != curPortal || cache.lastPortalInfo.pos != curInfo.pos
		    || cache.lastPortalInfo.ang != curInfo.ang)
		{
			// wish portal moved? clear cache
			cache.Clear();
		}
		else
		{
			// clear remote cache if the linked portal changed
			PortalInfo curLinkedInfo;
			edict_t* linkedEd = engine_server->PEntityOfEntIndex(curInfo.linked.GetEntryIndex());
			if (!GetPortalInfo(linkedEd, &curLinkedInfo))
				curLinkedInfo.isOpen = false;

			if (!curInfo.isOpen || cache.lastLinkedInfo.pos != curLinkedInfo.pos
			    || cache.lastLinkedInfo.ang != curLinkedInfo.ang
			    || cache.lastLinkedInfo.isOpen != curLinkedInfo.isOpen)
			{
				cache.remoteWorld.clear();
				cache.lastLinkedInfo = curLinkedInfo;
			}
		}
		cache.lastPortal = curPortal;
		cache.lastPortalInfo = curInfo;

		// collision simulator
		uintptr_t sim = (uintptr_t)portalEnt + portalFieldOffs.simulator;

		// draw portal hole when showing anything
		if (y_spt_draw_portal_env.GetBool() || y_spt_draw_portal_env_remote.GetBool()
		    || y_spt_draw_portal_env_ents.GetBool())
		{
			if (!cache.portalHole.Valid())
			{
				int numTris;
				auto verts = spt_collideToMesh.CreateCollideMesh(*(CPhysCollide**)(sim + 280), numTris);
				cache.portalHole = MB_STATIC(mb.AddTris(verts.get(), numTris, MC_PORTAL_HOLE););
			}
			mr.DrawMesh(cache.portalHole);
		}

		if (y_spt_draw_portal_env.GetBool())
			DrawLocalWorldGeometry(mr, sim);
		if (y_spt_draw_portal_env_remote.GetBool())
			DrawRemoteWorldGeometry(mr, sim);
		if (y_spt_draw_portal_env_ents.GetBool())
			DrawPortalEntities(mr, sim);
	}

	void DrawLocalWorldGeometry(MeshRendererDelegate& mr, uintptr_t sim)
	{
		if (!cache.localWorld.empty() && !cache.localWorld[0].Valid())
			cache.localWorld.clear();

		if (cache.localWorld.empty())
		{
			auto cacheLocalCollide = [this](const CPhysCollide* pCollide, const MeshColor& c)
			{
				int numTris;
				std::unique_ptr<Vector> verts = spt_collideToMesh.CreateCollideMesh(pCollide, numTris);
				if (verts.get() && numTris > 0)
					cache.localWorld.push_back(MB_STATIC(mb.AddTris(verts.get(), numTris, c);));
			};

			cacheLocalCollide(*(CPhysCollide**)(sim + 304), MC_LOCAL_WORLD_BRUSHES);
			cacheLocalCollide(*(CPhysCollide**)(sim + 376), MC_LOCAL_WALL_TUBE);
			cacheLocalCollide(*(CPhysCollide**)(sim + 404), MC_LOCAL_WALL_BRUSHES);
			const CUtlVector<char[28]>& staticProps = *(CUtlVector<char[28]>*)(sim + 332);
			for (int i = 0; i < staticProps.Count(); i++)
				cacheLocalCollide(*((CPhysCollide**)staticProps[i] + 2), MC_LOCAL_STATIC_PROPS);
		}
		for (const auto& staticMesh : cache.localWorld)
		{
			mr.DrawMesh(staticMesh,
			            [](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
			            { RenderCallbackZFightFix(infoIn, infoOut); });
		}
	}

	void DrawRemoteWorldGeometry(MeshRendererDelegate& mr, uintptr_t sim)
	{
		if (!cache.remoteWorld.empty() && !cache.remoteWorld[0].mesh.Valid())
			cache.remoteWorld.clear();

		auto cacheRemotePhysObj = [this](const IPhysicsObject* pPhysObj, const MeshColor& c)
		{
			if (!pPhysObj)
				return;
			int numTris;
			std::unique_ptr<Vector> verts = spt_collideToMesh.CreatePhysObjMesh(pPhysObj, numTris);
			if (verts.get() && numTris > 0)
			{
				matrix3x4_t mat;
				Vector pos;
				QAngle ang;
				pPhysObj->GetPosition(&pos, &ang);
				AngleMatrix(ang, pos, mat);
				cache.remoteWorld.emplace_back(mat, MB_STATIC(mb.AddTris(verts.get(), numTris, c);));
			}
		};

		if (cache.remoteWorld.empty())
		{
			cacheRemotePhysObj(*(IPhysicsObject**)(sim + 412), MC_REMOTE_WORLD_BRUSHES);
			const CUtlVector<IPhysicsObject*>& staticProps = *(CUtlVector<IPhysicsObject*>*)(sim + 416);
			for (int i = 0; i < staticProps.Count(); i++)
				cacheRemotePhysObj(staticProps[i], MC_REMOTE_STATIC_PROPS);
		}

		for (const auto& [mat, staticMesh] : cache.remoteWorld)
		{
			mr.DrawMesh(staticMesh,
			            [mat](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
			            {
				            infoOut.mat = mat;
				            RenderCallbackZFightFix(infoIn, infoOut);
			            });
		}
	}

	void DrawPortalEntities(MeshRendererDelegate& mr, uintptr_t sim)
	{
		auto drawEnt = [this, &mr](CBaseEntity* pEnt, MeshColor mc, bool outline)
		{
			if (!pEnt)
				return;

			IPhysicsObject* pPhysObj = spt_collideToMesh.GetPhysObj(pEnt);
			if (!pPhysObj)
				return;

			int idx = ((IHandleEntity*)pEnt)->GetRefEHandle().GetEntryIndex();
			auto& cachedEnt = cache.ents[idx][outline];

			// comparing two memory address to check for uniqueness, hopefully good enough
			if (cachedEnt.pEnt != pEnt || cachedEnt.pPhysObj != pPhysObj || !cachedEnt.mesh.Valid())
			{
				cachedEnt.pEnt = pEnt;
				cachedEnt.pPhysObj = pPhysObj;
				cachedEnt.mesh = spt_meshBuilder.CreateStaticMesh(
				    [=](MeshBuilderDelegate& mb)
				    {
					    int numTris;
					    auto verts = spt_collideToMesh.CreatePhysObjMesh(pPhysObj, numTris);
					    mb.AddTris(verts.get(), numTris, mc);
				    });
			}
			matrix3x4_t mat;
			Vector pos;
			QAngle ang;
			pPhysObj->GetPosition(&pos, &ang);
			AngleMatrix(ang, pos, mat);
			mr.DrawMesh(cachedEnt.mesh,
			            [=](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
			            {
				            infoOut.mat = mat;
				            RenderCallbackZFightFix(infoIn, infoOut);
				            if (outline)
					            infoOut.faces.skipRender = true;
				            else
					            infoOut.lines.skipRender = true;
			            });
		};

		auto drawAllEnts = [this, &drawEnt](CUtlVector<CBaseEntity*>& ents, MeshColor mc, bool outline)
		{
			for (int i = 0; i < ents.Size(); i++)
				drawEnt(ents[i], mc, outline);
		};

		drawAllEnts(*(CUtlVector<CBaseEntity*>*)(sim + 8664), MC_SHADOW_CLONES, true);
		drawAllEnts(*(CUtlVector<CBaseEntity*>*)(sim + 8684), MC_ENTS, true);

		if (!cache.lastPortalInfo.isOpen)
			return;

		/*
		* The player collides with entities far away from and in front of the portal because UTIL_Portal_TraceRay
		* uses a special entity enumerator. Specifically, an entity is considered solid to this trace if the
		* closest point on its OBB to a point 1007 units in front of the desired portal is in front of a plane
		* that is 7 units in front of the portal. It's a bit over-complicated and leads to some midly interesting
		* behavior for entities on the portal plane, but we gotta replicate it to show what we collide with.
		*/

		Vector portalPos = cache.lastPortalInfo.pos;
		Vector portalNorm;
		AngleVectors(cache.lastPortalInfo.ang, &portalNorm);
		Vector pt1007 = portalPos + portalNorm * 1007;

		VPlane testPlane{portalNorm, portalNorm.Dot(portalPos + portalNorm * 7)};

		for (int i = 2; i < MAX_EDICTS; i++)
		{
			edict_t* ed = interfaces::engine_server->PEntityOfEntIndex(i);
			if (!ed || !ed->GetIServerEntity())
				continue;
			CBaseEntity* pEnt = ed->GetIServerEntity()->GetBaseEntity();
			auto pCp = (CCollisionProperty*)((uint32_t)pEnt + portalFieldOffs.m_Collision);
			if (!pCp->IsSolid())
				continue;

			// optimization - if the bounding sphere doesn't intersect the plane then don't check the OBB

			SideType ballTest = testPlane.GetPointSide(pCp->WorldSpaceCenter(), pCp->BoundingRadius());

			if (ballTest == SIDE_BACK)
				continue;

			if (ballTest == SIDE_ON)
			{
				// can't use CCollisionProperty::CalcNearestPoint because SDK funny :/
				Vector localPt1007, localClosestTo1007, worldClosestTo1007;
				pCp->WorldToCollisionSpace(pt1007, &localPt1007);
				CalcClosestPointOnAABB(pCp->OBBMins(), pCp->OBBMaxs(), localPt1007, localClosestTo1007);
				pCp->CollisionToWorldSpace(localClosestTo1007, &worldClosestTo1007);

				if (testPlane.GetPointSideExact(worldClosestTo1007) == SIDE_BACK)
					continue;
			}

			bool isShadowClone = !strcmp(ed->GetClassName(), "physicsshadowclone");
			drawEnt(pEnt, isShadowClone ? MC_SHADOW_CLONES : MC_ENTS, false);
		}
	}
};

SgCollideVisFeature SgCollideVisFeature::featureInstance;

#endif
