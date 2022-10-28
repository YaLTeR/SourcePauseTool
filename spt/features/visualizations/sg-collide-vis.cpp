#include "stdafx.h"

#include "renderer\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

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

// clang-format off
#if 1 // 1 - black outline, 0 - default outline
#define MC_PORTAL_HOLE MeshColor::Wire({20, 255, 20, 255})
#define MC_LOCAL_WORLD_BRUSHES MeshColor{{255, 20, 20, 70}, {0, 0, 0, 255}}
#define MC_REMOTE_WORLD_BRUSHES MeshColor{{255, 150, 150, 15}, {0, 0, 0, 255}}
#define MC_LOCAL_WALL_TUBE MeshColor{{0, 255, 0, 200}, {0, 0, 0, 255}}
#define MC_LOCAL_WALL_BRUSHES MeshColor{{40, 40, 255, 60}, {0, 0, 0, 255}}
#define MC_LOCAL_STATIC_PROPS MeshColor{{255, 255, 40, 50}, {0, 0, 0, 255}}
#define MC_REMOTE_STATIC_PROPS MeshColor{{255, 255, 150, 15}, {0, 0, 0, 255}}
#define MC_OWNED_ENTS MeshColor::Wire({255, 100, 255, 255})
#define MC_SHADOW_CLONES MeshColor::Wire({255, 255, 255, 255})
#else
#define MC_PORTAL_HOLE MeshColor::Wire({20, 255, 20, 255})
#define MC_LOCAL_WORLD_BRUSHES MeshColor::Outline({255, 20, 20, 70})
#define MC_REMOTE_WORLD_BRUSHES MeshColor::Outline({255, 150, 150, 15})
#define MC_LOCAL_WALL_TUBE MeshColor::Outline({0, 255, 0, 200})
#define MC_LOCAL_WALL_BRUSHES MeshColor::Outline({40, 40, 255, 60})
#define MC_LOCAL_STATIC_PROPS MeshColor::Outline({255, 255, 40, 50})
#define MC_REMOTE_STATIC_PROPS MeshColor::Outline({255, 255, 150, 15})
#define MC_OWNED_ENTS MeshColor::Wire({255, 100, 255, 255})
#define MC_SHADOW_CLONES MeshColor::Wire({255, 255, 255, 255})
#endif
// clang-format on

#define PORTAL_CLASS "CProp_Portal"

