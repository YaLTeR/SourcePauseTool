#include "stdafx.h"

#include "mesh_builder.hpp"

#ifdef SPT_MESH_BUILDER_ENABLED

#include <memory>

#include "spt\feature.hpp"
#include "spt\features\ent_props.hpp"
#include "spt\utils\math.hpp"

/**************************************** COLLIDE TO MESH ****************************************/

namespace patterns
{
	PATTERNS(CPhysicsCollision__CreateDebugMesh,
	         "5135",
	         "83 EC 10 8B 4C 24 14 8B 01 8B 40 08 55 56 57 33 ED 8D 54 24 10 52",
	         "1910503",
	         "55 8B EC 83 EC 14 8B 4D 08 8B 01 8B 40 08 53 56 57 33 DB 8D 55 EC");
	PATTERNS(CPhysicsObject__GetPosition,
	         "5135",
	         "8B 49 08 81 EC 80 00 00 00 8D 04 24 50 E8 ?? ?? ?? ?? 8B 84 24 84 00 00 00 85 C0",
	         "1910503",
	         "55 8B EC 8B 49 08 81 EC 80 00 00 00 8D 45 80 50 E8 ?? ?? ?? ?? 8B 45 08 85 C0");
} // namespace patterns

class MbCollideFeature : public FeatureWrapper<MbCollideFeature>
{
public:
	DECL_MEMBER_THISCALL(int, CPhysicsCollision__CreateDebugMesh, const CPhysCollide* pCollide, Vector** outVerts);
	DECL_MEMBER_THISCALL(void, CPhysicsObject__GetPosition, Vector* worldPosition, QAngle* angles);

protected:
	void InitHooks() override
	{
		FIND_PATTERN(vphysics, CPhysicsCollision__CreateDebugMesh);
		FIND_PATTERN(vphysics, CPhysicsObject__GetPosition);
	}
};

static MbCollideFeature collideFeature;

bool MeshBuilderPro::CreateCollideWorks()
{
	return collideFeature.ORIG_CPhysicsCollision__CreateDebugMesh
	       && collideFeature.ORIG_CPhysicsObject__GetPosition;
}

std::unique_ptr<Vector> MeshBuilderPro::CreateCollideMesh(const CPhysCollide* pCollide, int& outNumFaces)
{
	if (!pCollide || !collideFeature.ORIG_CPhysicsCollision__CreateDebugMesh)
	{
		outNumFaces = 0;
		return nullptr;
	}
	Vector* outVerts;
	outNumFaces = collideFeature.ORIG_CPhysicsCollision__CreateDebugMesh(nullptr, 0, pCollide, &outVerts) / 3;
	return std::unique_ptr<Vector>(outVerts);
}

std::unique_ptr<Vector> MeshBuilderPro::CreateCPhysObjMesh(const CPhysicsObject* pPhysObj,
                                                           int& outNumFaces,
                                                           matrix3x4_t& outMat)
{
	if (!pPhysObj || !CreateCollideWorks())
	{
		outNumFaces = 0;
		SetIdentityMatrix(outMat);
		return nullptr;
	}
	Vector pos;
	QAngle ang;
	collideFeature.ORIG_CPhysicsObject__GetPosition((void*)pPhysObj, 0, &pos, &ang);
	AngleMatrix(ang, pos, outMat);
	return CreateCollideMesh(*((CPhysCollide**)pPhysObj + 3), outNumFaces);
}

std::unique_ptr<Vector> MeshBuilderPro::CreateCBaseEntMesh(const CBaseEntity* pEnt,
                                                           int& outNumFaces,
                                                           matrix3x4_t& outMat)
{
	int off = spt_entutils.GetFieldOffset("CBaseEntity", "m_CollisionGroup", true);
	if (!pEnt || !CreateCollideWorks() || off == utils::INVALID_DATAMAP_OFFSET)
	{
		outNumFaces = 0;
		SetIdentityMatrix(outMat);
		return nullptr;
	}
	off = (off + 4) / 4;
	return CreateCPhysObjMesh(*((CPhysicsObject**)pEnt + off), outNumFaces, outMat);
}

/**************************************** MESH CONSTRUCTION ****************************************/

