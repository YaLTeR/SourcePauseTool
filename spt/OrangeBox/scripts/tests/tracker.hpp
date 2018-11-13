#pragma once
#include <string>
#include <vector>

namespace scripts
{
	struct ValidationResult
	{
		bool successful;
		std::string errorMsg;
	};

	class Tracker
	{
	public:
		virtual std::string GenerateTestData() const = 0;
		virtual ValidationResult Validate(const std::string& expectedValue) const = 0;
		virtual std::string TrackerName() const = 0;
		void SetTickRange(int start, int end);
		int GetStartTick();
		int GetEndTick();
		bool InRange(int tick);
	private:
		int startTick;
		int endTick;
	};

	Tracker* GetVelocityTracker(const std::string& args);
	Tracker* GetPosTracker(const std::string& args);

	class VelocityTracker : public Tracker
	{
	public:
		VelocityTracker(const std::string& args);
		std::string GenerateTestData() const override;
		ValidationResult Validate(const std::string& expectedValue) const override;
		std::string TrackerName() const override;
	private:
		int decimals;
	};

	class PosTracker : public Tracker
	{
	public:
		PosTracker(const std::string& args);
		std::string GenerateTestData() const override;
		ValidationResult Validate(const std::string& expectedValue) const override;
		std::string TrackerName() const override;
	private:
		int decimals;
	};

	std::vector<std::unique_ptr<Tracker>> LoadTrackersFromFile(const std::string& fileName);
}