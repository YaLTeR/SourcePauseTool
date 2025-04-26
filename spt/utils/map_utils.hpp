#pragma once

#include <vector>
#include <string>

#include "interfaces.hpp"

namespace utils
{
	inline const char* GetLoadedMap()
	{
		return interfaces::engine_tool ? interfaces::engine_tool->GetCurrentMap() : "";
	}

	using SptLandmarkList = std::vector<std::pair<std::string, Vector>>;

	const SptLandmarkList& GetLandmarksInLoadedMap();
	// returns the landmark delta between two adjacent maps; if no shared landmarks are found returns <0,0,0>
	// ASSUMES THE MAPS ARE DIFFERENT
	Vector LandmarkDelta(const SptLandmarkList& from, const SptLandmarkList& to);
} // namespace utils