// https://stackoverflow.com/a/365068 to keep growth geometric but still use reserve
size_t MinBiggerPow2(size_t n)
{
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

Vector* Scratch(size_t n)
{
	static std::unique_ptr<Vector[]> scratch = nullptr;
	static size_t count = 0;
	if (count < n)
	{
		count = MinBiggerPow2(n);
		scratch.reset(new Vector[count]);
	}
	return scratch.get();
}

void MeshBuilderPro::MeshData::Reserve(size_t numExtraVerts, size_t numExtraIndices)
{
	verts.reserve(MinBiggerPow2(verts.size() + numExtraVerts));
	indices.reserve(MinBiggerPow2(indices.size() + numExtraIndices));
}

// Instead of using data->Reserve directly - give the numbers to this struct and it will check that
// your numbers are correct when it goes out of scope.
struct ReserveScope
{
	MeshBuilderPro::MeshData* md;
	const size_t origNumVerts, origNumIndices;
	const size_t expectedExtraVerts, expectedExtraIndices;

	ReserveScope(MeshBuilderPro::MeshData* md, size_t numExtraVerts, size_t numExtraIndices)
	    : md(md)
	    , origNumVerts(md->verts.size())
	    , origNumIndices(md->indices.size())
	    , expectedExtraVerts(numExtraVerts)
	    , expectedExtraIndices(numExtraIndices)
	{
		md->Reserve(numExtraVerts, numExtraIndices);
	}

	~ReserveScope()
	{
		size_t actualNewVerts = md->verts.size() - origNumVerts;
		size_t actualNewIndices = md->indices.size() - origNumIndices;
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

void MeshBuilderPro::AddLine(const Vector& v1, const Vector& v2, const color32& c)
{
	if (c.a == 0)
		return;
	ReserveScope rs(lineData, 2, 2);
	lineData->indices.push_back(lineData->verts.size());
	lineData->indices.push_back(lineData->verts.size() + 1);
	lineData->verts.push_back({v1, c});
	lineData->verts.push_back({v2, c});
}

void MeshBuilderPro::AddLines(const Vector* points, int numSegments, const color32& c)
{
	if (!points || numSegments <= 0 || c.a == 0)
		return;
	ReserveScope rs(lineData, numSegments * 2, numSegments * 2);
	for (int i = 0; i < numSegments * 2; i++)
	{
		lineData->indices.push_back(lineData->verts.size());
		lineData->verts.push_back({points[i], c});
	}
}

void MeshBuilderPro::AddLineStrip(const Vector* points, int numPoints, bool loop, const color32& c)
{
	if (!points || numPoints < 2 || c.a == 0)
		return;
	ReserveScope rs(lineData, numPoints, (numPoints - 1 + loop) * 2);
	for (int i = 0; i < numPoints; i++)
		lineData->verts.push_back({points[i], c});
	_AddLineStripIndices(lineData->verts.size() - numPoints, numPoints, loop);
}

void MeshBuilderPro::AddTri(const Vector& v1, const Vector& v2, const Vector& v3, const MeshColor& c)
{
	Vector v[] = {v1, v2, v3};
	AddTris(v, 1, c);
}

void MeshBuilderPro::AddTris(const Vector* verts, int numFaces, const MeshColor& c)
{
	if (!verts || numFaces <= 0)
		return;

	if (c.faceColor.a != 0)
	{
		ReserveScope rs(faceData, numFaces * 3, numFaces * 3);
		for (int i = 0; i < numFaces * 3; i++)
		{
			faceData->indices.push_back(faceData->verts.size());
			faceData->verts.push_back({verts[i], c.faceColor});
		}
	}

	if (c.lineColor.a != 0)
	{
		ReserveScope rs(lineData, numFaces * 3, numFaces * 6);
		for (int i = 0; i < numFaces; i++)
			AddLineStrip(verts + 3 * i, 3, true, c.lineColor);
	}
}

void MeshBuilderPro::AddQuad(const Vector& v1, const Vector& v2, const Vector& v3, const Vector v4, const MeshColor& c)
{
	Vector v[] = {v1, v2, v3, v4};
	AddPolygon(v, 4, c);
}

void MeshBuilderPro::AddQuads(const Vector* verts, int numFaces, const MeshColor& c)
{
	if (!verts || numFaces <= 0)
		return;
	if (c.faceColor.a != 0)
	{
		ReserveScope rs(faceData, numFaces * 4, numFaces * 6);
		for (int i = 0; i < numFaces; i++)
			AddPolygon(verts + 4 * i, 4, MeshColor::Face(c.faceColor));
	}
	if (c.lineColor.a != 0)
	{
		ReserveScope rs(lineData, numFaces * 4, numFaces * 8);
		for (int i = 0; i < numFaces; i++)
			AddPolygon(verts + 4 * i, 4, MeshColor::Wire(c.lineColor));
	}
}

void MeshBuilderPro::AddPolygon(const Vector* verts, int numVerts, const MeshColor& c)
{
	if (!verts || numVerts < 3)
		return;

	if (c.faceColor.a != 0)
	{
		ReserveScope rs(faceData, numVerts, (numVerts - 2) * 3);
		for (int i = 0; i < numVerts; i++)
			faceData->verts.push_back({verts[i], c.faceColor});
		_AddFacePolygonIndices(faceData->verts.size() - numVerts, numVerts, false);
	}

	if (c.lineColor.a != 0)
		AddLineStrip(verts, numVerts, true, c.lineColor);
}

void MeshBuilderPro::AddCircle(const Vector& pos, const QAngle& ang, float radius, int numPoints, const MeshColor& col)
{
	if (numPoints < 3 || radius < 0 || (col.faceColor.a == 0 && col.lineColor.a == 0))
		return;
	AddPolygon(_CreateCircleVerts(pos, ang, radius, numPoints), numPoints, col);
}

void MeshBuilderPro::AddBox(const Vector& pos,
                            const Vector& mins,
                            const Vector& maxs,
                            const QAngle& ang,
                            const MeshColor& c)
{
	if (c.faceColor.a == 0 && c.lineColor.a == 0)
		return;

	size_t origNumFaceVerts = faceData->verts.size();
	size_t origNumLineVerts = lineData->verts.size();
	_AddSubdivCube(0, c);

	Vector size, actualMins;
	matrix3x4_t scaleMat, offMat, finalMat;
	VectorAbs(maxs - mins, size);
	VectorMin(mins, maxs, actualMins); // in case anyone stupid (like me) swaps the mins/maxs
	scaleMat.Init({size.x, 0, 0}, {0, size.y, 0}, {0, 0, size.z}, actualMins); // set up box dimensions at origin
	AngleMatrix(ang, pos, offMat);                                             // rotate box and put at 'pos'
	MatrixMultiply(offMat, scaleMat, finalMat);

	for (size_t i = origNumFaceVerts; i < faceData->verts.size(); i++)
		utils::VectorTransform(finalMat, faceData->verts[i].pos);
	for (size_t i = origNumLineVerts; i < lineData->verts.size(); i++)
		utils::VectorTransform(finalMat, lineData->verts[i].pos);
}

void MeshBuilderPro::AddSphere(const Vector& pos, float radius, int numSubdivisions, const MeshColor& c)
{
	if (numSubdivisions < 0 || radius < 0 || (c.faceColor.a == 0 && c.lineColor.a == 0))
		return;

	size_t origNumFaceVerts = faceData->verts.size();
	size_t origNumLineVerts = lineData->verts.size();
	_AddSubdivCube(numSubdivisions, c);

	// center, normalize, scale, transform :)
	Vector subdivCubeOff((numSubdivisions + 1) / -2.f);
	for (size_t i = origNumFaceVerts; i < faceData->verts.size(); i++)
	{
		Vector& v = faceData->verts[i].pos;
		v += subdivCubeOff;
		VectorNormalize(v);
		v = v * radius + pos;
	}
	for (size_t i = origNumLineVerts; i < lineData->verts.size(); i++)
	{
		Vector& v = lineData->verts[i].pos;
		v += subdivCubeOff;
		VectorNormalize(v);
		v = v * radius + pos;
	}
}

void MeshBuilderPro::AddSweptBox(const Vector& start,
                                 const Vector& end,
                                 const Vector& mins,
                                 const Vector& maxs,
                                 const MeshColor& cStart,
                                 const MeshColor& cEnd,
                                 const MeshColor& cSweep)
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

	Vector actualMins, actualMaxs; // in case anyone stupid (like me) swaps these around
	VectorMin(mins, maxs, actualMins);
	VectorMax(mins, maxs, actualMaxs);

	// scale the boxes a bit to prevent z fighting
	const Vector nudge(0.04f);
	AddBox(end, actualMins - nudge * 1.5, actualMaxs - nudge * 1.5, vec3_angle, cEnd);
	AddBox(start, actualMins - nudge, actualMaxs - nudge, vec3_angle, cStart);

	// For a given box, we can encode a face of the box with a 0 or 1 corresponding to mins/maxs.
	// With 3 bits, you can encode the two "same" corners on both boxes.
	Vector mm[] = {actualMins, actualMaxs};

	switch (numAlignedAxes)
	{
	case 2:
	{
		Vector verts[16];
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
			verts[i][ax0] = boxOff[ax0] + mm[(ax1Sign + whichBox) % 2][ax0];
			verts[i][ax1] = boxOff[ax1] + mm[(i + 2) % 16 / 8][ax1];
			verts[i][ax2] = boxOff[ax2] + mm[(i + 6) % 16 / 8][ax2];
		}
		AddQuads(verts, 4, cSweep);
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

		size_t origNumFaceIndices = faceData->indices.size();

		// how to get from B2 to b2
		Vector diagOffsetForEndBox = actualMaxs - actualMins;
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
			Vector overlapAx1 = overlap;
			Vector overlapAx2 = overlap;
			overlapAx1[ax0] = overlapAx1[ax2] = 0;
			overlapAx2[ax0] = overlapAx2[ax1] = 0;
			// clang-format off
			AddTri(start + v2, start + v1 - Sign(diff[ax1]) * overlapAx1, end + v2, cSweep);
			AddTri(end + v3, start + v1 - Sign(diff[ax2]) * overlapAx2, start + v3, cSweep);
			AddTri(end + v2 + ax0Off, start + v1 - Sign(diff[ax1]) * overlapAx1 + ax0Off, start + v2 + ax0Off, cSweep);
			AddTri(start + v3 + ax0Off, start + v1 - Sign(diff[ax2]) * overlapAx2 + ax0Off, end + v3 + ax0Off, cSweep);
			// clang-format on
		}
		else
		{
			for (int i = 0; i < 2; i++)
			{
				bool doingFaces = i == 0;
				if (doingFaces ? cSweep.faceColor.a == 0 : cSweep.lineColor.a == 0)
					continue;

				size_t numVerts = doingFaces ? faceData->verts.size() : lineData->verts.size();
				auto& verts = doingFaces ? faceData->verts : lineData->verts;
				color32 c = doingFaces ? cSweep.faceColor : cSweep.lineColor;

				bool mirrorStrips = fabs(diff[ax1]) > fabs(diff[ax2]);
				// e.g. A1 B1 C1
				verts.push_back({start + v2, c});
				verts.push_back({start + v1, c});
				verts.push_back({start + v3, c});
				// e.g. A2 b2 C2
				verts.push_back({end + v2, c});
				verts.push_back({end + v1 + diagOffsetForEndBox, c});
				verts.push_back({end + v3, c});

				if (doingFaces)
				{
					_AddFaceTriangleStripIndices(numVerts + 3, numVerts, 3, false, !mirrorStrips);
				}
				else
				{
					_AddLineStripIndices(numVerts, 3, false);
					_AddLineStripIndices(numVerts + 3, 3, false);
				}
				// now do other side of cube by adding ax0Off to all the verts
				for (int j = 0; j < 6; j++)
				{
					VertexData& tmp = verts[numVerts + j];
					verts.push_back({tmp.pos + ax0Off, c});
				}
				numVerts += 6;
				if (doingFaces)
				{
					_AddFaceTriangleStripIndices(numVerts, numVerts + 3, 3, false, mirrorStrips);
				}
				else
				{
					_AddLineStripIndices(numVerts, 3, false);
					_AddLineStripIndices(numVerts + 3, 3, false);
				}
			}
		}

		AddQuad(start + v2 + ax0Off, start + v2, end + v2, end + v2 + ax0Off, cSweep);
		AddQuad(start + v3, start + v3 + ax0Off, end + v3 + ax0Off, end + v3, cSweep);

		if ((diff[ax1] < 0) != (diff[ax2] < 0)) // parity
			std::reverse(faceData->indices.begin() + origNumFaceIndices, faceData->indices.end());

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
				if ((flipBitOff + (diff.x > 1) + (diff.y > 1) + (diff.z > 1)) % 2) // parity
					AddQuad(end + off1, end + off2, start + off2, start + off1, cSweep);
				else
					AddQuad(start + off1, start + off2, end + off2, end + off1, cSweep);
			}
		}
		break;
	}
	}
}

