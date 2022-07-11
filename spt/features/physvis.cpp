#include "stdafx.h"
#include "physvis.hpp"
#include "interfaces.hpp"
#include "materialsystem\imaterialsystem.h"

const matrix3x4_t matrix3x4_identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);

PhysVisFeature spt_physvis;

namespace patterns
{
	namespace engine
	{
		PATTERNS(DebugDrawPhysCollide,
		         "5135",
		         "81 EC 38 02 00 00 53 55 33 DB 39 9C 24 48 02 00 00 56 57 75 24",
		         "1910503",
		         "55 8B EC 81 EC 3C 02 00 00 53 33 DB 56 57 39 5D 0C 75 20");
	}

	namespace client
	{
		PATTERNS(CRendering3dView__DrawTranslucentRenderables,
		         "5135",
		         "55 8B EC 83 EC 34 53 8B D9 8B 83 94 00 00 00 8B 13 56 8D B3 94 00 00 00",
		         "1910503",
		         "55 8B EC 81 EC 9C 00 00 00 53 56 8B F1 8B 86 E8 00 00 00 8B 16 57 8D BE E8 00 00 00");
	}

	namespace vphysics
	{
		PATTERNS(CPhysicsCollision__CreateDebugMesh,
		         "5135",
		         "83 EC 10 8B 4C 24 14 8B 01 8B 40 08 55 56 57 33 ED 8D 54 24 10 52",
		         "1910503",
		         "55 8B EC 83 EC 14 8B 4D 08 8B 01 8B 40 08 53 56 57 33 DB 8D 55 EC");
		PATTERNS(CPhysicsCollision__DestroyDebugMesh,
		         "5135",
		         "8B 44 24 08 50 E8 ?? ?? ?? ?? 59 C2 08 00",
		         "1910503",
		         "55 8B EC 8B 45 0C 50 E8 ?? ?? ?? ?? 83 C4 04 5D C2 08 00");
		PATTERNS(CPhysicsObject__GetPosition,
		         "5135",
		         "8B 49 08 81 EC 80 00 00 00 8D 04 24 50 E8 ?? ?? ?? ?? 8B 84 24 84 00 00 00 85 C0",
		         "1910503",
		         "55 8B EC 8B 49 08 81 EC 80 00 00 00 8D 45 80 50 E8 ?? ?? ?? ?? 8B 45 08 85 C0");
	} // namespace vphysics
} // namespace patterns

bool PhysVisFeature::AddPhysVisCallback(PhysVisCallback callback)
{
	if (!loadingSuccessful)
		return false;
	callbacks.push_back(callback);
	std::sort(callbacks.begin(),
	          callbacks.end(),
	          [](PhysVisCallback& lhs, PhysVisCallback& rhs) { return lhs.sortKey < rhs.sortKey; });
	return true;
}

bool PhysVisFeature::ShouldLoadFeature()
{
#if defined(SSDK2007) || defined(SSDK2013)
	return interfaces::materialSystem;
#else
	return false;
#endif
}

void PhysVisFeature::InitHooks()
{
	FIND_PATTERN(vphysics, CPhysicsObject__GetPosition);
	FIND_PATTERN(engine, DebugDrawPhysCollide);
	HOOK_FUNCTION(vphysics, CPhysicsCollision__CreateDebugMesh);
	HOOK_FUNCTION(vphysics, CPhysicsCollision__DestroyDebugMesh);
	HOOK_FUNCTION(client, CRendering3dView__DrawTranslucentRenderables);
}

void PhysVisFeature::LoadFeature() {}

void PhysVisFeature::UnloadFeature() {}

void PhysVisFeature::PreHook()
{
	loadingSuccessful = ORIG_CPhysicsObject__GetPosition && ORIG_DebugDrawPhysCollide
	                    && ORIG_CPhysicsCollision__CreateDebugMesh && ORIG_CPhysicsCollision__DestroyDebugMesh
	                    && ORIG_CRendering3dView__DrawTranslucentRenderables;
}

int __fastcall PhysVisFeature::HOOKED_CPhysicsCollision__CreateDebugMesh(const IPhysicsCollision* thisptr,
                                                                         int dummy,
                                                                         const CPhysCollide* pCollideModel,
                                                                         Vector** outVerts)
{
	auto& state = spt_physvis.renderState;
	int numVerts;
	if (state.verts)
	{
		numVerts = state.numVerts;
		*outVerts = state.verts;
	}
	else
	{
		numVerts = spt_physvis.ORIG_CPhysicsCollision__CreateDebugMesh(thisptr, dummy, pCollideModel, outVerts);
	}
	if (state.curDrawInfo && state.curDrawInfo->EditMeshFunc)
		numVerts = state.curDrawInfo->EditMeshFunc(*outVerts, numVerts, state.curMat);

	return numVerts;
}

