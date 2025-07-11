#include "stdafx.hpp"

#include "fcps_animate.hpp"
#include "fcps_event_queue.hpp"

#ifdef SPT_FCPS_ENABLED

#include <variant>
#include <algorithm>

#include "spt/features/hud.hpp"

// this function is specifically for the main ent because it's an AABB
static void FcpsDrawMainEntAabb(MeshRendererDelegate& mr,
                                const FcpsEvent& event,
                                const Vector& entCenter,
                                const ShapeColor& sc)
{
	mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
	    [&event, entCenter, sc](MeshBuilderDelegate& mb)
	    { mb.AddBox(entCenter, -event.detail.aabbExtents, event.detail.aabbExtents, vec3_angle, sc); }));
}

// this is for any ent via fcps_ent_idx
static void FcpsDrawEnt(MeshRendererDelegate& mr,
                        const FcpsEvent& event,
                        fcps_ent_idx idx,
                        const ShapeColor& physMeshSc,
                        bool drawObb,
                        const Vector& additionalOffset = vec3_origin)
{
	auto entInfo = FCPS_OPT_ELEM_PTR(event.detail.ents, idx);
	if (!entInfo)
		return;

	for (auto& entMesh : entInfo->meshes)
	{
		DynamicMesh mesh = spt_meshBuilder.CreateDynamicMesh(
		    [&entMesh, physMeshSc](MeshBuilderDelegate& mb)
		    {
			    if (entMesh.ballRadius > 0)
				    mb.AddSphere(entMesh.pos, entMesh.ballRadius, 2, physMeshSc);
			    else
				    mb.AddMbCompactMesh(entMesh.physMesh, physMeshSc);
		    });

		// capture by value
		Vector pos = entMesh.pos + additionalOffset;
		QAngle ang = entMesh.ang;
		mr.DrawMesh(mesh, [pos, ang](auto&, CallbackInfoOut& infoOut) { AngleMatrix(ang, pos, infoOut.mat); });
	}

	if (drawObb && entInfo->obbMins != entInfo->obbMaxs)
	{
		mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
		    [entInfo](MeshBuilderDelegate& mb)
		    {
			    mb.AddBox(entInfo->pos,
			              entInfo->obbMins,
			              entInfo->obbMaxs,
			              entInfo->ang,
			              fcps_colors.traceHitEntObb);
		    }));
	}
}

