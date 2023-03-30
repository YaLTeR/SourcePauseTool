#include "stdafx.hpp"

#include "internal_defs.hpp"
#include "mesh_builder_internal.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include <memory>

#include "..\mesh_builder.hpp"
#include "spt\utils\math.hpp"

Vector* Scratch(size_t n)
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

/*
* Whenever a user says they want to add anything to a mesh, we need to know which component to add it to. These
* macros help us do that. For now they are only set up for the basic internal materials, but this will need to be
* improved/changed to work for text & custom materials. For now the usage looks like this:
* 
* DECLARE_MULTIPLE_COMPONENTS(2); // I want to use faces & lines, prevents reallocations
* 
* // gets the appropriate vertex triangle data that has the correct ztest & opaque flag
* auto& faceData = GET_VDATA_FACES(faceColor);
* // same thing for line data
* auto& lineData = GET_VDATA_FACES(faceColor);
* 
* faceData.vertices.emplace_back(...);
* ...
*/

#define INDEX_COUNT_MULTIPLIER(wd) (((wd)&WD_BOTH) == WD_BOTH ? 2 : 1)

#define INVERT_WD(wd) (((wd)&WD_BOTH) == WD_BOTH ? WD_BOTH : (WindingDir)((wd & WD_BOTH) ^ WD_BOTH))

#define _MATERIAL_TYPE_FROM_COLOR(color_rgba, zTest) \
	((zTest) ? ((color_rgba).a == 255 ? MeshMaterialSimple::Opaque : MeshMaterialSimple::Alpha) \
	         : MeshMaterialSimple::AlphaNoZ)

#define GET_VDATA_FACES(color) \
	g_meshBuilderInternal.GetSimpleMeshComponent(MeshPrimitiveType::Triangles, \
	                                             _MATERIAL_TYPE_FROM_COLOR((color).faceColor, (color).zTestFaces))
#define GET_VDATA_LINES(color) \
	g_meshBuilderInternal.GetSimpleMeshComponent(MeshPrimitiveType::Lines, \
	                                             _MATERIAL_TYPE_FROM_COLOR((color).lineColor, (color).zTestLines))

#define DECLARE_MULTIPLE_COMPONENTS(count) (void)0

/*
* Instead of using data->Reserve directly - give the numbers to this struct and it will check that
* your numbers are correct when it goes out of scope.
* 
* TODO this needs to be changed to work with different materials or removed entirely. If we give users the option
* of filling the buffers directly this probably needs to be removed, and the above macros must be exposed somehow.
*/
struct ReserveScope
{
	MeshVertData& vd;
	const size_t origNumVerts, origNumIndices;
	const size_t expectedExtraVerts, expectedExtraIndices;
	// this breaks for different materials
	// inline static int depth[(int)MeshPrimitiveType::Count] = {};

	ReserveScope(MeshVertData& vd, size_t numExtraVerts, size_t numExtraIndices)
	    : vd(vd)
	    , origNumVerts(vd.verts.size())
	    , origNumIndices(vd.indices.size())
	    , expectedExtraVerts(numExtraVerts)
	    , expectedExtraIndices(numExtraIndices)
	{
		/*if (depth[(int)vd.type]++ == 0)
		{
			vd.verts.reserve_extra(numExtraVerts);
			vd.indices.reserve_extra(numExtraIndices);
		}*/
	}

	~ReserveScope()
	{
		// depth[(int)vd.type]--;
		size_t actualNewVerts = vd.verts.size() - origNumVerts;
		size_t actualNewIndices = vd.indices.size() - origNumIndices;
		(void)actualNewVerts;
		(void)actualNewIndices;
		AssertMsg2(actualNewVerts == expectedExtraVerts,
		           "Expected %u extra verts, got %u",
		           expectedExtraVerts,
		           actualNewVerts);
		AssertMsg2(actualNewIndices == expectedExtraIndices,
		           "Expected %u extra indices, got %u",
		           expectedExtraIndices,
		           actualNewIndices);
	}
};

void MeshBuilderDelegate::_AddLine(const Vector& v1, const Vector& v2, color32 c, MeshVertData& vdl)
{
	ReserveScope rs(vdl, 2, 2);
	vdl.indices.push_back(vdl.verts.size());
	vdl.indices.push_back(vdl.verts.size() + 1);
	vdl.verts.emplace_back(v1, c);
	vdl.verts.emplace_back(v2, c);
}

