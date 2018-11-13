#pragma once
#include <string>
#include <vector>
#include "tracker.hpp"
#include "test_item.hpp"

// Test contains the data describing a test
// Contains the relevant .srctas script related to it(string)
// Store trackers as a vector of unique_ptrs to the abstract base class
// Hook AfterFrames into the afterframes queue in ClientDLL
// Is a singleton, helps with the ClientDLL stuff
// There's not that much of a reason to include simultaneously running tests, so I think this is a fine approach

// Test description
// Tracker key - double tick range - tracker descriptor(use space separator, tick range in format start|end)

// Handling different trackers:
// Use a map that maps strings into functions that return Tracker pointers(same as with Conditions)

// Tracker:
//	- Doesn't understand the passage of time, all time control is handled by the Test class
//	- Test: I need your test output for this tick. Tracker: ok here's my data for this tick.
//	- Test: I have this data for this correct. Is it correct? Tracker: Yes/No, and here's why
//	- Contains its active tick range with access using the public functions GetStartTick() and GetEndTick()

// TestItem
//	- A simple tracker number - value pairing
// A vector of these can be loaded from a file/written to a file using a utility function provided in the same header
// The test data can be edited post generation


namespace scripts
{
	extern Tester g_Tester;

	// Singleton
	class Tester
	{
	public:
		Tester();
		void LoadTest(const std::string& testName);
		void AfterFrames();
		void TestIteration();
		void GenerationIteration();
		void TestDone();
		void GenerateTestData();
		void ValidationTest();

		void RunAllTests(const std::string& folder);
		void FetchAllExistingTests(const std::string& folder);
		void ResetIteration();
		void Reset();
	private:
		std::vector<std::unique_ptr<Tracker>> trackers;
		std::vector<TestItem> testItems;
		std::vector<std::string> testNames;
		std::vector<int> failedTests;

		int currentTestIndex;
		int successfulTests;
		bool runningTest;
		bool generatingData;

		int currentTestItem;
		int currentTick;
		bool successfulTest;
		int lastTick;
	};
}