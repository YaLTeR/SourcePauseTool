#include "stdafx.hpp"

#ifdef  BMS

#include "..\feature.hpp"
#include "convar.hpp"
#include "game_detection.hpp"

static void BMSFlashLightFixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue);

ConVar y_spt_bms_flashlight_fix("y_spt_bms_flashlight_fix",
								"0",
								0,
								"Disables all extra movement on flashlight.",
								BMSFlashLightFixCVarCallback);

// Gives the option to disable all extra flashlight movement, including delay, swaying, and bobbing.
class BMSFlashlightFixFeature : public FeatureWrapper<BMSFlashlightFixFeature>
{
public:
	void Toggle(bool enabled);


protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	
	// we make a lot of single value replacements and instruction overrides
	// so let's just just write the bytes ourselves...
private:

	DECL_BYTE_REPLACE(TimerFloatFirst, 4, 0x00, 0x00, 0x80, 0xBF);
	DECL_BYTE_REPLACE(TimerFloatSecond, 4, 0x00, 0x00, 0x80, 0xBF);

	DECL_BYTE_REPLACE(RandInscFirst, 5, 0xB8, 0x00, 0x00, 0x00, 0x00);
	DECL_BYTE_REPLACE(RandInscSecond, 5, 0xB8, 0x00, 0x00, 0x00, 0x00);
	DECL_BYTE_REPLACE(RandInscThird, 5, 0xB8, 0x00, 0x00, 0x00, 0x00);

	DECL_BYTE_REPLACE(Bob, 8, 
		0xF3, 0x0F, 0x5C, 0xC0, // subss xmm0,xmm0
		0x90, 0x90, 0x90, 0x90, // nop
	);
	DECL_BYTE_REPLACE(Alice, 6, // (up down bob)
		0xD8, 0xE0,				// fsub st(0),st(0)
		0x90, 0x90, 0x90, 0x90, // nop
	);
};

static BMSFlashlightFixFeature spt_bms_flashlight_fix;
static void BMSFlashLightFixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue)
{
	spt_bms_flashlight_fix.Toggle(((ConVar*)pConVar)->GetBool());
}

bool BMSFlashlightFixFeature::ShouldLoadFeature()
{
	if (!utils::DoesGameLookLikeBMSRetail() || utils::DoesGameLookLikeBMSLatest())
		return false;

	return true;
}

void BMSFlashlightFixFeature::InitHooks()
{
	// TODO: sigscan this so it can work for the other versions as well...
	GET_MODULE(client);
	uintptr_t clientStart = (uintptr_t)clientBase;

	auto ptrTimerFloatFirst = (clientStart + 0x698a64);
	auto ptrTimerFloatSecond = (clientStart + 0x178ADE + 6);
	uint floatFirst = *(uint*)ptrTimerFloatFirst;
	uint floatSecond = *(uint*)ptrTimerFloatSecond;
	if (floatFirst != 0x3d800000 || floatSecond != 0x3d800000)
		return;

	INIT_BYTE_REPLACE(TimerFloatFirst, ptrTimerFloatFirst);
	INIT_BYTE_REPLACE(TimerFloatSecond, ptrTimerFloatSecond);

	INIT_BYTE_REPLACE(RandInscFirst, clientStart + 0x1795de);
	INIT_BYTE_REPLACE(RandInscSecond, clientStart + 0x179607);
	INIT_BYTE_REPLACE(RandInscThird, clientStart + 0x179630);

	INIT_BYTE_REPLACE(Bob, clientStart + 0x179a16);
	INIT_BYTE_REPLACE(Alice, clientStart + 0x179ec1);

	InitConcommandBase(y_spt_bms_flashlight_fix);
}

void BMSFlashlightFixFeature::LoadFeature() {}

void BMSFlashlightFixFeature::UnloadFeature()
{
	DESTROY_BYTE_REPLACE(TimerFloatFirst);
	DESTROY_BYTE_REPLACE(TimerFloatSecond);

	DESTROY_BYTE_REPLACE(RandInscFirst);
	DESTROY_BYTE_REPLACE(RandInscSecond);
	DESTROY_BYTE_REPLACE(RandInscThird);

	DESTROY_BYTE_REPLACE(Bob);
	DESTROY_BYTE_REPLACE(Alice);
}

void BMSFlashlightFixFeature::Toggle(bool enabled)
{
	if (enabled)
	{
		DO_BYTE_REPLACE(TimerFloatFirst);
		DO_BYTE_REPLACE(TimerFloatSecond);

		DO_BYTE_REPLACE(RandInscFirst);
		DO_BYTE_REPLACE(RandInscSecond);
		DO_BYTE_REPLACE(RandInscThird);

		DO_BYTE_REPLACE(Bob);
		DO_BYTE_REPLACE(Alice);
	}
	else
	{
		UNDO_BYTE_REPLACE(TimerFloatFirst);
		UNDO_BYTE_REPLACE(TimerFloatSecond);

		UNDO_BYTE_REPLACE(RandInscFirst);
		UNDO_BYTE_REPLACE(RandInscSecond);
		UNDO_BYTE_REPLACE(RandInscThird);

		UNDO_BYTE_REPLACE(Bob);
		UNDO_BYTE_REPLACE(Alice);
	}
}


#endif //  BMS
