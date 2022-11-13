#include "stdafx.h"

#include "renderer\mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED

#include "spt\utils\math.hpp"
#include "spt\feature.hpp"
#include "spt\utils\signals.hpp"
#include "spt\utils\interfaces.hpp"

// purpose: serves as way to visually test the mesh builder as well as show how to use some of its features

ConVar y_spt_draw_mesh_examples("y_spt_draw_mesh_examples",
                                "0",
                                FCVAR_CHEAT | FCVAR_DONTRECORD,
                                "Draw a bunch of example meshes near the origin.");

static std::vector<class BaseMeshRenderTest*> tests;

class BaseMeshRenderTest
{
protected:
	BaseMeshRenderTest()
	{
		tests.push_back(this);
	}

public:
	virtual void TestFunc(MeshRenderer& mr) = 0;
};

class MeshTestFeature : public FeatureWrapper<MeshTestFeature>
{
protected:
	virtual void LoadFeature()
	{
		if (MeshRenderSignal.Works)
		{
			MeshRenderSignal.Connect(this, &MeshTestFeature::OnMeshRenderSignal);
			InitConcommandBase(y_spt_draw_mesh_examples);
		}
	};

	void OnMeshRenderSignal(MeshRenderer& mr)
	{
		float lastTime = time;
		time = interfaces::engine_tool->HostTime();
		if (timeSinceLast < 0)
			timeSinceLast = 0; // init
		else
			timeSinceLast = time - lastTime;

		if (!y_spt_draw_mesh_examples.GetBool())
			return;
		for (auto testCase : tests)
			testCase->TestFunc(mr);
	}

public:
	float time;
	float timeSinceLast = -666;
};

MeshTestFeature testFeature;

#define VEC_WRAP(x, y, z) Vector(x, y, z)

// creates a new struct in a new namespace inheriting from the base test and implements the TestFunc
#define BEGIN_TEST_CASE(desc, position) \
	namespace CONCATENATE(_Test, __COUNTER__) \
	{ \
		class _Test : BaseMeshRenderTest \
		{ \
			const Vector testPos = position; \
			virtual void TestFunc(MeshRenderer& mr) override; \
		}; \
		static _Test _inst; \
		void _Test::TestFunc(MeshRenderer& mr) \
		{ \
			if (interfaces::debugOverlay) \
				interfaces::debugOverlay->AddTextOverlay(testPos + Vector{0, 0, 150}, 0, desc);

#define END_TEST_CASE() \
	} \
	}

BEGIN_TEST_CASE("AddLine()", VEC_WRAP(0, 0, 0))
RENDER_DYNAMIC(mr, mb.AddLine(testPos + Vector(-70, -70, -80), testPos + Vector(70, 70, 80), {255, 0, 255, 100}););
END_TEST_CASE()

const Vector lineTestVerts[] = {{0, 0, 80}, {-70, 70, 80}, {-70, 70, -80}, {0, 0, -80}, {70, 70, -80}, {70, 70, 80}};
const int numLineTestVerts = sizeof(lineTestVerts) / sizeof(Vector);

