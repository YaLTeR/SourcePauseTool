#include <stdafx.hpp>

#include "tr_record_cache.hpp"
#include "tr_render_cache.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

void TrPlayerTrace::Clear()
{
	recordingCache.reset();
	renderingCache.reset();
	entityCache.reset();

	playerStandBboxIdx.Invalidate();
	playerDuckBboxIdx.Invalidate();

	std::apply([](auto&... vecs) { ((vecs.clear(), vecs.shrink_to_fit()), ...); }, _storage);
	std::apply([](auto&... holder) { ((holder.firstExportVersion = TR_INVALID_STRUCT_VERSION), ...); }, versions);
	numRecordedTicks = 0;
	hasStartRecordingBeenCalled = false;
}

TrRecordingCache& TrPlayerTrace::GetRecordingCache()
{
	Assert(recordingCache.get());
	return *recordingCache;
}

TrEntityCache& TrPlayerTrace::GetEntityCache() const
{
	if (!entityCache)
		entityCache = std::make_unique<TrEntityCache>();
	return *entityCache;
}

void TrPlayerTrace::KillEntityCache()
{
	entityCache.reset();
}

TrRenderingCache& TrPlayerTrace::GetRenderingCache() const
{
	if (!renderingCache)
		renderingCache = std::make_unique<TrRenderingCache>();
	return *renderingCache;
}

void TrPlayerTrace::KillRenderingCache()
{
	renderingCache.reset();
}

int TrPlayerTrace::GetServerTickAtTick(tr_tick atTick) const
{
	TrReadContextScope scope{*this};
	auto svStateIdx = GetAtTick<TrServerState>(atTick);
	return svStateIdx.IsValid() ? svStateIdx->GetServerTickFromThisAsLastState(atTick) : -1;
}

TrIdx<TrMap> TrPlayerTrace::GetMapAtTick(tr_tick atTick) const
{
	TrReadContextScope scope{*this};
	auto& transitions = Get<TrMapTransition>();
	return transitions.empty() || transitions[0].tick > atTick ? TrIdx<TrMap>{0}
	                                                           : GetAtTick<TrMapTransition>(atTick)->toMapIdx;
}

Vector TrPlayerTrace::GetAdjacentLandmarkDelta(std::span<const TrLandmark> fromLandmarkSp,
                                               std::span<const TrLandmark> toLandmarkSp) const
{
	TrReadContextScope scope{*this};
	for (auto& fromLandmark : fromLandmarkSp)
		for (auto& toLandmark : toLandmarkSp)
			if (fromLandmark.nameIdx == toLandmark.nameIdx)
				return **fromLandmark.posIdx - **toLandmark.posIdx;
	return vec3_origin;
}

#endif
