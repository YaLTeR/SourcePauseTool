#include "stdafx.hpp"
#ifdef OE
#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\utils\game_detection.hpp"
#include "afterticks.hpp"
#include "convar.hpp"
#include "dbg.h"

typedef void(__fastcall* _)(void* thisptr, int edx, void* a, void* b);
typedef void(__fastcall* _SlidingAndOtherStuff)(void* thisptr, int edx, void* a, void* b);

ConVar y_spt_on_slide_pause_for("y_spt_on_slide_pause_for",
                                "0",
                                0,
                                "Whenever sliding occurs in DMoMM, pause for this many ticks.");

// DMoMM stuff
class DMoMM : public FeatureWrapper<DMoMM>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void PreHook() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void* ORIG_MiddleOfSlidingFunction = nullptr;
	_SlidingAndOtherStuff ORIG_SlidingAndOtherStuff = nullptr;
	bool sliding = false;
	bool wasSliding = false;
	static void __fastcall HOOKED_SlidingAndOtherStuff(void* thisptr, int edx, void* a, void* b);
	static void HOOKED_MiddleOfSlidingFunction();
	void HOOKED_MiddleOfSlidingFunction_Func();
};

static DMoMM spt_dmomm;

bool DMoMM::ShouldLoadFeature()
{
	return utils::DoesGameLookLikeDMoMM();
}

namespace patterns
{
	PATTERNS(
	    MiddleOfSlidingFunction,
	    "dmomm",
	    "8B 16 8B CE FF 92 ?? ?? ?? ?? 8B 08 89 4C 24 1C 8B 50 04 89 54 24 20 8B 40 08 8D 4C 24 10 51 8D 54 24 20");
}

void DMoMM::InitHooks()
{
	HOOK_FUNCTION(server, MiddleOfSlidingFunction);
}

void DMoMM::PreHook()
{
	// Middle of DMoMM sliding function.
	if (ORIG_MiddleOfSlidingFunction)
	{
		InitConcommandBase(y_spt_on_slide_pause_for);
		ORIG_SlidingAndOtherStuff = (_SlidingAndOtherStuff)(&ORIG_MiddleOfSlidingFunction - 0x4bb);
		ADD_RAW_HOOK(server, SlidingAndOtherStuff);
		DevMsg("[server.dll] Found the sliding function at %p.\n", ORIG_SlidingAndOtherStuff);
	}
	else
	{
		ORIG_SlidingAndOtherStuff = nullptr;
		DevWarning("[server.dll] Could not find the sliding code!\n");
	}
}

void DMoMM::LoadFeature()
{
	sliding = false;
	wasSliding = false;
}

void DMoMM::UnloadFeature() {}

void __fastcall DMoMM::HOOKED_SlidingAndOtherStuff(void* thisptr, int edx, void* a, void* b)
{
	if (spt_dmomm.sliding)
	{
		spt_dmomm.sliding = false;
		spt_dmomm.wasSliding = true;
	}
	else
	{
		spt_dmomm.wasSliding = false;
	}

	return spt_dmomm.ORIG_SlidingAndOtherStuff(thisptr, edx, a, b);
}

void DMoMM::HOOKED_MiddleOfSlidingFunction_Func()
{
	sliding = true;

	if (!wasSliding)
	{
		const auto pauseFor = y_spt_on_slide_pause_for.GetInt();
		if (pauseFor > 0)
		{
			EngineConCmd("setpause");

			afterticks_entry_t entry;
			entry.ticksLeft = pauseFor;
			entry.command = "unpause";
			spt_afterticks.AddAfterticksEntry(entry);
		}
	}
}

__declspec(naked) void DMoMM::HOOKED_MiddleOfSlidingFunction()
{
	/**
		 * This is a hook in the middle of a function.
		 * Save all registers and EFLAGS to restore right before jumping out.
		 * Don't use local variables as they will corrupt the stack.
		 */
	__asm {
			pushad;
			pushfd;
	}

	spt_dmomm.HOOKED_MiddleOfSlidingFunction_Func();

	__asm {
			popfd;
			popad;
		/**
			 * It's important that nothing modifies the registers, the EFLAGS or the stack past this point,
			 * or the game won't be able to continue normally.
			 */

			jmp spt_dmomm.ORIG_MiddleOfSlidingFunction;
	}
}
#endif