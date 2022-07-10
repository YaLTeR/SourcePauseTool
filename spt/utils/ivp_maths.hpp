#pragma once

#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

#if defined(SSDK2007)
#include "SDK\orangebox\public\vphysics_interface.h"
#elif defined(SSDK2013)
#include "SDK\sdk2013\public\vphysics_interface.h"
#elif defined(BMS)
#include "SDK\bms\public\vphysics_interface.h"
#else
// just compile please :)
#define METERS_PER_INCH (0.0254f)
#endif

/*
* Source uses units in inches, ivp uses units in meters. Source uses xyz, ivp
* uses xzy. And so on... To prevent ambiguity when dealing with both kinds of
* units at the same time, in game code it looks like source structures are used
* with source units and ivp structures are used with ivp units. The names here
* closely resemble what ivp uses. I think using these types when working with
* ivp units is such a good idea to prevent ambiguity that I will personally slap
* you if you don't do it.
* - uncrafted
*/

typedef double IvpDouble;
typedef float IvpFloat;
typedef int IvpInt;
typedef unsigned int IvpUInt;

// IvpPoint and IvpFloatPoint are (for my purposes) identical but they use different types, MACRO TIME!
#define DECLARE_IVP_POINT(name, type) \
	struct name \
	{ \
		type k[3]; \
		type hesse; /* used for miscellaneous stuff */ \
		/* ctors */ \
		name() = default; \
		name(const Vector& in, bool location) : k{in.x, -in.z, in.y} \
		{ \
			/* if not location treat as unit vector direction */ \
			if (location) \
			{ \
				k[0] *= METERS_PER_INCH; \
				k[1] *= METERS_PER_INCH; \
				k[2] *= METERS_PER_INCH; \
			} \
		} \
		type& operator[](int idx) \
		{ \
			return k[idx]; \
		} \
		const type& operator[](int idx) const \
		{ \
			return k[idx]; \
		} \
	}

DECLARE_IVP_POINT(IvpPoint, IvpDouble);
DECLARE_IVP_POINT(IvpFloatPoint, IvpFloat);

// just rotations - columns are forward, left, up (source is forward, right, up)
struct IvpMatrix3
{
	IvpPoint m[3]; // rows
};

// rotation + translation
struct IvpMatrix : public IvpMatrix3
{
	IvpPoint translation; // column
};

// QUATERNIONS!!!!!
struct IvpQuat
{
	double x, y, z, w;

	IvpQuat(const Quaternion& in) : x(in.x), y(-in.z), z(-in.y), w(in.w) {}

	// I don't care about precision loss atm, especially since qangles used floats anyway
	IvpQuat(const QAngle& in) : IvpQuat(QAngleToQuat(in)) {}

private:
	static Quaternion QAngleToQuat(const QAngle& in)
	{
		Quaternion q;
		AngleQuaternion(in, q);
		return q;
	}
};
