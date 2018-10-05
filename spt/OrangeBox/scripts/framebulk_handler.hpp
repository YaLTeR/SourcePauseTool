#pragma once

#include "srctas_reader.hpp"

namespace scripts
{
	extern const std::string EMPTY_BULK;

	struct FrameBulkOutput
	{
		std::string command;
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
		FrameBulkOutput data;
	private:
		std::map<std::pair<int, int>, std::string> dataMap;
	};

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo);
}

