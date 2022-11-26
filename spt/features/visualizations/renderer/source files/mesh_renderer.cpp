#include "stdafx.h"

#include "..\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include <algorithm>

#include "..\mesh_defs_private.hpp"
#include "interfaces.hpp"
#include "signals.hpp"

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

// can't put this as a member in the feature without including defs_private in the header, so we make it a global
static std::vector<struct MeshUnitWrapper> g_meshUnitWrappers;

/**************************************** MESH RENDERER FEATURE ****************************************/

struct CPortalRender
{
#ifdef SSDK2007
	byte _pad[0x57c];
#else
	byte _pad[0x918];
#endif
	int m_iViewRecursionLevel;
};

namespace patterns
{
	PATTERNS(CRendering3dView__DrawOpaqueRenderables,
	         "5135",
	         "55 8D 6C 24 8C 81 EC 94 00 00 00",
	         "7462488",
	         "55 8B EC 81 EC 80 00 00 00 8B 15 ?? ?? ?? ??");
	PATTERNS(CRendering3dView__DrawTranslucentRenderables,
	         "5135",
	         "55 8B EC 83 EC 34 53 8B D9 8B 83 94 00 00 00 8B 13 56 8D B3 94 00 00 00",
	         "5135-hl2",
	         "55 8B EC 83 EC 34 83 3D ?? ?? ?? ?? 00",
	         "1910503",
	         "55 8B EC 81 EC 9C 00 00 00 53 56 8B F1 8B 86 E8 00 00 00 8B 16 57 8D BE E8 00 00 00",
	         "7462488",
	         "55 8B EC 81 EC A0 00 00 00 53 8B D9",
	         "7467727-hl2",
	         "55 8B EC 81 EC A0 00 00 00 83 3D ?? ?? ?? ?? 00");
	// this is the static OnRenderStart(), not the virtual one; only used for portal render depth
	PATTERNS(OnRenderStart,
	         "5135",
	         "56 8B 35 ?? ?? ?? ?? 8B 06 8B 50 64 8B CE FF D2 8B 0D ?? ?? ?? ??",
	         "7462488",
	         "55 8B EC 83 EC 10 53 56 8B 35 ?? ?? ?? ?? 8B CE 57 8B 06 FF 50 64 8B 0D ?? ?? ?? ??");
} // namespace patterns

bool MeshRendererFeature::ShouldLoadFeature()
{
	if (!interfaces::materialSystem)
	{
		DevWarning("Mesh rendering not available because materialSystem was not initialized!\n");
		return false;
	}
	return true;
}

void MeshRendererFeature::InitHooks()
{
	HOOK_FUNCTION(client, CRendering3dView__DrawOpaqueRenderables);
	HOOK_FUNCTION(client, CRendering3dView__DrawTranslucentRenderables);
	FIND_PATTERN(client, OnRenderStart);
}

void MeshRendererFeature::PreHook()
{
	signal.Works = Works();

	if (ORIG_OnRenderStart)
	{
		int idx = GetPatternIndex((void**)&ORIG_OnRenderStart);
		uint32_t offs[] = {18, 24};
		if (idx >= 0 && idx < sizeof(offs) / sizeof(uint32_t))
			g_pPortalRender = *(CPortalRender***)((uintptr_t)ORIG_OnRenderStart + offs[idx]);
	}

	g_meshMaterialMgr.Load();
}

void MeshRendererFeature::LoadFeature()
{
	if (!Works())
		return;
	RenderViewPre_Signal.Connect(this, &MeshRendererFeature::OnRenderViewPre_Signal);
	InitConcommandBase(y_spt_draw_mesh_debug);
}

void MeshRendererFeature::UnloadFeature()
{
	signal.Clear();
	FrameCleanup();
	g_meshMaterialMgr.Unload();
	StaticMesh::DestroyAll();
	g_pPortalRender = nullptr;
}

bool MeshRendererFeature::Works() const
{
	/*
	* 1) InitHooks: spt_overlay finds the render function.
	* 2) PreHook: spt_overlay may connect the RenderViewSignal. To not depend on feature load order
	*    we cannot check if the signal exists, but we can check if the RenderView function was found.
	* 3) LoadFeature: Anything that uses the mesh rendering system can check to see if the signal exists.
	*/
	return !!ORIG_CRendering3dView__DrawOpaqueRenderables && !!ORIG_CRendering3dView__DrawTranslucentRenderables
	       && spt_overlay.ORIG_CViewRender__RenderView;
}