static void FcpsTestRayPairDrawHelper(MeshRendererDelegate& mr, const FcpsTraceResult* tr1, const FcpsTraceResult* tr2)
{
	// at most one of these can be null
	if (!tr1 && !tr2)
		return;

	/*
	* trace.fraction actually means something if startsolid is true.
	* Pretend the fraction is 0 if this is the case.
	*/
	float f1Frac = !tr1 || tr1->trace.startsolid ? 0.f : tr1->trace.fraction;
	float f2Frac = !tr2 || tr2->trace.startsolid ? 0.f : tr2->trace.fraction;

	// make tr1 the not-null one and the one that travelled further
	if (!tr1 || f2Frac > f1Frac)
	{
		std::swap(tr1, tr2);
		std::swap(f1Frac, f2Frac);
	}

	__assume(tr1 != nullptr);

	/*
	* I've divided up the space of all possible traces into 7 different draw cases. Each letter
	* corresponds to an if block below.
	* 
	*     1.0 +-------------------------A
	*         |                      __/B
	*         |  f2Frac > f1Frac  __/GGGB
	*         |                __/GGGGGGB
	*         | ignore this __/GGGGGGGGGB
	*  f2Frac |  region  __/GGGGGGGGGGGGB
	*         |       __/FFFFFGGGGGGGGGGB
	*         |    __/FFFFFFFFFFFGGGGGGGB
	*         | __/FFFFFFFFFFFFFFFFFGGGGB
	*         |/FFFFFFFFFFFFFFFFFFFFFFFGB
	*     0.0 EDDDDDDDDDDDDDDDDDDDDDDDDDC
	*        0.0        f1Frac         1.0
	* 
	* Note that cases B,C,G are really weird. Intuitively you might assume that if there is a ray
	* trace A->B and it hits something, then B->A will also hit something. But this is not always
	* the case. Some examples:
	* 
	* - Cameras have a bad BBOX which does not fully include their mesh. This makes it possible for
	*   *some* rays to go through the mesh of the BBOX and not hit the camera.
	* 
	* - According to ancient memory and bill, displacements are only detectable from one direction
	*   with ray traces.
	* 
	* - Ragdolls sometimes react weirdly to ray traces.
	* 
	* To simplify the code below, we don't use tr1/tr2 directly. Instead, we generate pseudo trace
	* starts and ends using f1Frac/f2Frac.
	*/

	mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
	    [=](MeshBuilderDelegate& mb)
	    {
		    auto& fce = fcps_colors.entTest;
		    const Vector& ext = tr1->ray.extents;
		    Vector t1Start = tr1->ray.start;
		    Vector t2Start = tr1->ray.start + tr1->ray.delta;
		    Vector t1End, t2End;
		    VectorLerp(t1Start, t2Start, f1Frac, t1End);
		    VectorLerp(t2Start, t1Start, f2Frac, t2End);

		    if (f1Frac == 1.f && f2Frac == 1.f)
		    {
			    // case A: full successful trace between both corners
			    SweptBoxColor sbc{fce.endpoints, fce.sweepSuccess, fce.endpoints};
			    mb.AddSweptBox(t1Start, t2Start, -ext, ext, sbc);
		    }
		    else if (f1Frac == 0.f && f2Frac == 0.f)
		    {
			    // case E: both corners are inside something
			    SweptBoxColor sbc{fce.endpoints, fce.sweepFail, fce.endpoints};
			    mb.AddSweptBox(t1Start, t2Start, -ext, ext, sbc);
		    }
		    else if (f1Frac == 1.f && f2Frac == 0.f)
		    {
			    // case C (assymetrical): tr1 trace made it all the way and tr2 didn't start
			    SweptBoxColor sbc{fce.endpoints, fce.sweepSuccess, fce.sweepFail};
			    mb.AddSweptBox(t1Start, t2Start, -ext, ext, sbc);
		    }
		    else if (f2Frac == 0.f && f1Frac < 1.f)
		    {
			    // case D: only one corner started but didn't finish
			    SweptBoxColor sbc1{fce.endpoints, fce.sweepSuccess, fce.endpoints};
			    mb.AddSweptBox(t1Start, t1End, -ext, ext, sbc1);
			    SweptBoxColor sbc2{fce.invisible, fce.sweepFail, fce.endpoints};
			    mb.AddSweptBox(t1End, t2Start, -ext, ext, sbc2);
		    }
		    else if (f1Frac > 0.f && f2Frac > 0.f && f1Frac + f2Frac < 1.f)
		    {
			    // case F: some obstacle in the way with both rays blocked
			    SweptBoxColor sbc1{fce.endpoints, fce.sweepSuccess, fce.endpoints};
			    mb.AddSweptBox(t1Start, t1End, -ext, ext, sbc1);
			    SweptBoxColor sbc2{fce.invisible, fce.sweepFail, fce.endpoints};
			    mb.AddSweptBox(t1End, t2End, -ext, ext, sbc2);
			    SweptBoxColor sbc3{fce.invisible, fce.sweepSuccess, fce.endpoints};
			    mb.AddSweptBox(t2End, t2Start, -ext, ext, sbc3);
		    }
		    else if (f1Frac == 1.f && f2Frac > 0.f && f2Frac < 1.f)
		    {
			    // case B (assymetrical): first corner is fine, second corner is blocked
			    SweptBoxColor sbc1{fce.endpoints, fce.sweepSuccess, fce.endpoints};
			    mb.AddSweptBox(t1Start, t2End, -ext, ext, sbc1);
			    SweptBoxColor sbc2{fce.invisible, fce.assymetricOverlap, fce.endpoints};
			    mb.AddSweptBox(t2End, t2Start, -ext, ext, sbc2);
		    }
		    else if (f1Frac + f2Frac >= 1.f && f1Frac < 1.f)
		    {
			    // case G (assymetrical): neither ray made it all the way, but the successful part of the traces overlap
			    SweptBoxColor sbc1{fce.endpoints, fce.sweepSuccess, fce.endpoints};
			    mb.AddSweptBox(t1Start, t2End, -ext, ext, sbc1);
			    SweptBoxColor sbc2{fce.invisible, fce.assymetricOverlap, fce.endpoints};
			    mb.AddSweptBox(t2End, t1End, -ext, ext, sbc2);
			    SweptBoxColor sbc3{fce.invisible, fce.sweepSuccess, fce.endpoints};
			    mb.AddSweptBox(t1End, t2Start, -ext, ext, sbc3);
		    }
		    else
		    {
			    // can't use AssertMsg here
			    DebugBreak();
		    }
	    }));
}

