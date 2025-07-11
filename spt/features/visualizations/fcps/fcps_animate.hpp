#pragma once

#include "fcps_event.hpp"

#ifdef SPT_FCPS_ENABLED

#include <chrono>

/*
* The data for each animation step is a small struct which has the minimal amount of info to figure
* out what needs to be drawn from a given event. The animator builds a list of animation steps and
* calls the corresponding draw function for each step.
*/

// original entity position
struct FcpsAnimHello
{
};

// test ray to check if ent is in a passable space and where to put it if it is
struct FcpsAnimEntRay
{
	byte entRayIdx;
};

// used in detail mode - shows one or two traces between two corners
struct FcpsAnimTestRayPair
{
	byte nudgeIdx;
	byte testRayIdx1, testRayIdx2;
};

// used in coarse mode - shows all test rays for a nudge iteration
struct FcpsAnimTestRayAll
{
	byte nudgeIdx;
};

// final entity position
struct FcpsAnimGoodbye
{
};

#define FCPS_ANIMATION_STEPS(X) \
	X(FcpsAnimHello) \
	X(FcpsAnimEntRay) \
	X(FcpsAnimTestRayPair) \
	X(FcpsAnimTestRayAll) \
	X(FcpsAnimGoodbye)

#define _FCPS_ENUMERATE_ENUM(X) X##StepType,

enum FcpsStepType : byte
{
	FCPS_ANIMATION_STEPS(_FCPS_ENUMERATE_ENUM)

	    FcpsAnimStepTypeCount,
};

#define _FCPS_ENUMERATE_FIELD(X) X X##Data;

// C-style typed union, easier to work with than std::variant in this case
struct FcpsAnimationStep
{
	fcps_event_id id;
	uint32_t stepType : 8;
	uint32_t mappedIdx : 24; // maps between the coarse & detail lists
	union
	{
		FCPS_ANIMATION_STEPS(_FCPS_ENUMERATE_FIELD)
	};
};

/*
* The animator builds a list of animation steps from an event range. This allows easy forwards and
* backwards seeking through the animation.
*/
class FcpsAnimator
{
	FcpsAnimator() = default;
	FcpsAnimator(const FcpsAnimator&) = delete;

	using anim_clock = std::chrono::steady_clock;

	std::vector<FcpsAnimationStep> stepsCoarse;
	std::vector<FcpsAnimationStep> stepsDetail;
	int stepIdx = -1;
	anim_clock::duration curStepDur;
	std::optional<anim_clock::time_point> lastDrawTime;
	float lastStepHz;
	bool drawDetail;

	auto& CurrentStepList() const;
	void DrawCurrentStep(MeshRendererDelegate& mr);

public:
	static FcpsAnimator& Singleton()
	{
		static FcpsAnimator a;
		return a;
	}

	void Draw(MeshRendererDelegate& mr);
	bool BuildAnimationSteps(fcps_event_range r);

	auto GetCurStep() const
	{
		return stepIdx;
	}

	auto GetNumSteps() const
	{
		return drawDetail ? stepsDetail.size() : stepsCoarse.size();
	}

	void ManualNextStep()
	{
		if (GetNumSteps() == 0 || !IsDrawing())
			return;
		if (stepIdx < (int)GetNumSteps() - 1)
			stepIdx++;
	}

	void ManualPrevStep()
	{
		if (stepIdx > 0 && IsDrawing())
			stepIdx--;
	}

	bool IsDrawing() const
	{
		return stepIdx >= 0 && stepIdx < (int)GetNumSteps();
	}

	void StopDrawing();
};

#endif
