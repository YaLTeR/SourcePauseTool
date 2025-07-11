#include <stdafx.hpp>

#include "fcps_event_queue.hpp"
#include "fcps_animate.hpp"

#ifdef SPT_FCPS_ENABLED

#include "spt/feature.hpp"
#include "spt/features/ent_props.hpp"
#include "spt/utils/game_detection.hpp"
#include "spt/utils/interfaces.hpp"
#include "spt/utils/file.hpp"
#include "spt/utils/serialize.hpp"
#include "../imgui/imgui_interface.hpp"

#include "spt/flatbuffers_schemas/fb_forward_declare.hpp"
#include "spt/flatbuffers_schemas/gen/fcps_generated.h"

#include "thirdparty/x86.h"

#include <sstream>
#include <chrono>
#include <thread>

namespace patterns
{
	PATTERNS(FindClosestPassableSpace,
	         "portal1-3420",
	         "55 8B EC 83 E4 F0 A1 ?? ?? ?? ?? 81 EC 74 02 00 00 83 78 30 00 53 56 57",
	         "portal1-5135",
	         "55 8B EC 83 E4 F0 A1 ?? ?? ?? ?? 81 EC 84 02 00 00 83 78 30 00 53 56 57");
}

class FcpsFeature : public FeatureWrapper<FcpsFeature>
{
public:
	void WarnAboutQueueOverwrite()
	{
		Warning(
		    "The FCPS queue is full! Old events will be overwritten the next time an event is recorded or imported!\n");
	}

	DECL_HOOK_CDECL(bool,
	                FindClosestPassableSpace,
	                IServerEntity* pEntity,
	                const Vector& vIndecisivePush,
	                unsigned int fMask);

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

} static fcps_feat;

ConVar spt_fcps_record{
    "spt_fcps_record",
    "0",
    FCVAR_DONTRECORD,
    "Enables recording of FindClosestPassableSpace function calls",
};

ConVar spt_fcps_max_queue_size{
    "spt_fcps_max_queue_size",
    "100",
    FCVAR_DONTRECORD,
    "Soft limit on the maximum number of FCPS events to record before dropping the oldest ones",
    true,
    1,
    true,
    1000,
};

ConVar spt_fcps_draw_detail{
    "spt_fcps_draw_detail",
    "0",
    FCVAR_DONTRECORD,
    "If enabled, draws test rays one at a time during animation",
};

ConVar spt_fcps_animation_hz{
    "spt_fcps_animation_hz",
    "3",
    FCVAR_DONTRECORD,
    "Time between FCPS animation steps, set to 0 for manual step iteration",
    true,
    0,
    true,
    1000,
};

CON_COMMAND_F(spt_fcps_next_step,
              "Skip to the next animation step (useful when spt_fcps_animation_hz is 0)",
              FCVAR_DONTRECORD)
{
	FcpsAnimator::Singleton().ManualNextStep();
}

CON_COMMAND_F(spt_fcps_prev_step,
              "Skip to the previous animation step (useful when spt_fcps_animation_hz is 0)",
              FCVAR_DONTRECORD)
{
	FcpsAnimator::Singleton().ManualPrevStep();
}

#define FCPS_COMPRESSED_FILE_EXT ".fcps.xz"
#define FCPS_FB_INDENTIFIER "FCPS"

CON_COMMAND_F(spt_fcps_export,
              "Export a single event or a range of events to binary file; usage: 'export [(N)|(N^M)] file_name' "
              "(IDs can be negative to specify events relative to the end)",
              FCVAR_DONTRECORD)
{
	if (args.ArgC() <= 2)
	{
		Warning(
		    "usage: %s [(N)|(N^M)] [file_name]\n"
		    "Use %s to print a list of FCPS events.\n",
		    spt_fcps_export_command.GetName(),
		    "spt_fcps_dump_events");
		return;
	}
	auto& eq = FcpsEventQueue::Singleton();
	auto [success, range] = eq.StrToRange(args.Arg(1));
	if (!success)
	{
		Warning("'%s' is not a valid event or range of events, use '%s' to print valid events.\n",
		        args.Arg(1),
		        "spt_fcps_dump_events");
		return;
	}

	std::filesystem::path filePath{GetGameDir()};
	filePath /= args.Arg(2);
	filePath += FCPS_COMPRESSED_FILE_EXT;
	filePath = std::filesystem::absolute(filePath);

	ser::FileWriter fWr{filePath};
	ser::XzWriter xzWr{fWr};
	flatbuffers::FlatBufferBuilder fbb{};
	flatbuffers::Offset<fb::fcps::FcpsEventList> root = eq.Serialize(fbb, range, xzWr);
	fbb.Finish(root, FCPS_FB_INDENTIFIER);
	xzWr.WriteSpan(fbb.GetBufferSpan());
	xzWr.Finish();
	fWr.Finish();

	if (!xzWr.Ok())
	{
		Warning("Failed to write to file: %s\n", xzWr.GetStatus().errMsg.c_str());
		return;
	}
	Msg("Exported %" PRId64 " FCPS event%s to '%s'\n",
	    range.second - range.first + 1,
	    range.second - range.first == 0 ? "" : "s",
	    filePath.string().c_str());
}

