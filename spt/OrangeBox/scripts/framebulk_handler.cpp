#include "stdafx.h"
#include "framebulk_handler.hpp"
#include "..\..\utils\string_parsing.hpp"

namespace scripts
{
	typedef void(*CommandCallback) (FrameBulkInfo& frameBulkInfo);
	std::vector<CommandCallback> frameBulkHandlers;

	const auto STRAFE = std::pair<int, int>(0, 0);
	const auto STRAFE_TYPE = std::pair<int, int>(0, 1);
	const auto JUMP_TYPE = std::pair<int, int>(0, 2);
	const auto LGAGST = std::pair<int, int>(0, 3);
	const auto AUTOJUMP = std::pair<int, int>(0, 4);
	const auto DUCKSPAM = std::pair<int, int>(0, 5);
	const auto JUMPBUG = std::pair<int, int>(0, 6);
	const auto DUCK_BEFORE_COLLISION = std::pair<int, int>(0, 7);
	const auto DUCK_BEFORE_GROUND = std::pair<int, int>(0, 8);
	const auto USE_SPAM = std::pair<int, int>(0, 9);

	const auto FORWARD = std::pair<int, int>(1, 0);
	const auto LEFT = std::pair<int, int>(1, 1);
	const auto RIGHT = std::pair<int, int>(1, 2);
	const auto BACK = std::pair<int, int>(1, 3);
	const auto UP = std::pair<int, int>(1, 4);
	const auto DOWN = std::pair<int, int>(1, 5);

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

	void Field1(FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo[STRAFE] == "s")
		{
			frameBulkInfo.AddCommand("tas_strafe 1");

			if (!frameBulkInfo.IsInt(JUMP_TYPE) || !frameBulkInfo.IsInt(STRAFE_TYPE))
				throw std::exception("Jump type or strafe type was not a number!");

			frameBulkInfo.AddCommand("tas_strafe_jumptype " + frameBulkInfo[JUMP_TYPE]);
			frameBulkInfo.AddCommand("tas_strafe_type " + frameBulkInfo[STRAFE_TYPE]);
		}
		else
			frameBulkInfo.AddCommand("tas_strafe 0");

		if (frameBulkInfo[AUTOJUMP] == "j")
			frameBulkInfo.AddCommand("y_spt_autojump 1; +jump");
		else
			frameBulkInfo.AddCommand("y_spt_autojump 0; -jump");

		frameBulkInfo.AddPlusMinusCmd("y_spt_duckspam", frameBulkInfo[DUCKSPAM] == "d");

		// todo
#pragma warning(push)
#pragma warning(disable:4390)
		if (frameBulkInfo[USE_SPAM] == "u");
		if (frameBulkInfo[LGAGST] == "l");
		if (frameBulkInfo[JUMPBUG] == "b");
		if (frameBulkInfo[DUCK_BEFORE_COLLISION] == "c");
		if (frameBulkInfo[DUCK_BEFORE_GROUND] == "g");
#pragma warning(pop)
	}

	void Field2(FrameBulkInfo& frameBulkInfo)
	{
		frameBulkInfo.AddPlusMinusCmd("forward", frameBulkInfo[FORWARD] == "f");
		frameBulkInfo.AddPlusMinusCmd("moveleft", frameBulkInfo[LEFT] == "l");
		frameBulkInfo.AddPlusMinusCmd("moveright", frameBulkInfo[RIGHT] == "r");
		frameBulkInfo.AddPlusMinusCmd("back", frameBulkInfo[BACK] == "b");
		frameBulkInfo.AddPlusMinusCmd("moveup", frameBulkInfo[UP] == "u");
		frameBulkInfo.AddPlusMinusCmd("movedown", frameBulkInfo[DOWN] == "d");
	}

	void Field3(FrameBulkInfo& frameBulkInfo)
	{
		frameBulkInfo.AddPlusMinusCmd("jump", frameBulkInfo[JUMP] == "j");
		frameBulkInfo.AddPlusMinusCmd("duck", frameBulkInfo[DUCK] == "d");
		frameBulkInfo.AddPlusMinusCmd("use", frameBulkInfo[USE] == "u");
		frameBulkInfo.AddPlusMinusCmd("attack", frameBulkInfo[ATTACK1] == "1");
		frameBulkInfo.AddPlusMinusCmd("attack2", frameBulkInfo[ATTACK2] == "2");
		frameBulkInfo.AddPlusMinusCmd("reload", frameBulkInfo[RELOAD] == "r");
	}

	void Field4_5(FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkInfo.IsFloat(YAW_KEY))
		{
			if (frameBulkInfo[STRAFE] == "s")
				frameBulkInfo.AddCommand("tas_strafe_yaw " + frameBulkInfo[YAW_KEY]);
			else
				frameBulkInfo.AddCommand("_y_spt_setyaw " + frameBulkInfo[YAW_KEY]);
		}

		if (frameBulkInfo.IsFloat(PITCH_KEY))
			frameBulkInfo.AddCommand("_y_spt_setpitch " + frameBulkInfo[PITCH_KEY]);
	}

	void Field6(FrameBulkInfo& frameBulkInfo)
	{
		if (!frameBulkInfo.IsInt(TICKS))
			throw std::exception("Tick value was not a number!");

		int ticks = std::atoi(frameBulkInfo[TICKS].c_str());
		frameBulkInfo.data.ticks = ticks;
	}

	void Field7(FrameBulkInfo& frameBulkInfo)
	{
		frameBulkInfo.AddCommand(frameBulkInfo[COMMANDS]);
	}

	void InitHandlers()
	{
		frameBulkHandlers.push_back(Field1);
		frameBulkHandlers.push_back(Field2);
		frameBulkHandlers.push_back(Field3);
		frameBulkHandlers.push_back(Field4_5);
		frameBulkHandlers.push_back(Field6);
		frameBulkHandlers.push_back(Field7);
	}

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkHandlers.size() == 0)
			InitHandlers();

		for (auto handler : frameBulkHandlers)
			handler(frameBulkInfo);

		return frameBulkInfo.data;
	}

	void FrameBulkOutput::AddCommand(std::string newCmd)
	{
		command += ";" + newCmd;
	}

	FrameBulkInfo::FrameBulkInfo(std::istringstream& stream)
	{
		const char DELIMITER = '|';
		int section = 0;
		std::string line;

		do
		{
			std::getline(stream, line, DELIMITER);

			// The sections after the first three are single string(could map section index to value type here but probably not worth the effort)
			if (section > 2)
			{
				dataMap[std::make_pair(section, 0)] = line;
			}
			else
			{
				for (size_t i = 0; i < line.size(); ++i)
					dataMap[std::make_pair(section, i)] = line[i];
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

	void FrameBulkInfo::AddPlusMinusCmd(std::string command, bool set)
	{
		if (set)
			data.AddCommand("+" + command);
		else
			data.AddCommand("-" + command);
	}

}

