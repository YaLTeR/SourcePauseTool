#include "stdafx.h"

#include "mesh_defs_public.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include <algorithm>

#include "mesh_renderer.hpp"
#include "mesh_defs_private.hpp"
#include "interfaces.hpp"
#include "signals.hpp"
#include "spt\features\overlay.hpp"

#define VPROF_BUDGETGROUP_MESH_RENDERER _T("Mesh_Renderer")

ConVar y_spt_draw_mesh_debug("y_spt_draw_mesh_debug",
                             "0",
                             FCVAR_CHEAT | FCVAR_DONTRECORD,
                             "Draws the distance metric and AABBs of all meshes.");

/*
* We take advantage of two passes the game does: DrawOpaques and DrawTranslucents. Both are called for each view,
* and you may get a different view for each portal, saveglitch overlay, etc. We abstract this away from the user -
* when the user calls DrawMesh() we defer the drawing until those two passes. When rendering opaques, we can
* render the meshes in any order, but the same is not true for translucents. In that case we must figure out our
* own render order. Nothing we do will be perfect - but a good enough solution for many cases is to approximate
* the distance to each mesh from the camera as a single value and sort based on that.
* 
* TODO - try combining opaques & translucents into one mesh before rendering
* 
* This system would not exist without the absurd of times mlugg has helped me while making it; send him lots of
* love, gifts, and fruit baskets.
*/

/**************************************** OVERLAY FEATURE ****************************************/

struct CPortalRender
{
	byte _pad[0x57c];
	int m_iViewRecursionLevel;
};

namespace patterns
{
	PATTERNS(CRendering3dView__DrawOpaqueRenderables, "5135", "55 8D 6C 24 8C 81 EC 94 00 00 00");
	PATTERNS(CRendering3dView__DrawTranslucentRenderables,
	         "5135",
	         "55 8B EC 83 EC 34 53 8B D9 8B 83 94 00 00 00 8B 13 56 8D B3 94 00 00 00",
	         "1910503",
	         "55 8B EC 81 EC 9C 00 00 00 53 56 8B F1 8B 86 E8 00 00 00 8B 16 57 8D BE E8 00 00 00");
	PATTERNS(OnRenderStart, "5135", "56 8B 35 ?? ?? ?? ?? 8B 06 8B 50 64 8B CE FF D2 8B 0D ?? ?? ?? ??");
} // namespace patterns

extern class MeshRendererFeature g_meshRenderer;

class MeshRendererFeature : public FeatureWrapper<MeshRendererFeature>
{
public:
	CPortalRender** g_pPortalRender = nullptr;
	std::vector<struct MeshWrapperInternal> deferredMeshWrappers;

private:
	virtual bool ShouldLoadFeature() override
	{
		if (!interfaces::materialSystem)
		{
			DevWarning("Mesh rendering not available because materialSystem was not initialized!\n");
			return false;
		}
		return true;
	}

	virtual void InitHooks() override
	{
		HOOK_FUNCTION(client, CRendering3dView__DrawOpaqueRenderables);
		HOOK_FUNCTION(client, CRendering3dView__DrawTranslucentRenderables);
		FIND_PATTERN(client, OnRenderStart);
	}

	virtual void PreHook() override
	{
		/*
		* 1) InitHooks: spt_overlay finds the render function.
		* 2) PreHook: spt_overlay may connect the render signal. To not depend on feature load order
		*    we cannot check if the signal exists, but we can check if the render function was found.
		* 3) LoadFeature: Anything that uses the mesh rendering system can check to see if the signal exists.
		*/
		if (ORIG_CRendering3dView__DrawOpaqueRenderables && ORIG_CRendering3dView__DrawTranslucentRenderables
		    && spt_overlay.ORIG_CViewRender__Render)
		{
			MeshRenderSignal.Works = true;
		}
		else
		{
			return;
		}
		if (ORIG_OnRenderStart)
			g_pPortalRender = *(CPortalRender***)((uintptr_t)ORIG_OnRenderStart + 18);

		g_meshBuilderMaterials.Load();
	}

