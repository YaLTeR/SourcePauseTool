#pragma once

#ifndef OE
#include "mathlib\vector.h"
#include "mathlib\mathlib.h"
#else
#include "vector.h"
#endif

namespace utils
{
#ifndef M_PI
	const double M_PI = 3.14159265358979323846;
#endif
	const double M_RAD2DEG = 180 / M_PI;
	const double M_DEG2RAD = M_PI / 180;

	double NormalizeRad(double a);
	double NormalizeDeg(double a);
	float RandomFloat(float min, float max);
	void NormalizeQAngle(QAngle& angle);
	void GetMiddlePoint(const QAngle& angle1, const QAngle& angle2, QAngle& out);
#ifndef OE
	void VectorTransform(const matrix3x4_t& mat, Vector& v); // applies mat directly to v
#endif
}; // namespace utils