#include "stdafx.h"
#include "..\feature.hpp"
#include "..\cvars.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\strafe\strafestuff.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "aim.hpp"
#include "generic.hpp"
#include "playerio.hpp"
#include "interfaces.hpp"
#include "signals.hpp"

ConVar tas_strafe("tas_strafe", "0", FCVAR_TAS_RESET);
ConVar tas_strafe_type(
    "tas_strafe_type",
    "0",
    FCVAR_TAS_RESET,
    "TAS strafe types:\n\t0 - Max acceleration strafing,\n\t1 - Max angle strafing.\n\t2 - Max accel strafing with a speed cap.\n\t3 - W strafing.\n");
ConVar tas_strafe_dir(
    "tas_strafe_dir",
    "3",
    FCVAR_TAS_RESET,
    "TAS strafe dirs:\n\t0 - to the left,\n\t1 - to the right,\n\t3 - to the yaw given in tas_strafe_yaw.");
ConVar tas_strafe_yaw("tas_strafe_yaw", "", FCVAR_TAS_RESET, "Yaw to strafe to with tas_strafe_dir = 3.");
ConVar tas_strafe_buttons(
    "tas_strafe_buttons",
    "",
    FCVAR_TAS_RESET,
    "Sets the strafing buttons. The format is 4 digits: \"<AirLeft> <AirRight> <GroundLeft> <GroundRight>\". The default (auto-detect) is empty string: \"\".\n"
    "Table of buttons:\n\t0 - W\n\t1 - WA\n\t2 - A\n\t3 - SA\n\t4 - S\n\t5 - SD\n\t6 - D\n\t7 - WD\n");
ConVar tas_strafe_vectorial("tas_strafe_vectorial",
                            "0",
                            FCVAR_TAS_RESET,
                            "Determines if strafing uses vectorial strafing\n");
ConVar tas_strafe_vectorial_increment(
    "tas_strafe_vectorial_increment",
    "2.5",
    FCVAR_TAS_RESET,
    "Has no effect on tas_strafe_version >= 4. Determines how fast the player yaw angle moves towards the target yaw angle. 0 for no movement, 180 for instant snapping. Has no effect on strafing speed.\n");
ConVar tas_strafe_vectorial_offset("tas_strafe_vectorial_offset",
                                   "0",
                                   FCVAR_TAS_RESET,
                                   "Determines the target view angle offset from tas_strafe_yaw\n");
ConVar tas_strafe_vectorial_snap(
    "tas_strafe_vectorial_snap",
    "170",
    FCVAR_TAS_RESET,
    "Determines when the yaw angle snaps to the target yaw. Mainly used to prevent ABHing from resetting the yaw angle to the back on every jump.\n");
ConVar tas_strafe_allow_jump_override(
    "tas_strafe_allow_jump_override",
    "0",
    FCVAR_TAS_RESET,
    "Determines if the setyaw/pitch commands are ignored when jumping + TAS strafing. Primarily used in search mode for stucklaunches when the exact time of the jump isn't known prior to running the script.\n");
ConVar tas_strafe_capped_limit("tas_strafe_capped_limit",
                               "299.99",
                               FCVAR_TAS_RESET,
                               "Determines the speed cap while using capped strafing(type 4).\n");
ConVar tas_strafe_hull_is_line(
    "tas_strafe_hull_is_line",
    "0",
    FCVAR_TAS_RESET,
    "Treats the collision hull as a line for ground checks. A hack to fix ground detections while going through portals.");
ConVar tas_strafe_use_tracing(
    "tas_strafe_use_tracing",
    "1",
    FCVAR_TAS_RESET,
    "Use tracing for detecting the ground and whether or not the player can unduck. Only turn off to enable backwards compability with old scripts.");

ConVar tas_force_airaccelerate(
    "tas_force_airaccelerate",
    "",
    0,
    "Sets the value of airaccelerate used in TAS calculations. If empty, uses the value of sv_airaccelerate.\n\nShould be set to 15 for Portal.\n");
ConVar tas_force_wishspeed_cap(
    "tas_force_wishspeed_cap",
    "",
    0,
    "Sets the value of the wishspeed cap used in TAS calculations. If empty, uses the default value: 30.\n\nShould be set to 60 for Portal.\n");
ConVar tas_reset_surface_friction(
    "tas_reset_surface_friction",
    "1",
    0,
    "If enabled, the surface friction is assumed to be reset in the beginning of CategorizePosition().\n\nShould be set to 0 for Portal.\n");

