#include "stdafx.h"
#include "tracker.hpp"

namespace scripts
{
	void Tracker::SetTickRange(int start, int end)
	{
		if (start > end)
			throw std::exception("Start tick was before end");
		else if (start < 0 || end < 0)
			throw std::exception("Tick cannot be negative.");

		startTick = start;
		endTick = end;
	}

	int Tracker::GetStartTick()
	{
		return startTick;
	}

	int Tracker::GetEndTick()
	{
		return endTick;
	}

}

