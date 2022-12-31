#include "stdafx.h"
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

	
	// value replacements and single instruction overrides
	// we'll just write the bytes ourselves...
private:
	uintptr_t ptrTimerFloatFirst = 0;
	uintptr_t ptrTimerFloatSecond = 0;
	uint8_t timerFloatRestore[4] = {0x00, 0x00, 0x80, 0x3D}; // 0.0625
	uint8_t timerFloatWrite[4] = {0x00, 0x00, 0x80, 0xBF};   // -1

	uintptr_t ptrRandInscFirst = 0;
	uint8_t randFirstRestore[5] = {};
	uintptr_t ptrRandInscSecond = 0;
	uint8_t randSecondRestore[5] = {};
	uintptr_t ptrRandInscThird = 0;
	uint8_t randThirdRestore[5] = {};
	uint8_t randWrite[5] = {0xB8, 0x00, 0x00, 0x00, 0x00}; // mov eax,0x0

	uintptr_t ptrBobInsc = 0;
	uint8_t bobWrite[8] = 
	{
	    0xF3, 0x0F, 0x5C, 0xC0, // subss xmm0,xmm0
	    0x90, 0x90, 0x90, 0x90, // nop
	};
	uint8_t bobRestore[8] = {};

	uintptr_t ptrUpDownBobInsc = 0;
	uint8_t upDownBobWrite[6] = 
	{
	    0xD8, 0xE0,				// fsub st(0),st(0)
		0x90, 0x90, 0x90, 0x90, // nop
	};
	uint8_t upDownBobRestore[6] = {};
};

static BMSFlashlightFixFeature spt_bms_flashlight_fix;
static void BMSFlashLightFixCVarCallback(IConVar* pConVar, const char* pOldValue, float flOldValue)
{
	spt_bms_flashlight_fix.Toggle(((ConVar*)pConVar)->GetBool());
}

#define DEF(ptrDef, ptr, restore, write) \
	{ \
		ptrDef = ptr; \
		memcpy(restore, (void*)ptrDef, sizeof(write)); \
	}

bool BMSFlashlightFixFeature::ShouldLoadFeature()
{
	if (!utils::DoesGameLookLikeBMS() || utils::DoesGameLookLikeBMSMod() || utils::DoesGameLookLikeBMSLatest())
		return false;

	void* handle;
	void* clientStartArg = 0;
	size_t clientSize = 0;
	if (!MemUtils::GetModuleInfo(L"client.dll", &handle, &clientStartArg, &clientSize))
	{
		//ConWarning("Couldn't retieve client's info...");
		return false;
	}

	// TODO: sigscan this so it can work for the other versions as well...

	uintptr_t clientStart = (uintptr_t)clientStartArg;

	ptrTimerFloatFirst = (clientStart + 0x698a64);
	ptrTimerFloatSecond = (clientStart + 0x178ADE + 6);

	uint floatFirst = *(uint*)ptrTimerFloatFirst;
	uint floatSecond = *(uint*)ptrTimerFloatSecond;
	if (clientSize < 0x698a64 || floatFirst != 0x3d800000 || floatSecond != 0x3d800000)
	{
		return false;
	}

	DEF(ptrRandInscFirst, clientStart + 0x1795de, randFirstRestore, randWrite);
	DEF(ptrRandInscSecond, clientStart + 0x179607, randSecondRestore, randWrite);
	DEF(ptrRandInscThird, clientStart + 0x179630, randThirdRestore, randWrite);

	DEF(ptrBobInsc, clientStart + 0x179a16, bobRestore, bobWrite);
	DEF(ptrUpDownBobInsc, clientStart + 0x179ec1, upDownBobRestore, upDownBobWrite);

	sizeof(bobRestore);
	return true;
}

void BMSFlashlightFixFeature::InitHooks()
{
	InitConcommandBase(y_spt_bms_flashlight_fix);
}

void BMSFlashlightFixFeature::LoadFeature() {}

void BMSFlashlightFixFeature::UnloadFeature()
{
	Toggle(false);
}

#define WRITE(ptrDef, write) \
	{ \
		MemUtils::ReplaceBytes((void*)ptrDef, sizeof(write), write); \
	}
void BMSFlashlightFixFeature::Toggle(bool enabled)
{
	if (enabled)
	{
		WRITE(ptrTimerFloatFirst, timerFloatWrite);
		WRITE(ptrTimerFloatSecond, timerFloatWrite);

		WRITE(ptrRandInscFirst, randWrite);
		WRITE(ptrRandInscSecond, randWrite);
		WRITE(ptrRandInscThird, randWrite);

		WRITE(ptrBobInsc, bobWrite);
		WRITE(ptrUpDownBobInsc, upDownBobWrite);
	}
	else
	{
		WRITE(ptrTimerFloatFirst, timerFloatRestore);
		WRITE(ptrTimerFloatSecond, timerFloatRestore);

		WRITE(ptrRandInscFirst, randFirstRestore);
		WRITE(ptrRandInscSecond, randSecondRestore);
		WRITE(ptrRandInscThird, randThirdRestore);

		WRITE(ptrBobInsc, bobRestore);
		WRITE(ptrUpDownBobInsc, upDownBobRestore);
	}
}
