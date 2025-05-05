#include "stdafx.hpp"

#include <unordered_map>
#include <algorithm>

#include "spt/feature.hpp"

#include "tr_record_cache.hpp"
#include "tr_render_cache.hpp"
#include "tr_import_export.hpp"

#include "signals.hpp"
#include "spt/utils/ent_list.hpp"
#include "spt/utils/interfaces.hpp"
#include "spt/utils/file.hpp"
#include "spt/utils/game_detection.hpp"
#include "spt/features/hud.hpp"
#include "spt/features/visualizations/imgui/imgui_interface.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

class PlayerTraceFeature : public FeatureWrapper<PlayerTraceFeature>
{
public:
	TrPlayerTrace* StartRecording();
	TrPlayerTrace* StopRecording();
	void ChangeDisplayTick(int diff);
	void SetDisplayTick(tr_tick val);

	// only one active trace until we support import/export
	TrPlayerTrace tr;
	tr_tick activeDrawTick = 0;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	// TODO log FCPS & teleports reasons
	TrSegmentReason deferredSegmentReason = TR_SR_NONE;

	void OnTickSignal(bool simulating);
	void OnFinishRestoreSignal(void*);
	void OnMeshRenderSignal(MeshRendererDelegate& mr);
	void OnHudCallback();
};

static PlayerTraceFeature spt_player_trace_feat;

CON_COMMAND_F(spt_trace_start, "Starts recording the player trace", FCVAR_DONTRECORD)
{
	spt_player_trace_feat.StartRecording();
}

CON_COMMAND_F(spt_trace_stop, "Stops recording the player trace", FCVAR_DONTRECORD)
{
	TrPlayerTrace* activeTrace = spt_player_trace_feat.StopRecording();
	if (activeTrace)
		Msg("SPT: Done. Recorded for %d ticks.\n", activeTrace->numRecordedTicks);
	else
		Warning("SPT: No active trace!\n");
}

CON_COMMAND_F(spt_trace_next_tick, "Increments the trace draw tick", FCVAR_DONTRECORD)
{
	spt_player_trace_feat.ChangeDisplayTick(1);
}

CON_COMMAND_F(spt_trace_prev_tick, "Decrements the trace draw tick", FCVAR_DONTRECORD)
{
	spt_player_trace_feat.ChangeDisplayTick(-1);
}

CON_COMMAND_F(spt_trace_set_tick, "Sets the trace draw tick", FCVAR_DONTRECORD)
{
	if (args.ArgC() < 2)
	{
		Warning("Must provide an integer value\n");
		return;
	}
	spt_player_trace_feat.SetDisplayTick(strtoul(args[1], nullptr, 10));
}

CON_COMMAND_F(spt_trace_export, "Export trace to binary file", FCVAR_DONTRECORD)
{
	if (spt_player_trace_feat.tr.IsRecording())
	{
		Msg("Use %s to stop recording before exporting trace\n", spt_trace_stop_command.GetName());
		return;
	}
	if (args.ArgC() < 2)
	{
		Msg("Usage: %s <file_name>\n", spt_trace_export_command.GetName());
		return;
	}
	std::filesystem::path filePath{GetGameDir()};
	filePath /= args[1];
	filePath += TR_FILE_EXTENSION;
	filePath = std::filesystem::absolute(filePath);

	std::error_code ec;
	std::filesystem::create_directories(filePath.parent_path(), ec);
	if (ec)
	{
		Warning("Failed to create directories: %s\n", ec.message().c_str());
		return;
	}

	std::ofstream ofs{filePath, std::ios::binary};
	if (!ofs.is_open())
	{
		Warning("Failed to create file\n");
		return;
	}

	TrXzFileWriter wr{ofs};

	if (!wr.WriteTrace(spt_player_trace_feat.tr))
		Warning("Failed to write trace to file\n");
	else
		Msg("Wrote trace to '%s'.\n", filePath.string().c_str());
}

