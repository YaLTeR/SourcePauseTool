#include <stdafx.hpp>

#include "tr_entity_cache.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

void TrEntityCache::VerifySnapshotState() const
{
	if (!initialized)
		return;
	Assert(snapshotIdx.IsValid());
	if (snapshotIdx.IsValid() && snapshotDeltaIdx.IsValid())
	{
		Assert(snapshotIdx->snapDeltaIdx <= snapshotDeltaIdx);
		Assert(snapshotIdx->tick <= snapshotDeltaIdx->tick);
	}
}

void TrEntityCache::UpdateEntSnapshot(tr_tick toTick)
{
	if (initialized && toTick == tick)
		return;

	VerifySnapshotState();

	auto& tr = TrReadContextScope::Current();

	TrIdx<TrEntSnapshot> snapIdxLow = tr.GetAtTick<TrEntSnapshot>(toTick);
	auto snapIdxHigh = snapIdxLow + 1;

	if (!snapIdxLow.IsValid())
		return; // no baselines :/

	TrIdx<TrEntSnapshotDelta> snapDeltaIdx = tr.GetAtTick<TrEntSnapshotDelta>(toTick);
	if (!snapDeltaIdx.IsValid())
	{
		// no deltas, rely on snapshots only
		if (snapshotIdx == snapIdxLow && initialized)
			tick = toTick;
		else
			JumpToEntSnapshot(snapIdxLow);
		return;
	}

#define TR_DEBUG_UPDATE_SNAPSHOT 0

	// small optimization
	if (initialized && snapDeltaIdx == snapshotDeltaIdx)
	{
		tick = toTick;
#if TR_DEBUG_UPDATE_SNAPSHOT
		DevMsg("SPT: [%s] no deltas applied\n", __FUNCTION__);
#endif
		return;
	}

	// calculate most efficient approach to the desired delta

	enum SnapshotDeltaApproach
	{
		SDA_JUMP_TO_SNAPSHOT_THEN_INCREMENT,
		SDA_JUMP_TO_SNAPSHOT_THEN_DECREMENT,
		SDA_INCREMENT,
		SDA_DECREMENT,

		SDA_COUNT,
	};

	constexpr uint32_t JUMP_TO_SNAPSHOT_COST = 3;
	constexpr uint32_t SNAPSHOT_DELTA_COST = 1;

	SnapshotDeltaApproach approach = SDA_JUMP_TO_SNAPSHOT_THEN_INCREMENT;
	size_t cost = JUMP_TO_SNAPSHOT_COST + SNAPSHOT_DELTA_COST * (snapDeltaIdx - snapIdxLow->snapDeltaIdx);
	size_t nDeltas = snapDeltaIdx - snapIdxLow->snapDeltaIdx;

	if (snapIdxHigh.IsValid())
	{
		size_t newCost =
		    JUMP_TO_SNAPSHOT_COST + SNAPSHOT_DELTA_COST * (snapIdxHigh->snapDeltaIdx - snapDeltaIdx);
		if (newCost < cost)
		{
			approach = SDA_JUMP_TO_SNAPSHOT_THEN_DECREMENT;
			cost = newCost;
			nDeltas = snapIdxHigh->snapDeltaIdx - snapDeltaIdx;
		}
	}

	if (initialized)
	{
		if (tick <= toTick)
		{
			size_t newCost = SNAPSHOT_DELTA_COST * (snapDeltaIdx - snapshotDeltaIdx);
			if (newCost < cost)
			{
				approach = SDA_INCREMENT;
				cost = newCost;
				nDeltas = snapDeltaIdx - snapshotDeltaIdx;
			}
		}
		else
		{
			size_t newCost = SNAPSHOT_DELTA_COST * (snapshotDeltaIdx - snapDeltaIdx);
			if (newCost < cost)
			{
				approach = SDA_DECREMENT;
				cost = newCost;
				nDeltas = snapshotDeltaIdx - snapDeltaIdx;
			}
		}
	}

#if TR_DEBUG_UPDATE_SNAPSHOT
	static std::array<const char*, SDA_COUNT> approachStrs = {
	    "JUMP THEN INCREMENT",
	    "JUMP THEN DECREMENT",
	    "INCREMENT",
	    "DECREMENT",
	};
	DevMsg("SPT: [%s] using approach %s with %u deltas\n", __FUNCTION__, approachStrs[approach], nDeltas);
#endif

	switch (approach)
	{
	case SDA_JUMP_TO_SNAPSHOT_THEN_INCREMENT:
		JumpToEntSnapshot(snapIdxLow);
		[[fallthrough]];
	case SDA_INCREMENT:
		ForwardIterateSnapshotDeltas(toTick);
		break;
	case SDA_JUMP_TO_SNAPSHOT_THEN_DECREMENT:
		JumpToEntSnapshot(snapIdxHigh);
		[[fallthrough]];
	case SDA_DECREMENT:
		BackwardIterateSnapshotDeltas(toTick);
		break;
	default:
		Assert(0);
	}
}