std::optional<uint> MeshRendererFeature::CurrentPortalRenderDepth() const
{
	if (!g_pPortalRender || !*g_pPortalRender)
		return std::nullopt;
	return (**g_pPortalRender).m_iViewRecursionLevel;
}

HOOK_THISCALL(void, MeshRendererFeature, CRendering3dView__DrawOpaqueRenderables, int param)
{
	// HACK - param is a bool in SSDK2007 and enum in SSDK2013
	// render order shouldn't matter here
	spt_meshRenderer.ORIG_CRendering3dView__DrawOpaqueRenderables(thisptr, 0, param);
	if (spt_meshRenderer.Works())
		spt_meshRenderer.OnDrawOpaques(thisptr);
}

HOOK_THISCALL(void, MeshRendererFeature, CRendering3dView__DrawTranslucentRenderables, bool inSkybox, bool shadowDepth)
{
	// render order matters here, render our stuff on top of other translucents
	spt_meshRenderer.ORIG_CRendering3dView__DrawTranslucentRenderables(thisptr, 0, inSkybox, shadowDepth);
	if (spt_meshRenderer.Works())
		spt_meshRenderer.OnDrawTranslucents(thisptr);
}

/**************************************** MESH UNIT WRAPPER ****************************************/

struct MeshUnitWrapper
{
	// keep statics alive as long as we render
	const std::shared_ptr<MeshUnit> _staticMeshPtr;
	const DynamicMesh _dynamicInfo;
	const RenderCallback callback;

	CallbackInfoOut cbInfoOut;
	MeshPositionInfo newPosInfo;
	Vector camDistSqrTo;
	float camDistSqr; // translucent sorting metric

	inline static bool cachedCamMatValid = false;
	inline static VMatrix cachedCamMat;

	MeshUnitWrapper(const DynamicMesh& dynamicInfo, const RenderCallback& callback)
	    : _staticMeshPtr(nullptr), _dynamicInfo(dynamicInfo), callback(callback)
	{
	}

	MeshUnitWrapper(const std::shared_ptr<MeshUnit>& staticMeshPtr, const RenderCallback& callback)
	    : _staticMeshPtr(staticMeshPtr), _dynamicInfo{}, callback(callback)
	{
	}

	const MeshUnit& GetMeshUnit() const
	{
		return _staticMeshPtr ? *_staticMeshPtr : g_meshBuilderInternal.GetMeshUnit(_dynamicInfo);
	}

	// returns true if we're in the frustum
	bool ApplyCallbackAndCalcCamDist(const CViewSetup& cvs, const VPlane frustum[6])
	{
		const MeshUnit& meshUnit = GetMeshUnit();

		if (callback)
		{
			CallbackInfoIn infoIn = {cvs,
			                         meshUnit.posInfo,
			                         spt_meshRenderer.CurrentPortalRenderDepth(),
			                         spt_overlay.renderingOverlay};

			callback(infoIn, cbInfoOut = CallbackInfoOut{});
		}

		const MeshPositionInfo& origPos = meshUnit.posInfo;
		if (callback)
			TransformAABB(cbInfoOut.mat, origPos.mins, origPos.maxs, newPosInfo.mins, newPosInfo.maxs);

		const MeshPositionInfo& actualPosInfo = callback ? newPosInfo : meshUnit.posInfo;

		Vector center = (actualPosInfo.mins + actualPosInfo.maxs) / 2.f;
		switch (meshUnit.params.sortType)
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

		// check if mesh is outside frustum

		for (int i = 0; i < 6; i++)
			if (frustum[i].BoxOnPlaneSide(actualPosInfo.mins, actualPosInfo.maxs) == SIDE_BACK)
				return false;
		return true;
	}

