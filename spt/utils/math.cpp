#include "stdafx.h"

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
} // namespace utils