void MeshBuilderPro::AddCone(const Vector& pos,
                             const QAngle& ang,
                             float height,
                             float radius,
                             int numCirclePoints,
                             bool drawBase,
                             const MeshColor& c)
{
	if (height < 0 || radius < 0 || numCirclePoints < 3 || (c.faceColor.a == 0 && c.lineColor.a == 0))
		return;

	Vector* circleVerts = _CreateCircleVerts(pos, ang, radius, numCirclePoints);

	Vector tip;
	AngleVectors(ang, &tip);
	tip = pos + tip * height;

	if (c.faceColor.a != 0)
	{
		ReserveScope rs(faceData,
		                numCirclePoints + 1,
		                numCirclePoints * 3 + (drawBase ? 3 * (numCirclePoints - 2) : 0));
		size_t tipIdx = faceData->verts.size();
		faceData->verts.push_back({tip, c.faceColor});
		for (int i = 0; i < numCirclePoints; i++)
		{
			if (i > 0)
			{
				// faces on top of cone
				faceData->indices.push_back(faceData->verts.size() - 1);
				faceData->indices.push_back(faceData->verts.size());
				faceData->indices.push_back(tipIdx);
			}
			faceData->verts.push_back({circleVerts[i], c.faceColor});
		}
		// final face on top of cone
		faceData->indices.push_back(faceData->verts.size() - 1);
		faceData->indices.push_back(tipIdx + 1);
		faceData->indices.push_back(tipIdx);
		if (drawBase)
			_AddFacePolygonIndices(tipIdx + 1, numCirclePoints, true);
	}

	if (c.lineColor.a != 0)
	{
		ReserveScope rs(lineData, numCirclePoints + 1, numCirclePoints * 4);
		size_t tipIdx = lineData->verts.size();
		lineData->verts.push_back({tip, c.lineColor});
		AddLineStrip(circleVerts, numCirclePoints, true, c.lineColor);
		for (int i = 0; i < numCirclePoints; i++)
		{
			// lines from tip to base
			lineData->indices.push_back(tipIdx);
			lineData->indices.push_back(tipIdx + i + 1);
		}
	}
}

