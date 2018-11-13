#include "stdafx.h"
#include "test_item.hpp"
#include <fstream>
#include "..\..\..\utils\string_parsing.hpp"

namespace scripts
{
	std::vector<TestItem> GetTestData(const std::string & fileName)
	{
		std::vector<TestItem> data;
		std::ifstream is;
		is.open(fileName);

		if (!is.is_open())
			throw std::exception("Unable to open file for test data");

		std::string line;
		int tick;
		int no;
		std::string value;

		while (is.good())
		{
			std::getline(is, line);
			GetTriplet<int, int, std::string>(line, tick, no, value, ' ');
			data.push_back(TestItem(tick, no, value));
		}

		return data;
	}

	void WriteTestDataToFile(const std::vector<TestItem>& testData, const std::string & fileName)
	{
		std::ofstream os;
		os.open(fileName);

		for (auto& entry : testData)
		{
			os << entry.TrackerNo << " " << entry.Data << '\n';
		}

		os.close();
	}

	TestItem::TestItem(int tick, int no, const std::string & data)
	{
		Tick = tick;
		TrackerNo = no;
		Data = data;
	}

}

