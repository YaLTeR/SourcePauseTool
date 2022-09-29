#pragma once

// tons of stuff missing in OE :/
#ifndef OE
#define SPT_MESH_BUILDER_ENABLED

#include <vector>
#include <variant>

#include "interfaces.hpp"

#include "materialsystem\imaterial.h"
#include "materialsystem\imesh.h"

/*
* The renderer needs some position metric for meshes to figure out which order to render translucent meshes in.
* For simplicity, we only keep track of the each meshes' AABB and assume each mesh is a cubical cow. This position
* info is also passed to overlay render callbacks which can be used to tweak the meshes's model matrix during
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
* - BoxLike: useful for axis aligned boxes and planes. The distance to the mesh is defined as the distance to
*   the meshes' AABB.
* - PointLike: useful for non-axis-aligned planes. The distance to the mesh is defined as the distance to the
*   center of its AABB.
*/
enum class TranslucentSortType
{
	BoxLike,
	PointLike,
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
	const TranslucentSortType sortType = TranslucentSortType::BoxLike;
};

typedef std::function<void(class MeshBuilderPro& mb)> MeshCreateFunc;
typedef std::shared_ptr<class OverlayMesh> StaticOverlayMesh;

struct DynamicOverlayMesh
{
	const size_t dynamicOvrIdx;
	const int createdFrame;
};

#define MB_DYNAMIC(mbFunc, ...) MeshBuilderPro::CreateDynamicMesh([&](MeshBuilderPro& mb) { mbFunc }, {__VA_ARGS__})
#define MB_STATIC(mbFunc, ...) MeshBuilderPro::CreateStaticMesh([&](MeshBuilderPro& mb) { mbFunc }, {__VA_ARGS__})

/*
* The game uses a CMeshBuilder to create debug meshes, but we can't use it directly because parts of its
* implementation are private and/or not in the SDK. Not to worry - Introducing The MeshBuilderPro™!
* The MeshBuilderPro™ can be used to create meshes made of both faces and lines as well as different
* colors. The MeshBuilderPro™ also reuses internal mesh & vertex buffers for speed™ and efficiency™.
* 
* You can create two kinds of meshes: static and dynamic. Both kinds are made with a MeshCreateFunc which
* gives you a mesh builder instance that you can use to create your mesh (trying to create a mesh outside
* of a MeshCreateFunc func is undefined behavior).
* 
* For stuff that changes a lot or used in e.g. animations, you'll probably want to use dynamic meshes. These
* must be created in the OverlaySignal (doing so anywhere else is undefined behavior). Dynamic meshes will
* automatically get deleted before the next OverlaySignal; the mesh builder is optimized for creating the
* same dynamic meshes every frame.
* 
* Static meshes are for stuff that rarely changes and/or stuff that's expensive to recreate every frame.
* These guys are super efficient and can be created at any time (in LoadFeature() or after), and to render
* them you simply give your static mesh to the overlay renderer on whatever frames you want it to be drawn.
* Static meshes are shared pointers, so deletion is handled automatically once you've destroyed your last
* reference. Just like for dynamics, there is no way to 'edit' meshes after creation (aside from using the
* MeshRenderCallback).
* 
* Creating few big meshes is MUCH more efficient than many small ones, and this is great for opaque meshes.
* If your mesh has ANY translucent vertex however, the whole thing is considered translucent, and it's
* possible you may get meshes getting rendered in the wrong order relative to each other. For translucent
* meshes the best way to solve this is to break your mesh apart into small pieces, but this sucks for fps :(.
* 
* The mesh create func is executed immediately, so you can capture stuff by reference in a lambda.
*/
class MeshBuilderPro
{
public:
	static DynamicOverlayMesh CreateDynamicMesh(const MeshCreateFunc& createFunc,
	                                            const CreateMeshParams& params = {});

	static StaticOverlayMesh CreateStaticMesh(const MeshCreateFunc& createFunc,
	                                          const CreateMeshParams& params = {});

	// a single line segment
	void AddLine(const Vector& v1, const Vector& v2, const color32& c);

	// points is a pair-wise array of separate line segments
	void AddLines(const Vector* points, int numSegments, const color32& c);

	// points is an array of points connected by lines
	void AddLineStrip(const Vector* points, int numPoints, bool loop, const color32& c);

	// a single triangle oriented clockwise
	void AddTri(const Vector& v1, const Vector& v2, const Vector& v3, const MeshColor& c);

	// verts is a 3-pair-wise clockwise oriented array of points
	void AddTris(const Vector* verts, int numFaces, const MeshColor& c);

