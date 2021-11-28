#include "stdafx.h"
#include "graphics.hpp"
#include "convar.h"
#include "..\spt-serverplugin.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\OrangeBox\modules.hpp"
#include "ent_utils.hpp"
#include "property_getter.hpp"

ConVar y_spt_drawseams("y_spt_drawseams", "0", FCVAR_CHEAT, "Draws seamshot stuff.\n");

namespace vgui
{
	void DrawSeams(IVDebugOverlay* debugOverlay)
	{
		auto player = GetServerPlayer();
		if (!player)
			return;

		float va[3];
		EngineGetViewAngles(va);
		Vector cameraPosition = clientDLL.GetCameraOrigin();
		QAngle angles(va[0], va[1], va[2]);
		Vector vDirection;
		AngleVectors(angles, &vDirection);

		trace_t tr;
		serverDLL.TraceFirePortal(tr, cameraPosition, vDirection);

		if (tr.fraction < 1.0f)
		{
			trace_t edgeTr;
			const float CORNER_CHECK_LENGTH_SQR = 36.0f * 36.0f;
			utils::FindClosestPlane(tr, edgeTr, CORNER_CHECK_LENGTH_SQR);

			if (edgeTr.fraction < 1.0f)
			{
				const float distanceToSeam = 0.01f;
				const float smallPlaneMovement = 0.5f;
				const float bigPlaneMovement = 2.0f;

				Vector test1Start = edgeTr.endpos + tr.plane.normal * smallPlaneMovement
				                    + edgeTr.plane.normal * bigPlaneMovement;
				Vector test2Start = edgeTr.endpos + tr.plane.normal * bigPlaneMovement
				                    + edgeTr.plane.normal * smallPlaneMovement;

				Vector test1Target = edgeTr.endpos + tr.plane.normal * distanceToSeam;
				Vector test2Target = edgeTr.endpos + edgeTr.plane.normal * distanceToSeam;

				bool test1 =
				    utils::TestSeamshot(test1Start, test1Target, tr.plane, edgeTr.plane, QAngle());
				bool test2 =
				    utils::TestSeamshot(test2Start, test2Target, tr.plane, edgeTr.plane, QAngle());
				bool seamshot = test1 || test2;

				const int uiScale = 10;
				float lifeTime = engineDLL.GetTickrate() * 2;

				//calculating an edge vector for drawing
				Vector edge = edgeTr.plane.normal.Cross(tr.plane.normal);
				VectorNormalize(edge);
				debugOverlay->AddLineOverlay(edgeTr.endpos - edge * uiScale,
				                             edgeTr.endpos + edge * uiScale,
				                             seamshot ? 0 : 255,
				                             seamshot ? 255 : 0,
				                             0,
				                             true,
				                             lifeTime);

				// Lines in the direction of the planes
				debugOverlay->AddLineOverlay(edgeTr.endpos,
				                             edgeTr.endpos + edgeTr.plane.normal * uiScale,
				                             test1 ? 0 : 255,
				                             test1 ? 255 : 0,
				                             0,
				                             true,
				                             lifeTime);
				debugOverlay->AddLineOverlay(edgeTr.endpos,
				                             edgeTr.endpos + tr.plane.normal * uiScale,
				                             test2 ? 0 : 255,
				                             test2 ? 255 : 0,
				                             0,
				                             true,
				                             lifeTime);

				// Seamshot triangle
				if (seamshot)
				{
					Vector midPoint = edgeTr.endpos + edgeTr.plane.normal * (uiScale / 2.0)
					                  + tr.plane.normal * (uiScale / 2.0);
					debugOverlay->AddLineOverlay(midPoint,
					                             edgeTr.endpos + edgeTr.plane.normal * uiScale,
					                             test1 ? 0 : 255,
					                             test1 ? 255 : 0,
					                             0,
					                             true,
					                             lifeTime);
					debugOverlay->AddLineOverlay(midPoint,
					                             edgeTr.endpos + tr.plane.normal * uiScale,
					                             test2 ? 0 : 255,
					                             test2 ? 255 : 0,
					                             0,
					                             true,
					                             lifeTime);
				}
			}
		}
	}

	void DrawLines()
	{
		auto debugOverlay = GetDebugOverlay();

		if (!debugOverlay || !utils::playerEntityAvailable())
			return;

		if (y_spt_drawseams.GetBool())
			DrawSeams(debugOverlay);
	}
} // namespace vgui