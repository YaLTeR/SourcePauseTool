#include "stdafx.h"
#include "test.hpp"
#include "dbg.h"

namespace scripts
{
	Tester::Tester()
	{
		currentTick = 0;
		runningTest = false;
	}

	void Tester::AfterFrames()
	{
		if (runningTest)
			TestIteration();
		else
			GenerationIteration();

		++currentTick;

		if (currentTick >= lastTick)
			TestDone();
	}

	void Tester::TestIteration()
	{
		while (currentTestItem < testItems.size() && testItems[currentTestItem].Tick <= currentTick)
		{
			auto& item = testItems[currentTestItem];

			if (item.Tick < currentTick)
				continue;

			auto result = trackers[item.TrackerNo]->Validate(item.Data);

			if (!result.successful)
			{
				successfulTest = false;
				Msg("[TEST] Difference at tick %d: %s\n", item.Tick, result.errorMsg);
			}

			++currentTestItem;
		}
	}

	void Tester::GenerationIteration()
	{
		for (int i = 0; i < trackers.size(); ++i)
		{
			auto& tracker = trackers[i];

			if (tracker->InRange(currentTick))
			{
				std::string data = tracker->GenerateTestData();

				if (!tracker->Validate(data).successful)
				{
					std::string error("Tracker " + tracker->TrackerName() + " failed to self-validate on tick " + std::to_string(currentTick));
					throw std::exception(error.c_str());
				}

				testItems.push_back(TestItem(currentTick, i, tracker->GenerateTestData()));
			}
		}
	}

	void Tester::TestDone()
	{
		if (runningTest)
		{
			if (successfulTest)
				++successfulTests;
		}
		else
		{
			WriteTestDataToFile(testItems, testNames[currentTestIndex]);
		}

		++currentTestIndex;

		if (currentTestIndex >= testNames.size())
		{
			if(runningTest)
				Msg("%d / %d tests ran successfully\n", successfulTests, testNames.size());
			else
				Msg("Data automatically generated for %d tests", testNames.size());
		}
		else
		{
			LoadTest(testNames[currentTestIndex]);
		}
	}
	void Tester::ResetIteration()
	{
		trackers.clear();
		testItems.clear();
		testNames.clear();
		failedTests.clear();
		currentTestItem = 0;
		currentTick = 0;
		successfulTest = true;
		lastTick = 0;
	}
	void Tester::Reset()
	{
		ResetIteration();
		currentTestIndex = 0;
		successfulTests = 0;
		runningTest = false;
		generatingData = false;
	}
}

