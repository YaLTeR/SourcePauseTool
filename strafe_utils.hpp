#pragma once
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace Strafe
{
#ifndef M_PI
	const double M_PI = 3.14159265358979323846;
#endif
	const double M_RAD2DEG = 180 / M_PI;
	const double M_DEG2RAD = M_PI / 180;
	const double M_U_RAD = M_PI / 32768;
	const double M_U_DEG = 360.0 / 65536;
	const double M_INVU_RAD = 32768 / M_PI;
	const double M_INVU_DEG = 65536.0 / 360;
	const double M_U_DEG_HALF = 180.0 / 65536;

	const float VEC_HULL_MIN[3] = { -16, -16, -36 };
	const float VEC_HULL_MAX[3] = { 16,  16,  36 };
	const float VEC_DUCK_HULL_MIN[3] = { -16, -16, -18 };
	const float VEC_DUCK_HULL_MAX[3] = { 16,  16,  18 };

	template<typename T, std::size_t size = 3>
	inline void VecCopy(const T& from, T& to)
	{
		for (std::size_t i = 0; i < size; ++i)
			to[i] = from[i];
	}

	template<typename T1, typename T2, std::size_t size = 3>
	inline void VecAdd(const T1& a, const T2& b, T1& c)
	{
		for (std::size_t i = 0; i < size; ++i)
			c[i] = a[i] + b[i];
	}

	template<typename T1, typename T2, std::size_t size = 3>
	inline void VecSubtract(const T1& a, const T2& b, T1& c)
	{
		for (std::size_t i = 0; i < size; ++i)
			c[i] = a[i] - b[i];
	}

	template<typename T, std::size_t size = 3>
	inline void VecScale(const T& from, double scale, T& to)
	{
		for (std::size_t i = 0; i < size; ++i)
			to[i] = from[i] * scale;
	}

	template<typename T, std::size_t size = 3>
	inline bool IsZero(const T& vec)
	{
		for (std::size_t i = 0; i < size; ++i)
			if (vec[i] != 0)
				return false;

		return true;
	}

	template<typename T, std::size_t size = 3>
	inline double Length(const T& vec)
	{
		double squared = 0.0;
		for (std::size_t i = 0; i < size; ++i)
			squared += vec[i] * vec[i];
		return std::sqrt(squared);
	}

	template<typename T, std::size_t size = 3>
	inline void Normalize(const T& vec, T& out)
	{
		auto len = Length<T, size>(vec);
		if (len == 0)
			return;

		for (std::size_t i = 0; i < size; ++i)
			out[i] = vec[i] / len;
	}

	template<typename T1, typename T2, std::size_t size = 3>
	inline double DotProduct(const T1& a, const T2& b)
	{
		double result = 0.0;
		for (std::size_t i = 0; i < size; ++i)
			result += a[i] * b[i];
		return result;
	}

	template<typename T1, typename T2, std::size_t size = 3>
	inline double Distance(const T1& a, const T2& b)
	{
		using res_type = typename std::common_type<T1, T2>::type;
		res_type c[size];
		VecSubtract<T2, T1, size>(b, a, c);
		return Length<res_type, size>(c);
	}

	template<typename T1, typename T2, std::size_t size = 3>
	inline double AngleRad(const T1& a, const T2& b)
	{
		assert((!IsZero<T1, size>(a) && !IsZero<T2, size>(b)));
		assert((Distance<T1, T2, size>(a, b) != 0));

		double ls[] = { Length<T1, size>(a), Length<T2, size>(b) };
		return (DotProduct<T1, T2, size>(a, b) / (ls[0] * ls[1]));
	}

	template <typename T1, typename T2>
	inline void CrossProduct(const T1& a, const T2& b, typename std::common_type<T1, T2>::type& out)
	{
		out[0] = a[1] * b[2] - a[2] * b[1];
		out[1] = a[2] * b[0] - a[0] * b[2];
		out[2] = a[0] * b[1] - a[1] * b[0];
	}

	inline double AngleModRad(double a)
	{
		return M_U_RAD * (static_cast<int>(a * M_INVU_RAD) & 0xffff);
	}

	inline double AngleModDeg(double a)
	{
		return M_U_DEG * (static_cast<int>(a * M_INVU_DEG) & 0xffff);
	}

	// Return angle in [-Pi; Pi).
	inline double NormalizeRad(double a)
	{
		a = std::fmod(a, M_PI * 2);
		if (a >= M_PI)
			a -= 2 * M_PI;
		else if (a < -M_PI)
			a += 2 * M_PI;
		return a;
	}

	inline double NormalizeDeg(double a)
	{
		a = std::fmod(a, 360.0);
		if (a >= 180.0)
			a -= 360.0;
		else if (a < -180.0)
			a += 360.0;
		return a;
	}

	// Convert both arguments to doubles.
	inline double Atan2(double a, double b)
	{
		return std::atan2(a, b);
	}
}