CON_COMMAND_AUTOCOMPLETEFILE(spt_trace_import, "Load trace from binary file", FCVAR_DONTRECORD, "", TR_FILE_EXTENSION)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: %s <fileName>\n", spt_trace_import_command.GetName());
		return;
	}

	std::filesystem::path filePath{GetGameDir()};
	filePath /= args[1];
	filePath += TR_FILE_EXTENSION;
	filePath = std::filesystem::absolute(filePath);
	std::ifstream ifs{filePath, std::ios::binary};
	if (!ifs.is_open())
	{
		Warning("Failed to open file '%s'\n", filePath.string().c_str());
		return;
	}

	TrXzFileReader rd{ifs};

	char sptVersion[TR_MAX_SPT_VERSION_LEN];
	std::string errMsg;
	TrPlayerTrace newTr;
	if (!rd.ReadTrace(newTr, sptVersion, errMsg))
	{
		Warning("Failed to load trace from file: %s\n", rd.errMsg.empty() ? errMsg.c_str() : rd.errMsg.c_str());
		return;
	}
	Msg("Loaded trace from '%s' with %d ticks%s%s\n",
	    filePath.u8string().c_str(),
	    newTr.numRecordedTicks,
	    errMsg.empty() ? "" : " and warnings: ",
	    errMsg.c_str());
	spt_player_trace_feat.tr = std::move(newTr);
	spt_player_trace_feat.activeDrawTick = 0;
}

ConVar spt_draw_trace{"spt_draw_trace", "0", FCVAR_DONTRECORD, "Draw last recorded player trace."};
ConVar spt_hud_trace{"spt_hud_trace", "0", FCVAR_DONTRECORD, "Show info about the player trace."};
ConVar spt_trace_autoplay("spt_trace_autoplay", "0", FCVAR_DONTRECORD, "Play the trace recording in real time.");
ConVar spt_trace_ent_collect_radius("spt_trace_ent_collect_radius",
                                    "250",
                                    FCVAR_DONTRECORD,
                                    "The radius around the player used for entity collection");
ConVar spt_trace_draw_portal_collision_entities(
    "spt_trace_draw_portal_collision_entities",
    "0",
    FCVAR_DONTRECORD,
    "If enabled, draws all portalsimulator_collisionentity when drawing the trace.");
ConVar spt_trace_draw_path_cones(
    "spt_trace_draw_path_cones",
    "1",
    FCVAR_DONTRECORD,
    "If enabled, draws cones along the player path to indicate the player travel direction.");
ConVar spt_trace_draw_cam_style("spt_trace_draw_cam_style",
                                "0",
                                FCVAR_DONTRECORD,
                                "Player trace camera type:\n"
                                "  0 = camera frustum\n"
                                "  1 = box and line\n");
ConVar spt_trace_draw_contact_points("spt_trace_draw_contact_points",
                                     "1",
                                     FCVAR_DONTRECORD,
                                     "If enabled, draws recorded contact points for the player.");

bool PlayerTraceFeature::ShouldLoadFeature()
{
	return TickSignal.Works && interfaces::engine_tool;
}

void PlayerTraceFeature::LoadFeature()
{
	if (!spt_meshRenderer.signal.Works)
		return;
	TickSignal.Connect(this, &PlayerTraceFeature::OnTickSignal);
	FinishRestoreSignal.Connect(this, &PlayerTraceFeature::OnFinishRestoreSignal);
	spt_meshRenderer.signal.Connect(this, &PlayerTraceFeature::OnMeshRenderSignal);

	InitCommand(spt_trace_start);
	InitCommand(spt_trace_stop);
	InitCommand(spt_trace_next_tick);
	InitCommand(spt_trace_prev_tick);
	InitCommand(spt_trace_set_tick);
	InitCommand(spt_trace_export);
	InitCommand(spt_trace_import);

	if (AddHudCallback("trace", [](auto) { spt_player_trace_feat.OnHudCallback(); }, spt_hud_trace))
		SptImGui::RegisterHudCvarCheckbox(spt_hud_trace);

	InitConcommandBase(spt_trace_autoplay);
	InitConcommandBase(spt_trace_ent_collect_radius);
	if (utils::DoesGameLookLikePortal())
		InitConcommandBase(spt_trace_draw_portal_collision_entities);
	InitConcommandBase(spt_trace_draw_path_cones);
	InitConcommandBase(spt_trace_draw_cam_style);
	InitConcommandBase(spt_trace_draw_contact_points);
	InitConcommandBase(spt_draw_trace);

	spt_draw_trace.InstallChangeCallback(
	    [](IConVar* var, const char*, float)
	    {
		    if (!((ConVar*)var)->GetBool())
			    spt_player_trace_feat.tr.StopRendering();
	    });
}