	// a single quad oriented clockwise
	void AddQuad(const Vector& v1, const Vector& v2, const Vector& v3, const Vector v4, const MeshColor& c);

	// verts is a 4-pair-wise clockwise oriented array of points
	void AddQuads(const Vector* verts, int numFaces, const MeshColor& c);

	// verts is a clockwise oriented array of points, polygon is assumed to be simple convex
	void AddPolygon(const Vector* verts, int numVerts, const MeshColor& c);

	// 'pos' is the circle center, an 'ang' of <0,0,0> means the circle normal points towards x+
	void AddCircle(const Vector& pos, const QAngle& ang, float radius, int numPoints, const MeshColor& c);

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

private:
	// internal construction helper methods

	void _AddFaceTriangleStripIndices(size_t vIdx1, size_t vIdx2, size_t numVerts, bool loop, bool mirror = false);
	void _AddFacePolygonIndices(size_t vertsIdx, int numVerts, bool reverse);
	void _AddLineStripIndices(size_t vertsIdx, int numVerts, bool loop);
	void _AddSubdivCube(int numSubdivisions, const MeshColor& c);
	Vector* _CreateCircleVerts(const Vector& pos, const QAngle& ang, float radius, int numPoints);

private:
	// we're all private friends here :)
	friend class OverlayMesh;
	friend class OverlayRenderer;
	friend class OvrFeature;
	friend struct OverlayMeshInternal;
	friend struct ReserveScope;

	struct MeshData
	{
		std::vector<struct VertexData> verts;
		std::vector<unsigned short> indices;

		MeshData() = default;
		MeshData(MeshData&) = delete;
		MeshData(MeshData&&) = default;
		void Clear();
		void Reserve(size_t numExtraVerts, size_t numExtraIndices);
		IMesh* CreateIMesh(IMaterial* material, const CreateMeshParams& params, bool faces, bool dynamic) const;
	};

	static MeshBuilderPro singleton;

	int curFrameNum = 0;
	bool inOverlaySignal = false;

	// reused face/line buffers for static meshes, vector of buffers for dynamics
	MeshData staticMDataFaceBuf, staticMDataLineBuf;
	std::vector<std::pair<MeshData, MeshData>> dynamicMDataBufs;
	size_t numDynamicMeshes = 0;

	std::vector<OverlayMesh> dynamicOvrMeshes;

	// points to static/dynamic buffers
	MeshData* faceData = nullptr;
	MeshData* lineData = nullptr;

	IMaterial *matOpaque = nullptr, *matAlpha = nullptr, *matAlphaNoZ = nullptr;
	bool materialsInitialized = false;

	MeshBuilderPro() = default;
	MeshBuilderPro(const MeshBuilderPro&) = delete;

	IMaterial* GetMaterial(bool opaque, bool zTest, color32 colorMod);
	void ClearOldBuffers();
	void BeginNewMesh(bool dynamic);
	OverlayMesh* CreateOverlayMesh(const CreateMeshParams& params, bool dynamic);
	OverlayMesh& GetOvrMesh(const DynamicOverlayMesh& dynamicInfo);
};

class OverlayMesh
{
	friend class MeshBuilderPro;
	friend class OverlayRenderer;
	friend struct OverlayMeshInternal;

	struct MeshComponent
	{
		union
		{
			IMesh* iMesh; // for statics - possibly null
			int mDataIdx; // for dynamics - index the dynamic mesh data buffer
		};
		const bool opaque, faces, dynamic, zTest;

		MeshComponent(const OverlayMesh& outerOvrMesh, int mDataIdx, bool opaque, bool faces, bool dynamic);
		MeshComponent(MeshComponent&) = delete;
		MeshComponent(MeshComponent&&);
		~MeshComponent();
		bool Empty() const;
		IMesh* GetIMesh(const OverlayMesh& outerOvrMesh, IMaterial* material) const;
	};

	const MeshPositionInfo posInfo;
	const CreateMeshParams params; // MUST BE BEFORE MESH COMPONENTS FOR CORRECT INITIALIZATION ORDER
	MeshComponent faceComponent, lineComponent;

public:
	// public just for std::vector :)
	OverlayMesh(int mDataIdx,
	            const MeshPositionInfo& posInfo,
	            const CreateMeshParams& params,
	            bool facesOpaque,
	            bool linesOpaque,
	            bool dynamic);
	OverlayMesh(OverlayMesh&) = delete;
	OverlayMesh(OverlayMesh&&) = default;
};

// In the future this may contain more data like normals & texture coordinates,
// but for now we only support single-colored unlit generics.
struct VertexData
{
	Vector pos;
	color32 col;
};

#endif
