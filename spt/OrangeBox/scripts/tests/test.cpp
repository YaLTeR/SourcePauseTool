#include "stdafx.h"
#include "test.hpp"
#include "dbg.h"
#include "..\..\..\utils\file.hpp"
#include "..\..\spt-serverplugin.hpp"
#include "..\srctas_reader.hpp"
#include <filesystem>

namespace scripts
{
	Tester g_Tester;

	Tester::Tester()
	{
		ResetIteration();
		Reset();
		trackers[VELOCITY_NO] = std::unique_ptr<Tracker>(new VelocityTracker(9));
		trackers[POS_NO] = std::unique_ptr<Tracker>(new PosTracker(9));
		trackers[ANG_NO] = std::unique_ptr<Tracker>(new AngTracker(9));
	}

	void Tester::LoadTest(const std::string& testName, bool generating)
	{
		std::string folder(GetFolder(testName));
		if (std::experimental::filesystem::is_directory(folder))
		{
			RunAllTests(folder, generating);
			return;
		}

		ResetIteration();
		try
		{
			currentTest = testName;
			if (generating)
			{
				generatingData = true;
			}
			else
			{
				runningTest = true;
				testItems = GetTestData(TestDataFile(testName));
			}
			
			g_TASReader.ExecuteScript(testName);
			lastTick = g_TASReader.GetCurrentScriptLength() - 1;
		}
		catch(const std::exception& ex)
		{
			successfulTest = false;
			Msg("[TEST] Error loading test: %s\n", ex.what());
		}
	}

	bool Tester::RequiredFilesExist(const std::string& testName, bool generating)
	{
		if (generating)
			return FileExists(ScriptFile(testName));
		else
			return FileExists(ScriptFile(testName)) && FileExists(TestDataFile(testName));
	}

	std::string Tester::TestDataFile(const std::string& testName)
	{
		return GetGameDir() + "\\" + testName + DATA_EXT;
	}

	std::string Tester::ScriptFile(const std::string& testName)
	{
		return GetGameDir() + "\\" + testName + SCRIPT_EXT;
	}

	std::string Tester::GetFolder(const std::string& testName)
	{
		return GetGameDir() + "\\" + testName;
	}

	void Tester::OnAfterFrames()
	{
		try
		{
			if (!runningTest && !generatingData)
				return;

			++afterFramesTick;
			if (afterFramesTick >= lastTick)
				TestDone();
		}
		catch (const std::exception& ex)
		{
			Msg("[TEST] Error in test iteration: %s\n", ex.what());
		}
	}

	void Tester::DataIteration()
	{
		if (!successfulTest)
			return;
		try
		{
			if (runningTest)
				TestIteration();
			else if (generatingData)
				GenerationIteration();
			++dataTick;
		}
		catch (const std::exception& ex)
		{
			Msg("Error in test: %s\n", ex.what());
			successfulTest = false;
		}
	}

	void Tester::TestIteration()
	{
		while (currentTestItem < testItems.size() && testItems[currentTestItem].tick <= dataTick)
		{
			auto& item = testItems[currentTestItem];

			if (item.tick < dataTick)
				continue;

			auto& tracker = trackers[item.trackerNo];
			auto result = tracker->Validate(item.data);

			if (!result.successful)
			{
				std::ostringstream oss;
				oss << "Tracker \"" << tracker->TrackerName() << "\" difference at tick " << item.tick << " : \"" << result.errorMsg << "\"";
				throw std::exception(oss.str().c_str());
			}

			++currentTestItem;
		}
	}

	void Tester::GenerationIteration()
	{
		for (std::size_t i = 0; i < trackers.size(); ++i)
		{
			auto& tracker = trackers[i];
			std::string data = tracker->GenerateTestData();
			auto selfValidation = tracker->Validate(data);

			if (!selfValidation.successful)
			{
				std::ostringstream oss;
				oss << "Tracker " << tracker->TrackerName() << " failed to self-validate on tick " << dataTick << " : \"" << selfValidation.errorMsg << "\"";
				throw std::exception(oss.str().c_str());
			}

			testItems.push_back(TestItem(dataTick, i, tracker->GenerateTestData()));
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
			WriteTestDataToFile(testItems, TestDataFile(currentTest));
		}

		++currentTestIndex;

		if (successfulTest && runningTest)
			Msg("[TEST] Test ran successfully\n");
		else if (runningTest)
			Msg("[TEST] Test was unsuccesful\n");
		else
			Msg("[TEST] Data automatically generated for test\n");

		if (currentTestIndex >= testNames.size())
		{
			if (!testNames.empty())
			{
				if (runningTest)
					Msg("[TEST] %d / %d tests ran successfully\n", successfulTests, testNames.size());
				else
					Msg("[TEST] Data automatically generated for %d tests\n", testNames.size());
			}
				
			ResetIteration();
			Reset();
		}
		else
		{
			LoadTest(testNames[currentTestIndex], generatingData);
		}
	}

	void Tester::RunAllTests(const std::string& folder, bool generating)
	{
		Reset();

		for (auto& entry : std::experimental::filesystem::recursive_directory_iterator(folder))
		{
			auto& path = entry.path();
			auto& str = path.string().substr(GetGameDir().length() + 1);
			int extPos = str.find(SCRIPT_EXT);

			if (extPos != -1)
			{
				str = str.substr(0, extPos);
				
				if (RequiredFilesExist(str, generating))
				{
					testNames.push_back(str);
				}
			}
		}

		if (!testNames.empty())
			LoadTest(testNames[0], generating);
		else
			Msg("No valid tests found in the folder.\n");
	}

	void Tester::ResetIteration()
	{
		testItems.clear();
		currentTestItem = 0;
		afterFramesTick = 0;
		dataTick = 0;
		successfulTest = true;
		lastTick = 0;
		runningTest = false;
		generatingData = false;
	}

	void Tester::Reset()
	{
		testNames.clear();
		failedTests.clear();
		currentTestIndex = 0;
		successfulTests = 0;
	}
}