	virtual void LoadFeature() override
	{
		// if the mesh render signal works, we've set it in PreHook() and the RenderSignal will definitely work
		if (MeshRenderSignal.Works)
		{
			Assert(RenderSignal.Works);
			InitConcommandBase(y_spt_draw_mesh_debug);
			RenderSignal.Connect(this, &MeshRendererFeature::OnRenderSignal);
		}
	}

	virtual void UnloadFeature() override
	{
		g_meshBuilderMaterials.Unload();
	}

	DECL_MEMBER_CDECL(void, OnRenderStart);

	DECL_HOOK_THISCALL(void, CRendering3dView__DrawOpaqueRenderables, bool bShadowDepth)
	{
		// render order shouldn't matter here
		g_meshRenderer.ORIG_CRendering3dView__DrawOpaqueRenderables(thisptr, 0, bShadowDepth);
		g_meshRenderer.OnDrawOpaques(thisptr);
	}

	DECL_HOOK_THISCALL(void, CRendering3dView__DrawTranslucentRenderables, bool bInSkybox, bool bShadowDepth)
	{
		// render order matters here, render our stuff on top of other translucents
		g_meshRenderer.ORIG_CRendering3dView__DrawTranslucentRenderables(thisptr, 0, bInSkybox, bShadowDepth);
		g_meshRenderer.OnDrawTranslucents(thisptr);
	}

	void OnRenderSignal(void* thisptr, vrect_t*);
	void OnDrawOpaques(void* renderingView);
	void OnDrawTranslucents(void* renderingView);
};

MeshRendererFeature g_meshRenderer;

/**************************************** MATERIALS MANAGER ****************************************/

void MeshBuilderMatMgr::Load()
{
	KeyValues* kv;

	kv = new KeyValues("unlitgeneric");
	kv->SetInt("$vertexcolor", 1);
	matOpaque = interfaces::materialSystem->CreateMaterial("_spt_UnlitOpaque", kv);

	kv = new KeyValues("unlitgeneric");
	kv->SetInt("$vertexcolor", 1);
	kv->SetInt("$vertexalpha", 1);
	matAlpha = interfaces::materialSystem->CreateMaterial("_spt_UnlitTranslucent", kv);

	kv = new KeyValues("unlitgeneric");
	kv->SetInt("$vertexcolor", 1);
	kv->SetInt("$vertexalpha", 1);
	kv->SetInt("$ignorez", 1);
	matAlphaNoZ = interfaces::materialSystem->CreateMaterial("_spt_UnlitTranslucentNoZ", kv);

	materialsInitialized = matOpaque && matAlpha && matAlphaNoZ;
	if (!materialsInitialized)
		matOpaque = matAlpha = matAlphaNoZ = nullptr;
}

void MeshBuilderMatMgr::Unload()
{
	materialsInitialized = false;
	if (matOpaque)
		matOpaque->DecrementReferenceCount();
	if (matAlpha)
		matAlpha->DecrementReferenceCount();
	if (matAlphaNoZ)
		matAlphaNoZ->DecrementReferenceCount();
	matOpaque = matAlpha = matAlphaNoZ = nullptr;
}

IMaterial* MeshBuilderMatMgr::GetMaterial(bool opaque, bool zTest, color32 colorMod)
{
	if (!materialsInitialized || colorMod.a == 0)
		return nullptr;
	IMaterial* mat = zTest ? (opaque && colorMod.a ? matOpaque : matAlpha) : matAlphaNoZ;
	Assert(mat);
	mat->ColorModulate(colorMod.r / 255.f, colorMod.g / 255.f, colorMod.b / 255.f);
	mat->AlphaModulate(colorMod.a / 255.f);
	return mat;
}

