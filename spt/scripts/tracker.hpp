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
	};

	class VelocityTracker : public Tracker
	{
	public:
		VelocityTracker(int decimals);
		std::string GenerateTestData() const override;
		ValidationResult Validate(const std::string& expectedValue) const override;
		std::string TrackerName() const override;

	private:
		int decimals;
	};

	class PosTracker : public Tracker
	{
	public:
		PosTracker(int decimals);
		std::string GenerateTestData() const override;
		ValidationResult Validate(const std::string& expectedValue) const override;
		std::string TrackerName() const override;

	private:
		int decimals;
	};

	class AngTracker : public Tracker
	{
	public:
		AngTracker(int decimals);
		std::string GenerateTestData() const override;
		ValidationResult Validate(const std::string& expectedValue) const override;
		std::string TrackerName() const override;

	private:
		int decimals;
	};
} // namespace scripts