#include "stdafx.hpp"

#include "tr_record_cache.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

void TrRecordingCache::CollectEntityDelta(std::vector<EntSnapshotEntry>& newSnapshot)
{
	auto& creates = tr->Get<TrEntCreateDelta_v1>();
	auto& deltas = tr->Get<TrEntTransformDelta_v1>();
	auto& deletes = tr->Get<TrEntDeleteDelta_v1>();

	size_t nCreates = 0, nDeltas = 0, nDeletes = 0;

	auto fromEnd = entSnapshot.end();
	auto toEnd = newSnapshot.end();

	for (auto fromIt = entSnapshot.begin(), toIt = newSnapshot.begin(); fromIt != fromEnd || toIt != toEnd;)
	{
		while (fromIt != fromEnd && (toIt == toEnd || fromIt->entIdx < toIt->entIdx))
		{
			deletes.emplace_back(fromIt->entIdx, fromIt->entTransIdx);
			++nDeletes;
			++fromIt;
		}
		while (toIt != toEnd && (fromIt == fromEnd || toIt->entIdx < fromIt->entIdx))
		{
			creates.emplace_back(toIt->entIdx, toIt->entTransIdx);
			toIt->loggedAsCreateInLastDelta = true;
			++nCreates;
			++toIt;
		}
		while (fromIt != fromEnd && toIt != toEnd && fromIt->entIdx == toIt->entIdx)
		{
			if (fromIt->entTransIdx != toIt->entTransIdx)
			{
				deltas.emplace_back(toIt->entIdx, fromIt->entTransIdx, toIt->entTransIdx);
				++nDeltas;
			}
			toIt->loggedAsCreateInLastDelta = false;
			++fromIt;
			++toIt;
		}
	}

	tr->Get<TrEntSnapshotDelta_v1>().emplace_back(tr->numRecordedTicks,
	                                              TrSpan<TrEntCreateDelta_v1>{creates.size() - nCreates, nCreates},
	                                              TrSpan<TrEntTransformDelta_v1>{deltas.size() - nDeltas, nDeltas},
	                                              TrSpan<TrEntDeleteDelta_v1>{deletes.size() - nDeletes, nDeletes});

	++nEntDeltasWithoutSnapshot;
}

void TrRecordingCache::CollectEntitySnapshot()
{
	auto& snaps = tr->Get<TrEntSnapshot_v1>();
	auto& snapDeltas = tr->Get<TrEntSnapshotDelta_v1>();
	Assert(!snapDeltas.empty());
	if (!snaps.empty() && snaps.back().tick >= snapDeltas.back().tick)
		return;

	// only log additional creates that weren't logged in the snapshot delta

	auto& creates = tr->Get<TrEntCreateDelta_v1>();
	for (auto& entry : entSnapshot)
		if (!entry.loggedAsCreateInLastDelta)
			creates.emplace_back(entry.entIdx, entry.entTransIdx);

	snaps.emplace_back(snapDeltas.back().tick,
	                   TrSpan<TrEntCreateDelta_v1>{
	                       creates.size() - entSnapshot.size(),
	                       entSnapshot.size(),
	                   },
	                   snapDeltas.size() - 1);

	nEntDeltasWithoutSnapshot = 0;
}

#endif
