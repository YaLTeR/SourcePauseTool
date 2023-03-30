#pragma once

#include "mesh_builder_internal.hpp"
#include "..\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

/*
* We already have mesh units which are already wrappers of internal mesh representations, so you may be wondering
* why we need another wrapper. In addition to the mesh units, this wrapper contains the user's render callback if
* they gave one, and the position metric to the unit. The user can give us multiple of the same mesh with
* different callbacks, so each one would have its own unit wrapper.
*/
struct MeshUnitWrapper
{
	// keep statics alive as long as we're rendering, dynamics are kept alive in the mesh builder
	const std::shared_ptr<StaticMeshUnit> _staticMeshPtr;
	const DynamicMeshToken _dynamicToken;
	const RenderCallback callback;

	CallbackInfoOut cbInfoOut;
	MeshPositionInfo posInfo;
	Vector camDistSqrTo;
	float camDistSqr; // translucent sorting metric

	MeshUnitWrapper(DynamicMeshToken dynamicToken, const RenderCallback& callback = nullptr);

	MeshUnitWrapper(const std::shared_ptr<StaticMeshUnit>& staticMeshPtr, const RenderCallback& callback = nullptr);

	// returns true if this unit should be rendered
	bool ApplyCallbackAndCalcCamDist();

	// We pretend this mesh wrapper contains a mesh from this unit, but it could be a fused mesh.
	// All that matters is that we use its material and our callback.
	void Render(const IMeshWrapper mw);
};

struct MeshRendererInternal
{
	std::vector<MeshUnitWrapper> queuedUnitWrappers;

	struct
	{
		const CRendering3dView* rendering3dView;
		const CViewSetup* viewSetup;
		cplane_t frustum[FRUSTUM_NUMPLANES];
	} viewInfo;

	bool renderingSkyBox = false;

	// used to check if dynamic meshes are valid
	int frameNum = 0;
	bool inSignal = false;

	/*
	* For simplicity, we make debug meshes not do z-testing. If we do this we can queue debug meshes in
	* DrawOpaques() & DrawTranslucents() and then draw all of them at the very end. Suppose we have 3 portals
	* A,B,C that we can look through, and we can see portal C through portal B. Then the call stack of the
	* drawing functions sort of looks like this:
	* 
	* - DrawOpaques()      (main view)
	*   - DrawOpaques()      (view through portal A)
	*   - DrawTranslucents() (view through portal A)
	*   - DrawOpaques()      (view through portal B)
	*     - DrawOpaques()      (view through portal C)
	*     - DrawTranslucents() (view through portal C)
	*   - DrawTranslucents() (view through portal B)
	* - DrawTranslucents() (main view)
	* 
	* (This is a lie because the calls are actually recursed from DrawTranslucents() in game but we draw our
	* translucent stuff after the game so for the purpose of debug meshes our stack looks like ^that^).
	* 
	* If debug meshes were opaque, we would need to draw them in DrawOpaques(), so we'd need to know about all
	* translucent meshes *before* they get rendererd. This would require a more complicated queueing system.
	* 
	* Currently, when we start in DrawOpaques(), we'll "push" a debug slice to the debug description list, then we
	* add all opaque debug meshes to that slice. Then we recursively get all the portal views, and once we're done
	* with those we'll get to the corresponding DrawTranslucents() on the same depth as the DrawOpaques() call.
	* Once we're done with that, the top most slice will have all debug meshes for the current view, and we can
	* batch all of them together at the end of DrawTranslucents().
	*/
	struct
	{
		struct DebugMeshDesc
		{
			union
			{
				struct
				{
					Vector mins, maxs;
				} box; // mesh unit AABB

				struct
				{
					Vector crossPos;
					float size;
				} cross; // translucent mesh unit position metric
			};

			bool isBox;
			color32 color;

			DebugMeshDesc(){};

			DebugMeshDesc(const Vector& mins, const Vector& maxs, color32 c)
			    : box{mins, maxs}, isBox(true), color(c)
			{
			}

			DebugMeshDesc(const Vector& crossPos, float size, color32 c)
			    : cross{crossPos, size}, isBox(false), color(c)
			{
			}
		};

		std::vector<DebugMeshDesc> sharedDescriptionList;
		VectorStack<VectorSlice<DebugMeshDesc>> descriptionSlices;

	} debugMeshInfo;

	using DebugDescList = decltype(debugMeshInfo.descriptionSlices)::value_type;

	void OnRenderViewPre_Signal(void* thisptr, CViewSetup* cameraView);
	void FrameCleanup();
	void OnDrawOpaques(CRendering3dView* rendering3dView);
	void OnDrawTranslucents(CRendering3dView* rendering3dView);

	void SetupViewInfo(CRendering3dView* rendering3dView);
	void CollectRenderableComponents(std::vector<MeshComponent>& components, bool opaques);
	void AddDebugCrosses(ConstCompIntrvl intrvl, bool opaques);
	void AddDebugBox(ConstCompIntrvl intrvl, bool opaques);
	void DrawDebugMeshes();
	void DrawAll(ConstCompIntrvl intrvl, bool addDebugMeshes, bool opaques);

	void AddDebugCrosses(DebugDescList& debugList, ConstCompIntrvl intrvl);
	void AddDebugBox(DebugDescList& debugList, ConstCompIntrvl intrvl, bool opaques);
	void DrawDebugMeshes(DebugDescList& debugList);
};

inline MeshRendererInternal g_meshRendererInternal;

#endif