/**************************************** INTERNAL MESH WRAPPER ****************************************/

struct MeshWrapperInternal
{
	// keep statics alive as long as we render
	const std::shared_ptr<MeshWrapper> _staticMeshPtr;
	const DynamicMesh _dynamicInfo;
	const RenderCallback callback;

	CallbackInfoOut cbInfoOut;
	MeshPositionInfo newPosInfo;
	Vector camDistSqrTo;
	float camDistSqr; // translucent sorting metric

	inline static bool cachedCamMatValid = false;
	inline static VMatrix cachedCamMat;

	MeshWrapperInternal(const DynamicMesh& dynamicInfo, const RenderCallback& callback)
	    : _staticMeshPtr(nullptr), _dynamicInfo(dynamicInfo), callback(callback)
	{
	}

	MeshWrapperInternal(const std::shared_ptr<MeshWrapper>& staticMeshPtr, const RenderCallback& callback)
	    : _staticMeshPtr(staticMeshPtr), _dynamicInfo{}, callback(callback)
	{
	}

	const MeshWrapper& GetMeshWrapper() const
	{
		return _staticMeshPtr ? *_staticMeshPtr : g_meshBuilder.GetMeshWrapper(_dynamicInfo);
	}

	void ApplyCallbackAndCalcCamDist(const CViewSetup& cvs, bool inDrawOpaques)
	{
		const MeshWrapper& meshWrapper = GetMeshWrapper();

		if (callback)
		{
			CallbackInfoIn cbInfoIn = {cvs, meshWrapper.posInfo, std::nullopt};
			auto pRender = g_meshRenderer.g_pPortalRender;
			if (pRender && *pRender)
				cbInfoIn.portalRenderDepth = (**pRender).m_iViewRecursionLevel;
			callback(cbInfoIn, cbInfoOut = CallbackInfoOut{});
		}

		const MeshPositionInfo& origPos = meshWrapper.posInfo;
		if (callback)
			TransformAABB(cbInfoOut.mat, origPos.mins, origPos.maxs, newPosInfo.mins, newPosInfo.maxs);

		const MeshPositionInfo& actualPosInfo = callback ? newPosInfo : meshWrapper.posInfo;

		Vector center = (actualPosInfo.mins + actualPosInfo.maxs) / 2.f;
		switch (meshWrapper.params.sortType)
		{
		case TranslucentSortType::AABB_Center:
			camDistSqrTo = center;
			break;
		case TranslucentSortType::AABB_Surface:
			CalcClosestPointOnAABB(actualPosInfo.mins, actualPosInfo.maxs, cvs.origin, camDistSqrTo);
			if (cvs.origin == camDistSqrTo)
				camDistSqrTo = center; // if inside cube, use center idfk
			break;
		}
		camDistSqr = cvs.origin.DistToSqr(camDistSqrTo);
	}

