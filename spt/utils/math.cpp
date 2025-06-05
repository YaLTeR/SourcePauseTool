#include "stdafx.hpp"

#include <algorithm>
#include <random>
#include "math.hpp"

static std::mt19937 rng;

namespace utils
{
	// Return angle in [-180; 180).
	double NormalizeDeg(double a)
	{
		a = std::fmod(a, 360.0);
		if (a >= 180.0)
			a -= 360.0;
		else if (a < -180.0)
			a += 360.0;
		return static_cast<float>(a);
	}

	float RandomFloat(float min, float max)
	{
		std::uniform_real_distribution<float> uniformRandom(min, max);

		return uniformRandom(rng);
	}

	void NormalizeQAngle(QAngle& angle)
	{
		angle.x = NormalizeDeg(angle.x);
		angle.y = NormalizeDeg(angle.y);
	}

	void GetMiddlePoint(const QAngle& angle1, const QAngle& angle2, QAngle& out)
	{
		auto delta = angle2 - angle1;
		NormalizeQAngle(delta);
		delta /= 2;
		out = angle1 + delta;
		NormalizeQAngle(out);
	}

	float ScaleFOVByWidthRatio(float fovDegrees, float ratio)
	{
		float halfAngleRadians = fovDegrees * (0.5f * M_PI / 180.0f);
		float t = tan(halfAngleRadians);
		t *= ratio;
		float retDegrees = (180.0f / M_PI) * atan(t);
		return retDegrees * 2.0f;
	}

	// Return angle in [-Pi; Pi).
	double NormalizeRad(double a)
	{
		a = std::fmod(a, M_PI * 2);
		if (a >= M_PI)
			a -= 2 * M_PI;
		else if (a < -M_PI)
			a += 2 * M_PI;
		return a;
	}

	void VectorTransform(const matrix3x4_t& mat, Vector& v)
	{
		Vector tmp = v;
		VectorTransform(tmp, mat, v);
	}

} // namespace utils
