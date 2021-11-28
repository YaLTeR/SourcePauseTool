#pragma once

#ifdef OE
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

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
		float CalculateNewYaw(float newYaw);
		float CalculateNewPitch(float newPitch);
		void UpdateView(float& pitch, float& yaw);
		void SetJump();
	};

	void GetAimAngleIterative(const QAngle& target, QAngle& current, int commandOffset, const Vector& vecSpread);
	bool GetCone(int cone, Vector& out);
	QAngle DecayPunchAngle(QAngle m_vecPunchAngle, QAngle m_vecPunchAngleVel, int frames);
} // namespace aim
