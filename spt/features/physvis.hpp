#pragma once

#include "..\feature.hpp"
#include "engine\iserverplugin.h"
#include "tier3\tier3.h"
#include "cdll_int.h"

extern const matrix3x4_t matrix3x4_identity;

#ifdef OE
typedef void IPhysicsCollision; // just for OE to compile
#endif
typedef void CPhysicsObject;

typedef int(__fastcall* _CPhysicsCollision__CreateDebugMesh)(const IPhysicsCollision* thisptr,
                                                             int dummy,
                                                             const CPhysCollide* pCollideModel,
                                                             Vector** outVerts);

typedef void(__fastcall* _CPhysicsCollision__DestroyDebugMesh)(const IPhysicsCollision* thisptr,
                                                               int dummy,
                                                               int vertCount,
                                                               Vector* outVerts);

typedef void(__fastcall* _CPhysicsObject__GetPosition)(const void* thisptr,
                                                       int dummy,
                                                       Vector* worldPosition,
                                                       QAngle* angles);

typedef void(__cdecl* _DebugDrawPhysCollide)(const CPhysCollide* pCollide,
                                             IMaterial* pMaterial,
                                             const matrix3x4_t& transform,
                                             const color32& color,
                                             bool drawAxes);

typedef void(__fastcall* _CRendering3dView__DrawTranslucentRenderables)(void* thisptr,
                                                                        int edx,
                                                                        bool bInSkybox,
                                                                        bool bShadowDepth);

/* 
* NOTE: the matrix will be applied to the verts in ORIG_DebugDrawPhysCollide AFTER this function is called, it
* should not be used for transforming the verts. A null matrix means that the identity mat will be used. The
* vertex array will be allocated by the game's allocator, so if you're creating a new one make sure to free the
* old one and allocate the new one yourself using g_pMemAlloc (regular new/delete should default to this); the
* one used will be freed by the game after the mesh is rendered.
*/
typedef int (*_EditMeshFunc)(Vector*& verts, int numVerts, const matrix3x4_t* mat);

typedef struct
{
	std::function<void()> render;
	std::string sortKey;
} PhysVisCallback;

enum WireframeMode
{
	OnlyFaces,
	OnlyWire,
	FacesAndWire // color is used for faces, black wireframe
};

struct PhysVisDrawInfo
{
	WireframeMode wireframeMode;
	color32 color;
	// set this if you're editing the mesh created by CreateDebugMesh (or your own custom mesh func)
	_EditMeshFunc EditMeshFunc = nullptr;
};

/*
* This is akin to an advanced version of the debug overlay - the primary purpose of this is
* for drawing individual meshes. It hooks the same system that is used by vcollide_wireframe.
* It's not as fast as the main engine rendering system, but it's damn well fast enough for
* many needs. Note that meshes are unlit, render order matters, and rendering too many or using
* large meshes will crash the game (especially when looking through portals or going OOB).
*/
class PhysVisFeature : public FeatureWrapper<PhysVisFeature>
{
public:
	/*
	* The callbacks get called every frame. The sort key determines the order that the callbacks
	* are rendered in, "higher" keys (e.g. 'z') are rendered last and therefore on top.
	*/
	bool AddPhysVisCallback(PhysVisCallback callback);
	virtual bool ShouldLoadFeature() override;

	_CPhysicsCollision__CreateDebugMesh ORIG_CPhysicsCollision__CreateDebugMesh = nullptr;
	_CPhysicsCollision__CreateDebugMesh ORIG_CPhysicsCollision__DestroyDebugMesh = nullptr;
	_CPhysicsObject__GetPosition ORIG_CPhysicsObject__GetPosition = nullptr;

	// These functions can only be called from a PhysVisCallback. The first 3 allow the game to
	// allocate and free the verts array automatically.

	static void DrawCPhysCollide(const CPhysCollide* pCollide,
	                             const PhysVisDrawInfo& info,
	                             const matrix3x4_t* mat = nullptr);

	static void DrawCPhysicsObject(const CPhysicsObject* pPhysicsObject, const PhysVisDrawInfo& info);

	static void DrawCBaseEntity(const CBaseEntity* pEnt, const PhysVisDrawInfo& info);

	// this one does not free the verts array

	static void DrawCustomMesh(Vector* verts,
	                           int numVerts,
	                           const PhysVisDrawInfo& info,
	                           const matrix3x4_t* mat = nullptr);

protected:
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;
	virtual void PreHook() override;

private:
	static int __fastcall HOOKED_CPhysicsCollision__CreateDebugMesh(const IPhysicsCollision* thisptr,
	                                                                int dummy,
	                                                                const CPhysCollide* pCollideModel,
	                                                                Vector** outVerts);

	static void __fastcall HOOKED_CPhysicsCollision__DestroyDebugMesh(const IPhysicsCollision* thisptr,
	                                                                  int dummy,
	                                                                  int vertCount,
	                                                                  Vector* outVerts);

	static void __fastcall HOOKED_CRendering3dView__DrawTranslucentRenderables(void* thisptr,
	                                                                           int edx,
	                                                                           bool bInSkybox,
	                                                                           bool bShadowDepth);

	_DebugDrawPhysCollide ORIG_DebugDrawPhysCollide = nullptr;
	_CRendering3dView__DrawTranslucentRenderables ORIG_CRendering3dView__DrawTranslucentRenderables = nullptr;

	static void DrawPhysCollideWrapper(const CPhysCollide* pCollide,
	                                   const PhysVisDrawInfo& info,
	                                   const matrix3x4_t* m);

	std::vector<PhysVisCallback> callbacks;

	struct
	{
		bool renderingCallbacks = false;
		const PhysVisDrawInfo* curDrawInfo = nullptr;
		const matrix3x4_t* curMat = nullptr;
		// if drawing a custom mesh
		Vector* verts = nullptr;
		int numVerts;
	} renderState;

	bool loadingSuccessful = false;
};

extern PhysVisFeature spt_physvis;
