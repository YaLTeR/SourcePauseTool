#pragma once

#include "tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

namespace player_trace
{
	class TrEntityCache
	{
	public:
		using EntMap = std::unordered_map<TrIdx<TrEnt>, TrIdx<TrEntTransform>>;

	private:
		EntMap entMap;
		TrIdx<TrEntSnapshot> snapshotIdx;
		TrIdx<TrEntSnapshotDelta> snapshotDeltaIdx;
		tr_tick tick = 0;
		bool initialized = false;

		void UpdateEntSnapshot(tr_tick toTick);
		void VerifySnapshotState() const;
		void JumpToEntSnapshot(TrIdx<TrEntSnapshot> snapIdx);
		void ForwardIterateSnapshotDeltas(tr_tick toTick);
		void BackwardIterateSnapshotDeltas(tr_tick toTick);

	public:
		TrEntityCache() = default;
		TrEntityCache(const TrEntityCache&) = delete;

		TrIdx<TrEnt> hoveredEnt; // set by imgui, reset by renderer

		const EntMap& GetEnts(tr_tick atTick)
		{
			UpdateEntSnapshot(atTick);
			return entMap;
		}
	};

} // namespace player_trace

#endif
