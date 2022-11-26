#pragma once

#include "mesh_defs_public.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "view_shared.h"
#include "viewrender.h"

#include "mesh_builder.hpp" // so that users can import just one header

#include <optional>

#include "spt\feature.hpp"
#include "thirdparty\Signal.h"

/*
* cvs               - camera info for current view
* meshPosInfo       - position info about the mesh the callback is for
* portalRenderDepth - no value if not available, 0 if looking directly at mesh, 1 if looking through a
*                     portal, 2 if looking through two portals, etc.
* inOverlayView     - is the current view on an overlay (set to spt_overlay.renderingOverlay)
*/
struct CallbackInfoIn
{
	const CViewSetup& cvs;
	const MeshPositionInfo& meshPosInfo;
	std::optional<uint> portalRenderDepth;
	bool inOverlayView;
};

/*
* mat           - a matrix applied to the mesh for the current view
* colorModulate - a "multiplier" to change the color/alpha of the entire mesh, relatively slow
*                 e.g. 128 means multiply existing value by 0.5
* skipRender    - conditionally enable rendering on a per-view basis (e.g. only on overlays)
* 
* You can specify different values for the line/face components of your mesh.
*/
struct CallbackInfoOut
{
	matrix3x4_t mat = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};

	struct
	{
		color32 colorModulate{255, 255, 255, 255};
		bool skipRender = false;
	} faces, lines;
};

/*
* This is the only way to "edit" meshes after creation. This will be called for every render view (e.g. through
* portals, saveglitch overlay, etc.). The values of infoOut are given default values (see above), so you change
* just what you want. You can render the same dynamic/static mesh in a single MeshRenderSignal and use a different
* callback (this isn't gonna be much more efficient for dynamic meshes unless your mesh is huge).
* 
* Note: This callback will get called multiple times later during the frame (after the overlay signal), so take
* care with lambda captures and modifying external states - the resulting output should be deterministic.
*/
typedef std::function<void(const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)> RenderCallback;

// Simple macro for a dynamic mesh with no callback. Intellisense sucks huge dong for lambdas in this macro, so
// maybe avoid using it if you are doing something long and complicated.
#define RENDER_DYNAMIC(rdrDelegate, createFunc, ...) (rdrDelegate).DrawMesh(MB_DYNAMIC(createFunc, __VA_ARGS__))

class MeshRendererDelegate
{
public:
	void DrawMesh(const DynamicMesh& mesh, const RenderCallback& callback = nullptr);
	void DrawMesh(const StaticMesh& mesh, const RenderCallback& callback = nullptr);

private:
	friend class MeshRendererFeature;
	MeshRendererDelegate() = default;
	MeshRendererDelegate(MeshRendererDelegate&) = delete;
};

/*
* To use, connect yourself to the signal (after checking if it Works). In the callback, create meshes with the
* MeshBuilderPro™ (or create static meshes whenever) and feed them to the MeshRendererDelegate given to the
* signal. The signal will be triggered once at the start of every frame.
*/
class MeshRendererFeature : public FeatureWrapper<MeshRendererFeature>
{
private:
	struct CPortalRender** g_pPortalRender = nullptr;

public:
	// Works() method valid during or after PreHook()
	Gallant::Signal1<MeshRendererDelegate&> signal;

	std::optional<uint> CurrentPortalRenderDepth() const;

protected:
	bool ShouldLoadFeature() override;
	void InitHooks() override;
	void PreHook() override;
	void LoadFeature() override;
	void UnloadFeature() override;

private:
	struct
	{
		CRendering3dView* rendering3dView;
		CViewSetup* viewSetup;
		VPlane* frustum;
	} viewInfo;

	DECL_MEMBER_CDECL(void, OnRenderStart);
	DECL_HOOK_THISCALL(void, CRendering3dView__DrawOpaqueRenderables, int param);
	DECL_HOOK_THISCALL(void, CRendering3dView__DrawTranslucentRenderables, bool bInSkybox, bool bShadowDepth);

	bool Works() const;
	void FrameCleanup();
	void OnRenderViewPre_Signal(void* thisptr, CViewSetup* cameraView);
	void SetupViewInfo(CRendering3dView* rendering3dView);
	void OnDrawOpaques(void* renderingView);
	void OnDrawTranslucents(void* renderingView);
};

inline MeshRendererFeature spt_meshRenderer;

#endif
