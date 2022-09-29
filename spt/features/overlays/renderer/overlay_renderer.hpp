#pragma once

#include "mesh_builder.hpp"
#include "spt\features\restart.hpp"

#if defined(SPT_MESH_BUILDER_ENABLED) && defined(SSDK2007) && !defined(SPT_TAS_RESTART_ENABLED)
/*
* I use a signal from spt_overlay, but that uses spt_hud which only works in 2007. The solution could be as simple
* as hooking some other render function. In the distant utopian future we'll ideally stop using vguimatsurface
* anyways and just transfer all hud rendering to here using materialsystem.
* 
* If this is expanded to work on other versions, tas_restart MUST be tested while overlays are active (especially
* static ones). Any static meshes given to users will probably have to be destroyed and possibly automatically
* reconstructed (or maybe just make the user check if it's been destroyed). I'm thinking there will have to be
* some signals set up for tas_restart.
*/
#define SPT_OVERLAY_RENDERER_ENABLED

#include <optional>

#include "view_shared.h"

/*
* cvs                      - camera info for current view
* meshPosInfo              - position info about the mesh the callback is for
* currentPortalRenderDepth - no value if not available, 0 if looking directly at mesh, 1 if looking through a
*                            portal, 2 if looking through two portals, etc.
*/
struct CallbackInfoIn
{
	const CViewSetup& cvs;
	const MeshPositionInfo& meshPosInfo;
	std::optional<uint> currentPortalRenderDepth;
};

/*
* mat                      - a matrix that is applied to the mesh after rendering
* colorModulate            - a "multiplier" to change the color/alpha of the entire mesh,
*                            e.g. 128 means multiply existing value by 0.5
* skipRenderForCurrentView - conditionally enable rendering on a per-view basis
* 
* You can specify different values for the line/face components of your mesh.
*/
struct CallbackInfoOut
{
	matrix3x4_t mat = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};

	struct
	{
		color32 colorModulate{255, 255, 255, 255};
		bool skipRenderForCurrentView = false;
	} faces, lines;
};

/*
* This is the only way to "edit" meshes after creation. This will be called for every render view (e.g. through
* portals, saveglitch overlay, etc.). The values of infoOut are given default values (see above), so you change
* just what you want. You can render the same dynamic/static mesh in a single OverlaySignal and use a different
* callback (this isn't gonna be much more efficient for dynamic meshes unless your mesh is huge).
* 
* Note: This callback will get called multiple times later during the frame (after the overlay signal), so take
* care with lambda captures and modifying external states - the resulting output should be deterministic.
*/
typedef std::function<void(const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)> RenderCallback;

// Simple macro for a dynamic mesh with no callback. Intellisense sucks huge dong for lambdas in this macro, so
// maybe avoid using it if you are doing something long and complicated.
#define OVR_DYNAMIC(ovrInst, createFunc, ...) (ovrInst).DrawMesh(MB_DYNAMIC(createFunc, __VA_ARGS__))

/*
* A (mostly) superior version of the debugoverlay. Allows meshes to be drawn on a per frame instead of per tick
* basis. Doesn't handle text and hud stuff yet. In LoadFeature, connect yourself to the OverlaySignal and feed the
* renderer the meshes you've created with the mesh builder.
* 
* Example usage:
* 
* void OnOverlaySignal(OverlayRenderer& ovr)
* {
*     OVR_DYNAMIC(ovr, mb.AddLine({0, 0, 0}, {10, 10, 10}, {120, 200, 255, 255}););
*     
*     // ^this^ is equivalent to:
*     
*     ovr.DrawMesh(MeshBuilderPro::CreateDynamicMesh(
*         [&](MeshBuilderPro& mb)
*         {
*             mb.AddLine({0, 0, 0}, {10, 10, 10}, {120, 200, 255, 255});
*         }));
*     
*     // a common use case for static meshes - creating a complex mesh once and reusing it:
*     
*     static StaticOverlayMesh myMesh;
*     if (!myMesh)
*     {
*         myMesh = MB_STATIC({
*             mb.AddSphere(...);
*             mb.AddSphere(...);
*             mb.AddCylinder(...);
*        });
*     }
*     ovr.DrawMesh(myMesh);
* }
*/
class OverlayRenderer
{
public:
	void DrawMesh(const DynamicOverlayMesh& mesh, const RenderCallback& callback = nullptr);
	void DrawMesh(const StaticOverlayMesh& mesh, const RenderCallback& callback = nullptr);

private:
	friend class OvrFeature;

	OverlayRenderer() = default;
	OverlayRenderer(OverlayRenderer&) = delete;

	void OnRenderSignal(void* thisptr, vrect_t*);
	void OnDrawOpaques(void* renderingView);
	void OnDrawTranslucents(void* renderingView);

	static OverlayRenderer singleton;

	std::vector<struct OverlayMeshInternal> internalMeshes;

	bool inOverlaySignal;
};

#endif