void TrEntityCache::JumpToEntSnapshot(TrIdx<TrEntSnapshot> snapIdx)
{
	entMap.clear();
	for (auto& create : *snapIdx->createSp)
		entMap[create.entIdx] = {create.transIdx};

	snapshotIdx = snapIdx;
	snapshotDeltaIdx = snapIdx->snapDeltaIdx;
	tick = snapIdx->tick;
	initialized = true;
}

void TrEntityCache::ForwardIterateSnapshotDeltas(tr_tick toTick)
{
	VerifySnapshotState();
	TrIdx<TrEntSnapshot> snapIdxLow = snapshotIdx;

	while ((snapshotDeltaIdx + 1).IsValid() && snapshotDeltaIdx->tick < toTick)
	{
		auto& delta = **++snapshotDeltaIdx;

		for (auto& create : *delta.createSp)
			entMap.emplace(create.entIdx, create.transIdx);

		for (auto& transDelta : *delta.deltaSp)
		{
			auto it = entMap.find(transDelta.entIdx);
			if (it == entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delta non-existing ent");
				continue;
			}
			it->second = transDelta.toTransIdx;
		}

		for (auto& del : *delta.deleteSp)
		{
			auto it = entMap.find(del.entIdx);
			if (it == entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delete non-existing ent");
				continue;
			}
			entMap.erase(it);
		}

		tick = delta.tick;

		if ((snapIdxLow + 1).IsValid() && (snapIdxLow + 1)->tick <= delta.tick)
			snapshotIdx = ++snapIdxLow;
	}
	tick = toTick;
}

void TrEntityCache::BackwardIterateSnapshotDeltas(tr_tick toTick)
{
	VerifySnapshotState();
	TrIdx<TrEntSnapshot> snapIdxLow = snapshotIdx;

	while ((snapshotDeltaIdx - 1).IsValid() && snapshotDeltaIdx->tick > toTick)
	{
		auto& delta = **snapshotDeltaIdx--;

		for (auto& create : *delta.createSp)
		{
			auto it = entMap.find(create.entIdx);
			if (it == entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delete non-existing ent");
				continue;
			}
			entMap.erase(it);
		}

		for (auto& transDelta : *delta.deltaSp)
		{
			auto it = entMap.find(transDelta.entIdx);
			if (it == entMap.cend())
			{
				AssertMsg(0, "SPT: attempting to delta non-existing ent");
				continue;
			}
			it->second = transDelta.fromTransIdx;
		}

		for (auto& del : *delta.deleteSp)
			entMap.emplace(del.entIdx, del.oldTransIdx);

		tick = delta.tick;

		if ((snapIdxLow - 1).IsValid() && snapIdxLow->tick >= delta.tick)
			snapshotIdx = --snapIdxLow;
	}
	tick = toTick;
}

#endif
