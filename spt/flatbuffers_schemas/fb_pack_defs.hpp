#pragma once

// SDK/native includes
#include "spt/utils/math.hpp"
#include "basehandle.h"
#include "trace.h"
#include "spt/features/visualizations/fcps/fcps_event.hpp"

// flatbuffer includes
#include "fb_forward_declare.hpp"
#include "gen/math_generated.h"
#include "gen/ent_generated.h"
#include "gen/fcps_generated.h"

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

	inline fb::ent::CBaseHandle PackCBaseHandle(const CBaseHandle& h)
	{
		return fb::ent::CBaseHandle{(uint32_t)h.ToInt()};
	}

	inline CBaseHandle UnpackCBaseHandle(const fb::ent::CBaseHandle& h)
	{
		return CBaseHandle{h.val()};
	}

	FB_DECLARE_PACK_UNPACK_FN(CBaseHandle, PackCBaseHandle, UnpackCBaseHandle);

	inline fb::ent::CBaseTrace PackCBaseTrace(const CBaseTrace& t)
	{
		return fb::ent::CBaseTrace{
		    PackVector(t.startpos),
		    PackVector(t.endpos),
		    PackVPlane(VPlane{t.plane.normal, t.plane.dist}),
		    t.fraction,
		    t.contents,
		    t.dispFlags,
		    !!t.allsolid,
		    !!t.startsolid,
		};
	}

	inline CBaseTrace UnpackCBaseTrace(const fb::ent::CBaseTrace& t)
	{
		CBaseTrace out;
		out.startpos = UnpackVector(t.start_pos());
		out.endpos = UnpackVector(t.end_pos());
		auto p = UnpackVPlane(t.impact_plane());
		out.plane.normal = p.m_Normal;
		out.plane.dist = p.m_Dist;
		out.plane.type = 255; // note: type is not serialized, but it *probably* shouldn't matter
		SignbitsForPlane(&out.plane);
		out.fraction = t.fraction();
		out.contents = t.contents();
		out.dispFlags = t.displacement_flags();
		out.allsolid = !!t.all_solid();
		out.startsolid = !!t.start_solid();
		return out;
	}

	FB_DECLARE_PACK_UNPACK_FN(CBaseTrace, PackCBaseTrace, UnpackCBaseTrace);

#ifdef SPT_FCPS_ENABLED

	inline fb::fcps::FcpsCompactRay PackFcpsCompactRay(const FcpsCompactRay& r)
	{
		return fb::fcps::FcpsCompactRay{
		    PackVector(r.start),
		    PackVector(r.delta),
		    PackVector(r.extents),
		};
	}

	inline FcpsCompactRay UnpackFcpsCompactRay(const fb::fcps::FcpsCompactRay& r)
	{
		return FcpsCompactRay{
		    UnpackVector(r.start()),
		    UnpackVector(r.delta()),
		    UnpackVector(r.extents()),
		};
	}

	FB_DECLARE_PACK_UNPACK_FN(FcpsCompactRay, PackFcpsCompactRay, UnpackFcpsCompactRay);

	inline fb::fcps::FcpsTraceResult PackFcpsTraceResult(const FcpsTraceResult& tr)
	{
		return fb::fcps::FcpsTraceResult{
		    PackFcpsCompactRay(tr.ray),
		    PackCBaseTrace(tr.trace),
		    tr.hitEnt,
		};
	}

	inline FcpsTraceResult UnpackFcpsTraceResult(const fb::fcps::FcpsTraceResult& tr)
	{
		return FcpsTraceResult{
		    UnpackFcpsCompactRay(tr.ray()),
		    UnpackCBaseTrace(tr.trace()),
		    tr.hit_ent_idx(),
		};
	}

	FB_DECLARE_PACK_UNPACK_FN(FcpsTraceResult, PackFcpsTraceResult, UnpackFcpsTraceResult);

#endif

} // namespace fb_pack