void MeshBuilderDelegate::AddLine(const Vector& v1, const Vector& v2, LineColor c)
{
	if (c.lineColor.a == 0)
		return;
	_AddLine(v1, v2, c.lineColor, GET_VDATA_LINES(c));
}

void MeshBuilderDelegate::AddLines(const Vector* points, int nSegments, LineColor c)
{
	if (!points || nSegments <= 0 || c.lineColor.a == 0)
		return;
	auto& vdl = GET_VDATA_LINES(c);
	ReserveScope rs(vdl, nSegments * 2, nSegments * 2);
	for (int i = 0; i < nSegments * 2; i++)
	{
		vdl.indices.push_back(vdl.verts.size());
		vdl.verts.emplace_back(points[i], c.lineColor);
	}
}

void MeshBuilderDelegate::_AddLineStrip(const Vector* points, int nPoints, bool loop, color32 c, MeshVertData& vdl)
{
	if (!points || nPoints < 2)
		return;
	ReserveScope rs(vdl, nPoints, (nPoints - 1 + loop) * 2);
	for (int i = 0; i < nPoints; i++)
		vdl.verts.emplace_back(points[i], c);
	_AddLineStripIndices(vdl, vdl.verts.size() - nPoints, nPoints, loop);
}

void MeshBuilderDelegate::AddLineStrip(const Vector* points, int nPoints, bool loop, LineColor c)
{
	if (c.lineColor.a == 0)
		return;
	_AddLineStrip(points, nPoints, loop, c.lineColor, GET_VDATA_LINES(c));
}

void MeshBuilderDelegate::AddCross(const Vector& pos, float radius, LineColor c)
{
	if (c.lineColor.a == 0 || radius <= 0)
		return;
	static float axf = powf(1.f / 3, 0.5f);
	auto& vdl = GET_VDATA_LINES(c);
	ReserveScope rs(vdl, 8, 8);
	for (int x = -1; x <= 1; x += 2)
	{
		for (int y = -1; y <= 1; y += 2)
		{
			Vector v = Vector{x * axf, y * axf, axf} * radius;
			_AddLine(-v + pos, v + pos, c.lineColor, vdl);
		}
	}
}

void MeshBuilderDelegate::AddTri(const Vector& v1, const Vector& v2, const Vector& v3, ShapeColor c)
{
	Vector v[] = {v1, v2, v3};
	AddTris(v, 1, c);
}

void MeshBuilderDelegate::AddTris(const Vector* verts, int nFaces, ShapeColor c)
{
	DECLARE_MULTIPLE_COMPONENTS(2);
	_AddTris(verts, nFaces, c, GET_VDATA_FACES(c), GET_VDATA_LINES(c));
}

