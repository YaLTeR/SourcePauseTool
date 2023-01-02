#include "stdafx.hpp"
#include "..\feature.hpp"
#include "signals.hpp"
#include "..\scripts\tester.hpp"

class TestFeature : public FeatureWrapper<TestFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static TestFeature spt_test;

bool TestFeature::ShouldLoadFeature()
{
	return true;
}

void TestFeature::InitHooks() {}

void TestFeature::UnloadFeature() {}

CON_COMMAND(tas_test_generate, "Generates test data for given test.")
{
	if (args.ArgC() > 1)
	{
		scripts::g_Tester.LoadTest(args.Arg(1), true);
	}
}

CON_COMMAND(tas_test_validate, "Validates a test.")
{
	if (args.ArgC() > 1)
	{
		scripts::g_Tester.LoadTest(args.Arg(1), false);
	}
}

CON_COMMAND(tas_test_automated_validate, "Validates a test, produces a log file and exits the game.")
{
	if (args.ArgC() > 2)
	{
		scripts::g_Tester.RunAutomatedTest(args.Arg(1), false, args.Arg(2));
	}
}

void TestFeature::LoadFeature()
{
	if (AfterFramesSignal.Works && AdjustAngles.Works)
	{
		AfterFramesSignal.Connect(&scripts::g_Tester, &scripts::Tester::OnAfterFrames);
		AdjustAngles.Connect(&scripts::g_Tester, &scripts::Tester::DataIteration);

		InitCommand(tas_test_generate);
		InitCommand(tas_test_validate);
		InitCommand(tas_test_automated_validate);
	}
}
