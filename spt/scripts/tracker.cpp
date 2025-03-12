#include "stdafx.hpp"
#include "tracker.hpp"
#include "..\sptlib-wrapper.hpp"
#include "string_utils.hpp"
#include "..\features\playerio.hpp"

namespace scripts
{
	bool ValidDiff(const Vector& v, float maxDiff)
	{
		return std::abs(v.x) <= maxDiff && std::abs(v.y) <= maxDiff && std::abs(v.z) <= maxDiff;
	}

	ValidationResult VectorValidation(const Vector& current, const std::string& expectedValue, int decimals)
	{
		Vector expectedVel;
		ValidationResult result;
		GetTriplet<float>(expectedValue, expectedVel.x, expectedVel.y, expectedVel.z, '|');

		Vector diff = current - expectedVel;
		float maxDiff = std::powf(0.1f, decimals);

		if (ValidDiff(diff, maxDiff))
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

	std::string GenerateVectorData(Vector v, int decimals)
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(decimals) << v.x << '|' << v.y << '|' << v.z;

		return oss.str();
	}

	VelocityTracker::VelocityTracker(int decimals) : decimals(decimals) {}

	std::string VelocityTracker::GenerateTestData() const
	{
		return GenerateVectorData(spt_playerio.GetPlayerVelocity(), decimals);
	}

	ValidationResult VelocityTracker::Validate(const std::string& expectedValue) const
	{
		return VectorValidation(spt_playerio.GetPlayerVelocity(), expectedValue, decimals);
	}

	std::string VelocityTracker::TrackerName() const
	{
		return "velocity";
	}

	PosTracker::PosTracker(int decimals)
	{
		this->decimals = decimals;
	}

	std::string PosTracker::GenerateTestData() const
	{
		extern ConVar tas_strafe_version;
		return GenerateVectorData(spt_playerio.GetPlayerEyePos(tas_strafe_version.GetInt() >= 8), decimals);
	}

	ValidationResult PosTracker::Validate(const std::string& expectedValue) const
	{
		extern ConVar tas_strafe_version;
		return VectorValidation(spt_playerio.GetPlayerEyePos(tas_strafe_version.GetInt() >= 8), expectedValue, decimals);
	}

	std::string PosTracker::TrackerName() const
	{
		return "position";
	}

	AngTracker::AngTracker(int decimals) : decimals(decimals) {}

	std::string AngTracker::GenerateTestData() const
	{
		float va[3];
		EngineGetViewAngles(va);

		return GenerateVectorData(Vector(va[0], va[1], va[2]), decimals);
	}

	ValidationResult AngTracker::Validate(const std::string& expectedValue) const
	{
		float va[3];
		EngineGetViewAngles(va);

		return VectorValidation(Vector(va[0], va[1], va[2]), expectedValue, decimals);
	}

	std::string AngTracker::TrackerName() const
	{
		return "angle";
	}
} // namespace scripts
