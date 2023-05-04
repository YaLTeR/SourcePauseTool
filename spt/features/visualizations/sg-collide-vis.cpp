#include "stdafx.hpp"

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
#include "renderer\create_collide.hpp"

using interfaces::engine_server;

ConVar y_spt_draw_portal_env("y_spt_draw_portal_env",
                             "0",
                             FCVAR_CHEAT | FCVAR_DONTRECORD,
                             "Draw the geometry in a portal's physics environment; draws the following colors:\n"
                             "   - white: the portal hole\n"
                             "   - red: world geometry in front of the portal\n"
                             "   - blue: world geometry behind the portal\n"
                             "   - yellow: static props");

ConVar y_spt_draw_portal_env_type(
    "y_spt_draw_portal_env_type",
    "auto",
    FCVAR_CHEAT | FCVAR_DONTRECORD,
    "This determines what portal to use for all spt_draw_portal_* cvars. Accepts the following values:\n"
    "   - collide: draw what the player has collision with\n"
    "   - auto: prioritize what the player has collision with, otherwise use last drawn portal index\n"
    "   - blue/orange: look for specific portal color\n"
    "   - <index>: specify portal entity index");

ConVar y_spt_draw_portal_env_ents(
    "y_spt_draw_portal_env_ents",
    "0",
    FCVAR_CHEAT | FCVAR_DONTRECORD,
    "Draw entities owned by portal and shadow clones from remote portal; draws the following colors:\n"
    "   - white: the portal hole\n"
    "   - solid green: this entity collides with UTIL_Portal_TraceRay (and therefore so does the player)\n"
    "   - wireframe green: the portal owns this entity\n"
    "   - wireframe pink: this entity is a shadow clone specifically from the linked portal\n"
    "   - wireframe blue: this entity is a shadow clone");

ConVar y_spt_draw_portal_env_remote("y_spt_draw_portal_env_remote",
                                    "0",
                                    FCVAR_CHEAT | FCVAR_DONTRECORD,
                                    "Draw geometry from remote portal; draws the following colors:\n"
                                    "   - white: the portal hole\n"
                                    "   - light red: world geometry cloned from the linked portal\n"
                                    "   - light yellow: static props cloned from the linked portal");

#define SC_PORTAL_HOLE (ShapeColor{C_WIRE(255, 255, 255, 255)})
#define SC_LOCAL_WORLD_BRUSHES (ShapeColor{{255, 20, 20, 70}, {0, 0, 0, 255}})
#define SC_REMOTE_WORLD_BRUSHES (ShapeColor{{255, 150, 150, 15}, {0, 0, 0, 255}})
#define SC_LOCAL_WALL_TUBE (ShapeColor{{0, 255, 0, 200}, {0, 0, 0, 255}})
#define SC_LOCAL_WALL_BRUSHES (ShapeColor{{40, 40, 255, 60}, {0, 0, 0, 255}})
#define SC_LOCAL_STATIC_PROPS (ShapeColor{{255, 255, 40, 50}, {0, 0, 0, 255}})
#define SC_REMOTE_STATIC_PROPS (ShapeColor{{255, 255, 150, 15}, {0, 0, 0, 255}})

enum CachedEntFlags
{
	CEF_NONE = 0,
	CEF_OWNED = 1,                     // the portal owns this entity
	CEF_IS_CLONE_FROM_LINKED = 1 << 1, // this entity is a shadow clone from the linked portal
	CEF_IS_CLONE = 1 << 2,             // this entity is a shadow clone
	CEF_TRACE_COLLIDES = 1 << 3,       // UTIL_Portal_TraceRay collides with this entity
};

#define _GREEN_FACES _COLOR(0, 255, 0, 15)
#define _GREEN_LINES _COLOR(40, 255, 40, 255)
#define _PINK_LINES _COLOR(255, 100, 255, 255)
#define _BLUE_LINES _COLOR(100, 100, 255, 255)
#define _BLACK_LINES _COLOR(0, 0, 0, 255)