CON_COMMAND_AUTOCOMPLETEFILE(spt_fcps_import,
                             "Load exported FCPS event(s)",
                             FCVAR_DONTRECORD,
                             "",
                             FCPS_COMPRESSED_FILE_EXT)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: %s <fileName>\n", spt_fcps_import_command.GetName());
		return;
	}

	std::filesystem::path filePath{GetGameDir()};
	filePath /= args[1];
	filePath += FCPS_COMPRESSED_FILE_EXT;
	filePath = std::filesystem::absolute(filePath);
	ser::FileReader fRd{filePath.string().c_str()};
	ser::XzReader xzRd{fRd};

	auto rawSp = xzRd.GetRemaining();

	if (xzRd.Ok())
	{
		flatbuffers::Verifier v{
		    (const uint8_t*)rawSp.data(),
		    rawSp.size_bytes(),
		    flatbuffers::Verifier::Options{},
		};
		if (!v.VerifyBuffer<fb::fcps::FcpsEventList>(FCPS_FB_INDENTIFIER))
			xzRd.Err("flatbuffer verification failed");
	}

	auto& eq = FcpsEventQueue::Singleton();

	const fb::fcps::FcpsEventList* fbRoot =
	    xzRd.Ok() ? flatbuffers::GetRoot<fb::fcps::FcpsEventList>(rawSp.data()) : nullptr;

	if (fbRoot)
		eq.Deserialize(*fbRoot, xzRd);

	auto& status = xzRd.GetStatus();

	if (status.ok)
	{
		__assume(fbRoot);
		int nImported = (int)fbRoot->events()->size();
		fcps_event_id lastImportedId = eq.GetFirstQueueEventId() + eq.NumEventsInQueue() - 1;
		if (nImported == 1)
		{
			Msg("Imported 1 event (ID: %" FCPS_PR_ID ")\n", lastImportedId);
		}
		else
		{
			Msg("Imported %d events (IDs: %" FCPS_PR_ID "-%" FCPS_PR_ID ")\n",
			    nImported,
			    lastImportedId - nImported + 1,
			    lastImportedId);
		}
		if (eq.NumEventsOverLimit() > 0)
			fcps_feat.WarnAboutQueueOverwrite();
	}
	else
	{
		Warning("Failed to load trace from file: %s\n", status.errMsg.c_str());
		return;
	}
}

