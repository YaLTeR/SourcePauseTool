#include "stdafx.h"
#include "physvis.hpp"
#include "interfaces.hpp"
#include "materialsystem\imaterialsystem.h"

const matrix3x4_t matrix3x4_identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);

PhysVisFeature spt_physvis;

bool PhysVisFeature::AddPhysVisCallback(PhysVisCallback callback)
{
	if (!loadingSuccessful)
		return false;
	callbacks.push_back(callback);
	std::sort(callbacks.begin(), callbacks.end(), [](PhysVisCallback& lhs, PhysVisCallback& rhs) {
		return lhs.sortKey < rhs.sortKey;
	});
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
