#pragma once

#include "mesh_defs.hpp"
#include "spt\feature.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "mathlib\polyhedron.h"

#include "internal\ref_mgr.hpp"

struct LineColor
{
	color32 lineColor;
	bool zTestLines;

	LineColor() = default;

	LineColor(color32 color, bool zTest = true) : lineColor(color), zTestLines(zTest) {}
};

struct ShapeColor
{
	color32 faceColor, lineColor;
	bool zTestFaces, zTestLines;
	WindingDir wd;

	ShapeColor() = default;

	ShapeColor(color32 faceColor,
	           color32 lineColor,
	           bool zTestFaces = true,
	           bool zTestLines = true,
	           WindingDir wd = WD_CW)
	    : faceColor(faceColor), lineColor(lineColor), zTestFaces(zTestFaces), zTestLines(zTestLines), wd(wd)
	{
	}
};

struct SweptBoxColor
{
	ShapeColor cStart, cSweep, cEnd;

	SweptBoxColor() = default;

	SweptBoxColor(ShapeColor cStart, ShapeColor cSweep, ShapeColor cEnd)
	    : cStart(cStart), cSweep(cSweep), cEnd(cEnd)
	{
	}

	// clang-format off

	SweptBoxColor(color32 startFaceColor, color32 startLineColor,
	              color32 sweepFaceColor, color32 sweepLineColor,
	              color32 endFaceColor, color32 endLineColor,
	              bool zTestStart = true, bool zTestSweep = true, bool zTestEnd = true,
	              WindingDir wdStart = WD_CW, WindingDir wdSweep = WD_CW, WindingDir wdEnd = WD_CW)
	    : cStart(startFaceColor, startLineColor, zTestStart, wdStart)
	    , cSweep(sweepFaceColor, sweepLineColor, zTestSweep, wdSweep)
	    , cEnd(endFaceColor, endLineColor, zTestEnd, wdEnd)
	{
	}

	// clang-format on
};

// these macros can be used for ShapeColor & SweptBoxColor, e.g. ShapeColor{C_OUTLINE(255, 255, 255, 20)}

#define _COLOR(...) (color32{__VA_ARGS__})

#define C_FACE(...) _COLOR(__VA_ARGS__), _COLOR(0, 0, 0, 0)
#define C_WIRE(...) _COLOR(0, 0, 0, 0), _COLOR(__VA_ARGS__)
#define C_OUTLINE(...) \
	_COLOR(__VA_ARGS__), _COLOR(_COLOR(__VA_ARGS__).r, _COLOR(__VA_ARGS__).g, _COLOR(__VA_ARGS__).b, 255)

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
	void AddLine(const Vector& v1, const Vector& v2, LineColor c);

	// points is a pair-wise array of separate line segments
	void AddLines(const Vector* points, int nSegments, LineColor c);

	// points is an array of points connected by lines
	void AddLineStrip(const Vector* points, int nPoints, bool loop, LineColor c);

	// simple position indicator
	void AddCross(const Vector& pos, float radius, LineColor c);

	void AddTri(const Vector& v1, const Vector& v2, const Vector& v3, ShapeColor c);

	// verts is a 3-pair-wise array of points
	void AddTris(const Vector* verts, int nFaces, ShapeColor c);

	void AddQuad(const Vector& v1, const Vector& v2, const Vector& v3, const Vector& v4, ShapeColor c);

	// verts is a 4-pair-wise array of points
	void AddQuads(const Vector* verts, int nFaces, ShapeColor c);

	// verts is a array of points, polygon is assumed to be simple & convex
	void AddPolygon(const Vector* verts, int nVerts, ShapeColor c);

	// 'pos' is the circle center, an 'ang' of <0,0,0> means the circle normal points towards x+
	void AddCircle(const Vector& pos, const QAngle& ang, float radius, int nPoints, ShapeColor c);

	void AddEllipse(const Vector& pos, const QAngle& ang, float radiusA, float radiusB, int nPoints, ShapeColor c);

	void AddBox(const Vector& pos, const Vector& mins, const Vector& maxs, const QAngle& ang, ShapeColor c);

	// nSubdivisions >= 0, 0 subdivsions is just a cube :)
	void AddSphere(const Vector& pos, float radius, int nSubdivisions, ShapeColor c);

	void AddSweptBox(const Vector& start,
	                 const Vector& end,
	                 const Vector& mins,
	                 const Vector& maxs,
	                 const SweptBoxColor& c);

	// 'pos' is at the center of the cone base, an 'ang' of <0,0,0> means the cone tip will point towards x+
	void AddCone(const Vector& pos,
	             const QAngle& ang,
	             float height,
	             float radius,
	             int nCirclePoints,
	             bool drawBase,
	             ShapeColor c);

	// 'pos' is at the center of the base, an ang of <0,0,0> means the normals are facing x+/x- for top/base
	void AddCylinder(const Vector& pos,
	                 const QAngle& ang,
	                 float height,
	                 float radius,
	                 int nCirclePoints,
	                 bool drawCap1,
	                 bool drawCap2,
	                 ShapeColor c);

	// a 3D arrow with its tail base at 'pos' pointing towards 'target'
	void AddArrow3D(const Vector& pos,
	                const Vector& target,
	                float tailLength,
	                float tailRadius,
	                float tipHeight,
	                float tipRadius,
	                int nCirclePoints,
	                ShapeColor c);

	void AddCPolyhedron(const CPolyhedron* polyhedron, ShapeColor c);