void MeshBuilderDelegate::_AddTris(const Vector* verts, int nFaces, ShapeColor c, MeshVertData& vdf, MeshVertData& vdl)
{
	if (!verts || nFaces <= 0 || !(c.wd & WD_BOTH))
		return;

	if (c.faceColor.a != 0)
	{
		ReserveScope rs(vdf, nFaces * 3, nFaces * 3 * INDEX_COUNT_MULTIPLIER(c.wd));
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

	if (c.lineColor.a != 0)
	{
		ReserveScope rs(vdl, nFaces * 3, nFaces * 6);
		for (int i = 0; i < nFaces; i++)
			_AddLineStrip(verts + 3 * i, 3, true, c.lineColor, vdl);
	}
}

void MeshBuilderDelegate::AddQuad(const Vector& v1, const Vector& v2, const Vector& v3, const Vector& v4, ShapeColor c)
{
	Vector v[] = {v1, v2, v3, v4};
	AddPolygon(v, 4, c);
}

// don't expect this to be used much
void MeshBuilderDelegate::AddQuads(const Vector* verts, int nFaces, ShapeColor c)
{
	if (!verts || nFaces <= 0 || !(c.wd & WD_BOTH))
		return;
	DECLARE_MULTIPLE_COMPONENTS(2);
	auto& vdf = GET_VDATA_FACES(c);
	auto& vdl = GET_VDATA_LINES(c);
	for (int i = 0; i < nFaces; i++)
		_AddPolygon(verts + i * 4, 4, c, vdf, vdl);
}

void MeshBuilderDelegate::_AddPolygon(const Vector* verts,
                                      int nVerts,
                                      ShapeColor c,
                                      MeshVertData& vdf,
                                      MeshVertData& vdl)
{
	if (!verts || nVerts < 3 || !(c.wd & WD_BOTH))
		return;
	if (c.faceColor.a != 0)
	{
		ReserveScope rs(vdf, nVerts, (nVerts - 2) * 3 * INDEX_COUNT_MULTIPLIER(c.wd));
		for (int i = 0; i < nVerts; i++)
			vdf.verts.emplace_back(verts[i], c.faceColor);
		_AddFacePolygonIndices(vdf, vdf.verts.size() - nVerts, nVerts, c.wd);
	}
	if (c.lineColor.a != 0)
		_AddLineStrip(verts, nVerts, true, c.lineColor, vdl);
}

void MeshBuilderDelegate::AddPolygon(const Vector* verts, int nVerts, ShapeColor c)
{
	DECLARE_MULTIPLE_COMPONENTS(2);
	_AddPolygon(verts, nVerts, c, GET_VDATA_FACES(c), GET_VDATA_LINES(c));
}

void MeshBuilderDelegate::AddCircle(const Vector& pos, const QAngle& ang, float radius, int nPoints, ShapeColor c)
{
	AddEllipse(pos, ang, radius, radius, nPoints, c);
}

void MeshBuilderDelegate::AddEllipse(const Vector& pos,
                                     const QAngle& ang,
                                     float radiusA,
                                     float radiusB,
                                     int nPoints,
                                     ShapeColor c)
{
	if (nPoints < 3 || radiusA < 0 || radiusB < 0 || (c.faceColor.a == 0 && c.lineColor.a == 0)
	    || !(c.wd & WD_BOTH))
	{
		return;
	}
	AddPolygon(_CreateEllipseVerts(pos, ang, radiusA, radiusB, nPoints), nPoints, c);
}

void MeshBuilderDelegate::AddBox(const Vector& pos,
                                 const Vector& mins_,
                                 const Vector& maxs_,
                                 const QAngle& ang,
                                 ShapeColor c)
{
	if ((c.faceColor.a == 0 && c.lineColor.a == 0) || !(c.wd & WD_BOTH))
		return;

	DECLARE_MULTIPLE_COMPONENTS(2);
	auto& vdf = GET_VDATA_FACES(c);
	auto& vdl = GET_VDATA_LINES(c);
	size_t origNumFaceVerts = vdf.verts.size();
	size_t origNumLineVerts = vdl.verts.size();
	_AddUnitCube(c, vdf, vdl);

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
}

void MeshBuilderDelegate::AddSphere(const Vector& pos, float radius, int nSubdivisions, ShapeColor c)
{
	if (nSubdivisions < 0 || radius < 0 || (c.faceColor.a == 0 && c.lineColor.a == 0) || !(c.wd & WD_BOTH))
		return;

	DECLARE_MULTIPLE_COMPONENTS(2);
	auto& vdf = GET_VDATA_FACES(c);
	auto& vdl = GET_VDATA_LINES(c);
	size_t origNumFaceVerts = vdf.verts.size();
	size_t origNumLineVerts = vdl.verts.size();
	_AddSubdivCube(nSubdivisions, c, vdf, vdl);

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
}

void MeshBuilderDelegate::AddSweptBox(const Vector& start,
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

	// scale the boxes a bit to prevent z fighting
	const Vector nudge(0.04f);
	AddBox(end, mins - nudge * 1.5, maxs - nudge * 1.5, vec3_angle, c.cStart);
	AddBox(start, mins - nudge, maxs - nudge, vec3_angle, c.cEnd);

	DECLARE_MULTIPLE_COMPONENTS(2);
	auto& vdf = GET_VDATA_FACES(c.cSweep);
	auto& vdl = GET_VDATA_LINES(c.cSweep);

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
			return; // sweep is shorter than the box size

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
			if (i % 4 == 3)
				_AddPolygon(tmpQuad.data(), 4, c.cSweep, vdf, vdl);
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
			_AddTris(tmpTris.data(), tmpTris.size() / 3, c.cSweep, vdf, vdl);
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
					_AddFaceTriangleStripIndices(vdf,
					                             numVerts + 3,
					                             numVerts,
					                             3,
					                             false,
					                             !mirrorStrips,
					                             c.cSweep.wd);
				}
				else
				{
					_AddLineStripIndices(vdl, numVerts, 3, false);
					_AddLineStripIndices(vdl, numVerts + 3, 3, false);
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
					_AddFaceTriangleStripIndices(vdf,
					                             numVerts,
					                             numVerts + 3,
					                             3,
					                             false,
					                             mirrorStrips,
					                             c.cSweep.wd);
				}
				else
				{
					_AddLineStripIndices(vdl, numVerts, 3, false);
					_AddLineStripIndices(vdl, numVerts + 3, 3, false);
				}
			}
		}
		tmpQuad = {start + v2 + ax0Off, start + v2, end + v2, end + v2 + ax0Off};
		_AddPolygon(tmpQuad.data(), 4, c.cSweep, vdf, vdl);
		tmpQuad = {start + v3, start + v3 + ax0Off, end + v3 + ax0Off, end + v3};
		_AddPolygon(tmpQuad.data(), 4, c.cSweep, vdf, vdl);
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
				              : INVERT_WD(c.cSweep.wd); // parity
				_AddPolygon(tmpQuad.data(), 4, cTmp, vdf, vdl);
			}
		}
		break;
	}
	}
}

