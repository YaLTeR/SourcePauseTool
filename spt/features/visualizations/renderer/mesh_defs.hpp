#pragma once

#include "spt\features\overlay.hpp"

#if !defined(OE) && defined(SPT_OVERLAY_ENABLED)

#define SPT_MESH_RENDERING_ENABLED

/*
* Different game versions have different limits for how big meshes can get. If a dynamic mesh is too big then the
* game will call Error(), but if your static meshes are too big they simply won't get drawn. Use this for limiting
* sizes of really large meshes.
*/
void GetMaxMeshSize(size_t& maxVerts, size_t& maxIndices, bool dynamic);

// used internally for figuring out the render order of translucent meshes, but is also passed to callbacks
struct MeshPositionInfo
{
	Vector mins, maxs;
};

// winding direction - determines which side a face is visible from; has no effect on lines,
// code assumes that these values are what they are
enum WindingDir : unsigned char
{
	WD_CW = 1,       // clockwise
	WD_CCW = 1 << 1, // counter clockwise
	WD_BOTH = WD_CW | WD_CCW
};

struct StaticMeshUnit;

// a wrapper of a shared static mesh pointer, before rendering check if it's Valid() and if not recreate it
class StaticMesh
{
public:
	std::shared_ptr<StaticMeshUnit> meshPtr;

private:
	// keep a linked list through all static meshes for proper cleanup w/ spt_tas_restart_game
	StaticMesh *prev, *next;
	static StaticMesh* first;

	void AttachToFront();

public:
	StaticMesh();
	StaticMesh(const StaticMesh& other);
	StaticMesh(StaticMesh&& other);
	StaticMesh(StaticMeshUnit* mesh);
	StaticMesh& operator=(const StaticMesh& r);
	bool Valid() const;
	void Destroy();
	// this is public but don't use it :)
	static int DestroyAll();
	~StaticMesh();
};

// a token representing a dynamic mesh
struct DynamicMesh
{
	const size_t dynamicMeshIdx;
	const int createdFrame;
};

#endif