void MeshBuilderPro::AddCylinder(const Vector& pos,
                                 const QAngle& ang,
                                 float height,
                                 float radius,
                                 int numCirclePoints,
                                 bool drawCap1,
                                 bool drawCap2,
                                 const MeshColor& c)
{
	if (numCirclePoints < 3 || height < 0 || (c.faceColor.a == 0 && c.lineColor.a == 0))
		return;

	Vector heightOff;
	AngleVectors(ang, &heightOff);
	heightOff *= height;

	Vector* circleVerts = _CreateCircleVerts(pos, ang, radius, numCirclePoints);

	if (c.faceColor.a != 0)
	{
		ReserveScope rs(faceData,
		                numCirclePoints * 2,
		                numCirclePoints * 6 + (numCirclePoints - 2) * 3 * (drawCap1 + drawCap2));
		size_t idx1 = faceData->verts.size();
		for (int i = 0; i < numCirclePoints; i++)
			faceData->verts.push_back({circleVerts[i], c.faceColor});
		size_t idx2 = faceData->verts.size();
		for (int i = 0; i < numCirclePoints; i++)
			faceData->verts.push_back({circleVerts[i] + heightOff, c.faceColor});
		_AddFaceTriangleStripIndices(idx2, idx1, numCirclePoints, true);
		if (drawCap1)
			_AddFacePolygonIndices(idx1, numCirclePoints, true);
		if (drawCap2)
			_AddFacePolygonIndices(idx2, numCirclePoints, false);
	}

	if (c.lineColor.a != 0)
	{
		ReserveScope rs(lineData, numCirclePoints * 2, numCirclePoints * 6);
		size_t idx1 = lineData->verts.size();
		for (int i = 0; i < numCirclePoints; i++)
			lineData->verts.push_back({circleVerts[i], c.lineColor});
		size_t idx2 = lineData->verts.size();
		for (int i = 0; i < numCirclePoints; i++)
			lineData->verts.push_back({circleVerts[i] + heightOff, c.lineColor});
		_AddLineStripIndices(idx1, numCirclePoints, true);
		_AddLineStripIndices(idx2, numCirclePoints, true);
		for (int i = 0; i < numCirclePoints; i++)
		{
			lineData->indices.push_back(idx1 + i);
			lineData->indices.push_back(idx2 + i);
		}
	}
}

