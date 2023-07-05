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

struct ArrowParams
{
	// resolution (>=3)
	int nCirclePoints;
	// the length of the whole arrow
	float arrowLength;
	// the radius of the arrow base
	float radius;
	// must be between 0,1 exclusive; determines what portion of the arrow is made from the tip
	float tipLengthPortion;
	// a multiplier for the tip radius
	float tipRadiusScale;

	ArrowParams() = default;

	ArrowParams(int nCirclePoints,
	            float arrowLength,
	            float radius,
	            float tipLengthPortion = 0.3f,
	            float tipRadiusScale = 2.f)
	    : nCirclePoints(nCirclePoints)
	    , arrowLength(arrowLength)
	    , radius(radius)
	    , tipLengthPortion(tipLengthPortion)
	    , tipRadiusScale(tipRadiusScale)
	{
		AssertMsg(tipLengthPortion > 0 && tipLengthPortion < 1, "arrow will not be rendered");
	}
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
* Each of these functions returns true on success and false on failure. A failure likely means that the internal
* buffers have reached the maximum size and cannot fit the primitive, in which case the mesh is unchanged. It is
* only really necessary to check the return value for very large meshes (probably thousands or tens of thousands
* of primitives), and in the case of failure "spill" to another mesh. Some functions check for invalid parameters
* (e.g. negative nCirclePoints or nSubdivisions), and will return true without adding anything to the mesh.
* 
* Creating few big meshes is generally more efficient than many small ones, but dynamic meshes are automatically
* merged in some cases. If using translucent meshes, then smaller meshes may be required in some cases to increase
* the chance of correct relative render order, (but the correct render order is not guaranteed).
*/
class MeshBuilderDelegate
{
public:
	// a single line segment
	bool AddLine(const Vector& v1, const Vector& v2, LineColor c);

	// points is a pair-wise array of separate line segments
	bool AddLines(const Vector* points, int nSegments, LineColor c);

	// points is an array of points connected by lines
	bool AddLineStrip(const Vector* points, int nPoints, bool loop, LineColor c);

	// simple position indicator
	bool AddCross(const Vector& pos, float radius, LineColor c);

	bool AddTri(const Vector& v1, const Vector& v2, const Vector& v3, ShapeColor c);

	// verts is a 3-pair-wise array of points
	bool AddTris(const Vector* verts, int nFaces, ShapeColor c);

	bool AddQuad(const Vector& v1, const Vector& v2, const Vector& v3, const Vector& v4, ShapeColor c);

	// verts is a 4-pair-wise array of points
	bool AddQuads(const Vector* verts, int nFaces, ShapeColor c);

	// verts is a array of points, polygon is assumed to be simple & convex
	bool AddPolygon(const Vector* verts, int nVerts, ShapeColor c);

	// 'pos' is the circle center, an 'ang' of <0,0,0> means the circle normal points towards x+
	bool AddCircle(const Vector& pos, const QAngle& ang, float radius, int nPoints, ShapeColor c);

	bool AddEllipse(const Vector& pos, const QAngle& ang, float radiusA, float radiusB, int nPoints, ShapeColor c);

	bool AddBox(const Vector& pos, const Vector& mins, const Vector& maxs, const QAngle& ang, ShapeColor c);

	// nSubdivisions >= 0; the sphere looks kind of like a (nSubdivisions X nSubdivisions) pillowed rubik's cube
	bool AddSphere(const Vector& pos, float radius, int nSubdivisions, ShapeColor c);

	bool AddSweptBox(const Vector& start,
	                 const Vector& end,
	                 const Vector& mins,
	                 const Vector& maxs,
	                 const SweptBoxColor& c);

	// 'pos' is at the center of the cone base, an 'ang' of <0,0,0> means the cone tip will point towards x+
	bool AddCone(const Vector& pos,
	             const QAngle& ang,
	             float height,
	             float radius,
	             int nCirclePoints,
	             bool drawBase,
	             ShapeColor c);

	// 'pos' is at the center of the base, an ang of <0,0,0> means the normals are facing x+/x- for top/base
	bool AddCylinder(const Vector& pos,
	                 const QAngle& ang,
	                 float height,
	                 float radius,
	                 int nCirclePoints,
	                 bool drawCap1,
	                 bool drawCap2,
	                 ShapeColor c);

	// a 3D arrow with its tail base at 'pos' pointing towards 'target'
	bool AddArrow3D(const Vector& pos, const Vector& target, ArrowParams params, ShapeColor c);

	bool AddCPolyhedron(const CPolyhedron* polyhedron, ShapeColor c);

private:
	MeshBuilderDelegate() = default;
	MeshBuilderDelegate(MeshBuilderDelegate&) = delete;

	friend struct MeshBuilderInternal;
};

typedef std::function<void(MeshBuilderDelegate& mb)> MeshCreateFunc;

class TmpMesh
{
public:
	DynamicMesh FinalizeToDynamic();
	StaticMesh FinalizeToStatic();

private:
	TmpMesh();
};

// give this guy a function which accepts a mesh builder delegate -
// it'll be executed immediately so you can capture stuff by reference if you're using a lambda
class MeshBuilderPro
{
public:
	DynamicMesh CreateDynamicMesh(const MeshCreateFunc& createFunc);
	StaticMesh CreateStaticMesh(const MeshCreateFunc& createFunc);

	TmpMesh CreateTmpMesh();
};

inline MeshBuilderPro spt_meshBuilder;

#define MB_DYNAMIC(func) spt_meshBuilder.CreateDynamicMesh([&](MeshBuilderDelegate& mb) { func })
#define MB_STATIC(func) spt_meshBuilder.CreateStaticMesh([&](MeshBuilderDelegate& mb) { func })

#endif
