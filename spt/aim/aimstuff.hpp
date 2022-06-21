#pragma once

#ifdef OE
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

#include "..\strafe\strafestuff.hpp"

namespace aim
{
	struct ViewState
	{
		QAngle current;
		QAngle target;

		int ticksLeft;
		bool set;
		bool timedChange;
		bool jumpedLastTick;

		ViewState();
		float CalculateNewYaw(float newYaw, const Strafe::StrafeInput& strafeInput);
		float CalculateNewPitch(float newPitch, const Strafe::StrafeInput& strafeInput);
		void UpdateView(float& pitch, float& yaw, const Strafe::StrafeInput& strafeInput);
		void SetJump();
	};

	void GetAimAngleIterative(const QAngle& target, QAngle& current, int commandOffset, const Vector& vecSpread);
	bool GetCone(int cone, Vector& out);
	QAngle DecayPunchAngle(QAngle m_vecPunchAngle, QAngle m_vecPunchAngleVel, int frames);
} // namespace aim
