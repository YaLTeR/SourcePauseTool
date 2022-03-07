#include "stdafx.h"

#if defined(SSDK2007) || defined(SSDK2013)
#include "..\feature.hpp"
#include "icliententity.h"
#include "..\utils\game_detection.hpp"
#include "ent_utils.hpp"
#include "interfaces.hpp"
#include "..\cvars.hpp"
#include "tickrate.hpp"
#include "signals.hpp"
#include "playerio.hpp"

extern IClientEntity* getPortal(const char* arg, bool verbose);
extern IClientEntity* GetLinkedPortal(IClientEntity* portal);

ConVar y_spt_vag_trace("y_spt_vag_trace", "0", FCVAR_CHEAT, "Draws VAG trace.\n");
ConVar y_spt_vag_target("y_spt_vag_target", "0", FCVAR_CHEAT, "Draws VAG target trace.\n");

// VAG finding tool
class VagTrace : public FeatureWrapper<VagTrace>
{
public:
	void DrawTrace();

	Vector CalculateAG(Vector enter_origin, QAngle enter_angles, Vector exit_origin, QAngle exit_angles);

	void SetTarget(Vector target);

	Vector ReverseAG(Vector enter_origin, QAngle enter_angles, QAngle exit_angles);

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	Vector target = {0.0f, 0.0f, 0.0f};
};

VagTrace spt_vag_trace;

CON_COMMAND(y_spt_vag_target_set, "Set VAG target\n")
{
	spt_vag_trace.SetTarget(spt_playerio.GetPlayerEyePos());
}

void VagTrace::DrawTrace()
{
	bool vag_trace = y_spt_vag_trace.GetBool();
	bool vag_target = y_spt_vag_target.GetBool();
	if (!vag_trace && !vag_target)
		return;

	float lifeTime = spt_tickrate.GetTickrate() * 2;
	if (vag_target)
	{
		interfaces::debugOverlay->AddBoxOverlay(target,
		                                        Vector(-20, -20, -20),
		                                        Vector(20, 20, 20),
		                                        QAngle(0, 0, 0),
		                                        255,
		                                        255,
		                                        0,
		                                        100,
		                                        lifeTime);
	}

	auto enter_portal = getPortal(_y_spt_overlay_portal.GetString(), false);
	if (!enter_portal)
		return;
	auto exit_portal = GetLinkedPortal(enter_portal);
	if (!exit_portal)
		return;

	auto enter_origin = utils::GetPortalPosition(enter_portal);
	auto enter_angles = utils::GetPortalAngles(enter_portal);
	auto exit_origin = utils::GetPortalPosition(exit_portal);
	auto exit_angles = utils::GetPortalAngles(exit_portal);

	if (y_spt_vag_trace.GetBool())
	{
		interfaces::debugOverlay->AddLineOverlay(enter_origin, exit_origin, 0, 0, 255, true, lifeTime);
		interfaces::debugOverlay
		    ->AddLineOverlay(CalculateAG(enter_origin, enter_angles, exit_origin, exit_angles),
		                     enter_origin,
		                     0,
		                     255,
		                     0,
		                     true,
		                     lifeTime);

		if (abs(abs(exit_angles.x) - 90) < 0.04 && abs(exit_angles.z) < 0.04)
		{
			// Is floor/ceiling portal
			QAngle angle = exit_angles;
			angle.y = 179.0f;
			Vector prev = CalculateAG(enter_origin, enter_angles, exit_origin, angle);
			for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 1.0f)
			{
				Vector curr = CalculateAG(enter_origin, enter_angles, exit_origin, angle);
				interfaces::debugOverlay->AddLineOverlay(curr, prev, 255, 255, 255, false, lifeTime);
				prev = curr;
			}
		}
	}
	if (y_spt_vag_target.GetBool())
	{
		QAngle wall_angles[4] = {{0.0f, 0.0f, 0.0f},
		                         {0.0f, 90.0f, 0.0f},
		                         {0.0f, -90.0f, 0.0f},
		                         {0.0f, 180.0f, 0.0f}};
		Vector wall_normal[4] = {{1.0f, 0.0f, 0.0f},
		                         {0.0f, 1.0f, 0.0f},
		                         {0.0f, -1.0f, 0.0f},
		                         {-1.0f, 0.0f, 0.0f}};
		for (int i = 0; i < 4; i++)
		{
			Vector exit = ReverseAG(enter_origin, enter_angles, wall_angles[i]);
			interfaces::debugOverlay->AddBoxOverlay2(exit,
			                                         Vector(-1, -32, -54),
			                                         Vector(1, 32, 54),
			                                         wall_angles[i],
			                                         Color(255, 0, 0, 64),
			                                         Color(255, 0, 0, 255),
			                                         lifeTime);
			interfaces::debugOverlay
			    ->AddLineOverlay(exit, exit + wall_normal[i] * 20, 255, 0, 0, true, lifeTime);
		}

		QAngle angle(90.0f, 179.0f, 0.0f);
		Vector prev = ReverseAG(enter_origin, enter_angles, angle);
		for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 1.0f)
		{
			Vector curr = ReverseAG(enter_origin, enter_angles, angle);
			interfaces::debugOverlay->AddLineOverlay(curr, prev, 255, 0, 255, false, lifeTime);
			prev = curr;
		}

		angle = QAngle(-90.0f, 179.0f, 0.0f);
		prev = ReverseAG(enter_origin, enter_angles, angle);
		for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 1.0f)
		{
			Vector curr = ReverseAG(enter_origin, enter_angles, angle);
			interfaces::debugOverlay->AddLineOverlay(curr, prev, 0, 255, 255, false, lifeTime);
			prev = curr;
		}
	}
}

