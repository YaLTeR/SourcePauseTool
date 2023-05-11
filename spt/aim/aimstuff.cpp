#include "stdafx.hpp"

#include "aimstuff.hpp"

#include "convar.hpp"
#include "math.hpp"
#include "ent_utils.hpp"
#include "spt/sptlib-wrapper.hpp"
#include "spt\features\game_fixes\rng.hpp"
#include "..\spt-serverplugin.hpp"

#undef max
#undef min

extern ConVar tas_strafe_vectorial_offset;

namespace aim
{
	float angleChange(float value,
	                  float prevValue,
	                  float target,
	                  float anglespeed,
	                  bool timedChange,
	                  int ticksLeft,
	                  bool yaw,
	                  bool jumpedLastTick)
	{
		float diff;

		if (yaw && jumpedLastTick)
			value = prevValue;

		if (yaw)
		{
			diff = utils::NormalizeDeg(target - value);
		}
		else
		{
			diff = target - value;
		}

		if (!timedChange)
		{
			if (std::abs(diff) < anglespeed)
			{
				return target;
			}
			else
			{
				if (yaw)
					return utils::NormalizeDeg(value + std::copysignf(anglespeed, diff));
				else
					return value + std::copysignf(anglespeed, diff);
			}
		}
		else if (ticksLeft == 1)
		{
			return target;
		}
		else
		{
			if (yaw)
			{
				return utils::NormalizeDeg(value + diff / ticksLeft);
			}
			else
			{
				return value + diff / ticksLeft;
			}
		}
	}

	ViewState::ViewState()
	{
		targetID = -1;
		ticksLeft = 0;
		state = NO_AIM;
		timedChange = false;
		jumpedLastTick = false;
	}

	float ViewState::CalculateNewYaw(float newYaw, const Strafe::StrafeInput& strafeInput)
	{
		if (state == NO_AIM && strafeInput.Strafe && strafeInput.Vectorial && strafeInput.Version >= 4)
		{
			float targetYaw =
			    utils::NormalizeDeg(strafeInput.TargetYaw + tas_strafe_vectorial_offset.GetFloat());
			current[YAW] = angleChange(newYaw,
			                           current[YAW],
			                           targetYaw,
			                           strafeInput.AngleSpeed,
			                           false,
			                           0,
			                           true,
			                           jumpedLastTick);
		}
		else if (state != NO_AIM)
		{
			current[YAW] = angleChange(newYaw,
			                           current[YAW],
			                           target[YAW],
			                           strafeInput.AngleSpeed,
			                           timedChange,
			                           ticksLeft,
			                           true,
			                           jumpedLastTick);
		}
		else
		{
			current[YAW] = newYaw;
		}

		return current[YAW];
	}

	float ViewState::CalculateNewPitch(float newPitch, const Strafe::StrafeInput& strafeInput)
	{
		if (state != NO_AIM)
			current[PITCH] = angleChange(newPitch,
			                             current[PITCH],
			                             target[PITCH],
			                             strafeInput.AngleSpeed,
			                             timedChange,
			                             ticksLeft,
			                             false,
			                             jumpedLastTick);
		else
			current[PITCH] = newPitch;
		return current[PITCH];
	}

	void ViewState::UpdateView(float& pitch, float& yaw, const Strafe::StrafeInput& strafeInput)
	{
		pitch = CalculateNewPitch(pitch, strafeInput);
		yaw = CalculateNewYaw(yaw, strafeInput);

		if (timedChange && ticksLeft > 0)
		{
			--ticksLeft;
			if (ticksLeft == 0)
			{
				timedChange = false;
			}
		}

		jumpedLastTick = false;
	}

	void ViewState::SetJump()
	{
		jumpedLastTick = true;
	}

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define NDIV (1 + (IM - 1) / NTAB)
#define MAX_RANDOM_RANGE 0x7FFFFFFFUL
#define NTAB 32

#define AM (1.0 / IM)
#define EPS 1.2e-7
#define RNMX (1.0 - EPS)

	class RandomStream
	{
	public:
		void SetSeed(int iSeed);
		int GenerateRandomNumber();
		float RandomFloat(float flLow, float flHigh);
		int RandomInt(int iLow, int iHigh);

