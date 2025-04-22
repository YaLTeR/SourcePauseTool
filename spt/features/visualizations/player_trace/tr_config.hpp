#pragma once

#include "spt/features/visualizations/renderer/mesh_renderer.hpp"

#ifdef SPT_MESH_RENDERING_ENABLED
#define SPT_PLAYER_TRACE_ENABLED

namespace player_trace
{
	enum TrSegmentReason : int
	{
		TR_SR_FCPS,
		TR_SR_PLAYER_PORTALLED,
		TR_SR_SAVELOAD,
		TR_SR_MAP_TRANSITION,
		TR_SR_IMPLICIT, // never recorded, only used for drawing

		TR_SR_COLORED_COUNT,

		TR_SR_TRACE_START,

		TR_SR_NONE,
	};

	enum TrPlayerCameraDrawType
	{
		TR_PCDT_FRUSTUM,
		TR_PCDT_BOX_AND_LINE,

		TR_PCDT_COUNT,
	};

	struct
	{
		struct
		{
			std::array<ShapeColor, TR_SR_COLORED_COUNT> segmentEndPoints{
			    ShapeColor{C_OUTLINE(200, 100, 100, 255), true, true, WD_BOTH},
			    ShapeColor{C_OUTLINE(100, 100, 200, 255), true, true, WD_BOTH},
			    ShapeColor{C_OUTLINE(100, 200, 100, 255), true, true, WD_BOTH},
			    ShapeColor{C_OUTLINE(200, 200, 100, 255), true, true, WD_BOTH},
			    ShapeColor{C_OUTLINE(200, 200, 200, 255), true, true, WD_BOTH},
			};

			LineColor grounded{color32{255, 255, 255, 255}};
			LineColor air{color32{0, 255, 0, 255}};
			LineColor airSoftSpeedLocked{color32{200, 200, 0, 255}};
			LineColor airHardSpeedLocked{color32{100, 100, 0, 255}};

			float coneOpacity = .2f;
			float segmentEndpointOpacity = .2f;
		} playerPath;

		struct
		{
			/*
			* Colors pulled from:
			* - CHL2_Player::DrawDebugGeometryOverlays
			* - CBaseEntity::DrawDebugGeometryOverlays
			* - DebugDrawContactPoints
			*/
			ShapeColor qPhys = ShapeColor{C_WIRE(255, 100, 0, 255)};
			ShapeColor qPhysReduction = ShapeColor{C_OUTLINE(255, 0, 0, 100)};
			ShapeColor qPhysOrigin = ShapeColor{C_OUTLINE(255, 100, 200, 50)};
			ShapeColor vPhys = ShapeColor{C_OUTLINE(255, 255, 0, 16)};
			ShapeColor contactPt{C_OUTLINE(0, 255, 0, 32)};

			struct
			{
				struct
				{
					ShapeColor eyes = ShapeColor{C_OUTLINE(20, 200, 20, 50)};
					ShapeColor sgEyes = ShapeColor{C_OUTLINE(100, 200, 20, 50)};
				} frustum;

				struct
				{
					ShapeColor eyes = ShapeColor{C_FACE(0, 255, 255, 255)};
					ShapeColor sgEyes = ShapeColor{C_FACE(100, 200, 255, 255)};
				} boxAndLine;

			} camera;

		} playerHull;

		struct
		{
			color32 blue = {64, 160, 255, 127};
			color32 orange = {255, 160, 32, 127};
		} portals;

		struct
		{
			ShapeColor physMesh{C_OUTLINE(200, 20, 200, 50)};
			ShapeColor physMeshPortalCollisionEnt{C_OUTLINE(200, 60, 100, 20)};
			ShapeColor obb{C_WIRE(220, 150, 20, 255)};
			ShapeColor obbTrigger{C_OUTLINE(200, 200, 20, 10)};
			ShapeColor collectAABB{C_WIRE(150, 100, 100, 255)};
		} entities;
	} inline trColors;

	struct
	{
		struct
		{
			float contactNormalLength = 20;
			float originCubeSize = 1.f;
			float qPhysCenterCrossRadius = 5.f;
			bool drawOriginCube = true;
			bool drawQPhysCenter = true;

			struct
			{
				struct
				{
					float zFar = 8.f;
					float aspect = 16 / 9.f;
					float hatFloat = 1.f;
					float hatHeight = 3.f;
				} frustum;

				struct
				{
					float boxRadius = 1.f;
					float lineLength = 8.f;
				} boxAndLine;

			} camera;

		} playerHull;

		struct
		{
			struct
			{
				int nCirclePoints = 5;
				float length = 3.f;
				float radius = 0.7f;
				int tickInterval = 10;
			} cones;

			struct
			{
				int nCirclePoints = 20;
				float radius = 4.f;
				bool enabled = true;
			} segmentEndpoints;

			float maxDistBeforeImplicitBreakSqr = 130.f * 130.f; // max speed + crouch spamming
			uint32_t maxTicksToRenderAsDynamicMesh = 1000;

		} playerPath;

		struct
		{
			float twicePortalThickness = 2.f;
			ArrowParams arrowParams{5, 10.f, 1.f, .3f, 2.f};
			float hatFloat = 3.f;
			float hatHeight = 8.f;
		} portals;

		struct
		{
			int nBallMeshSubdivisions = 4;
			bool drawEntCollectRadius = true;
			bool drawObbCenter = true;
			float obbCenterCrossRadius = 4.f;
		} entities;
	} inline trStyles;

} // namespace player_trace

#endif