Vector VagTrace::CalculateAG(Vector enter_origin, QAngle enter_angles, Vector exit_origin, QAngle exit_angles)
{
	Vector exitForward, exitRight, exitUp;
	Vector enterForward, enterRight, enterUp;
	AngleVectors(enter_angles, &enterForward, &enterRight, &enterUp);
	AngleVectors(exit_angles, &exitForward, &exitRight, &exitUp);

	auto delta = enter_origin - exit_origin;

	Vector exit_portal_coords;
	exit_portal_coords.x = delta.Dot(exitForward);
	exit_portal_coords.y = delta.Dot(exitRight);
	exit_portal_coords.z = delta.Dot(exitUp);

	Vector transition(0, 0, 0);
	transition -= exit_portal_coords.x * enterForward;
	transition -= exit_portal_coords.y * enterRight;
	transition += exit_portal_coords.z * enterUp;

	return enter_origin + transition;
}

void VagTrace::SetTarget(Vector target_pos)
{
	this->target = target_pos;
}

Vector VagTrace::ReverseAG(Vector enter_origin, QAngle enter_angles, QAngle exit_angles)
{
	Vector transition = target - enter_origin;
	Vector enter[3];
	AngleVectors(enter_angles, &enter[0], &enter[1], &enter[2]);
	enter[0] *= -1;
	enter[1] *= -1;

	float det = enter[0][0] * (enter[1][1] * enter[2][2] - enter[2][1] * enter[1][2])
	            - enter[0][1] * (enter[1][0] * enter[2][2] - enter[1][2] * enter[2][0])
	            + enter[0][2] * (enter[1][0] * enter[2][1] - enter[1][1] * enter[2][0]);
	float invdet;
	if (det == 0)
	{
		invdet = 1;
	}
	else
	{
		invdet = 1 / det;
	}

	Vector inv_enter[3];
	inv_enter[0][0] = (enter[1][1] * enter[2][2] - enter[2][1] * enter[1][2]) * invdet;
	inv_enter[0][1] = (enter[0][2] * enter[2][1] - enter[0][1] * enter[2][2]) * invdet;
	inv_enter[0][2] = (enter[0][1] * enter[1][2] - enter[0][2] * enter[1][1]) * invdet;
	inv_enter[1][0] = (enter[1][2] * enter[2][0] - enter[1][0] * enter[2][2]) * invdet;
	inv_enter[1][1] = (enter[0][0] * enter[2][2] - enter[0][2] * enter[2][0]) * invdet;
	inv_enter[1][2] = (enter[1][0] * enter[0][2] - enter[0][0] * enter[1][2]) * invdet;
	inv_enter[2][0] = (enter[1][0] * enter[2][1] - enter[2][0] * enter[1][1]) * invdet;
	inv_enter[2][1] = (enter[2][0] * enter[0][1] - enter[0][0] * enter[2][1]) * invdet;
	inv_enter[2][2] = (enter[0][0] * enter[1][1] - enter[1][0] * enter[0][1]) * invdet;

	Vector exit_portal_coords;
	exit_portal_coords.x =
	    transition.x * inv_enter[0][0] + transition.y * inv_enter[1][0] + transition.z * inv_enter[2][0];
	exit_portal_coords.y =
	    transition.x * inv_enter[0][1] + transition.y * inv_enter[1][1] + transition.z * inv_enter[2][1];
	exit_portal_coords.z =
	    transition.x * inv_enter[0][2] + transition.y * inv_enter[1][2] + transition.z * inv_enter[2][2];

	Vector exit[3];
	AngleVectors(exit_angles, &exit[0], &exit[1], &exit[2]);

	det = exit[0][0] * (exit[1][1] * exit[2][2] - exit[2][1] * exit[1][2])
	      - exit[0][1] * (exit[1][0] * exit[2][2] - exit[1][2] * exit[2][0])
	      + exit[0][2] * (exit[1][0] * exit[2][1] - exit[1][1] * exit[2][0]);
	if (det == 0)
	{
		invdet = 1;
	}
	else
	{
		invdet = 1 / det;
	}

	Vector inv_exit_angles[3];
	inv_exit_angles[0][0] = (exit[1][1] * exit[2][2] - exit[2][1] * exit[1][2]) * invdet;
	inv_exit_angles[0][1] = (exit[0][2] * exit[2][1] - exit[0][1] * exit[2][2]) * invdet;
	inv_exit_angles[0][2] = (exit[0][1] * exit[1][2] - exit[0][2] * exit[1][1]) * invdet;
	inv_exit_angles[1][0] = (exit[1][2] * exit[2][0] - exit[1][0] * exit[2][2]) * invdet;
	inv_exit_angles[1][1] = (exit[0][0] * exit[2][2] - exit[0][2] * exit[2][0]) * invdet;
	inv_exit_angles[1][2] = (exit[1][0] * exit[0][2] - exit[0][0] * exit[1][2]) * invdet;
	inv_exit_angles[2][0] = (exit[1][0] * exit[2][1] - exit[2][0] * exit[1][1]) * invdet;
	inv_exit_angles[2][1] = (exit[2][0] * exit[0][1] - exit[0][0] * exit[2][1]) * invdet;
	inv_exit_angles[2][2] = (exit[0][0] * exit[1][1] - exit[1][0] * exit[0][1]) * invdet;

	Vector delta;

	delta.x = inv_exit_angles[0].Dot(exit_portal_coords);
	delta.y = inv_exit_angles[1].Dot(exit_portal_coords);
	delta.z = inv_exit_angles[2].Dot(exit_portal_coords);

	return -(delta - enter_origin);
}

bool VagTrace::ShouldLoadFeature()
{
	return utils::DoesGameLookLikePortal();
}

void VagTrace::LoadFeature()
{
	if (interfaces::debugOverlay && TickSignal.Works)
	{
		TickSignal.Connect(this, &VagTrace::DrawTrace);
		InitCommand(y_spt_vag_target_set);
		InitConcommandBase(y_spt_vag_trace);
		InitConcommandBase(y_spt_vag_target);
	}
}

void VagTrace::UnloadFeature() {}

#endif