	void RenderDebugMesh()
	{
		static StaticMesh debugBoxMesh, debugCrossMesh;

		if (!debugBoxMesh.Valid() || !debugCrossMesh.Valid())
		{
			// opaque and only use lines - other places assume this!

			debugBoxMesh = MB_STATIC(
			    {
				    const Vector v0 = vec3_origin;
				    mb.AddBox(v0, v0, Vector(1), vec3_angle, MeshColor::Wire({0, 255, 100, 255}));
			    },
			    ZTEST_NONE);

			debugCrossMesh = MB_STATIC(mb.AddCross({0, 0, 0}, 2, {255, 0, 0, 255}););
		}

		MeshUnit* boxMeshUnit = debugBoxMesh.meshPtr.get();
		MeshUnit* crossMeshUnit = debugCrossMesh.meshPtr.get();

		CMatRenderContextPtr context{interfaces::materialSystem};

		// figure out box mesh dims
		const MeshUnit& myWrapper = GetMeshUnit();
		Assert(!myWrapper.faceComponent.Empty() || !myWrapper.lineComponent.Empty());
		auto& myPosInfo = callback ? newPosInfo : myWrapper.posInfo;
		const Vector nudge(0.05);
		Vector size = (myPosInfo.maxs + nudge) - (myPosInfo.mins - nudge);
		matrix3x4_t mat({size.x, 0, 0}, {0, size.y, 0}, {0, 0, size.z}, myPosInfo.mins - nudge);

		IMaterial* boxMaterial = g_meshMaterialMgr.GetMaterial(boxMeshUnit->lineComponent.opaque,
		                                                       boxMeshUnit->lineComponent.zTest,
		                                                       {255, 255, 255, 255});
		Assert(boxMaterial);

		context->MatrixMode(MATERIAL_MODEL);
		context->PushMatrix();
		context->LoadMatrix(mat);
		context->Bind(boxMaterial);
		IMesh* iMesh = boxMeshUnit->lineComponent.GetIMesh(*boxMeshUnit, boxMaterial);
		if (iMesh)
			iMesh->Draw();

		const float smallest = 1, biggest = 15, falloff = 100;
		float maxBoxDim = VectorMaximum(size);
		// scale point mesh by the AABB size, plot this bad boy in desmos as a function of maxBoxDim
		float pointSize = -falloff * (biggest - smallest) / (maxBoxDim + falloff) + biggest;
		mat.Init({pointSize, 0, 0}, {0, pointSize, 0}, {0, 0, pointSize}, camDistSqrTo);

		IMaterial* crossMaterial = g_meshMaterialMgr.GetMaterial(crossMeshUnit->lineComponent.opaque,
		                                                         crossMeshUnit->lineComponent.zTest,
		                                                         {255, 255, 255, 255});
		Assert(crossMaterial);

		context->LoadMatrix(mat);
		context->Bind(crossMaterial);
		iMesh = crossMeshUnit->lineComponent.GetIMesh(*crossMeshUnit, crossMaterial);
		if (iMesh)
			iMesh->Draw();
		context->PopMatrix();
	}

	void Render(bool faces)
	{
		VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);

		const MeshUnit& meshUnit = GetMeshUnit();
		auto& component = faces ? meshUnit.faceComponent : meshUnit.lineComponent;
		Assert(!component.Empty());

		CMatRenderContextPtr context{interfaces::materialSystem};

		color32 cMod = {255, 255, 255, 255};
		if (callback)
		{
			auto& cbData = faces ? cbInfoOut.faces : cbInfoOut.lines;
			cMod = cbData.colorModulate;
		}
		IMaterial* material = g_meshMaterialMgr.GetMaterial(component.opaque, component.zTest, cMod);
		if (!material)
			return;

		IMesh* iMesh = component.GetIMesh(meshUnit, material);
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

/**************************************** MESH RENDER FEATURE ****************************************/

void MeshRendererFeature::FrameCleanup()
{
	g_meshUnitWrappers.clear();
	g_meshBuilderInternal.ClearOldBuffers();
}

void MeshRendererFeature::OnRenderViewPre_Signal(void* thisptr, CViewSetup* cameraView)
{
	// ensure we only run once per frame
	if (spt_overlay.renderingOverlay || !signal.Works)
		return;
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);
	FrameCleanup();
	g_meshRenderFrameNum++;
	g_inMeshRenderSignal = true;
	MeshRendererDelegate renderDelgate = {};
	signal(renderDelgate);
	g_inMeshRenderSignal = false;
}