static void FcpsDrawMainEntAtTraceIdx(MeshRendererDelegate& mr, const FcpsEvent& event, int idx)
{
	auto entRayInfo = FCPS_OPT_ELEM_PTR(event.detail.entTraces, idx);
	if (!entRayInfo)
		return;
	Vector mainEntOrigin = entRayInfo->ray.start + event.detail.entCenterToOriginDiff;
	FcpsDrawMainEntAabb(mr, event, entRayInfo->ray.start, fcps_colors.curPos);

	if (event.params.entHandle.GetEntryIndex() != 1)
	{
		FcpsDrawEnt(mr,
		            event,
		            event.detail.runOnEnt,
		            fcps_colors.mainEntPhysMeshes,
		            false,
		            mainEntOrigin - event.outcome.fromPos);
	}
}

#define FCPS_DEFINE_DRAW_FUNC(struct_name) \
	static void struct_name##Draw(MeshRendererDelegate& mr, \
	                              const FcpsEvent& event, \
	                              [[maybe_unused]] const struct_name& data)

FCPS_DEFINE_DRAW_FUNC(FcpsAnimHello)
{
	FcpsDrawMainEntAabb(mr, event, event.outcome.fromPos - event.detail.entCenterToOriginDiff, fcps_colors.curPos);
	FcpsDrawEnt(mr, event, event.detail.runOnEnt, fcps_colors.mainEntPhysMeshes, false);
}

FCPS_DEFINE_DRAW_FUNC(FcpsAnimEntRay)
{
	auto rayInfo = FCPS_OPT_ELEM_PTR(event.detail.entTraces, data.entRayIdx);
	if (!rayInfo)
	{
		Assert(0);
		return;
	}

	mr.DrawMesh(spt_meshBuilder.CreateDynamicMesh(
	    [=, &event](MeshBuilderDelegate& mb)
	    {
		    const Vector& ext = rayInfo->ray.extents;
		    SweptBoxColor sbc{fcps_colors.curPos, fcps_colors.entRaySweep, fcps_colors.entRayTarget};
		    mb.AddSweptBox(rayInfo->ray.start, rayInfo->ray.start + rayInfo->ray.delta, -ext, ext, sbc);
		    if (!rayInfo->trace.startsolid)
			    mb.AddBox(rayInfo->trace.endpos, -ext, ext, vec3_angle, fcps_colors.entRayHit);
	    }));

	FcpsDrawEnt(mr, event, rayInfo->hitEnt, fcps_colors.traceHitEntPhysMeshes, true);

	if (event.params.entHandle.GetEntryIndex() != 1)
	{
		Vector mainEntOrigin = rayInfo->ray.start + event.detail.entCenterToOriginDiff;
		FcpsDrawEnt(mr,
		            event,
		            event.detail.runOnEnt,
		            fcps_colors.mainEntPhysMeshes,
		            false,
		            mainEntOrigin - event.outcome.fromPos);
	}
}

FCPS_DEFINE_DRAW_FUNC(FcpsAnimTestRayPair)
{
	auto nudgeInfo = FCPS_OPT_ELEM_PTR(event.detail.nudges, data.nudgeIdx);
	if (!nudgeInfo)
		return;

	auto tr1 = FCPS_OPT_ELEM_PTR(nudgeInfo->testTraces, data.testRayIdx1);
	auto tr2 = FCPS_OPT_ELEM_PTR(nudgeInfo->testTraces, data.testRayIdx2);

	FcpsTestRayPairDrawHelper(mr, tr1, tr2);

	if (tr1 && tr1->hitEnt)
		FcpsDrawEnt(mr, event, tr1->hitEnt, fcps_colors.traceHitEntPhysMeshes, true);
	if (tr2 && (!tr1 || tr2->hitEnt != tr1->hitEnt))
		FcpsDrawEnt(mr, event, tr2->hitEnt, fcps_colors.traceHitEntPhysMeshes, true);

	FcpsDrawMainEntAtTraceIdx(mr, event, data.nudgeIdx);
}

