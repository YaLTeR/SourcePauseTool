#pragma once

#include "fcps_event.hpp"

#ifdef SPT_FCPS_ENABLED

#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"

#include <deque>

/*
* Manages the memory for all FCPS events. Since event objects are relatively large, we want to
* limit how many we have loaded at any time. The solution is to store events in an std::deque
* with a soft size limit.
* 
* 1) When events are recorded, the size limit is used and old events will be dropped from the queue.
* 
* 2) When events are imported, the size limit is used first to drop old events. Afterwards, the
*    imported events are added to queue with the size limit ignored.
* 
* There's just one missing edge case. When the user is animating event(s), it's possible for those
* events to get dropped from the queue mid-animation because of new imported/recorded events. The
* solution is to have a separate keep-alive list into which events are moved to when they're
* dropped from the queue. The keep-alive list is cleared when the user animates a different range
* of events.
*/
class FcpsEventQueue
{
	std::deque<FcpsEvent> queue;
	fcps_event_id startQueueId = 1;

	fcps_event_range keepAliveRange = FCPS_INVALID_EVENT_RANGE;
	std::vector<FcpsEvent> keepAliveList;

	void TrimFromFront(size_t nEvents);

	FcpsEventQueue() = default;
	FcpsEventQueue(const FcpsEventQueue&) = delete;

public:
	static FcpsEventQueue& Singleton()
	{
		static FcpsEventQueue q;
		return q;
	}

	FcpsEvent& NewEvent(bool obeySizeContraint);
	const FcpsEvent* GetEvent(fcps_event_id id) const;
	// like GetEvent() but does not check the keep-alive queue
	const FcpsEvent* GetEventInQueue(fcps_event_id id) const;
	void Reset();
	// if false is returned, nothing is changed
	bool SetKeepAliveRange(fcps_event_range r);
	std::pair<bool, fcps_event_range> StrToRange(const char* str) const;
	bool FcpsEventRangeValid(fcps_event_range r) const;

	size_t NumEventsOverLimit() const;

	size_t NumEvents() const
	{
		return queue.size();
	}

	fcps_event_id GetFirstEventId() const
	{
		return startQueueId;
	}

	fb::fb_off Serialize(flatbuffers::FlatBufferBuilder& fbb, fcps_event_range r) const;
	void Deserialize(const fb::fcps::FcpsEventList& fbEvents, ser::StatusTracker& stat);
};

#endif