ConVar tas_force_onground(
    "tas_force_onground",
    "0",
    FCVAR_TAS_RESET,
    "If enabled, strafing assumes the player is on ground regardless of what the prediction indicates. Useful for save glitch in Portal where the prediction always reports the player being in the air.\n");
ConVar tas_strafe_version("tas_strafe_version",
                          "4",
                          FCVAR_TAS_RESET,
                          "Strafe version. For backwards compatibility with old scripts.");

ConVar tas_strafe_afh_length("tas_strafe_afh_length", "0.0000000000000000001", FCVAR_TAS_RESET, "Magnitude of AFHs");
ConVar tas_strafe_afh("tas_strafe_afh", "0", FCVAR_TAS_RESET, "Should AFH?");
ConVar tas_strafe_lgagst(
    "tas_strafe_lgagst",
    "0",
    FCVAR_TAS_RESET,
    "If enabled, jumps automatically when it's faster to move in the air than on ground. Incomplete, intended use is for tas_strafe_glitchless only.\n");
ConVar tas_strafe_lgagst_minspeed(
    "tas_strafe_lgagst_minspeed",
    "30",
    FCVAR_TAS_RESET,
    "Prevents LGAGST from triggering when the player speed is below this value. The default should be fine.");
ConVar tas_strafe_lgagst_fullmaxspeed(
    "tas_strafe_lgagst_fullmaxspeed",
    "0",
    FCVAR_TAS_RESET,
    "If enabled, LGAGST assumes the player is standing regardless of the actual ducking state. Useful for when you land while crouching but intend to stand up immediately.\n");
ConVar tas_strafe_lgagst_min("tas_strafe_lgagst_min", "150", FCVAR_TAS_RESET, "");
ConVar tas_strafe_lgagst_max("tas_strafe_lgagst_max", "270", FCVAR_TAS_RESET, "");
ConVar tas_strafe_jumptype(
    "tas_strafe_jumptype",
    "1",
    FCVAR_TAS_RESET,
    "TAS jump strafe types:\n\t0 - Does nothing,\n\t1 - Looks directly opposite to desired direction (for games with ABH),\n\t2 - Looks in desired direction (games with speed boost upon jumping but no ABH),\n\t3 - Looks in direction that results in greatest speed loss (for glitchless TASes on game with ABH).\n");
ConVar tas_strafe_autojb("tas_strafe_autojb",
                         "0",
                         FCVAR_TAS_RESET,
                         "ONLY JUMPBUGS WHEN POSSIBLE. IF IT DOESN'T JUMPBUG IT'S NOT POSSIBLE.\n");
ConVar tas_script_printvars("tas_script_printvars",
                            "1",
                            0,
                            "Prints variable information when running .srctas scripts.\n");
ConVar tas_script_savestates("tas_script_savestates", "1", 0, "Enables/disables savestates in .srctas scripts.\n");
ConVar tas_script_onsuccess("tas_script_onsuccess", "", 0, "Commands to be executed when a search concludes.\n");
ConVar tas_anglespeed("tas_anglespeed",
                      "5",
                      FCVAR_CHEAT,
                      "Determines the speed of angle changes when using tas_aim or when TAS strafing\n");

#ifdef OE
ConVar y_spt_gamedir(
    "y_spt_gamedir",
    "",
    0,
    "Sets the game directory, that is used for loading tas scripts and tests. Use the full path for the folder e.g. C:\\Steam\\steamapps\\sourcemods\\hl2oe\\\n");
#endif

typedef void(__fastcall* _AdjustAngles)(void* thisptr, int edx, float frametime);

// Enables TAS strafing and view related functionality
class TASFeature : public Feature
{
private:
	void __fastcall HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime);
	static void __fastcall HOOKED_AdjustAngles(void* thisptr, int edx, float frametime);
	void Strafe(float* va, bool yawChanged);

private:
	_AdjustAngles ORIG_AdjustAngles;
	bool tasAddressesWereFound;

protected:
	virtual bool ShouldLoadFeature() override
	{
		return interfaces::engine != nullptr;
	}

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static TASFeature spt_tas;

void TASFeature::InitHooks()
{
	HOOK_FUNCTION(client, AdjustAngles);
}

void TASFeature::LoadFeature()
{
	AfterFramesSignal.Connect(&scripts::g_TASReader, &scripts::SourceTASReader::OnAfterFrames);
	tasAddressesWereFound = ORIG_AdjustAngles && spt_playerio.PlayerIOAddressesFound();

	if (!tasAddressesWereFound)
		Warning("The full game TAS solutions are not available.\n");
}

