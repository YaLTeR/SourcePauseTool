#pragma once

namespace scripts
{
	class Condition
	{
	public:
		virtual bool IsTrue(int tick, int totalTicks) const = 0; // Does the condition currently hold?
		virtual bool ShouldTerminate(int tick,
		                             int totalTicks) const = 0; // Should the current result be set to fail?
		virtual ~Condition() {}
	};

	class TickRangeCondition : public Condition
	{
	public:
		TickRangeCondition(int low, int high, bool reverse);
		bool IsTrue(int tick, int totalTicks) const override;
		bool ShouldTerminate(int tick, int totalTicks) const override;

	private:
		bool reverse;
		int lowTick;
		int highTick;
	};

	enum class Axis
	{
		AxisX,
		AxisY,
		AxisZ,
		TwoD,
		Abs
	};

	enum class AngleAxis
	{
		Pitch,
		Yaw
	};

	class PosSpeedCondition : public Condition
	{
	public:
		PosSpeedCondition(float low, float high, Axis axis, bool IsPos);
		bool IsTrue(int tick, int totalTicks) const override;
		bool ShouldTerminate(int tick, int totalTicks) const override;

	private:
		bool isPos;
		float low;
		float high;
		Axis axis;
	};

	class JBCondition : public Condition
	{
	public:
		JBCondition(float z);
		bool IsTrue(int tick, int totalTicks) const override;
		bool ShouldTerminate(int tick, int totalTicks) const override;

	private:
		float height;
	};

	class AliveCondition : public Condition
	{
	public:
		AliveCondition();
		bool IsTrue(int tick, int totalTicks) const override;
		bool ShouldTerminate(int tick, int totalTicks) const override;
	};

	class LoadCondition : public Condition
	{
	public:
		LoadCondition();
		bool IsTrue(int tick, int totalTicks) const override;
		bool ShouldTerminate(int tick, int totalTicks) const override;
	};

	class VelAngleCondition : public Condition
	{
		float low;
		float high;
		AngleAxis axis;

	public:
		VelAngleCondition(float low, float high, AngleAxis axis);
		bool IsTrue(int tick, int totalTicks) const override;
		bool ShouldTerminate(int tick, int totalTicks) const override;
	};

#if SSDK2007
	class PBubbleCondition : public Condition
	{
	public:
		PBubbleCondition(bool inBubble);
		bool IsTrue(int tick, int totalTicks) const override;
		bool ShouldTerminate(int tick, int totalTicks) const override;

	private:
		bool searchForInBubble;
	};
#endif
} // namespace scripts