FCPS_DEFINE_DRAW_FUNC(FcpsAnimTestRayAll)
{
	auto nudgeInfo = FCPS_OPT_ELEM_PTR(event.detail.nudges, data.nudgeIdx);
	if (!nudgeInfo)
		return;

	std::array<fcps_ent_idx, 58> hitEnts; // 8 permute 2
	auto hitEntIt = hitEnts.begin();
	auto hitEntEnd = hitEnts.cend();

	/*
	* Iterate over all test traces and look for pairs of traces that represent the same two corners
	* (they'll be consecutive elements). Also, gather all entities hit with the traces.
	*/

	auto testTraceIt = nudgeInfo->testTraces.begin();
	auto testTraceEnd = nudgeInfo->testTraces.cend();
	auto cornerBitsIt = nudgeInfo->testTraceCornerBits.begin();
	auto cornerBitsEnd = nudgeInfo->testTraceCornerBits.cend();

	for (; testTraceIt != testTraceEnd && cornerBitsIt != cornerBitsEnd; ++testTraceIt, ++cornerBitsIt)
	{
		const FcpsTraceResult* tr1 = &*testTraceIt;
		const FcpsTraceResult* tr2 = nullptr;

		if (testTraceIt + 1 != testTraceEnd && cornerBitsIt + 1 != cornerBitsEnd)
		{
			uint8_t tr1CornerBits = *cornerBitsIt;
			uint8_t tr2CornerBits = *(cornerBitsIt + 1);
			if (((tr1CornerBits >> 3) | ((tr1CornerBits & 7) << 3)) == tr2CornerBits)
			{
				// the next element is a pair with the current one
				tr2 = &*++testTraceIt;
				++cornerBitsIt;
			}
		}

		FcpsTestRayPairDrawHelper(mr, tr1, tr2);

		// gather hit ents

		if (hitEntIt != hitEntEnd)
			*hitEntIt++ = tr1->hitEnt;
		if (tr2 && hitEntIt != hitEntEnd)
			*hitEntIt++ = tr2->hitEnt;
	}

	// sort -> remove duplicates -> drop invalid ents -> draw all

	auto begin = hitEnts.begin();
	std::sort(begin, hitEntIt);
	hitEntIt = std::unique(begin, hitEntIt);

	for (auto it = begin; it != hitEntIt && !!FCPS_OPT_ELEM_PTR(event.detail.ents, *it); ++it)
		FcpsDrawEnt(mr, event, *it, fcps_colors.traceHitEntPhysMeshes, true);

	FcpsDrawMainEntAtTraceIdx(mr, event, data.nudgeIdx);
}

FCPS_DEFINE_DRAW_FUNC(FcpsAnimGoodbye)
{
	FcpsDrawMainEntAabb(mr,
	                    event,
	                    event.outcome.toPos - event.detail.entCenterToOriginDiff,
	                    event.outcome.result == FCPS_FAIL ? fcps_colors.fail : fcps_colors.success);

	// player vphys does not teleport from the Teleport() function - this might apply to other entities too
	Vector offset =
	    event.params.entHandle.GetEntryIndex() == 1 ? vec3_origin : event.outcome.toPos - event.outcome.fromPos;
	FcpsDrawEnt(mr, event, event.detail.runOnEnt, fcps_colors.mainEntPhysMeshes, false, offset);
}

#define _FCPS_CASE(X) \
	case FcpsStepType::X##StepType: \
		X##Draw(mr, *event, curStep.X##Data); \
		break;

auto& FcpsAnimator::CurrentStepList() const
{
	return drawDetail ? stepsDetail : stepsCoarse;
}

void FcpsAnimator::DrawCurrentStep(MeshRendererDelegate& mr)
{
	if (!IsDrawing())
		return;
	const FcpsAnimationStep& curStep = CurrentStepList()[stepIdx];
	const FcpsEvent* event = FcpsEventQueue::Singleton().GetEvent(curStep.id);
	if (!event)
	{
		Assert(0);
		return;
	}
	switch (curStep.stepType)
	{
		FCPS_ANIMATION_STEPS(_FCPS_CASE)
	default:
		AssertMsg(0, "SPT: undefined animation type");
	}
}

void FcpsAnimator::Draw(MeshRendererDelegate& mr)
{
	if (!IsDrawing())
		return;

	// handle switching between the detail & coarse lists

	if (drawDetail != spt_fcps_draw_detail.GetBool())
	{
		stepIdx = CurrentStepList()[stepIdx].mappedIdx;
		drawDetail ^= 1;
		curStepDur = anim_clock::duration::zero();
	}

	// remap curStepDur if we changed the animation hz

	float newStepHz = spt_fcps_animation_hz.GetFloat();
	if (lastStepHz == 0.f || newStepHz == 0.f)
		curStepDur = anim_clock::duration::zero();
	else if (lastStepHz != newStepHz)
		curStepDur *= lastStepHz / newStepHz;
	lastStepHz = newStepHz;

	// update curStepDur

	auto now = anim_clock::now();
	if (lastDrawTime.has_value() && newStepHz > 0)
		curStepDur += now - *lastDrawTime;
	lastDrawTime = now;

	/*
	* At first I attempted to draw the current step *and* any steps that we skipped over, but that
	* results in overdraw in some cases (e.g. main ent AABB getting drawn twice). Handling that is
	* an absolute pain and I don't think it's worth it, so just draw the most up-to-date one.
	*/

	using per = anim_clock::duration::period;
	int nStepsToSkip =
	    newStepHz <= 0 ? 0 : (int)std::floor((double)curStepDur.count() * newStepHz * per::num / per::den);

	if (nStepsToSkip > 0)
	{
		stepIdx += nStepsToSkip;
		curStepDur -= std::chrono::duration_cast<anim_clock::duration>(
		    std::chrono::duration<float>{nStepsToSkip / newStepHz});
	}

	if (IsDrawing())
		DrawCurrentStep(mr);
}