	private:
		int m_idum;
		int m_iy;
		int m_iv[NTAB];
	};

	void RandomStream::SetSeed(int iSeed)
	{
		m_idum = ((iSeed < 0) ? iSeed : -iSeed);
		m_iy = 0;
	}

	int RandomStream::GenerateRandomNumber()
	{
		int j;
		int k;

		if (m_idum <= 0 || !m_iy)
		{
			if (-(m_idum) < 1)
				m_idum = 1;
			else
				m_idum = -(m_idum);

			for (j = NTAB + 7; j >= 0; j--)
			{
				k = (m_idum) / IQ;
				m_idum = IA * (m_idum - k * IQ) - IR * k;
				if (m_idum < 0)
					m_idum += IM;
				if (j < NTAB)
					m_iv[j] = m_idum;
			}
			m_iy = m_iv[0];
		}
		k = (m_idum) / IQ;
		m_idum = IA * (m_idum - k * IQ) - IR * k;
		if (m_idum < 0)
			m_idum += IM;
		j = m_iy / NDIV;

		m_iy = m_iv[j];
		m_iv[j] = m_idum;

		return m_iy;
	}

	float RandomStream::RandomFloat(float flLow, float flHigh)
	{
		// float in [0,1)
		float fl = AM * GenerateRandomNumber();
		if (fl > RNMX)
		{
			fl = RNMX;
		}
		return (fl * (flHigh - flLow)) + flLow; // float in [low,high)
	}

	int RandomStream::RandomInt(int iLow, int iHigh)
	{
		//ASSERT(lLow <= lHigh);
		unsigned int maxAcceptable;
		unsigned int x = iHigh - iLow + 1;
		unsigned int n;
		if (x <= 1 || MAX_RANDOM_RANGE < x - 1)
		{
			return iLow;
		}

		// The following maps a uniform distribution on the interval [0,MAX_RANDOM_RANGE]
		// to a smaller, client-specified range of [0,x-1] in a way that doesn't bias
		// the uniform distribution unfavorably. Even for a worst case x, the loop is
		// guaranteed to be taken no more than half the time, so for that worst case x,
		// the average number of times through the loop is 2. For cases where x is
		// much smaller than MAX_RANDOM_RANGE, the average number of times through the
		// loop is very close to 1.
		//
		maxAcceptable = MAX_RANDOM_RANGE - ((MAX_RANDOM_RANGE + 1) % x);
		do
		{
			n = GenerateRandomNumber();
		} while (n > maxAcceptable);

		return iLow + (n % x);
	}

	// Rng cones
#define VECTOR_CONE_PRECALCULATED vec3_origin
#define VECTOR_CONE_1DEGREES Vector(0.00873, 0.00873, 0.00873)
#define VECTOR_CONE_2DEGREES Vector(0.01745, 0.01745, 0.01745)
#define VECTOR_CONE_3DEGREES Vector(0.02618, 0.02618, 0.02618)
#define VECTOR_CONE_4DEGREES Vector(0.03490, 0.03490, 0.03490)
#define VECTOR_CONE_5DEGREES Vector(0.04362, 0.04362, 0.04362)
#define VECTOR_CONE_6DEGREES Vector(0.05234, 0.05234, 0.05234)
#define VECTOR_CONE_7DEGREES Vector(0.06105, 0.06105, 0.06105)
#define VECTOR_CONE_8DEGREES Vector(0.06976, 0.06976, 0.06976)
#define VECTOR_CONE_9DEGREES Vector(0.07846, 0.07846, 0.07846)
#define VECTOR_CONE_10DEGREES Vector(0.08716, 0.08716, 0.08716)
#define VECTOR_CONE_15DEGREES Vector(0.13053, 0.13053, 0.13053)
#define VECTOR_CONE_20DEGREES Vector(0.17365, 0.17365, 0.17365)

