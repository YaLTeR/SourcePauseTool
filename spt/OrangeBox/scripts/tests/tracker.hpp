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
		virtual std::string GenerateTestData() = 0;
		virtual ValidationResult Validate(const std::string& expectedValue) = 0;
		virtual std::string TrackerName() = 0;
		void SetTickRange(int start, int end);
		int GetStartTick();
		int GetEndTick();
		bool InRange(int tick);
	private:
		int startTick;
		int endTick;
	};

	std::vector<std::unique_ptr<Tracker>> LoadTrackersFromFile(const std::string& fileName);
}