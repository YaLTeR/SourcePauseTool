#include "stdafx.hpp"

#include "fcps_event_queue.hpp"

#ifdef SPT_FCPS_ENABLED

#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"
#include "spt/flatbuffers_schemas/gen/fcps_generated.h"

#include <inttypes.h>
#include <format>

const FcpsEvent* FcpsEventQueue::GetEventInQueue(fcps_event_id id) const
{
	if (id < startQueueId || id >= startQueueId + queue.size())
		return nullptr;
	return &queue[id - startQueueId];
}

void FcpsEventQueue::TrimFromFront(size_t nEvents)
{
	for (size_t i = 0; i < nEvents; i++)
	{
		if (FcpsIdInRange(startQueueId + i, keepAliveRange))
			keepAliveList.push_back(std::move(queue.front()));
		queue.pop_front();
	}
	startQueueId += nEvents;
}

FcpsEvent& FcpsEventQueue::NewEvent(bool obeySizeContraint)
{
	auto& ret = queue.emplace_back();
	if (obeySizeContraint)
		TrimFromFront(NumEventsOverLimit());
	return ret;
}

const FcpsEvent* FcpsEventQueue::GetEvent(fcps_event_id id) const
{
	auto queueEvent = GetEventInQueue(id);
	if (queueEvent)
		return queueEvent;
	if (FcpsIdInRange(id, keepAliveRange))
		return &keepAliveList[id - keepAliveRange.first];
	return nullptr;
}

void FcpsEventQueue::Reset()
{
	*this = FcpsEventQueue{};
}

bool FcpsEventQueue::SetKeepAliveRange(fcps_event_range r)
{
	if (r.first <= 0 || r.second <= 0)
	{
		// no keep-alive events
		keepAliveList.clear();
		keepAliveRange = FCPS_INVALID_EVENT_RANGE;
		return true;
	}

	if (!EventRangeValid(r))
		return false;

	if (r.first >= GetFirstQueueEventId())
	{
		// new range is strictly inside the queue
		keepAliveList.clear();
	}
	else
	{
		// new range overlaps keep-alive range
		fcps_event_id curLastKeepAliveId = keepAliveRange.first + keepAliveList.size() - 1;
		size_t nEventsToRemoveFromFront = (size_t)(r.first - keepAliveRange.first);
		size_t nEventsToRemoveFromBack =
		    r.second >= curLastKeepAliveId ? 0 : (size_t)(curLastKeepAliveId - r.second);
		keepAliveList.erase(keepAliveList.begin(), keepAliveList.begin() + nEventsToRemoveFromFront);
		keepAliveList.erase(keepAliveList.end() - nEventsToRemoveFromBack, keepAliveList.end());
	}

	keepAliveRange = r;
	return true;
}

std::pair<bool, fcps_event_range> FcpsEventQueue::StrToRange(const char* str) const
{
	int64_t low, high;
	bool scanned = false;
	if (sscanf_s(str, " %" SCNd64 " ^ %" SCNd64 " ", &low, &high) == 2)
	{
		scanned = true;
	}
	else if (sscanf_s(str, " %" SCNd64 " ", &low) == 1)
	{
		high = low;
		scanned = true;
	}

	if (scanned)
	{
		fcps_event_id lowId, highId;
		lowId = low >= 0 ? low : startQueueId + NumEventsInQueue() + low;
		highId = high >= 0 ? high : startQueueId + NumEventsInQueue() + high;
		fcps_event_range r{lowId, highId};
		if (EventRangeValid(r))
			return {true, r};
	}

	return {false, FCPS_INVALID_EVENT_RANGE};
}

bool FcpsEventQueue::EventRangeValid(fcps_event_range r) const
{
	if (r.first > r.second || r.first <= 0 || r.second <= 0)
		return false;

	if (!GetEventInQueue(r.first))
	{
		if (!FcpsIdInRange(r.first, keepAliveRange))
			return false;
		if (FcpsIdInRange(r.second, keepAliveRange))
			return true;
		r.first = std::min(r.second, keepAliveRange.second + 1);
	}

	return GetEventInQueue(r.first) && GetEventInQueue(r.second);
}

size_t FcpsEventQueue::NumEventsOverLimit() const
{
	size_t softQueueSizeLimit = MAX(1, (size_t)spt_fcps_max_queue_size.GetInt());
	size_t queueSize = queue.size();
	return softQueueSizeLimit > queueSize ? 0 : queueSize - softQueueSizeLimit;
}

fb::fb_off FcpsEventQueue::Serialize(flatbuffers::FlatBufferBuilder& fbb,
                                     fcps_event_range r,
                                     ser::StatusTracker& stat) const
{
	if (!EventRangeValid(r))
	{
		stat.Err("attempted to serialize invalid FCPS event range");
		return 0;
	}

	std::vector<flatbuffers::Offset<fb::fcps::FcpsEvent>> fbEventOffs;
	for (auto id = r.first; id <= r.second; id++)
		fbEventOffs.push_back(GetEvent(id)->Serialize(fbb));

	return fb::fcps::CreateFcpsEventList(fbb, fbb.CreateVector(fbEventOffs)).o;
}

void FcpsEventQueue::Deserialize(const fb::fcps::FcpsEventList& fbEvents, ser::StatusTracker& stat)
{
	if (!stat.Ok())
		return;

	float maxImportEvents;
	if (!spt_fcps_max_queue_size.GetMax(maxImportEvents))
		maxImportEvents = 1000;

	size_t nNewEvents = fbEvents.events() ? fbEvents.events()->size() : 0;

	if (nNewEvents == 0 || nNewEvents >= (size_t)maxImportEvents)
	{
		stat.Err(
		    std::format("expected between 1 and {} FCPS events, got {}", (size_t)maxImportEvents, nNewEvents));
		return;
	}

	size_t oldSize = queue.size();
	size_t nEventsToEraseOnSuccess = NumEventsOverLimit();

	for (auto fbEventOff : *fbEvents.events())
	{
		auto& newEvent = NewEvent(false);
		newEvent.Deserialize(*fbEventOff, stat);
		if (!stat.Ok())
			break;
	}

	if (!stat.Ok())
		queue.resize(oldSize);
	else
		TrimFromFront(nEventsToEraseOnSuccess);
}

#endif
