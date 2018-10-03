#include "stdafx.h"
#include "framebulk_handler.hpp"
#include "..\..\utils\string_parsing.hpp"

namespace scripts
{
	typedef void(*CommandCallback) (FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo);
	std::vector<CommandCallback> frameBulkHandlers;

	const auto STRAFE = std::pair<int, int>(0, 0);
	const auto STRAFE_TYPE = std::pair<int, int>(0, 1);
	const auto JUMP_TYPE = std::pair<int, int>(0, 2);
	const auto AUTOJUMP = std::pair<int, int>(0, 3);
	const auto DUCKSPAM = std::pair<int, int>(0, 4);

	const auto W = std::pair<int, int>(1, 0);
	const auto A = std::pair<int, int>(1, 1);
	const auto S = std::pair<int, int>(1, 2);
	const auto D = std::pair<int, int>(1, 3);

	const auto JUMP = std::pair<int, int>(2, 0);
	const auto DUCK = std::pair<int, int>(2, 1);
	const auto USE = std::pair<int, int>(2, 2);
	const auto ATTACK1 = std::pair<int, int>(2, 3);
	const auto ATTACK2 = std::pair<int, int>(2, 4);
	const auto RELOAD = std::pair<int, int>(2, 5);

	const auto YAW_KEY = std::pair<int, int>(3, 0);
	const auto PITCH_KEY = std::pair<int, int>(4, 0);
	const auto TICKS = std::pair<int, int>(5, 0);
	const auto COMMANDS = std::pair<int, int>(6, 0);
	const std::string EMPTY_BULK = "-----|----|------|-|-|0|pause";

	void Jump(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo[AUTOJUMP] == "j")
			data.AddCommand("y_spt_autojump 1; +jump");
		else if (frameBulkInfo[JUMP] == "j")
			data.AddCommand("y_spt_autojump 0; +jump");
		else
			data.AddCommand("y_spt_autojump 0; -jump");
	}

	void Duck(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo[DUCKSPAM] == "d")
			data.AddCommand("+y_spt_duckspam; -duck");
		else if (frameBulkInfo[DUCK] == "d")
			data.AddCommand("-y_spt_duckspam; +duck");
		else
			data.AddCommand("-y_spt_duckspam; -duck");
	}

	void Strafe(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo[STRAFE] == "s")
		{
			data.AddCommand("tas_strafe 1");

			if (!frameBulkInfo.IsInt(JUMP_TYPE) || !frameBulkInfo.IsInt(STRAFE_TYPE))
				throw std::exception("Jump type or strafe type was not a number!");

			data.AddCommand("tas_strafe_jumptype " + frameBulkInfo[JUMP_TYPE]);
			data.AddCommand("tas_strafe_type " + frameBulkInfo[STRAFE_TYPE]);
		}
		else
			data.AddCommand("tas_strafe 0");
	}

	void Movement(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo[W] == "w")
			data.AddCommand("+forward");
		else
			data.AddCommand("-forward");

		if (frameBulkInfo[A] == "a")
			data.AddCommand("+moveleft");
		else
			data.AddCommand("-moveleft");

		if (frameBulkInfo[S] == "s")
			data.AddCommand("+back");
		else
			data.AddCommand("-back");

		if (frameBulkInfo[D] == "d")
			data.AddCommand("+moveright");
		else
			data.AddCommand("-moveright");
	}

	void Weapons(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo[ATTACK1] == "1")
			data.AddCommand("+attack");
		else
			data.AddCommand("-attack");

		if (frameBulkInfo[ATTACK2] == "2")
			data.AddCommand("+attack2");
		else
			data.AddCommand("-attack2");

		if (frameBulkInfo[RELOAD] == "r")
			data.AddCommand("+reload");
		else
			data.AddCommand("-reload");
	}

	void Angles(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo.IsFloat(YAW_KEY))
		{
			if (frameBulkInfo[STRAFE] == "s")
				data.AddCommand("tas_strafe_yaw " + frameBulkInfo[YAW_KEY]);
			else
				data.AddCommand("_y_spt_setyaw " + frameBulkInfo[YAW_KEY]);
		}

		if (frameBulkInfo.IsFloat(PITCH_KEY))
			data.AddCommand("_y_spt_setpitch " + frameBulkInfo[PITCH_KEY]);
	}

	void Ticks(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		if (!frameBulkInfo.IsInt(TICKS))
			throw std::exception("Tick value was not a number!");

		int ticks = std::atoi(frameBulkInfo[TICKS].c_str());
		data.ticks = ticks;
	}

	void Commands(FrameBulkOutput& data, FrameBulkInfo& frameBulkInfo)
	{
		data.AddCommand(frameBulkInfo[COMMANDS]);
	}

	void InitHandlers()
	{
		frameBulkHandlers.push_back(Jump);
		frameBulkHandlers.push_back(Duck);
		frameBulkHandlers.push_back(Strafe);
		frameBulkHandlers.push_back(Movement);
		frameBulkHandlers.push_back(Weapons);
		frameBulkHandlers.push_back(Angles);
		frameBulkHandlers.push_back(Ticks);
		frameBulkHandlers.push_back(Commands);
	}

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo)
	{
		FrameBulkOutput data;
		if (frameBulkHandlers.size() == 0)
			InitHandlers();

		for (auto handler : frameBulkHandlers)
			handler(data, frameBulkInfo);

		return data;
	}

	void FrameBulkOutput::AddCommand(std::string newCmd)
	{
		command += ";" + newCmd;
	}

	FrameBulkInfo::FrameBulkInfo(std::istringstream& stream)
	{
		const char DELIMITER = '|';
		int section = 0;
		std::string data;

		do
		{
			std::getline(stream, data, DELIMITER);

			// The sections after the first three are single string(could map section index to value type here but probably not worth the effort)
			if (section > 2)
			{
				dataMap[std::make_pair(section, 0)] = data;
			}
			else
			{
				for (int i = 0; i < data.size(); ++i)
					dataMap[std::make_pair(section, i)] = data[i];
			}

			++section;
		} while (stream.good());
	}

	std::string FrameBulkInfo::operator[](std::pair<int, int> i)
	{
		if (dataMap.find(i) == dataMap.end())
		{
			char buffer[50];
			std::sprintf(buffer, "Unable to find index (%i, %i) in frame bulk!", i.first, i.second);

			throw std::exception(buffer);
		}

		return dataMap[i];
	}

	bool FrameBulkInfo::IsInt(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<int>(value);
	}

	bool FrameBulkInfo::IsFloat(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<float>(value);
	}

}