	void RenderDebugMesh()
	{
		static StaticMesh debugBoxMesh, debugCrossMesh;

		if (!debugBoxMesh || !debugCrossMesh)
		{
			// opaque and only use lines - other places assume this!

			debugBoxMesh = MB_STATIC(
			    {
				    const Vector v0 = vec3_origin;
				    mb.AddBox(v0, v0, Vector(1), vec3_angle, MeshColor::Wire({0, 255, 100, 255}));
			    },
			    ZTEST_NONE);

			debugCrossMesh = MB_STATIC({
				for (int x = 0; x < 2; x++)
				{
					for (int y = 0; y < 2; y++)
					{
						Vector off(2 * x - 1, 2 * y - 1, 1);
						mb.AddLine(off, -off, {255, 50, 0, 255});
					}
				}
			});
		}

		CMatRenderContextPtr context{interfaces::materialSystem};

		// figure out box mesh dims
		const MeshWrapper& myWrapper = GetMeshWrapper();
		Assert(!myWrapper.faceComponent.Empty() || !myWrapper.lineComponent.Empty());
		auto& myPosInfo = callback ? newPosInfo : myWrapper.posInfo;
		const Vector nudge(0.05);
		Vector size = (myPosInfo.maxs + nudge) - (myPosInfo.mins - nudge);
		matrix3x4_t mat({size.x, 0, 0}, {0, size.y, 0}, {0, 0, size.z}, myPosInfo.mins - nudge);

		IMaterial* boxMaterial = g_meshBuilderMaterials.GetMaterial(debugBoxMesh->lineComponent.opaque,
		                                                            debugBoxMesh->lineComponent.zTest,
		                                                            {255, 255, 255, 255});
		Assert(boxMaterial);

		context->MatrixMode(MATERIAL_MODEL);
		context->PushMatrix();
		context->LoadMatrix(mat);
		context->Bind(boxMaterial);
		IMesh* iMesh = debugBoxMesh->lineComponent.GetIMesh(*debugBoxMesh, boxMaterial);
		if (iMesh)
			iMesh->Draw();

		const float smallest = 1, biggest = 15, falloff = 100;
		float maxBoxDim = VectorMaximum(size);
		// scale point mesh by the AABB size, plot this bad boy in desmos as a function of maxBoxDim
		float pointSize = -falloff * (biggest - smallest) / (maxBoxDim + falloff) + biggest;
		mat.Init({pointSize, 0, 0}, {0, pointSize, 0}, {0, 0, pointSize}, camDistSqrTo);

		IMaterial* crossMaterial = g_meshBuilderMaterials.GetMaterial(debugCrossMesh->lineComponent.opaque,
		                                                              debugCrossMesh->lineComponent.zTest,
		                                                              {255, 255, 255, 255});
		Assert(crossMaterial);

		context->LoadMatrix(mat);
		context->Bind(crossMaterial);
		iMesh = debugCrossMesh->lineComponent.GetIMesh(*debugCrossMesh, crossMaterial);
		if (iMesh)
			iMesh->Draw();
		context->PopMatrix();
	}

	void Render(bool faces)
	{
		VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);

		const MeshWrapper& meshWrapper = GetMeshWrapper();
		auto& component = faces ? meshWrapper.faceComponent : meshWrapper.lineComponent;
		Assert(!component.Empty());

		CMatRenderContextPtr context{interfaces::materialSystem};

		// UNDONE: projecting to the screen loses information - we should really compare against the frustom planes

		/*
		* Check if we can skip creating the IMesh*. Project all AABB corners on the screen and see
		* if the whole thing is off screen. Technically sort of creates a AABB around the projected
		* mesh AABB corners, so misses cases where the mesh AABB has a corner in front and a corner
		* behind the camera plane. Gives like 10 extra fps :p
		*/
		/*if (component.dynamic)
		{
			if (!cachedCamMatValid)
			{
				VMatrix view, projection;
				context->GetMatrix(MATERIAL_VIEW, &view);
				context->GetMatrix(MATERIAL_PROJECTION, &projection);
				cachedCamMat = projection * view;
				cachedCamMatValid = true;
			}

			auto& posInfo = callback ? newPosInfo : meshWrapper.posInfo;
			const Vector& mins = posInfo.mins;
			const Vector& maxs = posInfo.maxs;

			float min_x = INFINITY, max_x = -INFINITY;
			float min_y = INFINITY, max_y = -INFINITY;
			float min_z = INFINITY, max_z = -INFINITY;
			for (int i = 0; i < 8; i++)
			{
				Vector4D point{
				    (i & 1) ? maxs.x : mins.x,
				    (i & 2) ? maxs.y : mins.y,
				    (i & 4) ? maxs.z : mins.z,
				    1.0,
				};
				Vector4D result;
				cachedCamMat.V4Mul(point, result);
				result /= result.w;
				min_x = MIN(min_x, result.x);
				max_x = MAX(max_x, result.x);
				min_y = MIN(min_y, result.y);
				max_y = MAX(max_y, result.y);
				min_z = MIN(min_z, result.z);
				max_z = MAX(max_z, result.z);
			}
			if (min_x > 1.0 || max_x < -1.0 || min_y > 1.0 || max_y < -1.0 || min_z > 1.0 || max_z < 0.0)
				return;
		}*/