BEGIN_TEST_CASE("AddLines()", VEC_WRAP(200, 0, 0))
{
	Vector v[numLineTestVerts];
	for (int i = 0; i < numLineTestVerts; i++)
		v[i] = lineTestVerts[i] + testPos;
	RENDER_DYNAMIC(mr, mb.AddLines(v, numLineTestVerts / 2, {255, 120, 120, 255}););
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddLineStrip(loop=false)", VEC_WRAP(400, 0, 0))
{
	Vector v[numLineTestVerts];
	for (int i = 0; i < numLineTestVerts; i++)
		v[i] = lineTestVerts[i] + testPos;
	RENDER_DYNAMIC(mr, mb.AddLineStrip(v, numLineTestVerts, false, {120, 255, 120, 255}););
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddLineStrip(loop=true)", VEC_WRAP(600, 0, 0))
{
	Vector v[numLineTestVerts];
	for (int i = 0; i < numLineTestVerts; i++)
		v[i] = lineTestVerts[i] + testPos;
	RENDER_DYNAMIC(mr, mb.AddLineStrip(v, numLineTestVerts, true, {120, 120, 255, 255}););
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddPolygon()", VEC_WRAP(800, 0, 0))
{
	// clang-format off
	Vector v1[] = {testPos + Vector{-70, -70, 80}, testPos + Vector{0, 70, 80}, testPos + Vector{70, -70, 80}};
	Vector v2[] = {testPos + Vector{-70, -70, 30}, testPos + Vector{0, 70, 30}, testPos + Vector{80, 30, 30}, testPos + Vector{70, -70, 30}};
	Vector v3[] = {testPos + Vector{-70, -70, -30}, testPos + Vector{-70, 30, -30}, testPos + Vector{0, 70, -30}, testPos + Vector{80, 30, -30}, testPos + Vector{70, -70, -30}};
	// clang-format on

	// Add top to bottom and check if they get sorted properly:
	// looking through the top you should see mostly green (not yellow) and not be able to see the bottom wireframe
	RENDER_DYNAMIC(mr, mb.AddPolygon(v1, sizeof(v1) / sizeof(Vector), {{50, 255, 50, 220}, {255, 50, 255, 255}}););
	RENDER_DYNAMIC(mr, mb.AddPolygon(v2, sizeof(v2) / sizeof(Vector), MeshColor::Outline({255, 50, 50, 100})););
	RENDER_DYNAMIC(mr, mb.AddPolygon(v3, sizeof(v3) / sizeof(Vector), {{150, 150, 150, 255}, {0, 0, 255, 250}}););
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddCircle()", VEC_WRAP(1000, 0, 0))
{
	Vector dir(4, 2, 13);
	QAngle ang;
	VectorAngles(dir, ang);
	for (int i = 10; i >= 0; i--) // draw backwards and check for sorting
	{
		RENDER_DYNAMIC(
		    mr,
		    {
			    mb.AddCircle(testPos + Vector(-20, 0, -30) + dir * i,
			                 ang,
			                 20 + 8 * i,
			                 3 * (i + 1),
			                 MeshColor::Outline({(byte)(250 - i * 20), 200, (byte)(i * 20), 50}));
		    },
		    ZTEST_FACES | ZTEST_LINES,
		    CullType::Default,
		    TranslucentSortType::AABB_Center); // test that this works with y_spt_draw_mesh_debug
	}
	// jwst tribute
	RENDER_DYNAMIC(mr, {
		Vector center = testPos + Vector(0, 200, 100);
		for (int i = 0; i < 6; i++)
		{
			float s;
			float c;
			SinCos(M_PI_F / 3 * i, &s, &c);
			int jMax = 10;
			for (int j = 0; j <= jMax; j++)
			{
				Vector pos = center + Vector(s, 0, c) * 50 * (.025 * (j - jMax) + 1);
				QAngle ang(0, 90, 30);
				mb.AddCircle(pos, ang, 2 + j * 24.5 / jMax, 6, MeshColor::Wire({200, 255, 0, 255}));
			}
		}
	});
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddTris()", VEC_WRAP(1200, 0, 0))
{
	float c30, s30;
	SinCos(DEG2RAD(30), &s30, &c30);
	float triRad = 50;
	float topZ = 60;
	// clang-format off
	const Vector v[] = {{0, triRad, topZ}, {triRad * c30, -triRad * s30, topZ}, {-triRad * c30, -triRad * s30, topZ},
	                    v[1], v[0], (v[0] + v[1]) / 2 - Vector(0, 0, triRad * (1 + s30)),
	                    v[2], v[1], (v[1] + v[2]) / 2 - Vector(0, 0, triRad * (1 + s30)),
	                    v[0], v[2], (v[0] + v[2]) / 2 - Vector(0, 0, triRad * (1 + s30))};
	// clang-format on
	const int numVerts = sizeof(v) / sizeof(Vector);
	Vector vNew[numVerts];
	for (int i = 0; i < numVerts; i++)
		vNew[i] = v[i] + testPos;
	RENDER_DYNAMIC(mr, mb.AddTris(vNew, numVerts / 3, MeshColor::Outline({50, 150, 200, 30}));
	               , ZTEST_FACES | ZTEST_LINES, CullType::Reverse);
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddQuad()", VEC_WRAP(1400, 0, 0))
{
	// naive game of life implementation - alternate between two boards

	const size_t size = 35; // one row/column on each side for padding
	const int maxMinigameTicks = 250;
	const float secPerMinigameTick = .09;

	static std::vector<bool> state0(size * size);
	static std::vector<bool> state1(size * size);
	static float remainderTime = 0;
	static int minigameTick = maxMinigameTicks;
	remainderTime += testFeature.timeSinceLast;

	if (minigameTick >= maxMinigameTicks)
	{
		// randomize board
		for (size_t y = 1; y < size - 1; y++)
			for (size_t x = 1; x < size - 1; x++)
				state0[y * size + x] = rand() % 2;
		minigameTick = 0;
	}

	// update board state
	while (remainderTime >= secPerMinigameTick && minigameTick < maxMinigameTicks)
	{
		remainderTime -= secPerMinigameTick;
		minigameTick++;

		auto& curBoard = minigameTick % 2 ? state0 : state1;
		auto& newBoard = minigameTick % 2 ? state1 : state0;
		int numUpdated = 0;
		for (size_t y = 1; y < size - 1; y++)
		{
			for (size_t x = 1; x < size - 1; x++)
			{
				// clang-format off
				int numSurrounding =
					curBoard[(y + 1) * size + (x - 1)] + curBoard[(y + 1) * size + x] + curBoard[(y + 1) * size + (x + 1)] +
					curBoard[(y + 0) * size + (x - 1)] + 0                            + curBoard[(y + 0) * size + (x + 1)] +
					curBoard[(y - 1) * size + (x - 1)] + curBoard[(y - 1) * size + x] + curBoard[(y - 1) * size + (x + 1)];
				// clang-format on
				size_t curCellIdx = y * size + x;
				bool curCell = curBoard[curCellIdx];
				newBoard[curCellIdx] =
				    (curCell && (numSurrounding == 2 || numSurrounding == 3)) // stay alive
				    || (!curCell && numSurrounding == 3);                     // reproduce
				numUpdated += curCell != newBoard[curCellIdx];
			}
		}
		if (numUpdated < 10) // naive check to speed up simulation
			minigameTick += 2;
	}
	mr.DrawMesh(MeshBuilderPro::CreateDynamicMesh(
	    [&](MeshBuilderPro& mb)
	    {
		    const color32 c1 = {50, 50, 200, 255};
		    const color32 c2 = {150, 0, 0, 255};
		    color32 cLerp{};
		    float t = (float)minigameTick / maxMinigameTicks;
		    for (int i = 0; i < 4; i++)
			    ((byte*)&cLerp)[i] = (1 - t) * ((byte*)&c1)[i] + t * ((byte*)&c2)[i];

		    auto& curBoard = minigameTick % 2 ? state1 : state0;
		    const float tileSize = 4;
		    Vector boardCorner = testPos - Vector(tileSize * size / 2, tileSize * size / 2, 0);
		    for (size_t y = 1; y < size - 1; y++)
		    {
			    for (size_t x = 1; x < size - 1; x++)
			    {
				    if (!curBoard[y * size + x])
					    continue;
				    Vector tileCorner = boardCorner + Vector(x * tileSize, y * tileSize, 0);
				    mb.AddQuad(tileCorner,
				               tileCorner + Vector(0, tileSize, 0),
				               tileCorner + Vector(tileSize, tileSize, 0),
				               tileCorner + Vector(tileSize, 0, 0),
				               {cLerp, {150, 150, 150, 255}});
			    }
		    }
		    // outline board
		    mb.AddQuad(boardCorner,
		               boardCorner + Vector(0, tileSize * size, 0),
		               boardCorner + Vector(tileSize * size, tileSize * size, 0),
		               boardCorner + Vector(tileSize * size, 0, 0),
		               MeshColor::Wire({50, 50, 50, 255}));
	    },
	    {ZTEST_FACES | ZTEST_LINES, CullType::ShowBoth})); // test culling
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddBox()", VEC_WRAP(1600, 0, 0))
{
	const Vector mins(-16, -16, 0);
	const Vector maxs(16, 16, 72);
	const Vector minsInner = mins * 0.7;
	const Vector maxsInner = {16 * .7, 16 * .7, 72};
	RENDER_DYNAMIC(mr, {
		mb.AddBox(testPos, minsInner, maxsInner, vec3_angle, MeshColor::Outline({255, 0, 0, 100}));
		mb.AddBox(testPos, mins, maxs, vec3_angle, MeshColor::Wire({255, 100, 0, 255}));
		mb.AddBox(testPos, mins, maxs, {0, 0, 20}, MeshColor::Outline({255, 255, 0, 16}));
	});
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddSphere()", VEC_WRAP(1800, 0, 0))
{
	RENDER_DYNAMIC(mr, mb.AddSphere(testPos + Vector(0, 100, 20), 50, 8, MeshColor::Face({150, 100, 255, 50})););
	RENDER_DYNAMIC(mr, mb.AddSphere(testPos + Vector(60, 0, 20), 50, 4, MeshColor::Outline({0, 200, 200, 50})););
	RENDER_DYNAMIC(mr, mb.AddSphere(testPos + Vector(-60, 0, 20), 50, 9, {{20, 50, 80, 255}, {180, 99, 30, 255}}););
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddSweptBox()", VEC_WRAP(2000, 0, 0))
{
	/*
	* The sweep may be drawn differently if it has a zero component in any of the axes and
	* the index math for swept boxes is very non-trivial, so check all possible combinations
	* of the components of the sweep being 0. Also check cases where the sweep is small -
	* the may lead to weird cases where the sweep may overlap the start/end in unexpected ways.
	* 
	* Since there's just so many swept boxes, I'll actually make them all static cuz mah poor fps :(
	*/

	static std::vector<StaticMesh> boxes;

	if (boxes.size() == 0 || !boxes[0])
	{
		// create all
		boxes.clear();

		MeshColor cStart = MeshColor::Outline({0, 255, 0, 20});
		MeshColor cEnd = MeshColor::Outline({255, 0, 0, 20});
		MeshColor cSweep = MeshColor::Outline({150, 150, 150, 20});
		const Vector mins(-5, -7, -9);
		const Vector maxs(7, 9, 11);
		const float sweepMags[] = {10, 25}; // test small and large sweeps
		const float xOffs[] = {-50, 50};
		const float ySpacing = 90;

		{
			// no sweep
			Vector start = testPos + Vector(0, 0, 60);
			boxes.push_back(
			    MB_STATIC(mb.AddSweptBox(start, start + Vector(0.001), mins, maxs, cStart, cEnd, cSweep);));
		}

		for (int i = 0; i < 2; i++)
		{
			// test all (24!!!) edge cases
			int yOff = 0;
			for (int excludeAxIdx = 0; excludeAxIdx < 3; excludeAxIdx++)
			{
				int axIdx0 = (excludeAxIdx + 1) % 3;
				int axIdx1 = (excludeAxIdx + 2) % 3;
				for (int ax0 = 0; ax0 < 2; ax0++)
				{
					for (int ax1 = 0; ax1 < 2; ax1++)
					{
						for (int j = 0; j < 2; j++)
						{
							Vector start = testPos + Vector(xOffs[i], ++yOff * ySpacing, 0);
							Vector off;
							off[excludeAxIdx] = 0;
							off[axIdx0] = (ax0 - .5) * (j ? 1 : .2);
							off[axIdx1] = (ax1 - .5) * (j ? .2 : 1);
							VectorNormalize(off);
							Vector end = start + off * sweepMags[i];
							boxes.push_back(MB_STATIC(mb.AddSweptBox(
							    start, end, mins, maxs, cStart, cEnd, cSweep);));
						}
					}
				}
			}
			// test all corner cases
			yOff = 0;
			for (int ax0 = 0; ax0 < 2; ax0++)
			{
				for (int ax1 = 0; ax1 < 2; ax1++)
				{
					for (int ax2 = 0; ax2 < 2; ax2++)
					{
						Vector start = testPos + Vector(xOffs[i], ++yOff * ySpacing, 60);
						Vector off(ax0 - .5, ax1 - .5, ax2 - .5);
						VectorNormalize(off);
						Vector end = start + off * sweepMags[i];
						boxes.push_back(MB_STATIC(
						    mb.AddSweptBox(start, end, mins, maxs, cStart, cEnd, cSweep);));
					}
				}
			}
			// test all face cases
			yOff = 0;
			for (int ax = 0; ax < 3; ax++)
			{
				for (int reflect = 0; reflect < 2; reflect++)
				{
					Vector start = testPos + Vector(xOffs[i], ++yOff * ySpacing, 120);
					Vector off(0);
					off[ax] = reflect - .5;
					VectorNormalize(off);
					Vector end = start + off * sweepMags[i];
					boxes.push_back(
					    MB_STATIC(mb.AddSweptBox(start, end, mins, maxs, cStart, cEnd, cSweep);));
				}
			}
		}
	}
	for (auto& staticMesh : boxes)
	{
		Assert(staticMesh);
		mr.DrawMesh(staticMesh);
	}
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddCone()", VEC_WRAP(2200, 0, 0))
{
	{
		const Vector org = testPos + Vector(-50, 0, 0);
		const Vector tipOff(30, 50, 70);
		QAngle ang;
		VectorAngles(tipOff, ang);
		float h = tipOff.Length();
		RENDER_DYNAMIC(mr, mb.AddCone(org, ang, h, 20, 20, false, MeshColor::Outline({255, 255, 50, 20})););
	}
	{
		const Vector org = testPos + Vector(50, 0, 0);
		const Vector tipOff(-30, -50, -70);
		QAngle ang;
		VectorAngles(tipOff, ang);
		float h = tipOff.Length();
		RENDER_DYNAMIC(mr, mb.AddCone(org, ang, h, 20, 5, true, MeshColor::Outline({255, 50, 50, 20})););
	}
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddCylinder()", VEC_WRAP(2400, 0, 0))
{
	RENDER_DYNAMIC(mr, {
		mb.AddCylinder(testPos, vec3_angle, 20, 10, 5, true, true, MeshColor::Outline({255, 150, 0, 40}));
	});
	Vector org = testPos + Vector(0, 100, 0);
	RENDER_DYNAMIC(mr, {
		mb.AddCylinder(org, {0, 180, 0}, 20, 10, 10, false, true, MeshColor::Outline({255, 100, 0, 30}));
	});
	org += Vector(0, 100, 0);
	RENDER_DYNAMIC(mr, {
		mb.AddCylinder(org, {-80, 90, 0}, 20, 10, 15, true, false, MeshColor::Outline({255, 50, 0, 20}));
	});
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddArrow3D()", VEC_WRAP(2600, 0, 0))
{
	const Vector target = testPos + Vector(0, 100, 50);
	RENDER_DYNAMIC(mr, mb.AddCross(target, 7, {255, 0, 0, 255}););
	for (int i = 0; i < 4; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			Vector org = testPos + Vector((i - 1.5) * 25, k * 75, k * 30);
			unsigned char g = (unsigned char)(1.f / (1 + org.DistToSqr(target) / 2000) * 255);
			unsigned char b = (unsigned char)(1.f / (1 + org.DistToSqr(target) / 3000) * 255);
			RENDER_DYNAMIC(mr, {
				mb.AddArrow3D(org, target, 15, 1.5, 7, 3, 7, MeshColor::Outline({100, g, b, 20}));
			});
		}
	}
}
END_TEST_CASE()

BEGIN_TEST_CASE("Timmy", VEC_WRAP(0, -300, 0))
{
	static StaticMesh timmyMesh;
	if (!timmyMesh)
	{
		const Vector mins = {-20, -20, -20};
		const MeshColor c = {{100, 100, 255, 255}, {255, 0, 255, 255}};
		const QAngle a = vec3_angle;
		timmyMesh = MB_STATIC(mb.AddBox(vec3_origin, mins, -mins, a, c););
	}

	static QAngle ang(0, 0, 0);
	ang[0] = fmod(ang[0] + 89 * testFeature.timeSinceLast, 360);
	ang[1] = fmod(ang[1] + 199 * testFeature.timeSinceLast, 360);
	ang[2] = fmod(ang[2] + 317 * testFeature.timeSinceLast, 360);

	mr.DrawMesh(timmyMesh,
	            [this](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
	            {
		            float scale = sin(testFeature.time) / 4 + 1;
		            matrix3x4_t tmpMat;
		            SetScaleMatrix(scale, infoOut.mat);
		            AngleMatrix(ang, testPos, tmpMat);
		            MatrixMultiply(tmpMat, infoOut.mat, infoOut.mat);
		            infoOut.faces.colorModulate.a = (sin(testFeature.time) + 1) / 2.f * 255;
		            // timmy will be more yellow through portals
		            if (infoIn.portalRenderDepth.value_or(0) >= 1)
			            infoOut.faces.colorModulate.b = 0;
		            // timmy will be more green on the overlay
		            if (infoIn.inOverlayView)
			            infoOut.faces.colorModulate.r = 0;
	            });
}
END_TEST_CASE()

BEGIN_TEST_CASE("Lorenz Attractor", VEC_WRAP(200, -300, 0))
{
	// Intellisense doesn't do shit when you're in a macro, so I'm not gonna use RENDER_DYNAMIC_CALLBACK for something so big.
	// Also, clang-format does literally the most stupid thing possible sometimes and using these macros makes it better.

#define MB_CREATE_BEGIN MeshBuilderPro::CreateDynamicMesh([&](MeshBuilderPro & mb)
#define MB_CREATE_END )

	mr.DrawMesh(
	    MB_CREATE_BEGIN {
		    // keep the last 2500 positions and draw them every frame
		    static std::vector<Vector> verts = {{1, 1, 1}};
		    const size_t maxVerts = 2500;
		    static size_t lastIdx = 0;
		    // viridis colormap from python pyplot
		    static std::vector<color32> colors = {{253, 231, 37, 255},
		                                          {94, 201, 98, 200},
		                                          {33, 145, 140, 150},
		                                          {59, 82, 139, 100},
		                                          {68, 1, 84, 0}};
		    const float sigma = 10, beta = 8.f / 3, rho = 28, timestep = 0.007;

		    const float secPerStep = 0.004;
		    static float remainder = 0;
		    remainder += testFeature.timeSinceLast;

		    while (remainder >= secPerStep)
		    {
			    remainder -= secPerStep;
			    // calc next position
			    Vector& vPrev = verts[lastIdx++];
			    Vector delta(sigma * (vPrev.y - vPrev.x),
			                 vPrev.x * (rho - vPrev.z) - vPrev.y,
			                 vPrev.x * vPrev.y - beta * vPrev.z);
			    Vector vNext = vPrev + delta * timestep;

			    if (verts.size() < maxVerts)
				    verts.push_back(vNext);
			    else
				    verts[lastIdx %= maxVerts] = vNext;
		    }

		    // draw line segments between each point
		    Vector* prev = 0;
		    for (size_t i = 0; i < verts.size(); i++)
		    {
			    size_t idx = (maxVerts + lastIdx - i) % maxVerts;
			    Vector* cur = &verts[idx];
			    if (i > 0)
			    {
				    // figure out which colors to lerp between
				    float colorIdx = (float)i / ((float)maxVerts / (colors.size() - 1));
				    color32 c1 = colors[(int)colorIdx];
				    color32 c2 = colors[(int)colorIdx + 1];
				    color32 cLerp{};
				    for (int j = 0; j < 4; j++)
				    {
					    float lerp = colorIdx - (int)colorIdx;
					    ((byte*)&cLerp)[j] = (1 - lerp) * ((byte*)&c1)[j] + lerp * ((byte*)&c2)[j];
				    }
				    mb.AddLine(*prev, *cur, cLerp);
			    }
			    prev = cur;
		    }
		    // put ball at most recent position (draw last so it draws on top)
		    mb.AddSphere(verts[lastIdx], 0.6, 2, MeshColor::Face({200, 255, 200, 255}));
	    } MB_CREATE_END,
	    [this](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
	    {
		    // the Lorenz Attractor chills near the origin and is quite small, so scale it up and put at test case pos
		    SetScaleMatrix(3, infoOut.mat);
		    PositionMatrix(testPos, infoOut.mat);
	    });

#undef MB_CREATE_BEGIN
#undef MB_CREATE_END
}
END_TEST_CASE()

BEGIN_TEST_CASE("Reusing static/dynamic meshes", VEC_WRAP(400, -300, 0))
{
	// returns a create func given a color
	auto coloredCreateFunc = [](const MeshColor& c) {
		return [&](MeshBuilderPro& mb) { mb.AddBox(vec3_origin, {-10, -10, 0}, {10, 10, 50}, vec3_angle, c); };
	};

	// create once
	static StaticMesh staticMesh;
	if (!staticMesh)
		staticMesh = MeshBuilderPro::CreateStaticMesh(coloredCreateFunc(MeshColor::Outline({0, 0, 255, 20})));

	// recreate every frame
	DynamicMesh dynamicMesh =
	    MeshBuilderPro::CreateDynamicMesh(coloredCreateFunc(MeshColor::Outline({255, 0, 0, 20})));

	for (int i = 0; i < 20; i++)
	{
		auto callbackFunc = [this, i](const CallbackInfoIn& infoIn, CallbackInfoOut& infoOut)
		{
			float ts = sin(testFeature.time * 8 + i / 2.f);
			Vector scale(1 + ts * .2f, 0, 1 - ts * .2f);
			SetScaleMatrix(1 + ts * .2f, 1, 1 - ts * .2f, infoOut.mat);
			MatrixSetColumn(testPos + Vector(0, -i * 25, 0), 3, infoOut.mat);
		};
		if (i % 2)
			mr.DrawMesh(staticMesh, callbackFunc);
		else
			mr.DrawMesh(dynamicMesh, callbackFunc);
	}
}
END_TEST_CASE()

BEGIN_TEST_CASE("AddCPolyhedron", VEC_WRAP(600, -300, 0))
{
	static std::vector<VPlane> planes;
	planes.clear();

	// carve out a cube
	const float cubeRad = 40;
	for (int ax = 0; ax < 3; ax++)
	{
		for (int side = 0; side < 2; side++)
		{
			Vector n{0};
			n[ax] = side * 2 - 1;
			planes.emplace_back(n, cubeRad);
		}
	}

	// carve out a dodecahedron
	float dodecaRad = 50 + sin(testFeature.time) * 20;
	planes.emplace_back(Vector{0, 0, 1}, dodecaRad);
	planes.emplace_back(Vector{0, 0, -1}, dodecaRad);
	for (int i = 0; i < 2; i++)
	{
		float z = cos((i + 1) * M_PI_F / 3);
		float angOff = i * M_PI_F / 5;
		for (int j = 0; j < 5; j++)
		{
			float x, y;
			SinCos(j * M_PI_F / 2.5f + angOff, &x, &y);
			Vector n{x, y, z};
			planes.emplace_back(n * 0.8944272f, dodecaRad);
		}
	}

	CPolyhedron* polyhedron = GeneratePolyhedronFromPlanes((float*)planes.data(), planes.size(), 0.0001f, true);

	mr.DrawMesh(MeshBuilderPro::CreateDynamicMesh(
	                [&](MeshBuilderPro& mb) {
		                mb.AddCPolyhedron(polyhedron, MeshColor::Outline({200, 150, 50, 20}));
	                }),
	            [this](auto&, CallbackInfoOut& infoOut) { MatrixSetColumn(testPos, 3, infoOut.mat); });

	polyhedron->Release();
}
END_TEST_CASE()

#endif