void MeshBuilderDelegate::AddCone(const Vector& pos,
                                  const QAngle& ang,
                                  float height,
                                  float radius,
                                  int nCirclePoints,
                                  bool drawBase,
                                  ShapeColor c)
{
	if (height < 0 || radius < 0 || nCirclePoints < 3 || (c.faceColor.a == 0 && c.lineColor.a == 0)
	    || !(c.wd & WD_BOTH))
	{
		return;
	}

	Vector* circleVerts = _CreateEllipseVerts(pos, ang, radius, radius, nCirclePoints);

	Vector tip;
	AngleVectors(ang, &tip);
	tip = pos + tip * height;

	if (c.faceColor.a != 0)
	{
		auto& vdf = GET_VDATA_FACES(c);

		size_t totalNumIndices = nCirclePoints * 3;
		if (drawBase)
			totalNumIndices += 3 * (nCirclePoints - 2);
		ReserveScope rs(vdf, nCirclePoints + 1, totalNumIndices * INDEX_COUNT_MULTIPLIER(c.wd));

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
			_AddFacePolygonIndices(vdf, tipIdx + 1, nCirclePoints, INVERT_WD(c.wd));
	}

	if (c.lineColor.a != 0)
	{
		auto& vdl = GET_VDATA_LINES(c);
		ReserveScope rs(vdl, nCirclePoints + 1, nCirclePoints * 4);
		size_t tipIdx = vdl.verts.size();
		vdl.verts.emplace_back(tip, c.lineColor);
		AddLineStrip(circleVerts, nCirclePoints, true, c.lineColor);
		for (int i = 0; i < nCirclePoints; i++)
		{
			// lines from tip to base
			vdl.indices.push_back(tipIdx);
			vdl.indices.push_back(tipIdx + i + 1);
		}
	}
}

void MeshBuilderDelegate::AddCylinder(const Vector& pos,
                                      const QAngle& ang,
                                      float height,
                                      float radius,
                                      int nCirclePoints,
                                      bool drawCap1,
                                      bool drawCap2,
                                      ShapeColor c)
{
	if (nCirclePoints < 3 || height < 0 || (c.faceColor.a == 0 && c.lineColor.a == 0) || !(c.wd & WD_BOTH))
		return;

	Vector heightOff;
	AngleVectors(ang, &heightOff);
	heightOff *= height;

	Vector* circleVerts = _CreateEllipseVerts(pos, ang, radius, radius, nCirclePoints);

	if (c.faceColor.a != 0)
	{
		auto& vdf = GET_VDATA_FACES(c);
		ReserveScope rs(vdf,
		                nCirclePoints * 2,
		                (nCirclePoints * 6 + (nCirclePoints - 2) * 3 * (drawCap1 + drawCap2))
		                    * INDEX_COUNT_MULTIPLIER(c.wd));
		size_t idx1 = vdf.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdf.verts.emplace_back(circleVerts[i], c.faceColor);
		size_t idx2 = vdf.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdf.verts.emplace_back(circleVerts[i] + heightOff, c.faceColor);
		_AddFaceTriangleStripIndices(vdf, idx2, idx1, nCirclePoints, true, false, c.wd);
		if (drawCap1)
			_AddFacePolygonIndices(vdf, idx1, nCirclePoints, INVERT_WD(c.wd));
		if (drawCap2)
			_AddFacePolygonIndices(vdf, idx2, nCirclePoints, c.wd);
	}

	if (c.lineColor.a != 0)
	{
		auto& vdl = GET_VDATA_LINES(c);
		ReserveScope rs(vdl, nCirclePoints * 2, nCirclePoints * 6);
		size_t idx1 = vdl.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdl.verts.emplace_back(circleVerts[i], c.lineColor);
		size_t idx2 = vdl.verts.size();
		for (int i = 0; i < nCirclePoints; i++)
			vdl.verts.emplace_back(circleVerts[i] + heightOff, c.lineColor);
		_AddLineStripIndices(vdl, idx1, nCirclePoints, true);
		_AddLineStripIndices(vdl, idx2, nCirclePoints, true);
		for (int i = 0; i < nCirclePoints; i++)
		{
			vdl.indices.push_back(idx1 + i);
			vdl.indices.push_back(idx2 + i);
		}
	}
}

