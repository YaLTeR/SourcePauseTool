#pragma once

// SDK/native includes
#include "spt/utils/math.hpp"

// flatbuffer includes
#include "fb_forward_declare.hpp"
#include "gen/math_generated.h"

#include <type_traits>

/*
* Direct conversions between native & flatbuffer structs. This allows using e.g.
* CreateVectorOfNativeStructs. For anything more complicated (and tables etc), it's probably best
* to write Serialize/Deserialize methods in the native class yourself.
*/

namespace fb_pack
{
	template<typename>
	class ClsPackFnDeducerImpl
	{
		static_assert("no Pack function declared for this type");
	};

	template<typename>
	class ClsUnpackFnDeducerImpl
	{
		static_assert("no Unpack function declared for this type");
	};

	// generic Pack/Unpack to work for any type declared here

	template<typename T>
	auto Pack(const T& v)
	{
		return ClsPackFnDeducerImpl<T>{}(v);
	}

	template<typename T>
	auto Unpack(const T& v)
	{
		return ClsUnpackFnDeducerImpl<T>{}(v);
	}

	// meat for generic Pack/Unpack to work - put this guy below every Pack/Unpack implementation
#define FB_DECLARE_PACK_UNPACK_FN(nativeType, PackFn, UnpackFn) \
	template<> \
	class ClsPackFnDeducerImpl<nativeType> \
	{ \
	public: \
		auto operator()(const nativeType& v) \
		{ \
			return PackFn(v); \
		} \
	}; \
	using nativeType##_fb_unpacked_type = std::invoke_result_t<decltype(PackFn), nativeType>; \
	/* check that Unpack(Pack(x)) gives type x back */ \
	static_assert(std::is_same_v<nativeType, \
	                             std::invoke_result_t<decltype(UnpackFn), nativeType##_fb_unpacked_type>>, \
	              "fb_pack Pack/Unpack functions don't return expected types"); \
	template<> \
	class ClsUnpackFnDeducerImpl<nativeType##_fb_unpacked_type> \
	{ \
	public: \
		auto operator()(const nativeType##_fb_unpacked_type& v) \
		{ \
			return UnpackFn(v); \
		} \
	}

	inline fb::math::Vector3 PackVector(const Vector& v)
	{
		return fb::math::Vector3{v.x, v.y, v.z};
	}

	inline Vector UnpackVector(const fb::math::Vector3& v)
	{
		return Vector{v.x(), v.y(), v.z()};
	}

	FB_DECLARE_PACK_UNPACK_FN(Vector, PackVector, UnpackVector);

	inline fb::math::VPlane PackVPlane(const VPlane& p)
	{
		return fb::math::VPlane{PackVector(p.m_Normal), p.m_Dist};
	}

	inline VPlane UnpackVPlane(const fb::math::VPlane& p)
	{
		return VPlane{UnpackVector(p.normal()), p.dist()};
	}

	FB_DECLARE_PACK_UNPACK_FN(VPlane, PackVPlane, UnpackVPlane);

	inline fb::math::QAngle PackQAngle(const QAngle& q)
	{
		return fb::math::QAngle{q.x, q.y, q.z};
	}

	inline QAngle UnpackQAngle(const fb::math::QAngle& q)
	{
		return QAngle{q.x(), q.y(), q.z()};
	}

	FB_DECLARE_PACK_UNPACK_FN(QAngle, PackQAngle, UnpackQAngle);

	inline fb::math::Color32 PackColor32(const color32& c)
	{
		return fb::math::Color32{c.r, c.g, c.b, c.a};
	}

	inline color32 UnpackColor32(const fb::math::Color32& c)
	{
		return color32{c.r(), c.g(), c.b(), c.a()};
	}

	FB_DECLARE_PACK_UNPACK_FN(color32, PackColor32, UnpackColor32);

} // namespace fb_pack