private:
	MeshBuilderDelegate() = default;
	MeshBuilderDelegate(MeshBuilderDelegate&) = delete;

	friend struct MeshBuilderInternal;

	// internal construction helper methods

	// clang-format off

	void _AddLine(const Vector& v1, const Vector& v2, color32 c, struct MeshVertData& vd);
	void _AddLineStrip(const Vector* points, int nPoints, bool loop, color32 c, struct MeshVertData& vd);
	void _AddTris(const Vector* verts, int nFaces, ShapeColor c, struct MeshVertData& vdf, struct MeshVertData& vdl);
	void _AddPolygon(const Vector* verts, int nVerts, ShapeColor c, struct MeshVertData& vdf, struct MeshVertData& vdl);
	void _AddFaceTriangleStripIndices(struct MeshVertData& vdf, size_t vIdx1, size_t vIdx2, size_t nVerts, bool loop, bool mirror, WindingDir wd);
	void _AddFacePolygonIndices(struct MeshVertData& vdf, size_t vertsIdx, int nVerts, WindingDir wd);
	void _AddLineStripIndices(struct MeshVertData& vdl, size_t vertsIdx, int nVerts, bool loop);
	void _AddSubdivCube(int nSubdivisions, ShapeColor c, struct MeshVertData& vdf, struct MeshVertData& vdl);
	void _AddUnitCube(ShapeColor c, struct MeshVertData& vdf, struct MeshVertData& vdl);
	Vector* _CreateEllipseVerts(const Vector& pos, const QAngle& ang, float radiusA, float radiusB, int nPoints);

	// clang-format on
};

typedef std::function<void(MeshBuilderDelegate& mb)> MeshCreateFunc;

// give this guy a function which accepts a mesh builder delegate -
// it'll be executed immediately so you can capture stuff by reference if you're using a lambda
class MeshBuilderPro
{
public:
	DynamicMesh CreateDynamicMesh(const MeshCreateFunc& createFunc);
	StaticMesh CreateStaticMesh(const MeshCreateFunc& createFunc);
};

inline MeshBuilderPro spt_meshBuilder;

#define MB_DYNAMIC(func) spt_meshBuilder.CreateDynamicMesh([&](MeshBuilderDelegate& mb) { func })
#define MB_STATIC(func) spt_meshBuilder.CreateStaticMesh([&](MeshBuilderDelegate& mb) { func })

#endif
