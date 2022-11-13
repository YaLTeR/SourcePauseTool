#pragma once

#include "mesh_defs_public.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "view_shared.h"

#include "mesh_builder.hpp" // so that users can import just one header

#include <optional>

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
* mat           - a matrix that is applied to the mesh during rendering
* colorModulate - a "multiplier" to change the color/alpha of the entire mesh, relatively slow
*                 e.g. 128 means multiply existing value by 0.5
* skipRender    - conditionally enable rendering on a per-view basis
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
#define RENDER_DYNAMIC(renderer, createFunc, ...) (renderer).DrawMesh(MB_DYNAMIC(createFunc, __VA_ARGS__))

/*
* A (mostly) superior version of the debugoverlay. Allows meshes to be drawn on a per frame instead of per tick
* basis. Doesn't handle text and hud stuff yet. In LoadFeature, connect yourself to the MeshRenderSignal and feed the
* renderer the meshes you've created with the mesh builder.
* 
* Example usage:
* 
* void OnMeshRenderSignal(MeshRenderer& mr)
* {
*     RENDER_DYNAMIC(mr, mb.AddLine({0, 0, 0}, {10, 10, 10}, {120, 200, 255, 255}););
*     
*     // ^this^ is equivalent to:
*     
*     mr.DrawMesh(MeshBuilderPro::CreateDynamicMesh(
*         [&](MeshBuilderPro& mb)
*         {
*             mb.AddLine({0, 0, 0}, {10, 10, 10}, {120, 200, 255, 255});
*         }));
*     
*     // a common use case for static meshes - creating a complex mesh once and reusing it:
*     
*     static StaticMesh myMesh;
*     if (!myMesh)
*     {
*         myMesh = MB_STATIC({
*             mb.AddSphere(...);
*             mb.AddSphere(...);
*             mb.AddCylinder(...);
*        });
*     }
*     mr.DrawMesh(myMesh);
* }
*/
class MeshRenderer
{
public:
	void DrawMesh(const DynamicMesh& mesh, const RenderCallback& callback = nullptr);
	void DrawMesh(const StaticMesh& mesh, const RenderCallback& callback = nullptr);
};

#endif