CON_COMMAND_F(spt_fcps_dump_events, "Print all stored FCPS events", FCVAR_DONTRECORD)
{
	/*
	* This is a hack. Older versions of the engine have some sort of internal race condition
	* bullshit going on which causes stuff to print to the console out of order. This is really
	* annoying for showing the FCPS queue since we give the user the ability to handle ranges of
	* events. Setting mat_queue_mode to 0 or 1 fixes this issue, but that's a bit cringe for my
	* taste - you'd have to set it, then defer printing of the queue for a tick before you can set
	* it back. Also I don't know if that could have some effect on gameplay. Instead, evanlin
	* suggest I batch Msg calls. This works fine in debug builds since the extra overhead of the
	* string creation is slow enough for the engine to process the previous Msg call, but it
	* doesn't work in release builds. Best solution I've found so far is just to sleep between the
	* batched message calls. The delay is noticeable, but it's better than printing stuff out of
	* order.
	* 
	* Of course ideally we'd setup some hooks or whatever and get rid of the race conditions having
	* to do with console writing.
	*/
	class BufferedCmdWriter
	{
		std::string buf;
		PrintFunc prevPf = nullptr;
		bool flushedAtLeastOnce = false;

	public:
		~BufferedCmdWriter()
		{
			Flush();
		}

		void Write(const std::string s, PrintFunc pf)
		{
			// actual internal limit is 5020, idk if that's the same on all versions
			constexpr size_t lim = 4096;
			Assert(!!pf && s.size() <= lim);
			if (pf != prevPf || buf.size() + s.size() > lim)
				Flush();
			prevPf = pf;
			buf += s;
		}

		void Flush()
		{
			if (buf.empty() || !prevPf)
				return;
			if (flushedAtLeastOnce)
				std::this_thread::sleep_for(std::chrono::milliseconds{1});
			prevPf("%s", buf.c_str());
			buf.clear();
			prevPf = nullptr;
			flushedAtLeastOnce = true;
		}
	};

	auto& eq = FcpsEventQueue::Singleton();
	int maxQueueSize = spt_fcps_max_queue_size.GetInt();
	bool anyPrinted = false;
	bool lastWasInKeepAlive = true;

	BufferedCmdWriter bufCmd{};

	auto it = eq.begin();
	while (FcpsEventQueue::QueueItVal itVal = *(it++))
	{
		if (anyPrinted && lastWasInKeepAlive && !itVal.isKeepAlive)
			bufCmd.Write("^   Will disappear once animation is stopped   ^\n", Warning);
		PrintFunc pf =
		    itVal.id < eq.GetFirstQueueEventId() + eq.NumEventsInQueue() - maxQueueSize ? Warning : Msg;
		auto s = std::format("[{}]{} {}\n",
		                     itVal.id,
		                     itVal.event->imported ? " (IMPORTED)" : "",
		                     itVal.event->GetShortDescription());
		bufCmd.Write(s, pf);
		anyPrinted = true;
		lastWasInKeepAlive = itVal.isKeepAlive;
	}

	if (!anyPrinted)
	{
		Msg("No FCPS events, use '%s' to record events or '%s' to import events.\n",
		    spt_fcps_record.GetName(),
		    spt_fcps_import_command.GetName());
	}
}

CON_COMMAND_F(spt_draw_fcps_events,
              "Animate one or more FCPS events, or stop animating events with zero arguments; usage: 'draw [(N)|(N^M)] "
              "(IDs can be negative to specify events relative to the end)",
              FCVAR_DONTRECORD)
{
	auto& fa = FcpsAnimator::Singleton();
	if (args.ArgC() == 1 || !*args.Arg(1))
	{
		if (!fa.IsDrawing())
			Msg("No FCPS events to stop animating.\n");
		else
			fa.StopDrawing();
		return;
	}

	auto [success, range] = FcpsEventQueue::Singleton().StrToRange(args.Arg(1));
	if (success)
		success = fa.BuildAnimationSteps(range);

	if (!success)
	{
		Warning("'%s' is not a valid event or range of events, use '%s' to see valid events.\n",
		        args.Arg(1),
		        spt_fcps_dump_events_command.GetName());
	}
}

bool FcpsFeature::ShouldLoadFeature()
{
	return sv_use_find_closest_passable_space = g_pCVar->FindVar("sv_use_find_closest_passable_space");
}

void FcpsFeature::InitHooks()
{
	HOOK_FUNCTION(server, FindClosestPassableSpace);
}

bool FcpsFinishedSignalWorks()
{
	return !!fcps_feat.ORIG_FindClosestPassableSpace && !!sv_use_find_closest_passable_space
	       && spt_meshRenderer.signal.Works;
}

void FcpsFeature::LoadFeature()
{
	if (!(fcpsFinishedSignal.Works = FcpsFinishedSignalWorks()))
		return;

	InitConcommandBase(spt_fcps_record);
	InitConcommandBase(spt_fcps_max_queue_size);
	InitConcommandBase(spt_fcps_draw_detail);
	InitConcommandBase(spt_fcps_animation_hz);
	InitCommand(spt_fcps_dump_events);
	InitCommand(spt_draw_fcps_events);
	InitCommand(spt_fcps_next_step);
	InitCommand(spt_fcps_prev_step);
	InitCommand(spt_fcps_export);
	InitCommand(spt_fcps_import);

	spt_fcps_max_queue_size.InstallChangeCallback(
	    [](IConVar* var, const char*, float)
	    {
		    if (((ConVar*)var)->GetInt() < (int)FcpsEventQueue::Singleton().NumEventsInQueue())
			    fcps_feat.WarnAboutQueueOverwrite();
	    });

	spt_meshRenderer.signal.Connect(&FcpsAnimator::Singleton(), &FcpsAnimator::Draw);
}

