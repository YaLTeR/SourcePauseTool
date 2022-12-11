#pragma once

#include "mesh_defs_public.hpp"
#include "spt\feature.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "mathlib\polyhedron.h"

/*
* The game uses a CMeshBuilder to create meshes, but we can't use it directly because parts of its implementation
* are private and/or not in the SDK. Not to worry - Introducing The MeshBuilderPro™! The MeshBuilderPro™ can be
* used to create meshes made of both faces and lines as well as different colors. The MeshBuilderPro™ also reuses
* internal mesh & vertex buffers for speed™ and efficiency™.
* 
* You can create two kinds of meshes: static and dynamic. Both kinds are made with a MeshCreateFunc which gives
* you a mesh builder delegate that you can use to create your mesh (trying to create a mesh outside of a
* MeshCreateFunc func is undefined behavior). There is no way to edit meshes after creation except for a
* RenderCallback (see mesh_renderer.hpp).
* 
* For stuff that changes a lot or used in e.g. animations, you'll probably want to use dynamic meshes. These must
* be created in the MeshRenderer signal (doing so anywhere else is undefined behavior), and their cleanup is
* handle automatically.
* 
* Static meshes are for stuff that rarely changes and/or stuff that's expensive to recreate every frame. These
* guys are more efficient and can be created at any time (in LoadFeature() or after). To render them you simply
* give your static mesh to the meshrenderer on whatever frames you want it to be drawn. Before rendering, you must
* call their Valid() method to check if they're alive, and recreate them if not. Internally they are shared
* pointers, so they'll get deleted once you get rid of the last copy.
* 
* Creating few big meshes is MUCH more efficient than many small ones, and this is great for opaque meshes. If
* your mesh has ANY translucent vertex however, the whole thing is considered translucent, and it's possible you
* may get meshes getting rendered in the wrong order relative to each other. For translucent meshes the best way
* to solve this is to break your mesh apart into small pieces, but this sucks for fps :(.
*/
class MeshBuilderDelegate
{
public:
	// a single line segment
	void AddLine(const Vector& v1, const Vector& v2, const color32& c);

	// points is a pair-wise array of separate line segments
	void AddLines(const Vector* points, int numSegments, const color32& c);

	// points is an array of points connected by lines
	void AddLineStrip(const Vector* points, int numPoints, bool loop, const color32& c);

	// simple position indicator
	void AddCross(const Vector& pos, float radius, const color32& c);

	// a single triangle oriented clockwise
	void AddTri(const Vector& v1, const Vector& v2, const Vector& v3, const MeshColor& c);

	// verts is a 3-pair-wise clockwise oriented array of points
	void AddTris(const Vector* verts, int numFaces, const MeshColor& c);

	// a single quad oriented clockwise
	void AddQuad(const Vector& v1, const Vector& v2, const Vector& v3, const Vector& v4, const MeshColor& c);

	// verts is a 4-pair-wise clockwise oriented array of points
	void AddQuads(const Vector* verts, int numFaces, const MeshColor& c);

	// verts is a clockwise oriented array of points, polygon is assumed to be simple convex
	void AddPolygon(const Vector* verts, int numVerts, const MeshColor& c);

	// 'pos' is the circle center, an 'ang' of <0,0,0> means the circle normal points towards x+
	void AddCircle(const Vector& pos, const QAngle& ang, float radius, int numPoints, const MeshColor& c);

	void AddEllipse(const Vector& pos,
	                const QAngle& ang,
	                float radiusA,
	                float radiusB,
	                int numPoints,
	                const MeshColor& c);

	void AddBox(const Vector& pos, const Vector& mins, const Vector& maxs, const QAngle& ang, const MeshColor& c);

	// numSubdivisions >= 0, 0 subdivsions is just a cube :)
	void AddSphere(const Vector& pos, float radius, int numSubdivisions, const MeshColor& c);

	void AddSweptBox(const Vector& start,
	                 const Vector& end,
	                 const Vector& mins,
	                 const Vector& maxs,
	                 const MeshColor& cStart,
	                 const MeshColor& cEnd,
	                 const MeshColor& cSweep);

