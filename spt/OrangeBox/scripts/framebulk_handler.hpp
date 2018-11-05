#pragma once

#include "srctas_reader.hpp"

namespace scripts
{
	extern const std::string EMPTY_BULK;
	const int NO_AFTERFRAMES_BULK = -1;

	struct FrameBulkOutput
	{
		std::string initialCommand;
		std::string repeatingCommand;
		int ticks;

		void AddCommand(std::string newCmd);
	};

	class FrameBulkInfo
	{
	public:
		FrameBulkInfo(std::istringstream& stream);
		std::string operator[](std::pair<int, int> i);
		bool IsInt(std::pair<int, int> i);
		bool IsFloat(std::pair<int, int> i);
		void AddCommand(const std::string& cmd) { data.AddCommand(cmd); }
		void AddPlusMinusCmd(std::string command, bool set);
		void ValidateFieldFlags(FrameBulkInfo& frameBulkInfo, const std::string& fields, int index);
		FrameBulkOutput data;
	private:
		std::map<std::pair<int, int>, std::string> dataMap;
	};

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo);
}