void MeshBuilderPro::AddArrow3D(const Vector& pos,
                                const Vector& target,
                                float tailLength,
                                float tailRadius,
                                float tipHeight,
                                float tipRadius,
                                int numCirclePoints,
                                const MeshColor& c)
{
	if (numCirclePoints < 3 || (c.faceColor.a == 0 && c.lineColor.a == 0))
		return;

	Vector dir = target - pos;
	dir.NormalizeInPlace();
	QAngle ang;
	VectorAngles(dir, ang);

	AddCylinder(pos, ang, tailLength, tailRadius, numCirclePoints, true, false, c);
	// assumes AddCylinder() puts the "top" vertices at the end of the vert list for faces & lines
	size_t innerCircleFaceIdx = faceData->verts.size() - numCirclePoints;
	size_t innerCircleLineIdx = lineData->verts.size() - numCirclePoints;
	AddCone(pos + dir * tailLength, ang, tipHeight, tipRadius, numCirclePoints, false, c);
	// assumes AddCone() puts the base vertices at the end of the vert list for faces & lines
	size_t outerCircleFaceIdx = faceData->verts.size() - numCirclePoints;
	size_t outerCircleLineIdx = lineData->verts.size() - numCirclePoints;

	if (c.faceColor.a != 0)
		_AddFaceTriangleStripIndices(outerCircleFaceIdx, innerCircleFaceIdx, numCirclePoints, true);

	if (c.lineColor.a != 0)
	{
		for (int i = 0; i < numCirclePoints; i++)
		{
			lineData->indices.push_back(innerCircleLineIdx + i);
			lineData->indices.push_back(outerCircleLineIdx + i);
		}
	}
}