	bool GetCone(int cone, Vector& out)
	{
#define DegreeCase(degree) \
	case degree: \
		out = VECTOR_CONE_##degree##DEGREES; \
		return true

		switch (cone)
		{
			DegreeCase(1);
			DegreeCase(2);
			DegreeCase(3);
			DegreeCase(4);
			DegreeCase(5);
			DegreeCase(6);
			DegreeCase(7);
			DegreeCase(8);
			DegreeCase(9);
			DegreeCase(10);
			DegreeCase(15);
			DegreeCase(20);
		default:
			return false;
		}
	}

	static void GetRandomXY(float& x, float& y, int commandOffset)
	{
		static RandomStream random;

		float z;
		float shotBiasMin = -1.0f;
		float shotBiasMax = 1.0f;
		float bias = 1.0f;

		// 1.0 gaussian, 0.0 is flat, -1.0 is inverse gaussian
		float shotBias = ((shotBiasMax - shotBiasMin) * bias) + shotBiasMin;
		float flatness = (fabsf(shotBias) * 0.5);

		int seed = spt_rng.GetPredictionRandomSeed(commandOffset) & 255;
		random.SetSeed(seed);

		do
		{
			x = random.RandomFloat(-1, 1) * flatness + random.RandomFloat(-1, 1) * (1 - flatness);
			y = random.RandomFloat(-1, 1) * flatness + random.RandomFloat(-1, 1) * (1 - flatness);
			if (shotBias < 0)
			{
				x = (x >= 0) ? 1.0 - x : -1.0 - x;
				y = (y >= 0) ? 1.0 - y : -1.0 - y;
			}
			z = x * x + y * y;
		} while (z > 1);
	}

	// Iteratively improves the optimal aim angle
	void GetAimAngleIterative(const QAngle& target, QAngle& current, int commandOffset, const Vector& vecSpread)
	{
		// v = aim angle after spread
		// z = target aim angle
		// w = current aim angle(initially z)
		// w += v - z
		// However this is just an approximate solution, the spread applied changes when you change w
		// since the corresponding right/up vectors used in spread calculations change as well.
		// Iteratively updating the w vector converges to the correct result if spread is not too large.
		float x, y;
		GetRandomXY(x, y, commandOffset);

		Vector forward, right, up, vecDir;
		QAngle resultingAngle;

		AngleVectors(current, &forward, &right, &up);
		Vector result = forward + x * vecSpread.x * right + y * vecSpread.y * up; // This is the shot vector

		VectorAngles(result, resultingAngle); // Get the corresponding view angle

		// Then calculate the difference to the target angle wanted
		double diff[2];
		for (int i = 0; i < 2; ++i)
			diff[i] = utils::NormalizeDeg(utils::NormalizeDeg(resultingAngle[i]) - target[i]);

		current[0] -= diff[0];
		current[1] -= diff[1];
	}

	constexpr float PUNCH_DAMPING = 9.0f;
	constexpr float PUNCH_SPRING_CONSTANT = 65.0f;

	QAngle DecayPunchAngle(QAngle m_vecPunchAngle, QAngle m_vecPunchAngleVel, int frames)
	{
		const float frametime = 0.015f;
		for (int i = 0; i < frames; ++i)
		{
			if (m_vecPunchAngle.LengthSqr() > 0.001 || m_vecPunchAngleVel.LengthSqr() > 0.001)
			{
				m_vecPunchAngle += m_vecPunchAngleVel * frametime;
				float damping = 1 - (PUNCH_DAMPING * frametime);

				if (damping < 0)
				{
					damping = 0;
				}
				m_vecPunchAngleVel *= damping;

				// torsional spring
				// UNDONE: Per-axis spring constant?
				float springForceMagnitude = PUNCH_SPRING_CONSTANT * frametime;
				springForceMagnitude = clamp(springForceMagnitude, 0, 2);
				m_vecPunchAngleVel -= m_vecPunchAngle * springForceMagnitude;

				// don't wrap around
				m_vecPunchAngle.Init(clamp(m_vecPunchAngle.x, -89, 89),
				                     clamp(m_vecPunchAngle.y, -179, 179),
				                     clamp(m_vecPunchAngle.z, -89, 89));
			}
			else
			{
				m_vecPunchAngle.Init(0, 0, 0);
			}
		}

		return m_vecPunchAngle;
	}
} // namespace aim