bool FcpsAnimator::BuildAnimationSteps(fcps_event_range r)
{
	auto& eq = FcpsEventQueue::Singleton();
	if (!eq.SetKeepAliveRange(r))
		return false;

	stepIdx = 0;
	drawDetail = spt_fcps_draw_detail.GetBool();
	lastStepHz = spt_fcps_animation_hz.GetFloat();
	curStepDur = anim_clock::duration::zero();
	lastDrawTime.reset();

	stepsCoarse.clear();
	stepsDetail.clear();

	for (fcps_event_id id = r.first; id <= r.second; id++)
	{
		FcpsAnimationStep hello{
		    .id = id,
		    .stepType = FcpsAnimHelloStepType,
		    .FcpsAnimHelloData{},
		};
		hello.mappedIdx = stepsDetail.size();
		stepsCoarse.push_back(hello);
		hello.mappedIdx = stepsCoarse.size() - 1;
		stepsDetail.push_back(hello);

		auto event = eq.GetEvent(id);
		if (!event)
		{
			Assert(0);
			return false;
		}

		for (byte i = 0;; i++)
		{
			if (i < event->detail.entTraces.size())
			{
				FcpsAnimationStep entRay{
				    .id = id,
				    .stepType = FcpsAnimEntRayStepType,
				    .FcpsAnimEntRayData{.entRayIdx = i},
				};
				entRay.mappedIdx = stepsDetail.size();
				stepsCoarse.push_back(entRay);
				entRay.mappedIdx = stepsCoarse.size() - 1;
				stepsDetail.push_back(entRay);
			}

			if (i >= event->detail.entTraces.size() || i >= event->detail.nudges.size())
			{
				FcpsAnimationStep goodbye{
				    .id = id,
				    .stepType = FcpsAnimGoodbyeStepType,
				    .FcpsAnimGoodbyeData{},
				};
				goodbye.mappedIdx = stepsDetail.size();
				stepsCoarse.push_back(goodbye);
				goodbye.mappedIdx = stepsCoarse.size() - 1;
				stepsDetail.push_back(goodbye);
				break;
			}

			auto& nudge = event->detail.nudges[i];

			FcpsAnimationStep coarseTestRays{
			    .id = id,
			    .stepType = FcpsAnimTestRayAllStepType,
			    .mappedIdx = stepsDetail.size() - 1,
			    .FcpsAnimTestRayAllData{.nudgeIdx = i},
			};
			stepsCoarse.push_back(coarseTestRays);

			bool anyDetailRays = false;

			// same corner order as in FCPS
			byte j = 0;
			for (byte c1 = 0; c1 < 7; c1++)
			{
				for (byte c2 = c1 + 1; c2 < 8; c2++)
				{
					bool c1c2 = ~nudge.cornerOobBits & (1 << c1);
					bool c2c1 = ~nudge.cornerOobBits & (1 << c2);
					if (!c1c2 && !c2c1)
						continue;

					FcpsAnimationStep detailTestRayPair{
					    .id = id,
					    .stepType = FcpsAnimTestRayPairStepType,
					    .mappedIdx = stepsCoarse.size() - 1,
					    .FcpsAnimTestRayPairData{
					        .nudgeIdx = i,
					        .testRayIdx1 = c1c2 && j < nudge.testTraces.size() ? j++ : (byte)-1,
					        .testRayIdx2 = c2c1 && j < nudge.testTraces.size() ? j++ : (byte)-1,
					    },
					};
					stepsDetail.push_back(detailTestRayPair);
					anyDetailRays = true;
				}
			}

			if (anyDetailRays)
				stepsCoarse.back().mappedIdx++;
		}
	}
	return true;
}

void FcpsAnimator::StopDrawing()
{
	stepsCoarse.clear();
	stepsDetail.clear();
	FcpsEventQueue::Singleton().SetKeepAliveRange(FCPS_INVALID_EVENT_RANGE);
}

#endif
