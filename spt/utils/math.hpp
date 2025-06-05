#pragma once

#ifndef OE
#include "mathlib\vector.h"
#include "mathlib\mathlib.h"
#include "mathlib\vplane.h"
#else
#include "vector.h"
#include "mathlib.h"
#include "vplane.h"
#endif

inline const matrix3x4_t matrix3x4_identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

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
	float ScaleFOVByWidthRatio(float fovDegrees, float ratio);
	void VectorTransform(const matrix3x4_t& mat, Vector& v); // applies mat directly to v
}; // namespace utils