void MeshBuilderDelegate::AddArrow3D(const Vector& pos,
                                     const Vector& target,
                                     float tailLength,
                                     float tailRadius,
                                     float tipHeight,
                                     float tipRadius,
                                     int nCirclePoints,
                                     ShapeColor c)
{
	if (nCirclePoints < 3 || (c.faceColor.a == 0 && c.lineColor.a == 0))
		return;

	DECLARE_MULTIPLE_COMPONENTS(2);
	auto& vdf = GET_VDATA_FACES(c);
	auto& vdl = GET_VDATA_LINES(c);

	Vector dir = target - pos;
	dir.NormalizeInPlace();
	QAngle ang;
	VectorAngles(dir, ang);

	AddCylinder(pos, ang, tailLength, tailRadius, nCirclePoints, true, false, c);
	// assumes AddCylinder() puts the "top" vertices at the end of the vert list for faces & lines
	size_t innerCircleFaceIdx = vdf.verts.size() - nCirclePoints;
	size_t innerCircleLineIdx = vdl.verts.size() - nCirclePoints;
	AddCone(pos + dir * tailLength, ang, tipHeight, tipRadius, nCirclePoints, false, c);
	// assumes AddCone() puts the base vertices at the end of the vert list for faces & lines
	size_t outerCircleFaceIdx = vdf.verts.size() - nCirclePoints;
	size_t outerCircleLineIdx = vdl.verts.size() - nCirclePoints;

	if (c.faceColor.a != 0)
		_AddFaceTriangleStripIndices(vdf,
		                             outerCircleFaceIdx,
		                             innerCircleFaceIdx,
		                             nCirclePoints,
		                             true,
		                             false,
		                             c.wd);

	if (c.lineColor.a != 0)
	{
		ReserveScope rs(vdl, 0, nCirclePoints * 2);
		for (int i = 0; i < nCirclePoints; i++)
		{
			vdl.indices.push_back(innerCircleLineIdx + i);
			vdl.indices.push_back(outerCircleLineIdx + i);
		}
	}
}