		color32 cMod = {255, 255, 255, 255};
		if (callback)
		{
			auto& cbData = faces ? cbInfoOut.faces : cbInfoOut.lines;
			cMod = cbData.colorModulate;
		}
		IMaterial* material = g_meshBuilderMaterials.GetMaterial(component.opaque, component.zTest, cMod);
		if (!material)
			return;

		IMesh* iMesh = component.GetIMesh(meshWrapper, material);
		if (!iMesh)
			return;

		if (callback)
		{
			context->MatrixMode(MATERIAL_MODEL);
			context->PushMatrix();
			context->LoadMatrix(cbInfoOut.mat);
		}
		context->Bind(material);
		iMesh->Draw();
		if (component.dynamic)
			context->Flush();
		if (callback)
			context->PopMatrix();
	}
};

/**************************************** MESH RENDERER ****************************************/

void MeshRenderer::DrawMesh(const DynamicMesh& dynamicMesh, const RenderCallback& callback)
{
	if (!g_inMeshRenderSignal)
	{
		AssertMsg(0, "spt: Meshes can only be drawn in MeshRenderSignal!");
		return;
	}
	if (dynamicMesh.createdFrame != g_meshRenderFrameNum)
	{
		AssertMsg(0, "spt: Attempted to reuse dynamic mesh between frames");
		Warning("spt: Can only draw dynamic meshes on the frame they were created!\n");
		return;
	}
	const MeshWrapper& meshWrapper = g_meshBuilder.GetMeshWrapper(dynamicMesh);
	if (!meshWrapper.faceComponent.Empty() || !meshWrapper.lineComponent.Empty())
		g_meshRenderer.deferredMeshWrappers.emplace_back(dynamicMesh, callback);
}

void MeshRenderer::DrawMesh(const StaticMesh& staticMesh, const RenderCallback& callback)
{
	if (!g_inMeshRenderSignal)
	{
		AssertMsg(0, "spt: Meshes can only be drawn in MeshRenderSignal!");
		return;
	}
	if (!staticMesh)
	{
		AssertMsg(0, "spt: This static mesh has been destroyed!");
		Warning("spt: Attempting to draw an invalid static mesh!\n");
		return;
	}
	if (!staticMesh->faceComponent.Empty() || !staticMesh->lineComponent.Empty())
		g_meshRenderer.deferredMeshWrappers.emplace_back(staticMesh, callback);
}

/**************************************** MESH RENDERER FEATURE ****************************************/

void MeshRendererFeature::OnRenderSignal(void* thisptr, vrect_t*)
{
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);
	deferredMeshWrappers.clear();
	g_meshBuilder.ClearOldBuffers();
	g_meshRenderFrameNum++;
	g_inMeshRenderSignal = true;
	static MeshRenderer renderer;
	MeshRenderSignal(renderer);
	g_inMeshRenderSignal = false;
}

