#include <stdafx.hpp>

#include "tr_binary_internal.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

template<typename T, tr_struct_version STRUCT_VERSION = TR_LUMP_VERSION(T), typename... LUMP_DEPENDENCIES>
struct TrLumpUpgradeHandlerT : TrLumpUpgradeHandler
{
	TrLumpUpgradeHandlerT()
	    : TrLumpUpgradeHandler{TR_LUMP_NAME(T), STRUCT_VERSION, {TR_LUMP_NAME(LUMP_DEPENDENCIES)...}}
	{
	}
};

// upgrade player data to v2
struct : public TrLumpUpgradeHandlerT<TrPlayerData_v1>
{
	virtual bool HandleCompat(TrRestoreInternal& internal, TrLump& lump, std::vector<std::byte>& dataInOut) const
	{
		if (dataInOut.size() % sizeof(TrPlayerData_v1) != 0)
			return false;

		auto pd1Sp = std::span{(TrPlayerData_v1*)dataInOut.data(), dataInOut.size() / sizeof(TrPlayerData_v1)};
		std::vector<TrPlayerData_v2> pd2Vec;

		for (auto& pd1 : pd1Sp)
		{
			TrPlayerData_v2 pd2{
			    .tick = pd1.tick,
			    .qPosIdx = pd1.qPosIdx,
			    .qVelIdx = pd1.qVelIdx,
			    ._unused{},
			    .vVelIdx{},
			    .transEyesIdx = pd1.transEyesIdx,
			    .transSgEyesIdx = pd1.transSgEyesIdx,
			    .transVPhysIdx = pd1.transVPhysIdx,
			    .contactPtsSp = pd1.contactPtsSp,
			    .m_fFlags = pd1.m_fFlags,
			    .fov = pd1.fov,
			    .m_iHealth = pd1.m_iHealth,
			    .m_lifeState = pd1.m_lifeState,
			    .m_CollisionGroup = pd1.m_CollisionGroup,
			    .m_MoveType = pd1.m_MoveType,
			};
			pd2Vec.push_back(pd2);
		}
		size_t nBytes = pd2Vec.size() * sizeof(pd2Vec[0]);
		dataInOut.resize(nBytes);
		memcpy(dataInOut.data(), pd2Vec.data(), nBytes);
		lump.struct_version = TR_LUMP_VERSION(TrPlayerData_v2);
		return true;
	}
} static player_data_1_2_handler;

// remove instances of vec3_invalid since that uses FLT_MAX instead of NAN
struct : public TrLumpUpgradeHandlerT<Vector, 0>
{
	virtual bool HandleCompat(TrRestoreInternal& internal, TrLump& lump, std::vector<std::byte>& dataInOut) const
	{
		if (dataInOut.size() % sizeof(Vector) != 0)
			return false;
		auto vecSpan = std::span{(Vector*)dataInOut.data(), dataInOut.size() / sizeof(Vector)};
		for (Vector& v : vecSpan)
			if (v == vec3_invalid)
				v = Vector{NAN, NAN, NAN};
		lump.struct_version = 1;
		return true;
	}
} static vec_0_1_handler;

#endif
