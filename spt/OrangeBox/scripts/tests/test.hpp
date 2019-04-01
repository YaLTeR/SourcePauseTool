#pragma once
#include <string>
#include <vector>
#include "tracker.hpp"
#include "test_item.hpp"
#include <fstream>

// Test contains the data describing a test
// Store trackers as a vector of unique_ptrs to the abstract base class
// Hook AfterFrames into the afterframes queue in ClientDLL
// Is a singleton, helps with the ClientDLL stuff
// There's not that much of a reason to include simultaneously running tests, so I think this is a fine approach

// Tracker:
//	- Doesn't understand the passage of time, all time control is handled by the Test class
//	- Test: I need your test output for this tick. Tracker: ok here's my data for this tick.
//	- Test: I have this data for this tick. Is it correct? Tracker: Yes/No, and here's why
//	- Contains its active tick range with access using the public functions GetStartTick() and GetEndTick()

// TestItem
//	- A simple tracker number - value pairing
// A vector of these can be loaded/written from/to a file using a utility function provided in test_item.hpp
// The test data can be edited post generation


namespace scripts
{
	const std::string DATA_EXT = ".td";
	const int VELOCITY_NO = 0;
	const int POS_NO = 1;
	const int ANG_NO = 2;

	// Singleton
	class Tester
	{
	public:
		Tester();
		void LoadTest(const std::string& testName, bool generating, bool automatedTest = false);
		static bool RequiredFilesExist(const std::string& testName, bool generating);
		static std::string TestDataFile(const std::string& testName);
		static std::string ScriptFile(const std::string& testName);
		static std::string GetFolder(const std::string& testName);

		void OnAfterFrames();
		void DataIteration();
		void TestIteration();
		void GenerationIteration();
		void TestDone();

		void RunAutomatedTest(const std::string& folder, bool generating, const std::string& testFileName);
		void RunAllTests(const std::string& folder, bool generating, bool automatedTest = false);
		void ResetIteration();
		void Reset();
	private:
		void PrintTestMessage(std::string msg);
		void PrintTestMessage(const char* msg);
		void OpenLogFile(const std::string& testFileName);
		void CloseLogFile();
		const std::string& GetCurrentTestName();

		std::map<int, std::unique_ptr<Tracker>> trackers;
		std::vector<TestItem> testItems;
		std::vector<std::string> testNames;
		std::vector<int> failedTests;
		std::string currentTest;
		std::ofstream logFileStream;

		std::size_t currentTestIndex;
		int successfulTests;
		bool runningTest;
		bool generatingData;
		bool automatedTest;

		std::string testFileName;
		std::size_t currentTestItem;
		int afterFramesTick;
		int dataTick;
		bool successfulTest;
		int lastTick;
	};

	extern Tester g_Tester;
}