void MeshBuilderPro::_AddFaceTriangleStripIndices(size_t vIdx1, size_t vIdx2, size_t numVerts, bool loop, bool mirror)
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
	if (loop)
		Assert(numVerts >= 3);
	else
		Assert(numVerts >= 2);

	ReserveScope rs(faceData, 0, (numVerts - 1 + loop) * 6);

	for (size_t i = 0; i < numVerts - 1; i++)
	{
		if (mirror)
		{
			faceData->indices.push_back(vIdx1 + i);
			faceData->indices.push_back(vIdx2 + i);
			faceData->indices.push_back(vIdx1 + i + 1);
			faceData->indices.push_back(vIdx2 + i);
			faceData->indices.push_back(vIdx2 + i + 1);
			faceData->indices.push_back(vIdx1 + i + 1);
		}
		else
		{
			faceData->indices.push_back(vIdx1 + i);
			faceData->indices.push_back(vIdx2 + i);
			faceData->indices.push_back(vIdx2 + i + 1);
			faceData->indices.push_back(vIdx2 + i + 1);
			faceData->indices.push_back(vIdx1 + i + 1);
			faceData->indices.push_back(vIdx1 + i);
		}
	}
	if (loop)
	{
		if (mirror) // loop + mirror not tested
		{
			faceData->indices.push_back(vIdx1 + numVerts - 1);
			faceData->indices.push_back(vIdx2 + numVerts - 1);
			faceData->indices.push_back(vIdx1);
			faceData->indices.push_back(vIdx2 + numVerts - 1);
			faceData->indices.push_back(vIdx2);
			faceData->indices.push_back(vIdx1);
		}
		else
		{
			faceData->indices.push_back(vIdx1 + numVerts - 1);
			faceData->indices.push_back(vIdx2 + numVerts - 1);
			faceData->indices.push_back(vIdx2);
			faceData->indices.push_back(vIdx2);
			faceData->indices.push_back(vIdx1);
			faceData->indices.push_back(vIdx1 + numVerts - 1);
		}
	}
}

void MeshBuilderPro::_AddFacePolygonIndices(size_t vertsIdx, int numVerts, bool reverse)
{
	// Creates indices representing a filled convex polygon using existing verts at the given vert index for faces only.
	Assert(numVerts >= 3);
	ReserveScope rs(faceData, 0, (numVerts - 2) * 3);
	for (int i = 0; i < numVerts - 2; i++)
	{
		if (reverse)
		{
			faceData->indices.push_back(vertsIdx + i + 2);
			faceData->indices.push_back(vertsIdx + i + 1);
			faceData->indices.push_back(vertsIdx);
		}
		else
		{
			faceData->indices.push_back(vertsIdx);
			faceData->indices.push_back(vertsIdx + i + 1);
			faceData->indices.push_back(vertsIdx + i + 2);
		}
	}
}

