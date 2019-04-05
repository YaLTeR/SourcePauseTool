#pragma once

#include "srctas_reader.hpp"

namespace scripts
{
	const int NO_AFTERFRAMES_BULK = -1;

	struct FrameBulkOutput
	{
		std::string initialCommand;
		std::string repeatingCommand;
		int ticks;

		void AddCommand(const std::string& newCmd);
		void AddCommand(char initChar, const std::string& newCmd);
	};

	class FrameBulkInfo
	{
	public:
		FrameBulkInfo(std::istringstream& stream);
		const std::string& operator[](std::pair<int, int> i);
		bool IsInt(std::pair<int, int> i);
		bool IsFloat(std::pair<int, int> i);
		void AddCommand(const std::string& cmd)
		{
			data.AddCommand(cmd);
		}
		void AddPlusMinusCmd(const std::string& command, bool set);
		void ValidateFieldFlags(FrameBulkInfo& frameBulkInfo, const std::string& fields, int index);
		bool ContainsFlag(const std::pair<int, int>& key, const std::string& flag);
		bool IsSectionNoop(int field);
		FrameBulkOutput data;

	private:
		std::map<std::pair<int, int>, std::string> dataMap;
		std::set<int> noopSections;
	};

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo);
} // namespace scripts
