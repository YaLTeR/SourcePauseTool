#include "stdafx.hpp"
#ifndef OE
#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"
#include "convar.hpp"
#include "ent_props.hpp"
#include "file.hpp"
#include "tier1\CommandBuffer.h"
#include "interfaces.hpp"
#include "signals.hpp"
#include <charconv>
#include <vector>

struct RecordedCommand
{
	int tick;
	std::string cmd;
};

namespace patterns
{
	PATTERNS(CCommandBuffer__DequeueNextCommand, "5135", "53 56 8B F1 8D 9E");
}

// Recording a TAS script from player inputs
class TASRecordFeature : public FeatureWrapper<TASRecordFeature>
{
public:
	DECL_HOOK_THISCALL(bool, CCommandBuffer__DequeueNextCommand, CCommandBuffer*);

	bool recording = false;
	int currentTick = 0;
	QAngle prevViewAngles;
	std::vector<RecordedCommand> recordedCommands;

	PlayerField<Vector> m_vecViewAngles;

	void AddCommand(const char* cmd);
	void SaveToFile(const char* filepath);

	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static TASRecordFeature spt_tas_record;

static bool IsRecordable(const CCommand& command)
{
	// Could make this a user specifiable blacklist
	static const char* IGNORED_COMMAND_PREFIXES[] = {
	    "exec",
	    "sk_",
	    "tas_pause",
	    "connect",
	    "give",
	    "autosave",
	    "changelevel2",
	    "cl_predict",
	    "dsp_player",
	    "tas_experimental",
	};

	if (command.ArgC() == 0)
		return false;
	else if (interfaces::g_pCVar == nullptr)
		return true;

	const char* cmd = command.Arg(0);
	for (auto pref : IGNORED_COMMAND_PREFIXES)
	{
		const char* result = strstr(cmd, pref);
		if (result == cmd)
			return false;
	}

	// Make sure the command is not an alias
	return interfaces::g_pCVar->FindCommandBase(cmd) != nullptr;
}

CON_COMMAND(tas_experimental_record, "Record gameplay to .srctas script.")
{
	// Set these to some invalid value so any view angle input is added
	spt_tas_record.prevViewAngles[0] = 999.999f;
	spt_tas_record.prevViewAngles[1] = 999.999f;
	spt_tas_record.recording = true;
	spt_tas_record.recordedCommands.clear();
	spt_tas_record.currentTick = 0;
	Msg("Started new recording...\n");
}

CON_COMMAND(tas_experimental_record_save, "Save recording to .srctas.")
{
	if (args.ArgC() <= 1)
	{
		Msg("Usage: spt_tas_experimental_record_save <filepath>\n");
	}
	else
	{
		char path[261]; // max path + 1
		std::string dir = GetGameDir();
		int result = sprintf_s(path, "%s\\%s.srctas", dir.c_str(), args.Arg(1));

		if (result > 0)
		{
			spt_tas_record.SaveToFile(path);
			Msg("Script saved to path %s\n", path);
		}
		else
		{
			Msg("Filepath %s is too long\n", args.Arg(1));
		}
	}
}

CON_COMMAND(tas_experimental_record_stop, "Stop recording gameplay.")
{
	spt_tas_record.recording = false;
	Msg("Stopped recording\n");
}

bool TASRecordFeature::ShouldLoadFeature()
{
	return true;
}

static char* ToString(const CCommand& command)
{
	static char BUFFER[8192];

	std::size_t arg0_len = strlen(command.Arg(0));
	std::size_t args_len = strlen(command.ArgS());
	memcpy(BUFFER, command.Arg(0), arg0_len);
	BUFFER[arg0_len] = ' ';
	memcpy(BUFFER + arg0_len + 1, command.ArgS(), args_len + 1);

	return BUFFER;
}

// Remove keycodes from toggles
static void RemoveKeycodes(CCommand& command)
{
	const char* cmd = command.Arg(0);

	if (cmd && (cmd[0] == '+' || cmd[0] == '-'))
	{
		char BUFFER[128];
		strncpy(BUFFER, cmd, sizeof(BUFFER));
		command.Reset();
		command.Tokenize(BUFFER);
	}
}

static void Frame_Hook()
{
	if (spt_tas_record.recording)
	{
		spt_tas_record.currentTick += 1;
	}
}

static void ProcessMovement_Hook(void* pPlayer, void* pMove)
{
	if (!spt_tas_record.recording)
		return;

	char pitch[64], yaw[64];
	char CMD_BUFFER[256];
	float va[3];
	EngineGetViewAngles(va);

	// Only update viewangles when they have changed.
	// There is a potential nightmare case here where the view has moved as a result of movement (portal transition or otherwise)
	// and the player has exactly corrected their view back to the spot where it was on the previous tick
	if (va[PITCH] != spt_tas_record.prevViewAngles[PITCH] || va[YAW] != spt_tas_record.prevViewAngles[YAW])
	{
		auto pitch_result = std::to_chars(pitch, pitch + sizeof(pitch) - 1, va[PITCH]);
		auto yaw_result = std::to_chars(yaw, yaw + sizeof(yaw) - 1, va[YAW]);
		bool success = false;

		if (pitch_result.ec == std::errc() && yaw_result.ec == std::errc())
		{
			*pitch_result.ptr = '\0';
			*yaw_result.ptr = '\0';
			int retval = sprintf_s(CMD_BUFFER, "spt_setangles %s %s", pitch, yaw);

			if (retval > 0)
			{
				success = true;
				spt_tas_record.prevViewAngles[PITCH] = va[PITCH];
				spt_tas_record.prevViewAngles[YAW] = va[YAW];
				spt_tas_record.AddCommand(CMD_BUFFER);
			}
		}
		else
		{
			Warning("Failed to record viewangles (%f, %f)\n", va[PITCH], va[YAW]);
		}
	}
}

void TASRecordFeature::AddCommand(const char* buf)
{
	if (!recordedCommands.empty() && recordedCommands[recordedCommands.size() - 1].tick == currentTick)
	{
		recordedCommands[recordedCommands.size() - 1].cmd.push_back(';');
		recordedCommands[recordedCommands.size() - 1].cmd += buf;
	}
	else
	{
		RecordedCommand cmd;
		cmd.tick = currentTick;
		cmd.cmd = buf;
		recordedCommands.push_back(cmd);
	}
}

static void WriteToHandle(FILE* handle, const char* str)
{
	fwrite(str, 1, strlen(str), handle);
}

static void WriteInt(FILE* handle, int number)
{
	char BUFFER[32];
	sprintf(BUFFER, "%d", number);
	fwrite(BUFFER, 1, strlen(BUFFER), handle);
}

void TASRecordFeature::SaveToFile(const char* filepath)
{
	if (!recording && recordedCommands.empty())
	{
		Warning("No recording to write to file\n");
		return;
	}

	FILE* handle = fopen(filepath, "w");
	bool successful = true;

	if (handle)
	{
		const char* preamble = "version 2\nvars\nframes\n";
		WriteToHandle(handle, preamble);

		if (!recordedCommands.empty())
		{
			for (size_t i = 0; i < recordedCommands.size(); ++i)
			{
				auto& command = recordedCommands[i];

				WriteToHandle(handle, "<<<<<<<<<<|<<<<<<|<<<<<<<<|-|-|");

				if (i == recordedCommands.size() - 1)
				{
					fputc('1', handle);
				}
				else
				{
					auto& nextCommand = recordedCommands[i + 1];
					if (i == 0)
					{
						WriteInt(handle, nextCommand.tick - command.tick + 1);
					}
					else
					{
						WriteInt(handle, nextCommand.tick - command.tick);
					}
				}

				fputc('|', handle);
				WriteToHandle(handle, command.cmd.c_str());
				fputc('\n', handle);
			}
		}

		fclose(handle);
	}
	else
	{
		Warning("Failed to write to file %s\n", filepath);
	}

	if (successful)
	{
		recording = false;
		recordedCommands.clear();
	}
}

void TASRecordFeature::InitHooks()
{
	HOOK_FUNCTION(engine, CCommandBuffer__DequeueNextCommand);
}

void TASRecordFeature::LoadFeature()
{
	if (ProcessMovementPre_Signal.Works && FrameSignal.Works && ORIG_CCommandBuffer__DequeueNextCommand)
	{
		ProcessMovementPre_Signal.Connect(ProcessMovement_Hook);
		FrameSignal.Connect(Frame_Hook);

		InitCommand(tas_experimental_record);
		InitCommand(tas_experimental_record_save);
		InitCommand(tas_experimental_record_stop);
	}
}

void TASRecordFeature::UnloadFeature() {}

IMPL_HOOK_THISCALL(TASRecordFeature, bool, CCommandBuffer__DequeueNextCommand, CCommandBuffer*)
{
	bool rval = spt_tas_record.ORIG_CCommandBuffer__DequeueNextCommand(thisptr);

	if (spt_tas_record.recording && rval)
	{
		CCommand& cmd = const_cast<CCommand&>(thisptr->GetCommand());
		if (IsRecordable(cmd))
		{
			RemoveKeycodes(cmd);
			char* str = ToString(cmd);
			spt_tas_record.AddCommand(str);
		}
	}

	return rval;
}

#endif
