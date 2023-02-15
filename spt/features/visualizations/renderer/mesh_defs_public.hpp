#pragma once

#include "spt\features\overlay.hpp"

#if !defined(OE) && defined(SPT_OVERLAY_ENABLED)

#define SPT_MESH_RENDERING_ENABLED

struct MeshUnit;

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

// very basic color lerp for very basic needs
inline color32 color32RgbLerp(color32 a, color32 b, float f)
{
	f = clamp(f, 0, 1);
	return {
	    RoundFloatToByte(a.r * (1 - f) + b.r * f),
	    RoundFloatToByte(a.g * (1 - f) + b.g * f),
	    RoundFloatToByte(a.b * (1 - f) + b.b * f),
	    RoundFloatToByte(a.a * (1 - f) + b.a * f),
	};
}

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

// typical use case - check Valid() and if it returns false recreate
class StaticMesh
{
public:
	std::shared_ptr<MeshUnit> meshPtr;

private:
	// keep a linked list through all static meshes for proper cleanup w/ tas_restart
	StaticMesh *prev, *next;
	static StaticMesh* first;

	void AttachToFront();

public:
	StaticMesh();
	StaticMesh(const StaticMesh& other);
	StaticMesh(StaticMesh&& other);
	StaticMesh(MeshUnit* mesh);
	StaticMesh& operator=(const StaticMesh& r);
	bool Valid() const;
	void Destroy();
	static void DestroyAll();
	~StaticMesh();
};

struct DynamicMesh
{
	const size_t dynamicMeshIdx;
	const int createdFrame;
};

#endif
