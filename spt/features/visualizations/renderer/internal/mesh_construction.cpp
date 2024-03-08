#include "stdafx.hpp"

#include "internal_defs.hpp"
#include "mesh_builder_internal.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include <memory>

#include "..\mesh_builder.hpp"
#include "spt\utils\math.hpp"

/*
* Whenever a user says they want to add anything to a mesh, we need to know which component to add it to. These
* macros help us do that. For now they are only set up for the basic internal materials, but this will need to be
* improved/changed to work for text & custom materials. For now the usage looks like this:
* 
* // gets the appropriate mesh vert data that has the correct ztest & opaque flag as specified in the color
* auto& faceData = GET_MVD_FACES(color);
* // same thing for line data
* auto& lineData = GET_MVD_LINES(color);
* 
* faceData.vertices.emplace_back(...);
* ...
*/

#define _MATERIAL_TYPE_FROM_COLOR(color_rgba, zTest) \
	((zTest) ? ((color_rgba).a == 255 ? MeshMaterialSimple::Opaque : MeshMaterialSimple::Alpha) \
	         : MeshMaterialSimple::AlphaNoZ)

#define GET_MVD_FACES(color) \
	g_meshBuilderInternal.GetSimpleMeshComponent(MeshPrimitiveType::Triangles, \
	                                             _MATERIAL_TYPE_FROM_COLOR((color).faceColor, (color).zTestFaces))
#define GET_MVD_LINES(color) \
	g_meshBuilderInternal.GetSimpleMeshComponent(MeshPrimitiveType::Lines, \
	                                             _MATERIAL_TYPE_FROM_COLOR((color).lineColor, (color).zTestLines))

// winding direction macros

// WD_BOTH -> 2, WD_CW,WD_CCW -> 1
#define WD_INDEX_MULTIPLIER(wd) (((wd)&1) + (((wd)&2) >> 1))

// swaps lowest 2 bits; WD_BOTH -> WD_BOTH, WD_CW -> WD_CCW, WD_CCW -> WD_CW
#define WD_INVERT(wd) ((WindingDir)((((wd)&1) << 1) | (((wd)&2) >> 1)))

/*
* Each function that edits any mesh vert data buffers returns true on success and false on failure. A failure
* could be anything, but usually it means that the function would overflow the buffers. An empty primitive should
* return true. If false is returned, the function is expected to revert the state of the buffers to whatever the
* state was before being called. To aid with this we have these macros - there are two main ways to use them.
* 
* Method 1 (prefered):
* 
* MVD_CHECKPOINT(...);
* // figure out how many verts/indices this function will add ahead of time
* if (MVD_WILL_OVERFLOW(...)) {
*     MVD_ROLLBACK(...); // only necessary if any buffers were changed
*     return false;
* }
* // add stuff to buffers
* MVD_SIZE_VERIFY(...);
* return true;
* 
* 
* Method 2:
* 
* MVD_CHECKPOINT(...);
* // add stuff to buffers
* if (MVD_OVERFLOWED(...)) {
*     MVD_ROLLBACK(...);
*     return false;
* }
* return true;
* 
* 
* Method 1 is preferred because it provides an early exit condition, and it gives a sanity check to see if the
* expected number of verts/indices was added. However, it may not always be reasonable to calculate exactly how
* many verts/indices will be added by the function (when the primitive is made of many other primtivs, e.g.
* arrows, swept boxes), in which case method 2 may be used.
* 
* It's only necessary to check the return result of every function if method 2 is used. This is because by using
* method 1 you are "predicting" that you will use a specific number of verts/indices and asserting that amount to
* be correct at the very end of the function. In method 2 you are adding some number of elements that isn't
* precalculated, and MVD_OVERFLOWED(...) may return false even if any of the called functions failed.
*/

#define _MVD_MAX_VERTS g_meshBuilderInternal.tmpMesh.maxVerts
#define _MVD_MAX_INDICES g_meshBuilderInternal.tmpMesh.maxIndices

// save how many verts/indices we have
#define MVD_CHECKPOINT(mvd) \
	[[maybe_unused]] const size_t mvd##_origNumVerts = mvd.verts.size(); \
	[[maybe_unused]] const size_t mvd##_origNumIndices = mvd.indices.size(); \
	[[maybe_unused]] size_t mvd##_expectedVerts = mvd##_origNumVerts; \
	[[maybe_unused]] size_t mvd##_expectedIndices = mvd##_origNumIndices;

#define MVD_CHECKPOINT2(mvd1, mvd2) \
	MVD_CHECKPOINT(mvd1); \
	MVD_CHECKPOINT(mvd2);