// must contain all valid combinations of flags
static const std::unordered_map<int, ShapeColor> entColors{
    // clone from our linked portal that hasn't entered portal hole
    {CEF_OWNED | CEF_IS_CLONE_FROM_LINKED, {C_WIRE(_PINK_LINES)}},
    {CEF_OWNED | CEF_IS_CLONE_FROM_LINKED | CEF_IS_CLONE, {C_WIRE(_PINK_LINES)}},
    // clone from our linked portal that has entered portal hole
    {CEF_OWNED | CEF_IS_CLONE_FROM_LINKED | CEF_TRACE_COLLIDES, {_GREEN_FACES, _PINK_LINES}},
    {CEF_OWNED | CEF_IS_CLONE_FROM_LINKED | CEF_TRACE_COLLIDES | CEF_IS_CLONE, {_GREEN_FACES, _PINK_LINES}},
    // entity in front of and far away from portal
    {CEF_TRACE_COLLIDES, {_GREEN_FACES, _BLACK_LINES}},
    // entity in front of and owned by portal
    {CEF_OWNED | CEF_TRACE_COLLIDES, {_GREEN_FACES, _GREEN_LINES}},
    // entity is owned by the portal but behind it (e.g. getting MSG)
    {CEF_OWNED, {C_WIRE(_GREEN_LINES)}},
    // clone (from any non-linked portal) that's in front of the portal
    {CEF_IS_CLONE | CEF_TRACE_COLLIDES, {_GREEN_FACES, _BLUE_LINES}},
};

