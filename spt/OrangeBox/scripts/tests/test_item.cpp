#include "stdafx.h"
#include "test_item.hpp"
#include <fstream>
#include "..\..\..\utils\string_parsing.hpp"
#include "dbg.h"

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
		std::string tickS;
		int tick;
		std::string noS;
		int no;
		std::string value;

		while (is.good() && is.is_open())
		{
			std::getline(is, line);
			if (!line.empty())
			{
				GetStringTriplet(line, tickS, noS, value, ' ');
				tick = ParseValue<int>(tickS);
				no = ParseValue<int>(noS);
				data.push_back(TestItem(tick, no, value));
			}
		}

		return data;
	}

	void WriteTestDataToFile(const std::vector<TestItem>& testData, const std::string & fileName)
	{
		std::ofstream os;
		os.open(fileName);

		for (auto& entry : testData)
		{
			os << entry.Tick << ' ' <<  entry.TrackerNo << ' ' << entry.Data << '\n';
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

