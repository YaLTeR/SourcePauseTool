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
	}

	void Tester::LoadTest(const std::string & testName, bool generating)
	{
		ResetIteration();
		try
		{
			currentTest = testName;
			std::string trackerFile = TrackerFile(testName);
			trackers = LoadTrackersFromFile(trackerFile);
			
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
			Msg("[TEST] Test length is %d\n", lastTick);
		}
		catch(const std::exception& ex)
		{
			Msg("[TEST] Error loading test: %s\n", ex.what());
		}
		catch (...)
		{
			Msg("[TEST] Uncaught exception while loading test\n");
		}
	}

	bool Tester::RequiredFilesExist(const std::string & testName, bool generating)
	{
		if (generating)
			return FileExists(TrackerFile(testName)) && FileExists(ScriptFile(testName));
		else
			return FileExists(TrackerFile(testName)) && FileExists(ScriptFile(testName)) && FileExists(TestDataFile(testName));
	}

	std::string Tester::TestDataFile(const std::string & testName)
	{
		return GetGameDir() + "\\" + testName + DATA_EXT;
	}

	std::string Tester::TrackerFile(const std::string & testName)
	{
		return GetGameDir() + "\\" + testName + TRACKER_EXT;
	}

	std::string Tester::ScriptFile(const std::string & testName)
	{
		return GetGameDir() + "\\" + testName + SCRIPT_EXT;
	}

	void Tester::OnAfterFrames()
	{
		try
		{
			if (!runningTest && !generatingData)
				return;

			if (runningTest)
				TestIteration();
			else
				GenerationIteration();

			++currentTick;
			if (currentTick >= lastTick)
				TestDone();
		}
		catch (const std::exception& ex)
		{
			Msg("[TEST] Error in test iteration: %s\n", ex.what());
		}
		catch (...)
		{
			Msg("[TEST] Uncaught exception during test iteration\n");
		}
	}

	void Tester::TestIteration()
	{
		while (currentTestItem < testItems.size() && testItems[currentTestItem].Tick <= currentTick)
		{
			auto& item = testItems[currentTestItem];

			if (item.Tick < currentTick)
				continue;

			auto& tracker = trackers[item.TrackerNo];
			auto result = tracker->Validate(item.Data);

			if (!result.successful)
			{
				successfulTest = false;
				Msg("[TEST] Tracker %s difference at tick %d: %s\n", tracker->TrackerName().c_str(), item.Tick, result.errorMsg);
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
				auto selfValidation = tracker->Validate(data);

				if (!selfValidation.successful)
				{
					std::string error("[TEST] Tracker " + tracker->TrackerName() + " failed to self-validate on tick " + std::to_string(currentTick) + " : " + selfValidation.errorMsg + "\n");
					Msg(error.c_str());
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
			WriteTestDataToFile(testItems, TestDataFile(currentTest));
		}

		++currentTestIndex;

		if (currentTestIndex >= testNames.size())
		{
			if(runningTest && successfulTest && testNames.empty())
				Msg("[TEST] Test ran successfully\n");
			else if(runningTest)
				Msg("[TEST] %d / %d tests ran successfully\n", successfulTests, testNames.size());
			else if(testNames.empty())
				Msg("[TEST] Data automatically generated for test\n");
			else
				Msg("[TEST] Data automatically generated for %d tests\n", testNames.size());
			ResetIteration();
			Reset();
		}
		else
		{
			LoadTest(testNames[currentTestIndex], generatingData);
		}
	}

	void Tester::RunAllTests(const std::string & folder, bool generating)
	{
		Reset();

		for (auto& entry : std::experimental::filesystem::recursive_directory_iterator(folder))
		{
			auto& path = entry.path();
			auto& stem = path.relative_path().stem().generic_string();
			if (path.extension() == SCRIPT_EXT)
			{
				if (RequiredFilesExist(stem, generating))
				{
					testNames.push_back(stem);
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
		trackers.clear();
		testItems.clear();
		currentTestItem = 0;
		currentTick = 0;
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