#define PORTAL_CLASS "CProp_Portal"

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
			CachedEntFlags flags;

			void Clear()
			{
				pEnt = nullptr;
				pPhysObj = nullptr;
				mesh.Destroy();
				flags = CEF_NONE;
			}
		};

		StaticMesh portalHole;
		std::vector<StaticMesh> localWorld;
		std::vector<CachedRemoteGeo> remoteWorld;
		std::array<CachedEnt, MAX_EDICTS> ents{};

		CBaseHandle lastPortal{INVALID_EHANDLE_INDEX};
		PortalInfo lastPortalInfo{vec3_invalid};
		PortalInfo lastLinkedInfo{vec3_invalid};

		void Clear()
		{
			portalHole.Destroy();
			localWorld.clear();
			remoteWorld.clear();
			for (auto& cachedEnt : ents)
				cachedEnt.Clear();
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
		if (!spt_meshRenderer.signal.Works)
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
		for (int i = 1; i < MAX_EDICTS; i++)
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
				cache.portalHole = MB_STATIC(mb.AddTris(verts.get(), numTris, {SC_PORTAL_HOLE}););
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
			auto cacheLocalCollide = [this](const CPhysCollide* pCollide, const ShapeColor& c)
			{
				int numTris;
				std::unique_ptr<Vector> verts = spt_collideToMesh.CreateCollideMesh(pCollide, numTris);
				if (verts.get() && numTris > 0)
					cache.localWorld.push_back(MB_STATIC(mb.AddTris(verts.get(), numTris, c);));
			};

			cacheLocalCollide(*(CPhysCollide**)(sim + 304), SC_LOCAL_WORLD_BRUSHES);
			cacheLocalCollide(*(CPhysCollide**)(sim + 376), SC_LOCAL_WALL_TUBE);
			cacheLocalCollide(*(CPhysCollide**)(sim + 404), SC_LOCAL_WALL_BRUSHES);
			const CUtlVector<char[28]>& staticProps = *(CUtlVector<char[28]>*)(sim + 332);
			for (int i = 0; i < staticProps.Count(); i++)
				cacheLocalCollide(*((CPhysCollide**)staticProps[i] + 2), SC_LOCAL_STATIC_PROPS);
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

		auto cacheRemotePhysObj = [this](const IPhysicsObject* pPhysObj, const ShapeColor& c)
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
			cacheRemotePhysObj(*(IPhysicsObject**)(sim + 412), SC_REMOTE_WORLD_BRUSHES);
			const CUtlVector<IPhysicsObject*>& staticProps = *(CUtlVector<IPhysicsObject*>*)(sim + 416);
			for (int i = 0; i < staticProps.Count(); i++)
				cacheRemotePhysObj(staticProps[i], SC_REMOTE_STATIC_PROPS);
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
		if (!cache.lastPortalInfo.isOpen)
			return; // the sim's vectors are valid but empty and there's no point in drawing the trace test ents

		// we'll go through each ent and give it flags, then draw at the end
		int entFlags[MAX_EDICTS]{};

		auto addFlagsForSimEnts = [&entFlags](CUtlVector<CBaseEntity*>& entVec, CachedEntFlags entFlag)
		{
			for (int i = 0; i < entVec.Size(); i++)
			{
				if (!entVec[i])
					return;
				int idx = ((IHandleEntity*)entVec[i])->GetRefEHandle().GetEntryIndex();
				entFlags[idx] |= entFlag;
			}
		};

		addFlagsForSimEnts(*(CUtlVector<CBaseEntity*>*)(sim + 8664), CEF_IS_CLONE_FROM_LINKED);
		addFlagsForSimEnts(*(CUtlVector<CBaseEntity*>*)(sim + 8684), CEF_OWNED);

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

		for (int i = 1; i < MAX_EDICTS; i++)
		{
			edict_t* ed = interfaces::engine_server->PEntityOfEntIndex(i);
			if (!ed || !ed->GetIServerEntity())
				continue;
			CBaseEntity* pEnt = ed->GetIServerEntity()->GetBaseEntity();
			auto pCp = (CCollisionProperty*)((uint32_t)pEnt + portalFieldOffs.m_Collision);
			bool collisionSolid = pCp->IsSolid();

			// optimization - if the bounding sphere doesn't intersect the plane then don't check the OBB

			SideType ballTest = testPlane.GetPointSide(pCp->WorldSpaceCenter(), pCp->BoundingRadius());

			if (collisionSolid && ballTest == SIDE_FRONT)
				entFlags[i] |= CEF_TRACE_COLLIDES;

			if (collisionSolid && ballTest == SIDE_ON)
			{
				// can't use CCollisionProperty::CalcNearestPoint because SDK funny :/
				Vector localPt1007, localClosestTo1007, worldClosestTo1007;
				pCp->WorldToCollisionSpace(pt1007, &localPt1007);
				CalcClosestPointOnAABB(pCp->OBBMins(), pCp->OBBMaxs(), localPt1007, localClosestTo1007);
				pCp->CollisionToWorldSpace(localClosestTo1007, &worldClosestTo1007);

				if (testPlane.GetPointSideExact(worldClosestTo1007) == SIDE_FRONT)
					entFlags[i] |= CEF_TRACE_COLLIDES;
			}

			if (!strcmp(ed->GetClassName(), "physicsshadowclone"))
				entFlags[i] |= CEF_IS_CLONE;

			DrawSingleEntity(mr, pEnt, i, (CachedEntFlags)entFlags[i]);
		}
	}

	void DrawSingleEntity(MeshRendererDelegate& mr, CBaseEntity* pEnt, int entIndex, CachedEntFlags entFlags)
	{
		if (!pEnt || entFlags == CEF_NONE || entFlags == CEF_IS_CLONE)
			return;

		// don't draw just the trace test for the player, it's annoying and doesn't convey information
		if (entIndex == 1 && entFlags == CEF_TRACE_COLLIDES)
			return;

		IPhysicsObject* pPhysObj = spt_collideToMesh.GetPhysObj(pEnt);
		if (!pPhysObj)
			return;

		auto colorIt = entColors.find(entFlags);
		if (colorIt == entColors.end())
		{
			AssertMsg1(0, "sg-collide-vis: Attempting to look up color for invalid entFlags %d", entFlags);
			return;
		}

		auto& cachedEnt = cache.ents[entIndex];

		// comparing two memory address to check for uniqueness, hopefully good enough
		if (cachedEnt.pEnt != pEnt || cachedEnt.pPhysObj != pPhysObj || cachedEnt.flags != entFlags
		    || !cachedEnt.mesh.Valid())
		{
			cachedEnt.pEnt = pEnt;
			cachedEnt.pPhysObj = pPhysObj;
			cachedEnt.flags = entFlags;
			cachedEnt.mesh = spt_meshBuilder.CreateStaticMesh(
			    [=](MeshBuilderDelegate& mb)
			    {
				    int numTris;
				    auto verts = spt_collideToMesh.CreatePhysObjMesh(pPhysObj, numTris);
				    mb.AddTris(verts.get(), numTris, colorIt->second);
			    });
		}
		matrix3x4_t entMat;
		Vector pos;
		QAngle ang;
		pPhysObj->GetPosition(&pos, &ang);
		AngleMatrix(ang, pos, entMat);
		mr.DrawMesh(cachedEnt.mesh,
		            [entMat](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
		            {
			            infoOut.mat = entMat;
			            RenderCallbackZFightFix(infoIn, infoOut);
		            });
	}
};

SgCollideVisFeature SgCollideVisFeature::featureInstance;

#endif
