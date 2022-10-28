#pragma once

#include "spt\features\restart.hpp"

/*
* I use a signal from spt_overlay, but that uses spt_hud which only works in 2007. The solution could be as simple
* as hooking some other render function. In the distant utopian future we'll ideally stop using vguimatsurface
* anyways and just transfer all hud rendering to here using materialsystem.
* 
* If this is expanded to work on other versions, tas_restart MUST be tested while meshes are active (especially
* static ones). Any static meshes given to users will probably have to be destroyed and possibly automatically
* reconstructed (or maybe just make the user check if it's been destroyed). I'm thinking there will have to be
* some signals set up for tas_restart.
*/
#if !defined(OE) && defined(SSDK2007) && !defined(SPT_TAS_RESTART_ENABLED)

#define SPT_MESH_RENDERING_ENABLED

struct MeshWrapper;

/*
* The renderer needs some position metric for meshes to figure out which order to render translucent meshes in.
* For simplicity, we only keep track of the each meshes' AABB and assume each mesh is a cubical cow. This position
* info is also passed to mesh render callbacks which can be used to tweak the meshes's model matrix during
* rendering.
*/
struct MeshPositionInfo
{
	Vector mins, maxs;
};

struct MeshColor
{
	const color32 faceColor, lineColor;

	static MeshColor Face(const color32& faceColor)
	{
		return MeshColor{faceColor, {0, 0, 0, 0}};
	}

	static MeshColor Wire(const color32& lineColor)
	{
		return MeshColor{{0, 0, 0, 0}, lineColor};
	}

	static MeshColor Outline(const color32& faceColor)
	{
		return MeshColor{faceColor, {faceColor.r, faceColor.g, faceColor.b, 255}};
	}
};

enum ZTestFlags
{
	ZTEST_NONE = 0,
	ZTEST_FACES = 1,
	ZTEST_LINES = 2,
};

/*
* Rendering translucent meshes in the correct order is tricky - but you can provide a hint to the renderer for
* how you want your mesh to be sorted relative to other meshes. Given the camera and mesh position, we wish to
* define the distance from the camera to each mesh.
* - AABB_Surface: useful for axis aligned boxes and planes. The distance to the mesh is defined as the distance to
*   the meshes' AABB.
* - AABB_Center: useful for non-axis-aligned planes. The distance to the mesh is defined as the distance to the
*   center of its AABB.
*/
enum class TranslucentSortType
{
	AABB_Surface,
	AABB_Center,
};

// direction of normals for faces (has no effect on lines)
enum class CullType
{
	Default,
	Reverse,
	ShowBoth,
};

struct CreateMeshParams
{
	const int zTestFlags = ZTEST_FACES | ZTEST_LINES;
	const CullType cullType = CullType::Default;
	const TranslucentSortType sortType = TranslucentSortType::AABB_Surface;
};

// TODO test me with tas_restart
typedef std::shared_ptr<MeshWrapper> StaticMesh;

struct DynamicMesh
{
	const size_t dynamicMeshIdx;
	const int createdFrame;
};

#endif
