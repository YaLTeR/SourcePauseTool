#include "stdafx.h"
#include "tracker.hpp"
#include <map>
#include "..\..\modules\ClientDLL.hpp"
#include "..\..\modules.hpp"
#include "..\..\..\utils\string_parsing.hpp"
#include <iomanip>


namespace scripts
{
	typedef Tracker*(*TrackerFunction)(const std::string&);
	static std::map<std::string, TrackerFunction> trackerMap;

	ValidationResult VectorValidation(const Vector& current, const std::string& expectedValue, int decimals)
	{
		Vector expectedVel;
		ValidationResult result;
		GetTriplet<float>(expectedValue, expectedVel.x, expectedVel.y, expectedVel.z, '|');

		Vector diff = current - expectedVel;
		float maxDiff = std::pow(1, -decimals);
		Vector minV(-maxDiff, -maxDiff, -maxDiff);
		Vector maxV(maxDiff, maxDiff, maxDiff);

		if (diff.WithinAABox(minV, maxV))
		{
			result.successful = true;
		}
		else
		{
			result.successful = false;
			std::ostringstream oss;
			oss << "x delta: " << diff.x << " y delta: " << diff.y << " z delta: " << diff.z;
			result.errorMsg = oss.str();
		}

		return result;
	}

	void InitTrackers()
	{
		trackerMap["velocity"] = GetVelocityTracker;
		trackerMap["position"] = GetPosTracker;
	}

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

	bool Tracker::InRange(int tick)
	{
		return tick >= startTick && tick <= endTick;
	}

	Tracker * GetVelocityTracker(const std::string & args)
	{
		return new VelocityTracker(args);
	}

	Tracker * GetPosTracker(const std::string & args)
	{
		return new PosTracker(args);
	}

	std::vector<std::unique_ptr<Tracker>> LoadTrackersFromFile(const std::string& fileName)
	{
		if (trackerMap.empty())
			InitTrackers();


		std::vector<std::unique_ptr<Tracker>> result;

		std::ifstream is;
		is.open(fileName);

		std::string line;
		std::string tracker;
		std::string tickRange;
		int lowTick;
		int highTick;
		std::string args;

		while (is.good() && is.is_open())
		{
			std::getline(is, line);
			GetStringTriplet(line, tracker, tickRange, args, ' ');
			GetDoublet<int,int>(tickRange, lowTick, highTick, '|');
			Msg("low %d high %d\n", lowTick, highTick);

			if (trackerMap.find(tracker) == trackerMap.end())
				throw std::exception("Unknown tracker specified");

			Tracker* ptr(trackerMap[tracker](args));
			ptr->SetTickRange(lowTick, highTick);
			result.push_back(std::unique_ptr<Tracker>(ptr));
		}

		return result;
	}

	VelocityTracker::VelocityTracker(const std::string & args)
	{
		decimals = ParseValue<int>(args);
	}

	std::string VelocityTracker::GenerateTestData() const
	{
		auto vel = clientDLL.GetPlayerVelocity();
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(decimals) << vel.x << '|' << vel.y << '|' << vel.z;

		return oss.str();
	}

	ValidationResult VelocityTracker::Validate(const std::string & expectedValue) const
	{
		return VectorValidation(clientDLL.GetPlayerVelocity(), expectedValue, decimals);
	}

	std::string VelocityTracker::TrackerName() const
	{
		return "velocity";
	}

	PosTracker::PosTracker(const std::string & args)
	{
		decimals = ParseValue<int>(args);
	}

	std::string PosTracker::GenerateTestData() const
	{
		auto pos = clientDLL.GetPlayerEyePos();
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(decimals) << pos.x << '|' << pos.y << '|' << pos.z;

		return oss.str();
	}

	ValidationResult PosTracker::Validate(const std::string & expectedValue) const
	{
		return VectorValidation(clientDLL.GetPlayerEyePos(), expectedValue, decimals);
	}

	std::string PosTracker::TrackerName() const
	{
		return "position";
	}

}

