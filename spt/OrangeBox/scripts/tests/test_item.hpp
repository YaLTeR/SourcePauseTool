#pragma once
#include <string>
#include <vector>

namespace scripts
{

	struct TestItem
	{
		TestItem(int tick, int no, const std::string& data);

		int Tick;
		int TrackerNo;
		std::string Data;
	};

	std::vector<TestItem> GetTestData(const std::string& fileName);
	void WriteTestDataToFile(const std::vector<TestItem>& testData, const std::string& fileName);
}