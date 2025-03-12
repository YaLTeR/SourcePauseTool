#include "stdafx.hpp"

#include "condition.hpp"

#include "spt\utils\portal_utils.hpp"

#include "ent_utils.hpp"
#include "math.hpp"
#include "..\features\property_getter.hpp"
#include "..\features\playerio.hpp"

namespace scripts
{
	TickRangeCondition::TickRangeCondition(int low, int high, bool reverse)
	    : lowTick(low), highTick(high), reverse(reverse)
	{
		if (low >= high)
			throw std::exception("Low tick should be lower than high");
	}

	bool TickRangeCondition::IsTrue(int tick, int totalTicks) const
	{
		int value;

		if (reverse)
			value = totalTicks - tick;
		else
			value = tick;

		return value >= lowTick && value <= highTick;
	}

	bool TickRangeCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		if (reverse)
			return (totalTicks - tick) < lowTick;
		else
			return tick > highTick;
	}

	PosSpeedCondition::PosSpeedCondition(float low, float high, Axis axis, bool isPos)
	    : low(low), high(high), axis(axis), isPos(isPos)
	{
		if (low >= high)
			throw std::exception("Low value should be lower than high");
	}

	bool PosSpeedCondition::IsTrue(int tick, int totalTicks) const
	{
		if (!utils::spt_clientEntList.GetPlayer())
			return false;

		Vector v;

		if (isPos)
			v = spt_playerio.GetPlayerEyePos(true);
		else
			v = spt_playerio.GetPlayerVelocity();

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
			throw std::exception("Unknown type for speed/position condition");
		}

		return val >= low && val <= high;
	}

	bool PosSpeedCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	JBCondition::JBCondition(float z) : height(z) {}

	bool JBCondition::IsTrue(int tick, int totalTicks) const
	{
		return utils::CanJB(height).canJB;
	}

	bool JBCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	AliveCondition::AliveCondition() {}

	bool AliveCondition::IsTrue(int tick, int totalTicks) const
	{
		return !utils::spt_clientEntList.GetPlayer() || spt_propertyGetter.GetProperty<int>(1, "m_iHealth") > 0;
	}

	bool AliveCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return utils::spt_clientEntList.GetPlayer()
		       && spt_propertyGetter.GetProperty<int>(1, "m_iHealth") <= 0;
	}

	LoadCondition::LoadCondition() {}

	bool LoadCondition::IsTrue(int tick, int totalTicks) const
	{
		return !utils::spt_clientEntList.GetPlayer();
	}

	bool LoadCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

	VelAngleCondition::VelAngleCondition(float low, float high, AngleAxis axis) : low(low), high(high), axis(axis)
	{
		if (utils::NormalizeDeg(high - low) < 0)
			throw std::exception("Low should be lower than high");
	}

	bool VelAngleCondition::IsTrue(int tick, int totalTicks) const
	{
		if (!utils::spt_clientEntList.GetPlayer())
			return false;

		Vector v = spt_playerio.GetPlayerVelocity();
		QAngle angles;
		VectorAngles(v, Vector(0, 0, 1), angles);
		float f;

		if (axis == AngleAxis::Pitch)
			f = angles.x;
		else
			f = angles.y;

		return utils::NormalizeDeg(f - low) >= 0 && utils::NormalizeDeg(f - high) <= 0;
	}

	bool VelAngleCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}

#if SSDK2007
	PBubbleCondition::PBubbleCondition(bool inBubble) : searchForInBubble(inBubble) {}

	bool PBubbleCondition::IsTrue(int tick, int totalTicks) const
	{
		return !utils::GetEnvironmentPortal() ^ searchForInBubble;
	}

	bool PBubbleCondition::ShouldTerminate(int tick, int totalTicks) const
	{
		return false;
	}
#endif
} // namespace scripts
