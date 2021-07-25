#pragma once
#pragma once

#include "stdafx.hpp"
#include <string>
#include <vector>
#include "spt/sptlib-wrapper.hpp"

#define MAX_TICK_DELTA 4

struct afterticks_entry_t
{
	afterticks_entry_t(long long int ticksLeft, std::string command)
	    : ticksLeft(ticksLeft), command(std::move(command))
	{
	}
	afterticks_entry_t() {}
	long long int ticksLeft;
	std::string command;
};

class TicksQueue
{
public:
	TicksQueue(){};

	void AddToQueue(long long int ticksLeft, std::string command)
	{
		_queue.push_back(afterticks_entry_t(ticksLeft, command));
	};

	void AddToQueue(afterticks_entry_t command)
	{
		_queue.push_back(command);
	};

	void Update(int tick)
	{
		int diff = tick - _lastUpdateTick;
		_lastUpdateTick = tick;

		if (_lastUpdateTick == 0 || tick == 0 || diff <= 0)
			return;

		if (diff >= MAX_TICK_DELTA)
			diff = 1;

		for (auto cmd = _queue.begin(); cmd != _queue.end();)
		{
			cmd->ticksLeft -= diff;
			if (cmd->ticksLeft <= 0)
			{
				EngineConCmd(cmd->command.c_str());
				_queue.erase(cmd);
			}
			else
				++cmd;
		}
	}

protected:
	long long int _lastUpdateTick;
	std::vector<afterticks_entry_t> _queue;
};