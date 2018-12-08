#pragma once
#include <string>
#include <vector>

namespace scripts
{
	struct TestItem
	{
		TestItem(int tick, int no, const std::string& data);

		int tick;
		int trackerNo;
		std::string data;
	};

	std::vector<TestItem> GetTestData(const std::string& fileName);
	void WriteTestDataToFile(const std::vector<TestItem>& testData, const std::string& fileName);
}