// Since we're rendering geometry directly on top of the world, we get a LOT of z-fighting by default.
// I used to fix this by extruding each triangle towards its normal by some amount. With the current
// callback system the way I fix this is to shrink the mesh towards the camera by this factor.
#define CALLBACK_SHRINK_FACTOR 0.9995f

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
	} portalFieldOffs;

	PlayerField<int> envPortalField;

	struct PortalInfo
	{
		Vector pos;
		QAngle ang;
		CBaseHandle linked;
		bool isOrange, isActivated, isOpen;
	};

	CBaseHandle lastPortal{INVALID_EHANDLE_INDEX};
	PortalInfo lastPortalInfo{vec3_invalid};
	PortalInfo lastLinkedInfo{vec3_invalid};

	struct
	{
		bool collide, _auto, blue, orange, idx;
		int idxVal;
	} userWishes;

	struct
	{
		std::vector<StaticMesh> localWorld;
		std::vector<std::pair<matrix3x4_t, StaticMesh>> remoteWorld;

		void Clear()
		{
			localWorld.clear();
			remoteWorld.clear();
		}
	} cache;

	virtual bool ShouldLoadFeature() override
	{
		return utils::DoesGameLookLikePortal();
	}

	virtual void LoadFeature() override
	{
		if (!spt_collideToMesh.Works())
			return;

		// cache all field stuff & check that spt_entutils works
		portalFieldOffs.pos = spt_entutils.GetFieldOffset(PORTAL_CLASS, "m_vecAbsOrigin", true);
		portalFieldOffs.ang = spt_entutils.GetFieldOffset(PORTAL_CLASS, "m_angAbsRotation", true);
		portalFieldOffs.isOrange = spt_entutils.GetFieldOffset(PORTAL_CLASS, "m_bIsPortal2", true);
		portalFieldOffs.linked = spt_entutils.GetFieldOffset(PORTAL_CLASS, "m_hLinkedPortal", true);
		portalFieldOffs.isActivated = spt_entutils.GetFieldOffset(PORTAL_CLASS, "m_bActivated", true);
		portalFieldOffs.simulator = spt_entutils.GetFieldOffset(PORTAL_CLASS, "m_vPortalCorners", true);

		for (int i = 0; i < sizeof(portalFieldOffs) / sizeof(int); i++)
			if (reinterpret_cast<int*>(&portalFieldOffs)[i] == utils::INVALID_DATAMAP_OFFSET)
				return;
		envPortalField = spt_entutils.GetPlayerField<int>("m_hPortalEnvironment", PropMode::Server);
		if (!envPortalField.ServerOffsetFound())
			return;

		portalFieldOffs.simulator += sizeof(Vector) * 4;

		if (!MeshRenderSignal.Works)
			return;

		MeshRenderSignal.Connect(this, &SgCollideVisFeature::OnMeshRenderSignal);
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
			    wish._auto = !_stricmp(typeStr, "auto");
			    wish.blue = !_stricmp(typeStr, "blue") || wish._auto;
			    wish.orange = !_stricmp(typeStr, "orange") || wish._auto;
			    if (!wish.collide && !wish._auto && !wish.blue && !wish.orange)
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
		int lastUsedPortalIndex = lastPortal.GetEntryIndex();
		if (lastUsedPortalIndex > 0)
		{
			edict_t* ed = engine_server->PEntityOfEntIndex(lastUsedPortalIndex);
			PortalInfo tmpInfo;
			if (GetPortalInfo(ed, &tmpInfo) && PortalMatchesUserWish(tmpInfo))
			{
				// check if the last used index refers to a different colored portal, saveloads can mess with that
				if (!userWishes._auto)
					return ed;
				if (userWishes._auto && tmpInfo.isOrange == lastPortalInfo.isOrange)
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
				if (tmpInfo.isOrange == lastPortalInfo.isOrange && tmpInfo.pos == lastPortalInfo.pos
				    && tmpInfo.ang == lastPortalInfo.ang)
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

	void OnMeshRenderSignal(MeshRenderer& mr)
	{
		edict_t* portalEd = GetSgDrawPortal();
		if (!portalEd)
			return;
		CBaseHandle curPortal = portalEd->GetNetworkable()->GetEntityHandle()->GetRefEHandle();
		CBaseEntity* portalEnt = portalEd->GetIServerEntity()->GetBaseEntity();
		PortalInfo curInfo;
		GetPortalInfo(portalEd, &curInfo);
		if (lastPortal != curPortal || lastPortalInfo.pos != curInfo.pos || lastPortalInfo.ang != curInfo.ang)
			cache.Clear();
		lastPortal = curPortal;
		lastPortalInfo = curInfo;

		uintptr_t sim = (uintptr_t)portalEnt + portalFieldOffs.simulator;

		// PS_SD_Static_World_StaticProps_ClippedProp_t
		struct ClippedProp
		{
			char __pad1[8];
			CPhysCollide* collide;
			char __pad2[16];
		};

		if (y_spt_draw_portal_env.GetBool())
		{
			if (cache.localWorld.size() == 0)
			{
				CacheLocalCollide(*(CPhysCollide**)(sim + 280), MC_PORTAL_HOLE);
				CacheLocalCollide(*(CPhysCollide**)(sim + 304), MC_LOCAL_WORLD_BRUSHES);
				CacheLocalCollide(*(CPhysCollide**)(sim + 376), MC_LOCAL_WALL_TUBE);
				CacheLocalCollide(*(CPhysCollide**)(sim + 404), MC_LOCAL_WALL_BRUSHES);
				const CUtlVector<ClippedProp>& staticProps = *(CUtlVector<ClippedProp>*)(sim + 332);
				for (int i = 0; i < staticProps.Count(); i++)
					CacheLocalCollide(staticProps[i].collide, MC_LOCAL_STATIC_PROPS);
			}
			for (const auto& staticMesh : cache.localWorld)
			{
				mr.DrawMesh(
				    staticMesh,
				    [](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
				    {
					    matrix3x4_t scaleMat;
					    SetScaleMatrix(CALLBACK_SHRINK_FACTOR, scaleMat);
					    matrix3x4_t transMat({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, infoIn.cvs.origin);
					    // move to camera, scale towards <0,0,0>, move back
					    infoOut.mat.Init({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, -infoIn.cvs.origin);
					    MatrixMultiply(scaleMat, infoOut.mat, infoOut.mat);
					    MatrixMultiply(transMat, infoOut.mat, infoOut.mat);
				    });
			}
		}
		if (y_spt_draw_portal_env_remote.GetBool())
		{
			// invalidate remote cache if the linked portal changed
			PortalInfo curLinkedInfo;
			edict_t* linkedEd = engine_server->PEntityOfEntIndex(curInfo.linked.GetEntryIndex());
			if (!GetPortalInfo(linkedEd, &curLinkedInfo))
				curLinkedInfo.isOpen = false;
			if (!curInfo.isOpen || lastLinkedInfo.pos != curLinkedInfo.pos
			    || lastLinkedInfo.ang != curLinkedInfo.ang || lastLinkedInfo.isOpen != curLinkedInfo.isOpen)
			{
				cache.remoteWorld.clear();
				lastLinkedInfo = curLinkedInfo;
			}

			if (cache.remoteWorld.size() == 0)
			{
				CacheRemoteCPhysObj(*(CPhysicsObject**)(sim + 412), MC_REMOTE_WORLD_BRUSHES);
				const auto& staticProps = *(CUtlVector<CPhysicsObject*>*)(sim + 416);
				for (int i = 0; i < staticProps.Count(); i++)
					CacheRemoteCPhysObj(staticProps[i], MC_REMOTE_STATIC_PROPS);
			}
			for (const auto& [matRef, staticMesh] : cache.remoteWorld)
			{
				matrix3x4_t mat{matRef};
				mr.DrawMesh(
				    staticMesh,
				    [mat](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
				    {
					    matrix3x4_t iTransMat({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, -infoIn.cvs.origin);
					    matrix3x4_t scaleMat;
					    SetScaleMatrix(CALLBACK_SHRINK_FACTOR, scaleMat);
					    matrix3x4_t transMat({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, infoIn.cvs.origin);
					    // apply CPhysicsObject matrix, then do the same steps as for the collides
					    infoOut.mat = mat;
					    MatrixMultiply(iTransMat, infoOut.mat, infoOut.mat);
					    MatrixMultiply(scaleMat, infoOut.mat, infoOut.mat);
					    MatrixMultiply(transMat, infoOut.mat, infoOut.mat);
				    });
			}
		}
		if (y_spt_draw_portal_env_ents.GetBool())
		{
			// not caching ents since the model may change for e.g. players crouching, not a significant issue

			auto drawEnts = [&mr](const CUtlVector<CBaseEntity*>& ents, const MeshColor& c)
			{
				for (int i = 0; i < ents.Size(); i++)
				{
					int numTris;
					matrix3x4_t mat;
					auto verts = spt_collideToMesh.CreateEntMesh(ents[i], numTris, mat);
					auto mesh = MB_DYNAMIC(mb.AddTris(verts.get(), numTris, c););
					mr.DrawMesh(mesh, [mat](auto&, auto& infoOut) { infoOut.mat = mat; });
				}
			};
			drawEnts(*(CUtlVector<CBaseEntity*>*)(sim + 8684), MC_OWNED_ENTS);
			drawEnts(*(CUtlVector<CBaseEntity*>*)(sim + 8664), MC_SHADOW_CLONES);
		}
	}

	void CacheLocalCollide(const CPhysCollide* pCollide, const MeshColor& c)
	{
		int numTris;
		std::unique_ptr<Vector> verts = spt_collideToMesh.CreateCollideMesh(pCollide, numTris);
		if (verts.get() && numTris > 0)
			cache.localWorld.push_back(MB_STATIC(mb.AddTris(verts.get(), numTris, c);));
	}

	void CacheRemoteCPhysObj(const CPhysicsObject* pPhysObj, const MeshColor& c)
	{
		int numTris;
		matrix3x4_t mat;
		std::unique_ptr<Vector> verts = spt_collideToMesh.CreateCPhysObjMesh(pPhysObj, numTris, mat);
		if (verts.get() && numTris > 0)
			cache.remoteWorld.emplace_back(mat, MB_STATIC(mb.AddTris(verts.get(), numTris, c);));
	}
};

SgCollideVisFeature SgCollideVisFeature::featureInstance;

#endif
