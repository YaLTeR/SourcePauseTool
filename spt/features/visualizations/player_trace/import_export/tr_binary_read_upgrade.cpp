#include <stdafx.hpp>

#include "tr_binary.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

template<typename T, tr_struct_version STRUCT_VERSION = TR_LUMP_VERSION(T), typename... LUMP_DEPENDENCIES>
struct TrLumpUpgradeHandlerT : TrLumpUpgradeHandler
{
	TrLumpUpgradeHandlerT()
	    : TrLumpUpgradeHandler{TR_LUMP_NAME(T), STRUCT_VERSION, {TR_LUMP_NAME(LUMP_DEPENDENCIES)...}}
	{
	}

	template<typename T>
	static std::span<T> BytesToSpan(std::span<std::byte> sp)
	{
		return {(T*)sp.data(), sp.size() / sizeof(T)};
	}
};

// since I set TR_INVALID_STRUCT_VERSION to be 0, upgrade all structs from 0 to at least 1
template<typename T>
struct TrLumpUpgradeFromV0ToV1 : TrLumpUpgradeHandlerT<T, 0>
{
	static_assert(TR_INVALID_STRUCT_VERSION == 0);

	virtual bool HandleCompat(TrRestore& restore, TrLump& lump, std::vector<std::byte>& dataInOut) const
	{
		// pretend that version 0 never even existed
		lump.structVersion = lump.firstExportVersion = 1;
		return true;
	}
};

static TrLumpUpgradeFromV0ToV1<char> char_0_1_handler;
static TrLumpUpgradeFromV0ToV1<Vector> vec_0_1_handler;
static TrLumpUpgradeFromV0ToV1<TrIdx<Vector>> vec_idx_0_1_handler;
static TrLumpUpgradeFromV0ToV1<QAngle> ang_0_1_handler;
static TrLumpUpgradeFromV0ToV1<TrIdx<TrTransform_v1>> trans_idx_0_1_handler;
static TrLumpUpgradeFromV0ToV1<TrIdx<TrPortal_v1>> portal_idx_0_1_handler;
static TrLumpUpgradeFromV0ToV1<TrIdx<TrPhysicsObject_v1>> phys_obj_idx_0_1_handler;
static TrLumpUpgradeFromV0ToV1<TrEnt_v1> ent_0_1_handler;
static TrLumpUpgradeFromV0ToV1<TrIdx<TrPlayerContactPoint_v1>> contact_pt_idx_0_1_handler;

// upgrade player data to v2
struct : public TrLumpUpgradeHandlerT<TrPlayerData_v1>
{
	virtual bool HandleCompat(TrRestore& restore, TrLump& lump, std::vector<std::byte>& dataInOut) const
	{
		if (dataInOut.size() % sizeof(TrPlayerData_v1) != 0)
			return false;

		auto pd1Sp = BytesToSpan<TrPlayerData_v1>(dataInOut);
		std::vector<TrPlayerData_v2> pd2Vec;
		for (auto& pd1 : pd1Sp)
			pd2Vec.emplace_back(pd1); // do the upgrade
		size_t nBytes = pd2Vec.size() * sizeof(pd2Vec[0]);
		dataInOut.resize(nBytes);
		memcpy(dataInOut.data(), pd2Vec.data(), nBytes);
		lump.structVersion = TR_LUMP_VERSION(TrPlayerData_v2);
		return true;
	}
} static player_data_1_2_handler;

// remove instances of vec3_invalid since that uses FLT_MAX instead of NAN
struct : public TrLumpUpgradeHandlerT<Vector, 1>
{
	virtual bool HandleCompat(TrRestore& restore, TrLump& lump, std::vector<std::byte>& dataInOut) const
	{
		if (dataInOut.size() % sizeof(Vector) != 0)
			return false;
		auto vecSpan = BytesToSpan<Vector>(dataInOut);
		for (Vector& v : vecSpan)
			if (v == vec3_invalid)
				v = Vector{NAN, NAN, NAN};
		lump.structVersion = 2;
		return true;
	}
} static vec_1_2_handler;

// fixup old TrSegmentReason order
struct : public TrLumpUpgradeHandlerT<TrSegmentStart, 1>
{
	virtual bool HandleCompat(TrRestore& restore, TrLump& lump, std::vector<std::byte>& dataInOut) const
	{
		if (dataInOut.size() % sizeof(TrSegmentStart_v1) != 0)
			return false;
		auto segSpan = BytesToSpan<TrSegmentStart_v1>(dataInOut);
		for (TrSegmentStart_v1& s : segSpan)
			if (s.reason == 6) // old value for TR_SR_TRACE_START
				s.reason = TR_SR_TRACE_START;
		lump.structVersion = 2;
		return true;
	}
} segStart_1_2_handler;

#endif