void TASFeature::UnloadFeature() {}

void __fastcall TASFeature::HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime)
{
	spt_playerio.Set_cinput_thisptr(thisptr);
	ORIG_AdjustAngles(thisptr, edx, frametime);

	if (!spt_playerio.pCmd)
		return;

	float va[3];
	bool yawChanged = false;
	EngineGetViewAngles(va);
	spt_aim.HandleAiming(va, yawChanged);

	if (tasAddressesWereFound && tas_strafe.GetBool())
	{
		Strafe(va, yawChanged);
	}

	EngineSetViewAngles(va);
	spt_generic.AdjustAngles_hook();
}

void TASFeature::Strafe(float* va, bool yawChanged)
{
	auto vars = spt_playerio.GetMovementVars();
	auto pl = spt_playerio.GetPlayerData();

	bool jumped = false;

	auto btns = Strafe::StrafeButtons();
	bool usingButtons = (sscanf(tas_strafe_buttons.GetString(),
	                            "%hhu %hhu %hhu %hhu",
	                            &btns.AirLeft,
	                            &btns.AirRight,
	                            &btns.GroundLeft,
	                            &btns.GroundRight)
	                     == 4);
	auto type = static_cast<Strafe::StrafeType>(tas_strafe_type.GetInt());
	auto dir = static_cast<Strafe::StrafeDir>(tas_strafe_dir.GetInt());

	Strafe::ProcessedFrame out;
	out.Jump = false;

	if (!vars.CantJump && vars.OnGround)
	{
		if (tas_strafe_lgagst.GetBool())
		{
			bool jump = Strafe::LgagstJump(pl, vars);
			if (jump)
			{
				vars.OnGround = false;
				out.Jump = true;
				jumped = true;
			}
		}

		if (spt_playerio.TryJump())
		{
			vars.OnGround = false;
			jumped = true;
		}
	}

	Strafe::Friction(pl, vars.OnGround, vars);

	if (tas_strafe_vectorial
	        .GetBool()) // Can do vectorial strafing even with locked camera, provided we are not jumping
		Strafe::StrafeVectorial(pl,
		                        vars,
		                        jumped,
		                        type,
		                        dir,
		                        tas_strafe_yaw.GetFloat(),
		                        va[YAW],
		                        out,
		                        yawChanged);
	else if (!yawChanged) // not changing yaw, can do regular strafe
		Strafe::Strafe(
		    pl, vars, jumped, type, dir, tas_strafe_yaw.GetFloat(), va[YAW], out, btns, usingButtons);

	spt_playerio.SetTASInput(va, out);
}

void __fastcall TASFeature::HOOKED_AdjustAngles(void* thisptr, int edx, float frametime)
{
	spt_tas.HOOKED_AdjustAngles_Func(thisptr, edx, frametime);
}

CON_COMMAND(
    tas_script_load,
    "Loads and executes an .srctas script. If an extra ticks argument is given, the script is played back at maximal FPS and without rendering until that many ticks before the end of the script. Usage: tas_load_script [script] [ticks]")
{
	if (args.ArgC() == 2)
		scripts::g_TASReader.ExecuteScript(args.Arg(1));
	else if (args.ArgC() == 3)
	{
		scripts::g_TASReader.ExecuteScriptWithResume(args.Arg(1), std::stoi(args.Arg(2)));
	}
	else
		Msg("Loads and executes an .srctas script. Usage: tas_load_script [script]\n");
}

CON_COMMAND(tas_script_search, "Starts a variable search for an .srctas script. Usage: tas_load_search [script]")
{
	if (args.ArgC() > 1)
		scripts::g_TASReader.StartSearch(args.Arg(1));
	else
		Msg("Starts a variable search for an .srctas script. Usage: tas_load_search [script]\n");
}

CON_COMMAND(tas_script_result_success, "Signals a successful result in a variable search.")
{
	scripts::g_TASReader.SearchResult(scripts::SearchResult::Success);
}

CON_COMMAND(tas_script_result_fail, "Signals an unsuccessful result in a variable search.")
{
	scripts::g_TASReader.SearchResult(scripts::SearchResult::Fail);
}

CON_COMMAND(tas_script_result_stop, "Signals a stop in a variable search.")
{
	scripts::g_TASReader.SearchResult(scripts::SearchResult::NoSearch);
}
