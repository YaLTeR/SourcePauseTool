#pragma once

namespace scripts
{
	class Condition
	{
	public:
		virtual bool IsTrue(int tick, int totalTicks) = 0; // Does the condition currently hold?
		virtual bool ShouldTerminate(int tick, int totalTicks) = 0; // Should the current result be set to fail?
	};

	class TickRangeCondition : public Condition
	{
	public:
		TickRangeCondition(int low, int high, bool reverse);
		bool IsTrue(int tick, int totalTicks);
		bool ShouldTerminate(int tick, int totalTicks);
	private:
		bool reverse;
		int lowTick;
		int highTick;
	};

	enum Axis {AxisX, AxisY, AxisZ, TwoD, Abs};

	class PosSpeedCondition : public Condition
	{
	public:
		PosSpeedCondition(float low, float high, Axis axis, bool IsPos);
		bool IsTrue(int tick, int totalTicks);
		bool ShouldTerminate(int tick, int totalTicks);
	private:
		bool isPos;
		float low;
		float high;
		Axis axis;
	};
}