// restore buffer state back to checkpoint
#define MVD_ROLLBACK(mvd) \
	{ \
		mvd.verts.resize(mvd##_origNumVerts); \
		mvd.indices.resize(mvd##_origNumIndices); \
	}

#define MVD_ROLLBACK2(mvd1, mvd2) \
	{ \
		MVD_ROLLBACK(mvd1); \
		MVD_ROLLBACK(mvd2); \
	}

#define MVD_OVERFLOWED(mvd) (mvd.verts.size() >= _MVD_MAX_VERTS || mvd.indices.size() >= _MVD_MAX_INDICES)

#define MVD_OVERFLOWED2(mvd1, mvd2) (MVD_OVERFLOWED(mvd1) || MVD_OVERFLOWED(mvd2))

// overflow prediction, saves expected counts to be used in MVD_SIZE_VERIFY(...)
#define MVD_WILL_OVERFLOW(mvd, numExtraVerts, numExtraIndices) \
	((mvd##_expectedVerts = mvd.verts.size() + (numExtraVerts)) >= _MVD_MAX_VERTS \
	 || (mvd##_expectedIndices = mvd.indices.size() + (numExtraIndices)) >= _MVD_MAX_INDICES)

// in a debug build, checks if the counts passed to MVD_WILL_OVERFLOW(...) was correct
#define MVD_SIZE_VERIFY(mvd) \
	{ \
		AssertMsg2(mvd.verts.size() == mvd##_expectedVerts, \
		           "Expected %u extra verts, got %u", \
		           mvd##_expectedVerts - mvd##_origNumVerts, \
		           mvd.verts.size() - mvd##_origNumVerts); \
		AssertMsg2(mvd.indices.size() == mvd##_expectedIndices, \
		           "Expected %u extra indices, got %u", \
		           mvd##_expectedIndices - mvd##_origNumIndices, \
		           mvd.indices.size() - mvd##_origNumIndices); \
	}

#define MVD_SIZE_VERIFY2(mvd1, mvd2) \
	{ \
		MVD_SIZE_VERIFY(mvd1); \
		MVD_SIZE_VERIFY(mvd2); \
	}

/**************************************** HELPER FUNCTIONS ****************************************/

static bool MvdAddLine(MeshVertData& vdl, const Vector& v1, const Vector& v2, color32 c)
{
	if (c.a == 0)
		return true;

	MVD_CHECKPOINT(vdl);
	if (MVD_WILL_OVERFLOW(vdl, 2, 2))
		return false;

	vdl.indices.push_back(vdl.verts.size());
	vdl.indices.push_back(vdl.verts.size() + 1);
	vdl.verts.emplace_back(v1, c);
	vdl.verts.emplace_back(v2, c);

	MVD_SIZE_VERIFY(vdl);
	return true;
}

static bool MvdAddLineStripIndices(MeshVertData& vdl, size_t vertsIdx, int nVerts, bool loop)
{
	Assert(nVerts >= 2);
	MVD_CHECKPOINT(vdl);
	if (MVD_WILL_OVERFLOW(vdl, 0, (nVerts - 1 + (loop && nVerts > 2)) * 2))
		return false;

	for (int i = 0; i < nVerts - 1; i++)
	{
		vdl.indices.push_back(vertsIdx + i);
		vdl.indices.push_back(vertsIdx + i + 1);
	}
	if (loop && nVerts > 2)
	{
		vdl.indices.push_back(vertsIdx + nVerts - 1);
		vdl.indices.push_back(vertsIdx);
	}

	MVD_SIZE_VERIFY(vdl);
	return true;
}

static bool MvdAddLineStrip(MeshVertData& vdl, const Vector* points, int nPoints, bool loop, color32 c)
{
	if (!points || nPoints < 2 || c.a == 0)
		return true;

	MVD_CHECKPOINT(vdl);
	if (MVD_WILL_OVERFLOW(vdl, nPoints, (nPoints - 1 + loop) * 2))
		return false;

	for (int i = 0; i < nPoints; i++)
		vdl.verts.emplace_back(points[i], c);
	MvdAddLineStripIndices(vdl, vdl.verts.size() - nPoints, nPoints, loop);

	MVD_SIZE_VERIFY(vdl);
	return true;
}

static bool MvdAddTris(MeshVertData& vdf, MeshVertData& vdl, const Vector* verts, int nFaces, ShapeColor c)
{
	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;

	if (!verts || nFaces <= 0 || !(c.wd & WD_BOTH) || (!doFaces && !doLines))
		return true;

	MVD_CHECKPOINT2(vdf, vdl);

	if (doFaces && MVD_WILL_OVERFLOW(vdf, nFaces * 3, nFaces * 3 * WD_INDEX_MULTIPLIER(c.wd)))
		return false;
	if (doLines && MVD_WILL_OVERFLOW(vdl, nFaces * 3, nFaces * 6))
		return false;

	if (doFaces)
	{
		for (int i = 0; i < nFaces; i++)
		{
			size_t vIdx = vdf.verts.size();
			vdf.verts.emplace_back(verts[3 * i + 0], c.faceColor);
			vdf.verts.emplace_back(verts[3 * i + 1], c.faceColor);
			vdf.verts.emplace_back(verts[3 * i + 2], c.faceColor);
			if (c.wd & WD_CW)
			{
				vdf.indices.push_back(vIdx + 0);
				vdf.indices.push_back(vIdx + 1);
				vdf.indices.push_back(vIdx + 2);
			}
			if (c.wd & WD_CCW)
			{
				vdf.indices.push_back(vIdx + 2);
				vdf.indices.push_back(vIdx + 1);
				vdf.indices.push_back(vIdx + 0);
			}
		}
	}

	if (doLines)
	{
		for (int i = 0; i < nFaces; i++)
		{
			for (int k = 0; k < 3; k++)
				vdl.verts.emplace_back(verts[3 * i + k], c.lineColor);
			MvdAddLineStripIndices(vdl, vdl.verts.size() - 3, 3, true);
		}
	}

	MVD_SIZE_VERIFY2(vdf, vdl);
	return true;
}

static bool MvdAddFaceTriangleStripIndices(MeshVertData& vdf,
                                           size_t vIdx1,
                                           size_t vIdx2,
                                           size_t nVerts,
                                           bool loop,
                                           bool mirror,
                                           WindingDir wd)
{
	/*
	* Creates indices representing a filled triangle strip using existing verts at the given vert indices for faces only.
	* +---+---+---+  <- verts at vIdx1
	* |\  |\  |\  |
	* | \ | \ | \ |
	* |  \|  \|  \|
	* +---+---+---+  <- verts at vIdx2
	* If mirroring, the triangles lean the other way (but still face the same direction), only used for swept boxes.
	*/
	Assert(wd & WD_BOTH);
	if (loop)
		Assert(nVerts >= 3);
	else
		Assert(nVerts >= 2);

	MVD_CHECKPOINT(vdf);
	if (MVD_WILL_OVERFLOW(vdf, 0, (nVerts - 1 + loop) * 6 * WD_INDEX_MULTIPLIER(wd)))
		return false;

	size_t origSize = vdf.indices.size();

	for (size_t i = 0; i < nVerts - 1; i++)
	{
		if (mirror)
		{
			vdf.indices.push_back(vIdx1 + i);
			vdf.indices.push_back(vIdx2 + i);
			vdf.indices.push_back(vIdx1 + i + 1);
			vdf.indices.push_back(vIdx2 + i);
			vdf.indices.push_back(vIdx2 + i + 1);
			vdf.indices.push_back(vIdx1 + i + 1);
		}
		else
		{
			vdf.indices.push_back(vIdx1 + i);
			vdf.indices.push_back(vIdx2 + i);
			vdf.indices.push_back(vIdx2 + i + 1);
			vdf.indices.push_back(vIdx2 + i + 1);
			vdf.indices.push_back(vIdx1 + i + 1);
			vdf.indices.push_back(vIdx1 + i);
		}
	}
	if (loop)
	{
		if (mirror) // loop + mirror not tested
		{
			vdf.indices.push_back(vIdx1 + nVerts - 1);
			vdf.indices.push_back(vIdx2 + nVerts - 1);
			vdf.indices.push_back(vIdx1);
			vdf.indices.push_back(vIdx2 + nVerts - 1);
			vdf.indices.push_back(vIdx2);
			vdf.indices.push_back(vIdx1);
		}
		else
		{
			vdf.indices.push_back(vIdx1 + nVerts - 1);
			vdf.indices.push_back(vIdx2 + nVerts - 1);
			vdf.indices.push_back(vIdx2);
			vdf.indices.push_back(vIdx2);
			vdf.indices.push_back(vIdx1);
			vdf.indices.push_back(vIdx1 + nVerts - 1);
		}
	}

	if ((wd & WD_BOTH) == WD_BOTH)
	{
		vdf.indices.add_range(vdf.indices.rbegin(), vdf.indices.rend() - origSize);
	}
	else if ((wd & WD_BOTH) == WD_CCW)
	{
		std::reverse(vdf.indices.begin() + origSize, vdf.indices.end());
	}

	MVD_SIZE_VERIFY(vdf);
	return true;
}

static bool MvdAddFacePolygonIndices(MeshVertData& vdf, size_t vertsIdx, int nVerts, WindingDir wd)
{
	// Creates indices representing a filled convex polygon using existing verts at the given vert index for faces only.
	Assert(nVerts >= 3);
	Assert(wd & WD_BOTH);

	MVD_CHECKPOINT(vdf);
	if (MVD_WILL_OVERFLOW(vdf, 0, (nVerts - 2) * 3 * WD_INDEX_MULTIPLIER(wd)))
		return false;

	for (int i = 0; i < nVerts - 2; i++)
	{
		if (wd & WD_CW)
		{
			vdf.indices.push_back(vertsIdx);
			vdf.indices.push_back(vertsIdx + i + 1);
			vdf.indices.push_back(vertsIdx + i + 2);
		}
		if (wd & WD_CCW)
		{
			vdf.indices.push_back(vertsIdx + i + 2);
			vdf.indices.push_back(vertsIdx + i + 1);
			vdf.indices.push_back(vertsIdx);
		}
	}

	MVD_SIZE_VERIFY(vdf);
	return true;
}

static bool MvdAddPolygon(MeshVertData& vdf, MeshVertData& vdl, const Vector* verts, int nVerts, ShapeColor c)
{
	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;

	if (!verts || nVerts < 3 || !(c.wd & WD_BOTH) || (!doFaces && !doLines))
		return true;

	MVD_CHECKPOINT2(vdf, vdl);
	if (doFaces && MVD_WILL_OVERFLOW(vdf, nVerts, (nVerts - 2) * 3 * WD_INDEX_MULTIPLIER(c.wd)))
		return false;

	if (doLines && !MvdAddLineStrip(vdl, verts, nVerts, true, c.lineColor))
		return false;

	if (doFaces)
	{
		for (int i = 0; i < nVerts; i++)
			vdf.verts.emplace_back(verts[i], c.faceColor);
		MvdAddFacePolygonIndices(vdf, vdf.verts.size() - nVerts, nVerts, c.wd);
	}

	MVD_SIZE_VERIFY(vdf);
	return true;
}

static bool MvdAddUnitCube(MeshVertData& vdf, MeshVertData& vdl, ShapeColor c)
{
	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;

	if ((!doFaces && !doLines) || !(c.wd & WD_BOTH))
		return true;

	// we create a unit cube with mins at <0,0,0> and maxs at <1,1,1>
	// I had to work all this out on a whiteboard if it seems completely unintuitive

	static std::array<Vector, 8>
	    verts{Vector{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

	static std::array<VertIndex, 36> faceIndices{0, 1, 3, 1, 2, 3, 4, 5, 1, 4, 1, 0, 5, 6, 2, 5, 2, 1,
	                                             6, 7, 3, 6, 3, 2, 7, 4, 0, 7, 0, 3, 7, 6, 5, 7, 5, 4};

	static std::array<VertIndex, 24> lineIndices{0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
	                                             6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

	MVD_CHECKPOINT2(vdf, vdl);

	if (doFaces && MVD_WILL_OVERFLOW(vdf, verts.size(), faceIndices.size() * WD_INDEX_MULTIPLIER(c.wd)))
		return false;
	if (doLines && MVD_WILL_OVERFLOW(vdl, verts.size(), lineIndices.size()))
		return false;

	if (doFaces)
	{
		size_t origVertCount = vdf.verts.size();
		size_t origIdxCount = vdf.indices.size();

		for (Vector pos : verts)
			vdf.verts.emplace_back(pos, c.faceColor);

		if (c.wd & WD_CW)
			vdf.indices.add_range(faceIndices.cbegin(), faceIndices.cend());
		if (c.wd & WD_CCW)
			vdf.indices.add_range(faceIndices.crbegin(), faceIndices.crend());

		for (size_t i = origIdxCount; i < vdf.indices.size(); i++)
			vdf.indices[i] += origVertCount;
	}
	if (doLines)
	{
		size_t origVertCount = vdl.verts.size();
		size_t origIdxCount = vdl.indices.size();

		for (Vector pos : verts)
			vdl.verts.emplace_back(pos, c.lineColor);
		vdl.indices.add_range(lineIndices.cbegin(), lineIndices.cend());

		for (size_t i = origIdxCount; i < vdl.indices.size(); i++)
			vdl.indices[i] += origVertCount;
	}

	MVD_SIZE_VERIFY2(vdf, vdl);
	return true;
}

static bool MvdAddSubdivCube(MeshVertData& vdf, MeshVertData& vdl, int nSubdivisions, ShapeColor c)
{
	if (nSubdivisions == 0)
		return MvdAddUnitCube(vdf, vdl, c);

	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;
	if (nSubdivisions < 0 || (!doFaces && !doLines))
		return true;

	/*
	* Each cube face will look like this for e.g. 2 subdivisions:
	* ┌───┬───┬───┐
	* │   │   │   │
	* ├───┼───┼───┤
	* │   │   │   │
	* ├───┼───┼───┤
	* │   │   │   │
	* └───┴───┴───┘
	* Mins are at <0,0,0> & maxs are at <subdivs + 1, subdivs + 1, subdivs + 1>.
	* I used to make this out of quads but the current alg uses 1.5-4 times less verts depending on nSubdivisions.
	*/

	const int sideLength = nSubdivisions + 1;
	const int nLayerVerts = sideLength * 4;

	const int numTotalVerts = 2 * (3 * sideLength + 1) * (sideLength + 1);
	const int numTotalFaceIndices = 36 * sideLength * sideLength * WD_INDEX_MULTIPLIER(c.wd);
	const int numTotalLineIndices = 12 * sideLength * (2 * sideLength + 1);

	MVD_CHECKPOINT2(vdf, vdl);

	if (MVD_WILL_OVERFLOW(vdf, doFaces ? numTotalVerts : 0, doFaces ? numTotalFaceIndices : 0)
	    || MVD_WILL_OVERFLOW(vdl, doLines ? numTotalVerts : 0, doLines ? numTotalLineIndices : 0))
	{
		return false;
	}

	// fill in everything except for caps by winding around the cube
	for (int z = 0; z <= sideLength; z++)
	{
		int x = 0;
		int y = 0;
		for (int i = 0; i < nLayerVerts; i++)
		{
			// add all verts for this layer in a counter-clockwise order
			if (i < sideLength)
				x++;
			else if (i < sideLength * 2)
				y++;
			else if (i < sideLength * 3)
				x--;
			else
				y--;

			if (doFaces)
				vdf.verts.push_back({Vector(x, y, z), c.faceColor});
			if (doLines)
				vdl.verts.push_back({Vector(x, y, z), c.lineColor});
		}
		if (doFaces && z > 0)
		{
			size_t n = vdf.verts.size();
			MvdAddFaceTriangleStripIndices(vdf,
			                               n - nLayerVerts * 2,
			                               n - nLayerVerts,
			                               nLayerVerts,
			                               true,
			                               false,
			                               c.wd);
		}
		if (doLines)
		{
			size_t n = vdl.verts.size();
			MvdAddLineStripIndices(vdl, n - nLayerVerts, nLayerVerts, true);
			if (z > 0)
			{
				for (int k = 0; k < nLayerVerts; k++)
				{
					vdl.indices.push_back(n + k - nLayerVerts * 2);
					vdl.indices.push_back(n + k - nLayerVerts);
				}
			}
		}
	}

	// fill caps
	for (int top = 0; top <= 1; top++)
	{
		int z = top * sideLength;
		for (int x = 0; x <= sideLength; x++)
		{
			for (int y = 0; y <= sideLength; y++)
			{
				if (doFaces)
					vdf.verts.push_back({Vector(x, y, z), c.faceColor});
				if (doLines)
					vdl.verts.push_back({Vector(x, y, z), c.lineColor});
			}
			if (doFaces && x > 0)
			{
				size_t a = vdf.verts.size() - (sideLength + 1) * 2;
				size_t b = vdf.verts.size() - (sideLength + 1);
				if (top)
					std::swap(a, b);
				MvdAddFaceTriangleStripIndices(vdf, a, b, sideLength + 1, false, false, c.wd);
			}
			if (doLines)
			{
				size_t n = vdl.verts.size();
				MvdAddLineStripIndices(vdl, n - 1 - sideLength, sideLength + 1, false);
				if (x > 0)
				{
					for (int k = 0; k < sideLength; k++)
					{
						vdl.indices.push_back(n + k - (sideLength + 1) * 2);
						vdl.indices.push_back(n + k - (sideLength + 1));
					}
				}
			}
		}
	}

	MVD_SIZE_VERIFY2(vdf, vdl);
	return true;
}

static Vector* Scratch(size_t n)
{
	static std::unique_ptr<Vector[]> scratch = nullptr;
	static size_t count = 0;
	if (count < n)
	{
		count = SmallestPowerOfTwoGreaterOrEqual(n);
		scratch.reset(new Vector[count]);
	}
	return scratch.get();
}

// note that this does *not* take the absolute value of the radii
static Vector* CreateEllipseVerts(const Vector& pos, const QAngle& ang, float radiusA, float radiusB, int nPoints)
{
	VMatrix mat;
	mat.SetupMatrixOrgAngles(pos, ang);
	Vector* scratch = Scratch(nPoints);
	for (int i = 0; i < nPoints; i++)
	{
		float s, c;
		SinCos(M_PI_F * 2 / nPoints * i, &s, &c);
		// oriented clockwise, normal is towards x+ for an angle of <0,0,0>
		scratch[i] = mat * Vector(0, s * radiusA, c * radiusB);
	}
	return scratch;
}

/**************************************** Mesh Builder Delegate ****************************************/

bool MeshBuilderDelegate::AddLine(const Vector& v1, const Vector& v2, LineColor c)
{
	return MvdAddLine(GET_MVD_LINES(c), v1, v2, c.lineColor);
}

bool MeshBuilderDelegate::AddLines(const Vector* points, int nSegments, LineColor c)
{
	if (!points || nSegments <= 0 || c.lineColor.a == 0)
		return true;

	auto& vdl = GET_MVD_LINES(c);
	MVD_CHECKPOINT(vdl);
	if (MVD_WILL_OVERFLOW(vdl, nSegments * 2, nSegments * 2))
		return false;

	for (int i = 0; i < nSegments * 2; i++)
	{
		vdl.indices.push_back(vdl.verts.size());
		vdl.verts.emplace_back(points[i], c.lineColor);
	}

	MVD_SIZE_VERIFY(vdl);
	return true;
}

bool MeshBuilderDelegate::AddLineStrip(const Vector* points, int nPoints, bool loop, LineColor c)
{
	return MvdAddLineStrip(GET_MVD_LINES(c), points, nPoints, loop, c.lineColor);
}

bool MeshBuilderDelegate::AddCross(const Vector& pos, float radius, LineColor c)
{
	if (c.lineColor.a == 0)
		return true;

	auto& vdl = GET_MVD_LINES(c);
	MVD_CHECKPOINT(vdl);
	if (MVD_WILL_OVERFLOW(vdl, 8, 8))
		return false;

	for (int x = -1; x <= 1; x += 2)
	{
		for (int y = -1; y <= 1; y += 2)
		{
			static float axf = powf(1.f / 3, 0.5f);
			Vector v = Vector{x * axf, y * axf, axf} * radius;
			MvdAddLine(vdl, -v + pos, v + pos, c.lineColor);
		}
	}

	MVD_SIZE_VERIFY(vdl);
	return true;
}

bool MeshBuilderDelegate::AddTri(const Vector& v1, const Vector& v2, const Vector& v3, ShapeColor c)
{
	Vector v[] = {v1, v2, v3};
	return AddTris(v, 1, c);
}

bool MeshBuilderDelegate::AddTris(const Vector* verts, int nFaces, ShapeColor c)
{
	return MvdAddTris(GET_MVD_FACES(c), GET_MVD_LINES(c), verts, nFaces, c);
}

bool MeshBuilderDelegate::AddQuad(const Vector& v1, const Vector& v2, const Vector& v3, const Vector& v4, ShapeColor c)
{
	Vector v[] = {v1, v2, v3, v4};
	return AddPolygon(v, 4, c);
}

// prolly not used much so not optimized
bool MeshBuilderDelegate::AddQuads(const Vector* verts, int nFaces, ShapeColor c)
{
	if (!verts || nFaces <= 0 || !(c.wd & WD_BOTH))
		return true;

	auto& vdf = GET_MVD_FACES(c);
	auto& vdl = GET_MVD_LINES(c);
	MVD_CHECKPOINT2(vdf, vdl);

	for (int i = 0; i < nFaces; i++)
	{
		if (!MvdAddPolygon(vdf, vdl, verts + i * 4, 4, c))
		{
			MVD_ROLLBACK2(vdf, vdl);
			return false;
		}
	}
	return true;
}

bool MeshBuilderDelegate::AddPolygon(const Vector* verts, int nVerts, ShapeColor c)
{
	return MvdAddPolygon(GET_MVD_FACES(c), GET_MVD_LINES(c), verts, nVerts, c);
}

bool MeshBuilderDelegate::AddCircle(const Vector& pos, const QAngle& ang, float radius, int nPoints, ShapeColor c)
{
	return AddEllipse(pos, ang, radius, radius, nPoints, c);
}

bool MeshBuilderDelegate::AddEllipse(const Vector& pos,
                                     const QAngle& ang,
                                     float radiusA,
                                     float radiusB,
                                     int nPoints,
                                     ShapeColor c)
{
	if (nPoints < 3 || (c.faceColor.a == 0 && c.lineColor.a == 0) || !(c.wd & WD_BOTH))
		return true;
	if (radiusA < 0 != radiusB < 0)
		c.wd = WD_INVERT(c.wd);
	return AddPolygon(CreateEllipseVerts(pos, ang, radiusA, radiusB, nPoints), nPoints, c);
}

bool MeshBuilderDelegate::AddBox(const Vector& pos,
                                 const Vector& mins_,
                                 const Vector& maxs_,
                                 const QAngle& ang,
                                 ShapeColor c)
{
	if ((c.faceColor.a == 0 && c.lineColor.a == 0) || !(c.wd & WD_BOTH))
		return true;

	auto& vdf = GET_MVD_FACES(c);
	auto& vdl = GET_MVD_LINES(c);
	size_t origNumFaceVerts = vdf.verts.size();
	size_t origNumLineVerts = vdl.verts.size();

	if (!MvdAddUnitCube(vdf, vdl, c))
		return false;

	Vector size, mins;
	matrix3x4_t scaleMat, offMat, finalMat;
	VectorAbs(maxs_ - mins_, size);
	VectorMin(mins_, maxs_, mins); // in case anyone stupid (like me) swaps the mins/maxs
	scaleMat.Init({size.x, 0, 0}, {0, size.y, 0}, {0, 0, size.z}, mins); // set up box dimensions at origin
	AngleMatrix(ang, pos, offMat);                                       // rotate box and put at 'pos'
	MatrixMultiply(offMat, scaleMat, finalMat);

	for (size_t i = origNumFaceVerts; i < vdf.verts.size(); i++)
		utils::VectorTransform(finalMat, vdf.verts[i].pos);
	for (size_t i = origNumLineVerts; i < vdl.verts.size(); i++)
		utils::VectorTransform(finalMat, vdl.verts[i].pos);

	return true;
}

bool MeshBuilderDelegate::AddSphere(const Vector& pos, float radius, int nSubdivisions, ShapeColor c)
{
	if (nSubdivisions < 0 || (c.faceColor.a == 0 && c.lineColor.a == 0) || !(c.wd & WD_BOTH))
		return true;

	radius = fabsf(radius); // :)

	auto& vdf = GET_MVD_FACES(c);
	auto& vdl = GET_MVD_LINES(c);
	size_t origNumFaceVerts = vdf.verts.size();
	size_t origNumLineVerts = vdl.verts.size();

	if (!MvdAddSubdivCube(vdf, vdl, nSubdivisions, c))
		return false;

	// center, normalize, scale, transform :)
	Vector subdivCubeOff((nSubdivisions + 1) / -2.f);
	for (size_t i = origNumFaceVerts; i < vdf.verts.size(); i++)
	{
		Vector& v = vdf.verts[i].pos;
		v += subdivCubeOff;
		VectorNormalize(v);
		v = v * radius + pos;
	}
	for (size_t i = origNumLineVerts; i < vdl.verts.size(); i++)
	{
		Vector& v = vdl.verts[i].pos;
		v += subdivCubeOff;
		VectorNormalize(v);
		v = v * radius + pos;
	}
	return true;
}

bool MeshBuilderDelegate::AddSweptBox(const Vector& start,
                                      const Vector& end,
                                      const Vector& _mins,
                                      const Vector& _maxs,
                                      const SweptBoxColor& c)
{
	/*
	* This looks a bit different from the debug overlay swept box, the differences are:
	* - here we actually draw the faces (debug overlay only does wireframe)
	* - we draw the full boxes
	* - we check for axes alignment to make drawings prettier
	* 
	* If you can figure out how this code works on your own then you should probably be doing
	* more important things for humanity, like curing cancer - there's 3 completely different
	* cases and one actually has 3 subcases. A spell can be cast to understand this code, but
	* it requires an ointment that costs 25 gp made from mushroom powder, saffron, and fat.
	*/

	// check for alignment
	Vector diff = end - start;
	bool alignedOn[3];
	int numAlignedAxes = 0;
	for (int i = 0; i < 3; i++)
	{
		alignedOn[i] = fabsf(diff[i]) < 0.01;
		numAlignedAxes += alignedOn[i];
	}

	Vector mins, maxs; // in case anyone stupid (like me) swaps these around
	VectorMin(_mins, _maxs, mins);
	VectorMax(_mins, _maxs, maxs);

	// 6 MVDs!!! we need to be able to revert all of them back to their original state if we overflow

	auto& vdf_s = GET_MVD_FACES(c.cStart);
	auto& vdl_s = GET_MVD_LINES(c.cStart);
	auto& vdf_e = GET_MVD_FACES(c.cEnd);
	auto& vdl_e = GET_MVD_LINES(c.cEnd);
	// these are the ones we're working on in this function
	auto& vdf = GET_MVD_FACES(c.cSweep);
	auto& vdl = GET_MVD_LINES(c.cSweep);

	MVD_CHECKPOINT2(vdf_s, vdl_s);
	MVD_CHECKPOINT2(vdf_e, vdl_e);
	MVD_CHECKPOINT2(vdf, vdl);

	// scale the boxes a bit to prevent z fighting
	const Vector startEndNudge(0.04f);

	if (!AddBox(end, mins - startEndNudge * 1.5, maxs - startEndNudge * 1.5, vec3_angle, c.cEnd))
		return false;

	if (!AddBox(start, mins - startEndNudge, maxs - startEndNudge, vec3_angle, c.cStart))
	{
		MVD_ROLLBACK2(vdf_e, vdl_e);
		return false;
	}

	// For a given box, we can encode a face of the box with a 0 or 1 corresponding to mins/maxs.
	// With 3 bits, you can encode the two "same" corners on both boxes.
	Vector mm[] = {mins, maxs};

	std::array<Vector, 4> tmpQuad;

	switch (numAlignedAxes)
	{
	case 2:
	{
		int ax0 = alignedOn[0] ? (alignedOn[1] ? 2 : 1) : 0; // extruding along this axis

		if (abs(diff[ax0]) < mm[1][ax0] - mm[0][ax0])
			return true; // sweep is shorter than the box size

		int ax1 = (ax0 + 1) % 3;
		int ax2 = (ax0 + 2) % 3;
		/*
		* Connect face A of the start box with face B of the end box. Side by side we have:
		* 1┌───┐2   1┌───┐2
		*  │ A │     │ B │
		* 0└───┘3   0└───┘3
		* We visit verts {(A0, B0, B1, A1), (A1, B1, B2, A2), ...}.
		* Converting each pair of verts in the above list to ax1/ax2 we want:
		* ax1 (pretend this is x in the above diagram): {(0, 0), (0, 1), (1, 1), (1, 0)}
		* ax2 (pretend this is y in the above diagram): {(0, 1), (1, 1), (1, 0), (0, 0)}
		*/
		int ax1Sign = Sign(diff[ax0]) == -1;
		for (int i = 0; i < 16; i++)
		{
			int whichBox = ((i + 1) % 4 / 2 + ax1Sign) % 2;
			const Vector& boxOff = whichBox ? start : end;
			tmpQuad[i % 4][ax0] = boxOff[ax0] + mm[(ax1Sign + whichBox) % 2][ax0];
			tmpQuad[i % 4][ax1] = boxOff[ax1] + mm[(i + 2) % 16 / 8][ax1];
			tmpQuad[i % 4][ax2] = boxOff[ax2] + mm[(i + 6) % 16 / 8][ax2];
			if (i % 4 == 3 && !MvdAddPolygon(vdf, vdl, tmpQuad.data(), 4, c.cSweep))
				goto rollback_all;
		}
		break;
	}
	case 1:
	{
		int ax0 = alignedOn[0] ? 0 : (alignedOn[1] ? 1 : 2); // aligned on this axis
		int ax1 = (ax0 + 1) % 3;
		int ax2 = (ax0 + 2) % 3;
		/*
		* Looking at a box edge on, we see two faces like so:
		* A   B   C
		* ┌───┬───┐
		* │   │   │
		* └───┴───┘
		* D   E   F
		* From the perspective of the aligned axis, we wish to connect to 2 boxes like:
		*          C1
		*    ┌──────┐xx
		*    │      │  xxx
		*    │    B1│     xxx
		*  A1└──────┘..      xxx
		*    xx        .        xx
		*      xxx      ..┌──────┐C2
		*         xxx     │b2    │
		*            xxx  │      │
		*               xx└──────┘B2
		*                 A2
		* Where 'x' is shown in the wireframe but '.' is not (only generated for face quads).
		* We'll encode a corner as 3 bits ZYX (e.g. +y is 0b010, -y is 00b00), this allows us
		* to do e.g. 'bits ^ (1 << ax0)' to go to the other corner along ax0. Then we simply
		* index into 'mm' and add start/end to get the final position. In the above diagram
		* ax0 tells us if we're on A,B,C or D,E,F. By starting at corner B, we can flip the
		* bits (1 << ax1) or (1 << ax2) in our encoding scheme to get to corners A or C.
		*/

		// how to get from B2 to b2
		Vector diagOffsetForEndBox = maxs - mins;
		diagOffsetForEndBox[ax0] = 0;
		if (diff[ax1] > 1)
			diagOffsetForEndBox[ax1] *= -1;
		if (diff[ax2] > 1)
			diagOffsetForEndBox[ax2] *= -1;

		int refCBits = (diff.x > 0) | (diff.y > 0) << 1 | (diff.z > 0) << 2; // reference corner bits (corner B)

		int cBits[3] = {refCBits, refCBits ^ (1 << ax1), refCBits ^ (1 << ax2)};
		Vector v1(mm[cBits[0] & 1].x, mm[(cBits[0] & 2) >> 1].y, mm[(cBits[0] & 4) >> 2].z); // B
		Vector v2(mm[cBits[1] & 1].x, mm[(cBits[1] & 2) >> 1].y, mm[(cBits[1] & 4) >> 2].z); // A/C
		Vector v3(mm[cBits[2] & 1].x, mm[(cBits[2] & 2) >> 1].y, mm[(cBits[2] & 4) >> 2].z); // C/A

		Vector size = mm[1] - mm[0];
		Vector ax0Off = size;
		ax0Off[ax1] = ax0Off[ax2] = 0;

		Vector diffAbs;
		VectorAbs(diff, diffAbs);
		Vector overlap = size - diffAbs;

		if (overlap[ax1] > 0 && overlap[ax2] > 0)
		{
			// start/end overlap
			Vector overlapAx1 = overlap, overlapAx2 = overlap;
			overlapAx1[ax0] = overlapAx1[ax2] = 0;
			overlapAx2[ax0] = overlapAx2[ax1] = 0;
			// clang-format off
			std::array<Vector, 12> tmpTris{
			    start + v2,          start + v1 - Sign(diff[ax1]) * overlapAx1,          end   + v2,
			    end   + v3,          start + v1 - Sign(diff[ax2]) * overlapAx2,          start + v3,
			    end   + v2 + ax0Off, start + v1 - Sign(diff[ax1]) * overlapAx1 + ax0Off, start + v2 + ax0Off,
			    start + v3 + ax0Off, start + v1 - Sign(diff[ax2]) * overlapAx2 + ax0Off, end   + v3 + ax0Off,
			};
			// clang-format on
			if (!MvdAddTris(vdf, vdl, tmpTris.data(), tmpTris.size() / 3, c.cSweep))
				goto rollback_all;
		}
		else
		{
			for (int i = 0; i < 2; i++)
			{
				bool doingFaces = i == 0;
				if (doingFaces ? c.cSweep.faceColor.a == 0 : c.cSweep.lineColor.a == 0)
					continue;

				size_t numVerts = doingFaces ? vdf.verts.size() : vdl.verts.size();
				auto& verts = doingFaces ? vdf.verts : vdl.verts;
				color32 rgba = doingFaces ? c.cSweep.faceColor : c.cSweep.lineColor;

				bool mirrorStrips = fabs(diff[ax1]) > fabs(diff[ax2]);
				// e.g. A1 B1 C1
				verts.emplace_back(start + v2, rgba);
				verts.emplace_back(start + v1, rgba);
				verts.emplace_back(start + v3, rgba);
				// e.g. A2 b2 C2
				verts.emplace_back(end + v2, rgba);
				verts.emplace_back(end + v1 + diagOffsetForEndBox, rgba);
				verts.emplace_back(end + v3, rgba);

				if (doingFaces)
				{
					if (!MvdAddFaceTriangleStripIndices(
					        vdf, numVerts + 3, numVerts, 3, false, !mirrorStrips, c.cSweep.wd))
					{
						goto rollback_all;
					}
				}
				else if (!MvdAddLineStripIndices(vdl, numVerts, 3, false)
				         || !MvdAddLineStripIndices(vdl, numVerts + 3, 3, false))
				{
					goto rollback_all;
				}

				// now do other side of cube by adding ax0Off to all the verts
				for (int j = 0; j < 6; j++)
				{
					VertexData& tmp = verts[numVerts + j];
					verts.emplace_back(tmp.pos + ax0Off, rgba);
				}
				numVerts += 6;
				if (doingFaces)
				{
					if (!MvdAddFaceTriangleStripIndices(
					        vdf, numVerts, numVerts + 3, 3, false, mirrorStrips, c.cSweep.wd))
					{
						goto rollback_all;
					}
				}
				else if (!MvdAddLineStripIndices(vdl, numVerts, 3, false)
				         || !MvdAddLineStripIndices(vdl, numVerts + 3, 3, false))
				{
					goto rollback_all;
				}
			}
		}
		tmpQuad = {start + v2 + ax0Off, start + v2, end + v2, end + v2 + ax0Off};
		if (!MvdAddPolygon(vdf, vdl, tmpQuad.data(), 4, c.cSweep))
			goto rollback_all;

		tmpQuad = {start + v3, start + v3 + ax0Off, end + v3 + ax0Off, end + v3};
		if (!MvdAddPolygon(vdf, vdl, tmpQuad.data(), 4, c.cSweep))
			goto rollback_all;

		break;
	}
	case 0:
	{
		/*
		* We'll use the same encoding scheme as in case 1. If we look at our box from a corner, we see a hexagonal
		* shape. We encode this corner as refCBits, then go to an adjacent corner cBits1 by flipping a bit, and
		* finally flip another (different) bit to get to cBits2. This gives us a way to fill in a face by using
		* all possible edges cBits1-cBits2 on a single box, (we connect the edge between the boxes to get a quad).
		*/
		int refCBits = (diff.x > 0) | (diff.y > 0) << 1 | (diff.z > 0) << 2; // start corner
		for (int flipBit = 0; flipBit < 3; flipBit++)
		{
			int cBits1 = refCBits ^ (1 << flipBit); // one corner away from ref
			for (int flipBitOff = 0; flipBitOff < 2; flipBitOff++)
			{
				int cBits2 = cBits1 ^ (1 << ((flipBit + flipBitOff + 1) % 3)); // 2 away from ref
				Vector off1(mm[cBits1 & 1].x, mm[(cBits1 & 2) >> 1].y, mm[(cBits1 & 4) >> 2].z);
				Vector off2(mm[cBits2 & 1].x, mm[(cBits2 & 2) >> 1].y, mm[(cBits2 & 4) >> 2].z);
				tmpQuad = {end + off1, end + off2, start + off2, start + off1};
				ShapeColor cTmp = c.cSweep;
				cTmp.wd = ((flipBitOff + (diff.x > 1) + (diff.y > 1) + (diff.z > 1)) % 2)
				              ? c.cSweep.wd
				              : WD_INVERT(c.cSweep.wd); // parity
				if (!MvdAddPolygon(vdf, vdl, tmpQuad.data(), 4, cTmp))
					goto rollback_all;
			}
		}
		break;
	}
	}

	if (MVD_OVERFLOWED2(vdf, vdl))
	{
	rollback_all:
		MVD_ROLLBACK2(vdf_s, vdl_s);
		MVD_ROLLBACK2(vdf_e, vdl_e);
		MVD_ROLLBACK2(vdf, vdl);
		return false;
	}
	return true;
}

bool MeshBuilderDelegate::AddCone(const Vector& pos,
                                  const QAngle& ang,
                                  float height,
                                  float radius,
                                  int nCirclePoints,
                                  bool drawBase,
                                  ShapeColor c)
{
	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;

	if (nCirclePoints < 3 || (!doFaces && !doLines) || !(c.wd & WD_BOTH))
		return true;

	if (height < 0)
		c.wd = WD_INVERT(c.wd);

	Vector* circleVerts = CreateEllipseVerts(pos, ang, radius, radius, nCirclePoints);

	Vector tip;
	AngleVectors(ang, &tip);
	tip = pos + tip * height;

	size_t totalNumIndices = nCirclePoints * 3;
	if (drawBase)
		totalNumIndices += 3 * (nCirclePoints - 2);

	auto& vdf = GET_MVD_FACES(c);
	auto& vdl = GET_MVD_LINES(c);
	MVD_CHECKPOINT2(vdf, vdl);

	if (doFaces && MVD_WILL_OVERFLOW(vdf, nCirclePoints + 1, totalNumIndices * WD_INDEX_MULTIPLIER(c.wd)))
		return false;
	if (doLines && MVD_WILL_OVERFLOW(vdl, nCirclePoints + 1, nCirclePoints * 4))
		return false;

	if (doFaces)
	{
		size_t tipIdx = vdf.verts.size();
		vdf.verts.emplace_back(tip, c.faceColor);
		for (int i = 0; i < nCirclePoints; i++)
		{
			if (i > 0)
			{
				// faces on top of cone
				if (c.wd & WD_CW)
				{
					vdf.indices.push_back(vdf.verts.size() - 1);
					vdf.indices.push_back(vdf.verts.size());
					vdf.indices.push_back(tipIdx);
				}
				if (c.wd & WD_CCW)
				{
					vdf.indices.push_back(tipIdx);
					vdf.indices.push_back(vdf.verts.size());
					vdf.indices.push_back(vdf.verts.size() - 1);
				}
			}
			vdf.verts.emplace_back(circleVerts[i], c.faceColor);
		}
		// final face on top of cone
		if (c.wd & WD_CW)
		{
			vdf.indices.push_back(vdf.verts.size() - 1);
			vdf.indices.push_back(tipIdx + 1);
			vdf.indices.push_back(tipIdx);
		}
		if (c.wd & WD_CCW)
		{
			vdf.indices.push_back(tipIdx);
			vdf.indices.push_back(tipIdx + 1);
			vdf.indices.push_back(vdf.verts.size() - 1);
		}
		if (drawBase)
			MvdAddFacePolygonIndices(vdf, tipIdx + 1, nCirclePoints, WD_INVERT(c.wd));
	}

	if (doLines)
	{
		size_t tipIdx = vdl.verts.size();
		vdl.verts.emplace_back(tip, c.lineColor);
		MvdAddLineStrip(vdl, circleVerts, nCirclePoints, true, c.lineColor);
		for (int i = 0; i < nCirclePoints; i++)
		{
			// lines from tip to base
			vdl.indices.push_back(tipIdx);
			vdl.indices.push_back(tipIdx + i + 1);
		}
	}

	MVD_SIZE_VERIFY2(vdf, vdl);
	return true;
}

bool MeshBuilderDelegate::AddCylinder(const Vector& pos,
                                      const QAngle& ang,
                                      float height,
                                      float radius,
                                      int nCirclePoints,
                                      bool drawCap1,
                                      bool drawCap2,
                                      ShapeColor c)
{
	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;

	if (nCirclePoints < 3 || (!doFaces && !doLines) || !(c.wd & WD_BOTH))
		return true;

	if (height < 0)
		c.wd = WD_INVERT(c.wd);

	auto& vdf = GET_MVD_FACES(c);
	auto& vdl = GET_MVD_LINES(c);
	MVD_CHECKPOINT2(vdf, vdl);

	if (doFaces
	    && MVD_WILL_OVERFLOW(vdf,
	                         nCirclePoints * 2,
	                         (nCirclePoints * 6 + (nCirclePoints - 2) * 3 * (drawCap1 + drawCap2))
	                             * WD_INDEX_MULTIPLIER(c.wd)))
	{
		return false;
	}
	if (doLines && MVD_WILL_OVERFLOW(vdl, nCirclePoints * 2, nCirclePoints * 6))
		return false;

	Vector heightOff;
	AngleVectors(ang, &heightOff);
	heightOff *= height;

	Vector* circleVerts = CreateEllipseVerts(pos, ang, radius, radius, nCirclePoints);

	if (doFaces)
	{
		size_t idx1 = vdf.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdf.verts.emplace_back(circleVerts[i], c.faceColor);
		size_t idx2 = vdf.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdf.verts.emplace_back(circleVerts[i] + heightOff, c.faceColor);
		MvdAddFaceTriangleStripIndices(vdf, idx2, idx1, nCirclePoints, true, false, c.wd);
		if (drawCap1)
			MvdAddFacePolygonIndices(vdf, idx1, nCirclePoints, WD_INVERT(c.wd));
		if (drawCap2)
			MvdAddFacePolygonIndices(vdf, idx2, nCirclePoints, c.wd);
	}

	if (doLines)
	{
		size_t idx1 = vdl.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdl.verts.emplace_back(circleVerts[i], c.lineColor);
		size_t idx2 = vdl.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdl.verts.emplace_back(circleVerts[i] + heightOff, c.lineColor);
		MvdAddLineStripIndices(vdl, idx1, nCirclePoints, true);
		MvdAddLineStripIndices(vdl, idx2, nCirclePoints, true);
		for (int i = 0; i < nCirclePoints; i++)
		{
			vdl.indices.push_back(idx1 + i);
			vdl.indices.push_back(idx2 + i);
		}
	}

	MVD_SIZE_VERIFY2(vdf, vdl);
	return true;
}

bool MeshBuilderDelegate::AddArrow3D(const Vector& pos, const Vector& target, ArrowParams params, ShapeColor c)
{
	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;

	if (params.nCirclePoints < 3 || (!doFaces && !doLines) || params.tipLengthPortion <= 0
	    || params.tipLengthPortion >= 1)
	{
		return true;
	}

	// The cone and cylinder parts handle the wd correctly, but since we access their buffers directly we have to
	// do a couple hacks to make sure the wd of the whole arrow gets handled in a way that makes sense.
	params.tipRadiusScale = fabsf(params.tipRadiusScale);

	auto& vdf = GET_MVD_FACES(c);
	auto& vdl = GET_MVD_LINES(c);
	MVD_CHECKPOINT2(vdf, vdl);

	Vector dir = target - pos;
	dir.NormalizeInPlace();
	// VectorAngles is garbage!
	if (fabsf(dir[0]) < 1e-3 && fabsf(dir[1]) < 1e-3)
		dir[0] = dir[1] = 0;
	QAngle ang;
	VectorAngles(dir, ang);

	float baseLength = params.arrowLength * (1 - params.tipLengthPortion);

	if (!AddCylinder(pos, ang, baseLength, params.radius, params.nCirclePoints, true, false, c))
		return false;

	// assumes AddCylinder() puts the "top" vertices at the end of the vert list for faces & lines
	size_t innerCircleFaceIdx = vdf.verts.size() - params.nCirclePoints;
	size_t innerCircleLineIdx = vdl.verts.size() - params.nCirclePoints;

	float tipLength = params.arrowLength * params.tipLengthPortion;
	float tipRadius = params.radius * params.tipRadiusScale;

	if (!AddCone(pos + dir * baseLength, ang, tipLength, tipRadius, params.nCirclePoints, false, c))
	{
		MVD_ROLLBACK2(vdf, vdl);
		return false;
	}
	// assumes AddCone() puts the base vertices at the end of the vert list for faces & lines
	size_t outerCircleFaceIdx = vdf.verts.size() - params.nCirclePoints;
	size_t outerCircleLineIdx = vdl.verts.size() - params.nCirclePoints;

	// params.arrowLength < 0: another check to make sure the wd is handled correctly
	if (doFaces
	    && !MvdAddFaceTriangleStripIndices(vdf,
	                                       outerCircleFaceIdx,
	                                       innerCircleFaceIdx,
	                                       params.nCirclePoints,
	                                       true,
	                                       false,
	                                       params.arrowLength < 0 ? WD_INVERT(c.wd) : c.wd))
	{
		MVD_ROLLBACK2(vdf, vdl);
		return false;
	}

	if (doLines)
	{
		if (MVD_WILL_OVERFLOW(vdl, 0, params.nCirclePoints * 2))
		{
			MVD_ROLLBACK2(vdf, vdl);
			return false;
		}
		for (int i = 0; i < params.nCirclePoints; i++)
		{
			vdl.indices.push_back(innerCircleLineIdx + i);
			vdl.indices.push_back(outerCircleLineIdx + i);
		}
		MVD_SIZE_VERIFY(vdl);
	}
	return true;
}

bool MeshBuilderDelegate::AddCPolyhedron(const CPolyhedron* polyhedron, ShapeColor c)
{
	const bool doFaces = c.faceColor.a != 0;
	const bool doLines = c.lineColor.a != 0;

	if (!polyhedron || !(c.wd & WD_BOTH) || (!doFaces && !doLines))
		return true;

	auto& vdf = GET_MVD_FACES(c);
	auto& vdl = GET_MVD_LINES(c);
	MVD_CHECKPOINT2(vdf, vdl);

	if (doFaces)
	{
		size_t totalIndices = 0;
		for (int p = 0; p < polyhedron->iPolygonCount; p++)
			totalIndices += (polyhedron->pPolygons[p].iIndexCount - 2) * 3;
		if (MVD_WILL_OVERFLOW(vdf, polyhedron->iVertexCount, totalIndices * WD_INDEX_MULTIPLIER(c.wd)))
			return false;
	}
	if (doLines && MVD_WILL_OVERFLOW(vdl, polyhedron->iVertexCount, polyhedron->iLineCount * 2))
		return false;

	if (doFaces)
	{
		const size_t initIdx = vdf.verts.size();
		for (int v = 0; v < polyhedron->iVertexCount; v++)
			vdf.verts.emplace_back(polyhedron->pVertices[v], c.faceColor);
		for (int p = 0; p < polyhedron->iPolygonCount; p++)
		{
			// Writing the indices straight from the polyhedron data instead of converting to polygons first.
			// To make a tri we do the same thing as in AddPolygon - fix a vert and iterate over the rest.

			const Polyhedron_IndexedPolygon_t& polygon = polyhedron->pPolygons[p];
			unsigned short firstIdx = 0, prevIdx = 0;

			for (int i = 0; i < polygon.iIndexCount; i++)
			{
				Polyhedron_IndexedLineReference_t& ref = polyhedron->pIndices[polygon.iFirstIndex + i];
				Polyhedron_IndexedLine_t& line = polyhedron->pLines[ref.iLineIndex];
				unsigned short curIdx = initIdx + line.iPointIndices[ref.iEndPointIndex];
				switch (i)
				{
				case 0:
					firstIdx = curIdx;
				case 1:
					break;
				default:
					if (c.wd & WD_CW)
					{
						vdf.indices.push_back(firstIdx);
						vdf.indices.push_back(prevIdx);
						vdf.indices.push_back(curIdx);
					}
					if (c.wd & WD_CCW)
					{
						vdf.indices.push_back(curIdx);
						vdf.indices.push_back(prevIdx);
						vdf.indices.push_back(firstIdx);
					}
					break;
				}
				prevIdx = curIdx;
			}
		}
	}

	if (doLines)
	{
		// edges are more straightforward since the polyhedron stores edges as pairs of indices already
		const size_t initIdx = vdl.verts.size();
		for (int v = 0; v < polyhedron->iVertexCount; v++)
			vdl.verts.emplace_back(polyhedron->pVertices[v], c.lineColor);
		for (int i = 0; i < polyhedron->iLineCount; i++)
		{
			Polyhedron_IndexedLine_t& line = polyhedron->pLines[i];
			vdl.indices.push_back(initIdx + line.iPointIndices[0]);
			vdl.indices.push_back(initIdx + line.iPointIndices[1]);
		}
	}

	MVD_SIZE_VERIFY2(vdf, vdl);
	return true;
}

#endif