void MeshRendererFeature::SetupViewInfo(CRendering3dView* rendering3dView)
{
	viewInfo.rendering3dView = rendering3dView;
	// if only more stuff was public ://
	viewInfo.viewSetup = (CViewSetup*)((uintptr_t)rendering3dView + 8);
	viewInfo.frustum = *(VPlane**)(viewInfo.viewSetup + 1); // inward facing
}

void MeshRendererFeature::OnDrawOpaques(void* renderingView)
{
	VPROF_BUDGET(__FUNCTION__, VPROF_BUDGETGROUP_MESH_RENDERER);
	SetupViewInfo((CRendering3dView*)renderingView);
	MeshUnitWrapper::cachedCamMatValid = false;

	for (auto& meshInternal : g_meshUnitWrappers)
	{
		bool bDraw = meshInternal.ApplyCallbackAndCalcCamDist(*viewInfo.viewSetup, viewInfo.frustum);
		const MeshUnit& meshUnit = meshInternal.GetMeshUnit();
		/*
		* Check both components (faces & lines), skip rendering if:
		* * the mesh is outside the frustum
		* - the component is empty
		* - the component is translucent
		* - there was a callback w/ alpha modulation (forcing the mesh to be translucent)
		* - there was a callback & and the user disabled rendering for the current view
		*/
		for (int i = 0; i < 2; i++)
		{
			auto& component = i == 0 ? meshUnit.faceComponent : meshUnit.lineComponent;
			if (!bDraw || component.Empty() || !component.opaque)
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
	SetupViewInfo((CRendering3dView*)renderingView);

	static std::vector<MeshUnitWrapper*> showDebugMeshFor;
	static std::vector<std::pair<MeshUnitWrapper*, bool>> sortedTranslucents; // <mesh_ptr, is_face_component>
	sortedTranslucents.clear();
	showDebugMeshFor.clear();
	MeshUnitWrapper::cachedCamMatValid = false;

	for (auto& meshInternal : g_meshUnitWrappers)
	{
		bool drawDebugMesh = meshInternal.ApplyCallbackAndCalcCamDist(*viewInfo.viewSetup, viewInfo.frustum);
		const MeshUnit& meshUnit = meshInternal.GetMeshUnit();
		/*
		* Check both components (faces & lines), skip rendering if:
		* - the component is outside the frustum
		* - the component is empty
		* - there was no callback & the component is opaque
		* - there was a callback, the component is opaque, and the alpha was kept at 1
		* - there was a callback & and the user disabled rendering for the current view
		* If the component is not empty and rendering is not disabled for both faces/lines then we draw the debug mesh.
		*/
		for (int i = 0; i < 2; i++)
		{
			auto& component = i == 0 ? meshUnit.faceComponent : meshUnit.lineComponent;
			if (!drawDebugMesh || component.Empty())
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
	          [](const std::pair<MeshUnitWrapper*, bool>& a, const std::pair<MeshUnitWrapper*, bool>& b)
	          {
		          bool zTestA = a.second ? a.first->GetMeshUnit().faceComponent.zTest
		                                 : a.first->GetMeshUnit().lineComponent.zTest;
		          bool zTestB = b.second ? b.first->GetMeshUnit().faceComponent.zTest
		                                 : b.first->GetMeshUnit().lineComponent.zTest;
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

/**************************************** MESH RENDERER ****************************************/

void MeshRendererDelegate::DrawMesh(const DynamicMesh& dynamicMesh, const RenderCallback& callback)
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
	const MeshUnit& meshUnit = g_meshBuilderInternal.GetMeshUnit(dynamicMesh);
	if (!meshUnit.faceComponent.Empty() || !meshUnit.lineComponent.Empty())
		g_meshUnitWrappers.emplace_back(dynamicMesh, callback);
}

void MeshRendererDelegate::DrawMesh(const StaticMesh& staticMesh, const RenderCallback& callback)
{
	if (!g_inMeshRenderSignal)
	{
		AssertMsg(0, "spt: Meshes can only be drawn in MeshRenderSignal!");
		return;
	}
	if (!staticMesh.Valid())
	{
		AssertMsg(0, "spt: This static mesh has been destroyed!");
		Warning("spt: Attempting to draw an invalid static mesh!\n");
		return;
	}
	if (!staticMesh.meshPtr->Empty())
		g_meshUnitWrappers.emplace_back(staticMesh.meshPtr, callback);
}

#endif