void PlayerTraceFeature::UnloadFeature()
{
	StopRecording();
	tr.Clear();
}

TrPlayerTrace* PlayerTraceFeature::StartRecording()
{
	tr.StartRecording();
	activeDrawTick = 0;
	deferredSegmentReason = TR_SR_NONE;
	return &tr;
}

TrPlayerTrace* PlayerTraceFeature::StopRecording()
{
	if (tr.IsRecording())
	{
		tr.StopRecording();
		return &tr;
	}
	return nullptr;
}

void PlayerTraceFeature::ChangeDisplayTick(int diff)
{
	if (!spt_draw_trace.GetBool())
		return;
	activeDrawTick = (tr_tick)clamp((int64_t)activeDrawTick + diff, 0, std::numeric_limits<tr_tick>::max());
}

void PlayerTraceFeature::SetDisplayTick(tr_tick val)
{
	if (!spt_draw_trace.GetBool())
		return;
	activeDrawTick = val;
}

void PlayerTraceFeature::OnTickSignal(bool simulating)
{
	if (tr.IsRecording())
		tr.HostTickCollect(true, deferredSegmentReason, spt_trace_ent_collect_radius.GetFloat());

	deferredSegmentReason = TR_SR_NONE;

	if (spt_trace_autoplay.GetBool())
		ChangeDisplayTick(1);
}

void PlayerTraceFeature::OnFinishRestoreSignal(void*)
{
	deferredSegmentReason = TR_SR_SAVELOAD;
}

void PlayerTraceFeature::OnMeshRenderSignal(MeshRendererDelegate& mr)
{
	if (!spt_draw_trace.GetBool())
		return;
	activeDrawTick = clamp(activeDrawTick, 0, tr.numRecordedTicks - 1);
	tr.GetRenderingCache().RenderAll(mr, activeDrawTick);
}

void PlayerTraceFeature::OnHudCallback()
{
	bool drawing = spt_draw_trace.GetBool();

	if (drawing)
	{
		spt_hud_feat.DrawTopHudElement(L"Trace draw tick: %u/%u",
		                               activeDrawTick,
		                               tr.numRecordedTicks == 0 ? 0 : tr.numRecordedTicks - 1);
	}
	else
	{
		spt_hud_feat.DrawTopHudElement(L"Recorded trace ticks: %d", tr.numRecordedTicks);
	}
	spt_hud_feat.DrawTopHudElement(L"Trace server tick: %d", tr.GetServerTickAtTick(activeDrawTick));

	float displayUsage = (float)tr.GetMemoryUsage();
	const wchar* suffixes[] = {L"B", L"KiB", L"MiB", L"GiB"};
	int i = 0;
	while (displayUsage > 1024 && i < ARRAYSIZE(suffixes) - 1)
	{
		displayUsage /= 1024.f;
		i++;
	}

	spt_hud_feat.DrawTopHudElement(L"Trace memory usage: %.*f%s", i > 0 ? 2 : 0, displayUsage, suffixes[i]);
}

bool player_trace::GetActiveTracePos(Vector& pos, QAngle& ang, float& fov)
{
	auto& tr = spt_player_trace_feat.tr;
	TrReadContextScope scope{tr};
	auto plDataIdx = tr.GetAtTick<TrPlayerData>(spt_player_trace_feat.activeDrawTick);
	if (!plDataIdx.IsValid())
		return false;
	// TODO setting for seeing from sg eyes
	pos = **plDataIdx->transEyesIdx->posIdx;
	ang = **plDataIdx->transEyesIdx->angIdx;
	fov = plDataIdx->fov;
	return true;
}

#endif