void MeshRendererFeature::OnDrawOpaques(void* renderingView)
{
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);
	CViewSetup& cvs = *(CViewSetup*)((uintptr_t)renderingView + 8);
	MeshWrapperInternal::cachedCamMatValid = false;

	for (auto& meshInternal : deferredMeshWrappers)
	{
		meshInternal.ApplyCallbackAndCalcCamDist(cvs, true);
		const MeshWrapper& meshWrapper = meshInternal.GetMeshWrapper();
		/*
		* Check both components (faces & lines), skip rendering if:
		* - the component is empty
		* - the component is translucent
		* - there was a callback w/ alpha modulation (forcing the mesh to be translucent)
		* - there was a callback & and the user disabled rendering for the current view
		*/
		for (int i = 0; i < 2; i++)
		{
			auto& component = i == 0 ? meshWrapper.faceComponent : meshWrapper.lineComponent;
			if (component.Empty() || !component.opaque)
				continue;
			if (meshInternal.callback)
			{
				auto& cbInfo = meshInternal.cbInfoOut;
				auto& cbComponentData = i == 0 ? cbInfo.faces : cbInfo.lines;
				if (cbComponentData.skipRender || cbComponentData.colorModulate.a < 1)
					continue;
			}
			meshInternal.Render(i == 0);
		}
	}
}

void MeshRendererFeature::OnDrawTranslucents(void* renderingView)
{
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);
	CViewSetup& cvs = *(CViewSetup*)((uintptr_t)renderingView + 8);

	static std::vector<MeshWrapperInternal*> showDebugMeshFor;
	static std::vector<std::pair<MeshWrapperInternal*, bool>> sortedTranslucents; // <mesh_ptr, is_face_component>
	sortedTranslucents.clear();
	showDebugMeshFor.clear();
	MeshWrapperInternal::cachedCamMatValid = false;

	for (auto& meshInternal : deferredMeshWrappers)
	{
		meshInternal.ApplyCallbackAndCalcCamDist(cvs, false);
		const MeshWrapper& meshWrapper = meshInternal.GetMeshWrapper();
		/*
		* Check both components (faces & lines), skip rendering if:
		* - the component is empty
		* - there was no callback & the component is opaque
		* - there was a callback, the component is opaque, and the alpha was kept at 1
		* - there was a callback & and the user disabled rendering for the current view
		* If the component is not empty and rendering is not disabled for both faces/lines then we draw the debug mesh.
		*/
		bool drawDebugMesh = false;
		for (int i = 0; i < 2; i++)
		{
			auto& component = i == 0 ? meshWrapper.faceComponent : meshWrapper.lineComponent;
			if (component.Empty())
				continue;
			if (meshInternal.callback)
			{
				auto& cbInfo = meshInternal.cbInfoOut;
				auto& cbComponentData = i == 0 ? cbInfo.faces : cbInfo.lines;
				if (cbComponentData.skipRender)
					continue;
				drawDebugMesh = true;
				if (component.opaque && cbComponentData.colorModulate.a == 1)
					continue;
			}
			else if (component.opaque)
			{
				drawDebugMesh = true;
				continue;
			}
			drawDebugMesh = true;
			sortedTranslucents.emplace_back(&meshInternal, i == 0);
		}
		if (drawDebugMesh && y_spt_draw_mesh_debug.GetBool())
			showDebugMeshFor.push_back(&meshInternal);
	}

	std::sort(sortedTranslucents.begin(),
	          sortedTranslucents.end(),
	          [](const std::pair<MeshWrapperInternal*, bool>& a, const std::pair<MeshWrapperInternal*, bool>& b)
	          {
		          bool zTestA = a.second ? a.first->GetMeshWrapper().faceComponent.zTest
		                                 : a.first->GetMeshWrapper().lineComponent.zTest;
		          bool zTestB = b.second ? b.first->GetMeshWrapper().faceComponent.zTest
		                                 : b.first->GetMeshWrapper().lineComponent.zTest;
		          // $ignorez stuff needs to be rendered last on top of everything - materials with it
		          // don't change the z buffer so anything rendered after can still overwrite them
		          if (zTestA != zTestB)
			          return zTestA;
		          return a.first->camDistSqr > b.first->camDistSqr;
	          });

	for (auto& translucentInfo : sortedTranslucents)
		translucentInfo.first->Render(translucentInfo.second);

	for (auto meshInternal : showDebugMeshFor)
		meshInternal->RenderDebugMesh();
}

#endif