	// 'pos' is at the center of the cone base, an 'ang' of <0,0,0> means the cone tip will point towards x+
	void AddCone(const Vector& pos,
	             const QAngle& ang,
	             float height,
	             float radius,
	             int numCirclePoints,
	             bool drawBase,
	             const MeshColor& c);

	// 'pos' is at the center of the base, an ang of <0,0,0> means the normals are facing x+/x- for top/base
	void AddCylinder(const Vector& pos,
	                 const QAngle& ang,
	                 float height,
	                 float radius,
	                 int numCirclePoints,
	                 bool drawCap1,
	                 bool drawCap2,
	                 const MeshColor& c);

	// a 3D arrow with its tail base at 'pos' pointing towards 'target'
	void AddArrow3D(const Vector& pos,
	                const Vector& target,
	                float tailLength,
	                float tailRadius,
	                float tipHeight,
	                float tipRadius,
	                int numCirclePoints,
	                const MeshColor& c);

	void AddCPolyhedron(const CPolyhedron* polyhedron, const MeshColor& c);

private:
	friend class MeshBuilderPro;
	// internal construction helper methods

	void _AddFaceTriangleStripIndices(size_t vIdx1, size_t vIdx2, size_t numVerts, bool loop, bool mirror = false);
	void _AddFacePolygonIndices(size_t vertsIdx, int numVerts, bool reverse);
	void _AddLineStripIndices(size_t vertsIdx, int numVerts, bool loop);
	void _AddSubdivCube(int numSubdivisions, const MeshColor& c);
	Vector* _CreateEllipseVerts(const Vector& pos, const QAngle& ang, float radiusA, float radiusB, int numPoints);

	MeshBuilderDelegate() = default;
	MeshBuilderDelegate(MeshBuilderDelegate&) = delete;
};

typedef std::function<void(MeshBuilderDelegate& mb)> MeshCreateFunc;

// give this guy a function which accepts a mesh builder delegate -
// it'll be executed immediately so you can capture stuff by reference if you're using a lambda
class MeshBuilderPro
{
public:
	DynamicMesh CreateDynamicMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params = {});
	StaticMesh CreateStaticMesh(const MeshCreateFunc& createFunc, const CreateMeshParams& params = {});
};

inline MeshBuilderPro spt_meshBuilder;

#define MB_DYNAMIC(func, ...) spt_meshBuilder.CreateDynamicMesh([&](MeshBuilderDelegate& mb) { func }, {__VA_ARGS__})
#define MB_STATIC(func, ...) spt_meshBuilder.CreateStaticMesh([&](MeshBuilderDelegate& mb) { func }, {__VA_ARGS__})

#endif

class CPhysCollide;
class IPhysicsObject;
class CBaseEntity;

// collide stuff - returned mesh is an array of tris, consider caching and/or using static meshes
class CreateCollideFeature : public FeatureWrapper<CreateCollideFeature>
{
public:
	// the verts don't contain pos/rot info, so they'll be at the origin
	std::unique_ptr<Vector> CreateCollideMesh(const CPhysCollide* pCollide, int& outNumTris);

	// you'll need to transform the verts by applying a matrix you can create with pPhysObj->GetPosition()
	std::unique_ptr<Vector> CreatePhysObjMesh(const IPhysicsObject* pPhysObj, int& outNumTris);

	// you'll need to transform the verts like described above
	std::unique_ptr<Vector> CreateEntMesh(const CBaseEntity* pEnt, int& outNumTris);

	// can be used after InitHooks()
	static IPhysicsObject* GetPhysObj(const CBaseEntity* pEnt);

	// can be used (after InitHooks()) to check if the above functions do anything
	bool Works();

protected:
	void InitHooks() override;
	void UnloadFeature() override;

private:
	inline static bool cachedOffset = false;
	inline static int cached_phys_obj_off;

	DECL_MEMBER_THISCALL(int, CPhysicsCollision__CreateDebugMesh, const CPhysCollide* pCollide, Vector** outVerts);
};

inline CreateCollideFeature spt_collideToMesh;
