#include "stdafx.hpp"
#include "tester.hpp"
#include <filesystem>
#include "..\spt-serverplugin.hpp"
#include "..\sptlib-wrapper.hpp"
#include "file.hpp"
#include "srctas_reader.hpp"
#include "dbg.h"

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

	void Tester::LoadTest(const std::string& testName, bool generating, bool autoTest)
	{
		this->automatedTest = autoTest;
		std::string folder(GetFolder(testName));
		if (std::filesystem::is_directory(folder))
		{
			RunAllTests(folder, generating, autoTest);
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
			lastTick = g_TASReader.GetCurrentScriptLength();
		}
		catch (const std::exception& ex)
		{
			successfulTest = false;
			std::ostringstream oss;
			oss << "Error in loading test: " << ex.what();
			PrintTestMessage(oss.str());
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
			std::ostringstream oss;
			oss << "Error in test iteration: " << ex.what();
			PrintTestMessage(oss.str());
		}
	}

	void Tester::DataIteration()
	{
		if (!successfulTest)
			return;
		try
		{
			if (runningTest)
			{
				TestIteration();
				++dataTick;
			}
			else if (generatingData)
			{
				GenerationIteration();
				++dataTick;
			}
		}
		catch (const std::exception& ex)
		{
			std::ostringstream oss;
			oss << "Error in test : " << ex.what();
			PrintTestMessage(oss.str());
			successfulTest = false;
			++dataTick;
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
				oss << "Tracker \"" << tracker->TrackerName() << "\" difference at tick " << item.tick
				    << " : \"" << result.errorMsg << "\"";
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
				oss << "Tracker " << tracker->TrackerName() << " failed to self-validate on tick "
				    << dataTick << " : \"" << selfValidation.errorMsg << "\"";
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

		if (successfulTest && runningTest)
			PrintTestMessage("Test ran successfully");
		else if (runningTest)
			PrintTestMessage("Test was unsuccessful");
		else
			PrintTestMessage("Data automatically generated for test");

		++currentTestIndex;

		if (currentTestIndex >= testNames.size())
		{
			if (!testNames.empty())
			{
				std::ostringstream oss;

				if (runningTest)
					oss << successfulTests << " / " << testNames.size()
					    << " tests ran successfully";
				else
					oss << "Data automatically generated for " << testNames.size()
					    << " tests ran successfully";

				PrintTestMessage(oss.str());
			}

			ResetIteration();
			Reset();

			if (automatedTest)
			{
				CloseLogFile();
				EngineConCmd("quit");
			}
		}
		else
		{
			LoadTest(testNames[currentTestIndex], generatingData, automatedTest);
		}
	}

	void Tester::RunAutomatedTest(const std::string& folder, bool generating, const std::string& fileName)
	{
		OpenLogFile(fileName);
		LoadTest(folder, generating, true);
	}

	void Tester::RunAllTests(const std::string& folder, bool generating, bool automated)
	{
		this->automatedTest = automated;
		Reset();

		for (auto& entry : std::filesystem::recursive_directory_iterator(folder))
		{
			auto& path = entry.path();
			auto str = path.string().substr(GetGameDir().length() + 1);
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
			LoadTest(testNames[0], generating, automatedTest);
		else
			PrintTestMessage("No valid tests found in the folder.");
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
	void Tester::PrintTestMessage(std::string msg)
	{
		PrintTestMessage(msg.c_str());
	}
	void Tester::PrintTestMessage(const char* msg)
	{
		std::string testName;

		if (currentTestIndex >= 0 && currentTestIndex < testNames.size())
			testName = GetCurrentTestName();
		else
			testName.clear();

		if (automatedTest)
		{
			logFileStream << "[TEST] " << testName << " : " << msg << '\n';
		}
		else
		{
			Msg("[TEST] %s : %s\n", testName.c_str(), msg);
		}
	}
	void Tester::OpenLogFile(const std::string& fileName)
	{
		logFileStream.open(fileName);
	}
	void Tester::CloseLogFile()
	{
		logFileStream.close();
	}
	const std::string& Tester::GetCurrentTestName()
	{
		return testNames[currentTestIndex];
	}
} // namespace scripts