void MeshBuilderPro::_AddLineStripIndices(size_t vertsIdx, int numVerts, bool loop)
{
	Assert(numVerts >= 2);
	ReserveScope rs(lineData, 0, (numVerts - 1 + (loop && numVerts > 2)) * 2);
	for (int i = 0; i < numVerts - 1; i++)
	{
		lineData->indices.push_back(vertsIdx + i);
		lineData->indices.push_back(vertsIdx + i + 1);
	}
	if (loop && numVerts > 2)
	{
		lineData->indices.push_back(vertsIdx + numVerts - 1);
		lineData->indices.push_back(vertsIdx);
	}
}

void MeshBuilderPro::_AddSubdivCube(int numSubdivisions, const MeshColor& c)
{
	bool doFaces = c.faceColor.a != 0;
	bool doLines = c.lineColor.a != 0;
	if (numSubdivisions < 0 || (!doFaces && !doLines))
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
	* I used to make this out of quads but the current alg uses 1.5-4 times less verts depending on numSubdivisions.
	*/

	int sideLength = numSubdivisions + 1;
	int nLayerVerts = sideLength * 4;

	int numTotalVerts = 2 * (3 * sideLength + 1) * (sideLength + 1);
	int numTotalFaceIndices = 36 * sideLength * sideLength;
	int numTotalLineIndices = 12 * sideLength * (2 * sideLength + 1);
	ReserveScope rsf(faceData, doFaces ? numTotalVerts : 0, doFaces ? numTotalFaceIndices : 0);
	ReserveScope rsl(lineData, doLines ? numTotalVerts : 0, doLines ? numTotalLineIndices : 0);

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
				faceData->verts.push_back({Vector(x, y, z), c.faceColor});
			if (doLines)
				lineData->verts.push_back({Vector(x, y, z), c.lineColor});
		}
		if (doFaces && z > 0)
		{
			size_t n = faceData->verts.size();
			_AddFaceTriangleStripIndices(n - nLayerVerts * 2, n - nLayerVerts, nLayerVerts, true);
		}
		if (doLines)
		{
			size_t n = lineData->verts.size();
			_AddLineStripIndices(n - nLayerVerts, nLayerVerts, true);
			if (z > 0)
			{
				for (int k = 0; k < nLayerVerts; k++)
				{
					lineData->indices.push_back(n + k - nLayerVerts * 2);
					lineData->indices.push_back(n + k - nLayerVerts);
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
					faceData->verts.push_back({Vector(x, y, z), c.faceColor});
				if (doLines)
					lineData->verts.push_back({Vector(x, y, z), c.lineColor});
			}
			if (doFaces && x > 0)
			{
				size_t n = faceData->verts.size();
				size_t a = n - (sideLength + 1);
				size_t b = n - (sideLength + 1) * 2;
				_AddFaceTriangleStripIndices(top ? a : b, top ? b : a, sideLength + 1, false);
			}
			if (doLines)
			{
				size_t n = lineData->verts.size();
				_AddLineStripIndices(n - 1 - sideLength, sideLength + 1, false);
				if (x > 0)
				{
					for (int k = 0; k < sideLength; k++)
					{
						lineData->indices.push_back(n + k - (sideLength + 1) * 2);
						lineData->indices.push_back(n + k - (sideLength + 1));
					}
				}
			}
		}
	}
}

Vector* MeshBuilderPro::_CreateCircleVerts(const Vector& pos, const QAngle& ang, float radius, int numPoints)
{
	VMatrix mat;
	mat.SetupMatrixOrgAngles(pos, ang);
	mat = mat.Scale(Vector(radius));
	Vector* scratch = Scratch(numPoints);
	for (int i = 0; i < numPoints; i++)
	{
		float s, c;
		SinCos(M_PI_F * 2 / numPoints * i, &s, &c);
		// oriented clockwise, normal is towards x+ for an angle of <0,0,0>
		scratch[i] = mat * Vector(0, s, c);
	}
	return scratch;
}

#endif
