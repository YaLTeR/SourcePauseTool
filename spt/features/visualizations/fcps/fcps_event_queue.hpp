#pragma once

#include "fcps_event.hpp"

#ifdef SPT_FCPS_ENABLED

#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"

#include <deque>

/*
* The queue manages the memory for all FCPS events. Since event objects are relatively large, we
* want to limit how many we have loaded at any time. The solution is to store events in an
* std::deque with a soft size limit.
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

	fcps_event_range GetKeepAliveRange() const
	{
		return keepAliveRange;
	}

	std::pair<bool, fcps_event_range> StrToRange(const char* str) const;
	bool EventRangeValid(fcps_event_range r) const;

	size_t NumEventsOverLimit() const;

	size_t NumEventsInQueue() const
	{
		return queue.size();
	}

	fcps_event_id GetFirstQueueEventId() const
	{
		return startQueueId;
	}

	struct QueueItVal
	{
		bool isKeepAlive;
		fcps_event_id id;
		const FcpsEvent* event;

		operator bool() const
		{
			return !!event;
		}
	};

	// a wrapper to iterate over all events (including keep-alive events)
	class QueueIt
	{
		fcps_event_id curId;

	public:
		QueueIt(fcps_event_id id) : curId(id) {}

		QueueItVal operator*() const
		{
			auto& eq = FcpsEventQueue::Singleton();
			const FcpsEvent* queueEvent = eq.GetEventInQueue(curId);
			const FcpsEvent* keepAliveEvent = !!queueEvent ? nullptr : eq.GetEvent(curId);
			return QueueItVal{
			    .isKeepAlive = !!keepAliveEvent,
			    .id = curId,
			    .event = queueEvent ? queueEvent : keepAliveEvent,
			};
		}

		QueueIt operator++(int)
		{
			auto ret = *this;
			auto& eq = FcpsEventQueue::Singleton();
			fcps_event_id lastKeepAliveId = eq.GetKeepAliveRange().second;
			fcps_event_id firstQueueId = eq.GetFirstQueueEventId();
			if (curId == lastKeepAliveId && lastKeepAliveId < firstQueueId)
				curId = firstQueueId;
			else
				++curId;
			return ret;
		}
	};

	QueueIt begin() const
	{
		if (keepAliveRange.first > 0 && keepAliveRange.second > 0)
			return QueueIt{std::min(keepAliveRange.first, startQueueId)};
		else
			return QueueIt{startQueueId};
	}

	fb::fb_off Serialize(flatbuffers::FlatBufferBuilder& fbb, fcps_event_range r, ser::StatusTracker& stat) const;
	void Deserialize(const fb::fcps::FcpsEventList& fbEvents, ser::StatusTracker& stat);
};

#endif