void __fastcall PhysVisFeature::HOOKED_CPhysicsCollision__DestroyDebugMesh(const IPhysicsCollision* thisptr,
                                                                           int dummy,
                                                                           int vertCount,
                                                                           Vector* outVerts)
{
	if (!spt_physvis.renderState.verts)
		g_pMemAlloc->Free(outVerts); // only free if we're not drawing a custom mesh
}

void __fastcall PhysVisFeature::HOOKED_CRendering3dView__DrawTranslucentRenderables(void* thisptr,
                                                                                    int edx,
                                                                                    bool bInSkybox,
                                                                                    bool bShadowDepth)
{
	spt_physvis.ORIG_CRendering3dView__DrawTranslucentRenderables(thisptr, edx, bInSkybox, bShadowDepth);
	// this seems to be the best spot in the pipeline to render meshes, otherwise stuff gets drawn on top of them
	spt_physvis.renderState.renderingCallbacks = true;
	for (auto& callback : spt_physvis.callbacks)
		callback.render();
	spt_physvis.renderState.renderingCallbacks = false;
}

void PhysVisFeature::DrawPhysCollideWrapper(const CPhysCollide* pCollide,
                                            const PhysVisDrawInfo& info,
                                            const matrix3x4_t* m)
{
	spt_physvis.renderState.curMat = m;
	spt_physvis.renderState.curDrawInfo = &info;
	auto& actualMat = m ? *m : matrix3x4_identity;
	color32 color = info.color;
	switch (info.wireframeMode)
	{
	case OnlyFaces:
	case FacesAndWire:
	{
		const char* faceMatName = "debug/debugtranslucentvertexcolor";
		IMaterial* material = interfaces::materialSystem->FindMaterial(faceMatName, TEXTURE_GROUP_OTHER);
		spt_physvis.ORIG_DebugDrawPhysCollide(pCollide, material, actualMat, color, false);
		if (info.wireframeMode == OnlyFaces)
			break;
		color = {0, 0, 0, 250}; // fall through and use a black wireframe
	}
	case OnlyWire:
		spt_physvis.ORIG_DebugDrawPhysCollide(pCollide, nullptr, actualMat, color, false);
	}
	spt_physvis.renderState.curDrawInfo = nullptr;
	spt_physvis.renderState.curMat = nullptr;
}

void PhysVisFeature::DrawCPhysCollide(const CPhysCollide* pCollide, const PhysVisDrawInfo& info, const matrix3x4_t* mat)
{
	if (!spt_physvis.renderState.renderingCallbacks || !pCollide)
		return;
	DrawPhysCollideWrapper(pCollide, info, mat);
}

void PhysVisFeature::DrawCPhysicsObject(const CPhysicsObject* pPhysicsObject, const PhysVisDrawInfo& info)
{
	if (!spt_physvis.renderState.renderingCallbacks || !pPhysicsObject)
		return;
	Vector pos;
	QAngle ang;
	matrix3x4_t mat;
	spt_physvis.ORIG_CPhysicsObject__GetPosition(pPhysicsObject, 0, &pos, &ang);
	AngleMatrix(ang, pos, mat);
	DrawCPhysCollide(*((CPhysCollide**)pPhysicsObject + 3), info, &mat);
}

void PhysVisFeature::DrawCBaseEntity(const CBaseEntity* pEnt, const PhysVisDrawInfo& info)
{
	if (!spt_physvis.renderState.renderingCallbacks || !pEnt)
		return;
#if defined(SSDK2007)
	DrawCPhysicsObject(*((void**)pEnt + 106), info);
#elif defined(SSDK2013)
	DrawCPhysicsObject(*((void**)pEnt + 120), info);
#endif
}

void PhysVisFeature::DrawCustomMesh(Vector* verts, int numVerts, const PhysVisDrawInfo& info, const matrix3x4_t* mat)
{
	if (!spt_physvis.renderState.renderingCallbacks || !verts)
		return;
	auto& state = spt_physvis.renderState;
	state.verts = verts;
	state.numVerts = numVerts;
	state.curDrawInfo = &info;
	DrawPhysCollideWrapper(nullptr, info, mat);
	state.curDrawInfo = nullptr;
	state.verts = nullptr;
}