void FcpsFeature::UnloadFeature()
{
	sv_use_find_closest_passable_space = nullptr;
	fcpsFinishedSignal.Clear();
	fcpsFinishedSignal.Works = false;
	FcpsEventQueue::Singleton().Reset();
}

// utilities from fcps_record.cpp
void FcpsGetEntityOriginAngles(void* pEntity, Vector* pos, QAngle* ang);
FcpsCaller FcpsReturnAddressToFcpsCaller(uint8_t* retAddr);

IMPL_HOOK_CDECL(FcpsFeature,
                bool,
                FindClosestPassableSpace,
                IServerEntity* pEntity,
                const Vector& vIndecisivePush,
                unsigned int fMask)
{
	// we don't know ahead of time if this event needs to and can be stored in the queue, so keep it on the stack for now
	FcpsEvent tmpEvent;
	FcpsEvent* pEvent = &tmpEvent;

	bool record = fcpsFinishedSignal.Works ? spt_fcps_record.GetBool() : false;
	bool runOrigFcps;
	bool showStats;
	bool returnVal = false; // make the compiler warnings go away

	if (record)
	{
		runOrigFcps = false;
		showStats = true;

		FcpsImpl(*pEvent, pEntity, vIndecisivePush, fMask, _ReturnAddress());

		switch (pEvent->outcome.result)
		{
		case FCPS_SUCESS:
			returnVal = true;
			break;
		case FCPS_FAIL:
			returnVal = false;
			break;
		case FCPS_IMPLEMENTATION_INCOMPLETE:
			Warning("SPT: FCPS override not supported for this game version, running original FCPS\n");
			record = false;
			showStats = false;
			runOrigFcps = true;
			break;
		case FCPS_NOT_RUN_CVAR_DISABLED:
			DevMsg("SPT: FCPS not run because %s is '%s'\n",
			       sv_use_find_closest_passable_space->GetName(),
			       sv_use_find_closest_passable_space->GetString());
			record = false;
			returnVal = true;
			showStats = false;
			break;
		case FCPS_NOT_RUN_HAS_MOVE_PARENT:
			DevMsg("SPT: FCPS not run because entity at index %d has a move parent\n",
			       pEntity->GetRefEHandle().GetEntryIndex());
			record = false;
			returnVal = true;
			showStats = false;
			break;
		default:
			Assert(0);
		}
	}
	else
	{
		runOrigFcps = true;
		showStats = false;
	}

	if (runOrigFcps)
	{
		pEvent->gameInfo.Populate();

		auto& params = pEvent->params;
		params.entHandle = pEntity->GetRefEHandle();
		params.vIndecisivePush = vIndecisivePush;
		params.fMask = fMask;
		params.caller = FcpsReturnAddressToFcpsCaller((uint8_t*)_ReturnAddress());

		FcpsGetEntityOriginAngles(pEntity, &pEvent->outcome.fromPos, &pEvent->outcome.ang);
		returnVal = fcps_feat.ORIG_FindClosestPassableSpace(pEntity, vIndecisivePush, fMask);
		FcpsGetEntityOriginAngles(pEntity, &pEvent->outcome.toPos, nullptr);
		pEvent->outcome.result = returnVal ? FCPS_SUCESS : FCPS_FAIL;
	}

	if (record)
	{
		auto& eq = FcpsEventQueue::Singleton();

		// move event to the queue
		auto& qEvent = eq.NewEvent(true);
		std::swap(*pEvent, qEvent);
		pEvent = &qEvent;
		if (showStats)
		{
			Msg("SPT: FCPS (event %" FCPS_PR_ID "): ",
			    eq.GetFirstQueueEventId() + eq.NumEventsInQueue() - 1);
			Msg("%s\n", pEvent->GetShortDescription().c_str());
		}
	}
	else
	{
		Assert(!showStats);
	}

	if (fcpsFinishedSignal.Works)
		fcpsFinishedSignal(*pEvent);

	return returnVal;
}

#endif
