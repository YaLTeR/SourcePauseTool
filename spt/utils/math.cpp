#include "stdafx.h"

#include "math.hpp"
#include <algorithm>

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