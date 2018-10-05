#include "stdafx.h"
#include "condition.hpp"
#include "..\modules\ClientDLL.hpp"
#include "..\modules.hpp"

namespace scripts
{
	TickRangeCondition::TickRangeCondition(int low, int high, bool reverse) : lowTick(low), highTick(high), reverse(reverse)
	{
	}

	bool TickRangeCondition::IsTrue(int tick, int totalTicks)
	{
		int value;

		if (reverse)
			value = totalTicks - tick;
		else
			value = tick;

		return value >= lowTick && value <= highTick;
	}

	bool TickRangeCondition::ShouldTerminate(int tick, int totalTicks)
	{
		if (reverse)
			return (totalTicks - tick) < lowTick;
		else
			return tick > highTick;
	}

	PosSpeedCondition::PosSpeedCondition(float low, float high, Axis axis, bool isPos) : low(low), high(high), axis(axis), isPos(isPos)
	{

	}

	bool PosSpeedCondition::IsTrue(int tick, int totalTicks)
	{
		Vector v;

		if (isPos)
			v = clientDLL.GetPlayerEyePos();
		else
			v = clientDLL.GetPlayerVelocity();

		float val;

		switch (axis)
		{
		case Axis::AxisX:
			val = v.x;
			break;
		case Axis::AxisY:
			val = v.y;
			break;
		case Axis::AxisZ:
			val = v.z;
			break;
		case Axis::TwoD:
			val = v.Length2D();
			break;
		case Axis::Abs:
			val = v.Length();
			break;
		default:
			throw std::exception("Unknown type for speed/position condition!");
		}

		return val >= low && val <= high;
	}

	bool PosSpeedCondition::ShouldTerminate(int tick, int totalTicks)
	{
		return false;
	}

}