void MeshBuilderDelegate::AddCPolyhedron(const CPolyhedron* polyhedron, ShapeColor c)
{
	if (!polyhedron || !(c.wd & WD_BOTH))
		return;
	if (c.faceColor.a != 0)
	{
		auto& vdf = GET_VDATA_FACES(c);
		size_t totalIndices = 0;
		for (int p = 0; p < polyhedron->iPolygonCount; p++)
			totalIndices += (polyhedron->pPolygons[p].iIndexCount - 2) * 3;
		ReserveScope rs(vdf, polyhedron->iVertexCount, totalIndices * INDEX_COUNT_MULTIPLIER(c.wd));
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
	if (c.lineColor.a != 0)
	{
		// edges are more straightforward since the polyhedron stores edges as pairs of indices already
		auto& vdl = GET_VDATA_LINES(c);
		ReserveScope rs(vdl, polyhedron->iVertexCount, polyhedron->iLineCount * 2);
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
}

void MeshBuilderDelegate::_AddFaceTriangleStripIndices(MeshVertData& vdf,
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

	ReserveScope rs(vdf, 0, (nVerts - 1 + loop) * 6 * INDEX_COUNT_MULTIPLIER(wd));
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
	else
	{
		return;
	}
}

void MeshBuilderDelegate::_AddFacePolygonIndices(MeshVertData& vdf, size_t vertsIdx, int nVerts, WindingDir wd)
{
	// Creates indices representing a filled convex polygon using existing verts at the given vert index for faces only.
	Assert(nVerts >= 3);
	Assert(wd & WD_BOTH);
	ReserveScope rs(vdf, 0, (nVerts - 2) * 3 * INDEX_COUNT_MULTIPLIER(wd));
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
}

void MeshBuilderDelegate::_AddLineStripIndices(MeshVertData& vdl, size_t vertsIdx, int nVerts, bool loop)
{
	Assert(nVerts >= 2);
	ReserveScope rs(vdl, 0, (nVerts - 1 + (loop && nVerts > 2)) * 2);
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
}

void MeshBuilderDelegate::_AddSubdivCube(int nSubdivisions, ShapeColor c, MeshVertData& vdf, MeshVertData& vdl)
{
	if (nSubdivisions == 0)
	{
		_AddUnitCube(c, vdf, vdl);
		return;
	}

	bool doFaces = c.faceColor.a != 0;
	bool doLines = c.lineColor.a != 0;
	if (nSubdivisions < 0 || (!doFaces && !doLines))
		return;

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

	int sideLength = nSubdivisions + 1;
	int nLayerVerts = sideLength * 4;

	int numTotalVerts = 2 * (3 * sideLength + 1) * (sideLength + 1);
	int numTotalFaceIndices = 36 * sideLength * sideLength * INDEX_COUNT_MULTIPLIER(c.wd);
	int numTotalLineIndices = 12 * sideLength * (2 * sideLength + 1);
	ReserveScope rsf(vdf, doFaces ? numTotalVerts : 0, doFaces ? numTotalFaceIndices : 0);
	ReserveScope rsl(vdl, doLines ? numTotalVerts : 0, doLines ? numTotalLineIndices : 0);

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
			_AddFaceTriangleStripIndices(vdf,
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
			_AddLineStripIndices(vdl, n - nLayerVerts, nLayerVerts, true);
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
				_AddFaceTriangleStripIndices(vdf, a, b, sideLength + 1, false, false, c.wd);
			}
			if (doLines)
			{
				size_t n = vdl.verts.size();
				_AddLineStripIndices(vdl, n - 1 - sideLength, sideLength + 1, false);
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
}

void MeshBuilderDelegate::_AddUnitCube(ShapeColor c, MeshVertData& vdf, MeshVertData& vdl)
{
	if ((c.faceColor.a == 0 && c.lineColor.a == 0) || !(c.wd & WD_BOTH))
		return;

	// we create a unit cube with mins at <0,0,0> and maxs at <1,1,1>
	// I had to work all this out on a whiteboard if it seems completely unintuitive

	static std::array<Vector, 8>
	    verts{Vector{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};

	static std::array<VertIndex, 36> faceIndices{0, 1, 3, 1, 2, 3, 4, 5, 1, 4, 1, 0, 5, 6, 2, 5, 2, 1,
	                                             6, 7, 3, 6, 3, 2, 7, 4, 0, 7, 0, 3, 7, 6, 5, 7, 5, 4};

	static std::array<VertIndex, 24> lineIndices{0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
	                                             6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

	if (c.faceColor.a != 0)
	{
		ReserveScope rs(vdf, verts.size(), faceIndices.size() * INDEX_COUNT_MULTIPLIER(c.wd));

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
	if (c.lineColor.a != 0)
	{
		ReserveScope rs(vdl, verts.size(), lineIndices.size());

		size_t origVertCount = vdl.verts.size();
		size_t origIdxCount = vdl.indices.size();

		for (Vector pos : verts)
			vdl.verts.emplace_back(pos, c.lineColor);
		vdl.indices.add_range(lineIndices.cbegin(), lineIndices.cend());

		for (size_t i = origIdxCount; i < vdl.indices.size(); i++)
			vdl.indices[i] += origVertCount;
	}
}

Vector* MeshBuilderDelegate::_CreateEllipseVerts(const Vector& pos,
                                                 const QAngle& ang,
                                                 float radiusA,
                                                 float radiusB,
                                                 int nPoints)
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

#endif
