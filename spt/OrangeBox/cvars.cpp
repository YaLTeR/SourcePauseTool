#include "cvars.hpp"

#include "convar.h"
#include "modules.hpp"


ConVar y_spt_hdtf_uncap("y_spt_hdtf_uncap", "0", FCVAR_ARCHIVE, "Enables/Disables speed cap");
ConVar y_spt_hdtf_viewroll("y_spt_hdtf_viewroll", "1", FCVAR_ARCHIVE, "Enables/Disables slight view rolling when turning");
ConVar y_spt_pause("y_spt_pause", "0", FCVAR_ARCHIVE);
ConVar y_spt_motion_blur_fix("y_spt_motion_blur_fix", "0");
ConVar y_spt_autojump("y_spt_autojump", "0", FCVAR_ARCHIVE);
ConVar y_spt_additional_jumpboost("y_spt_additional_jumpboost",
                                  "0",
                                  0,
                                  "1 = Add in ABH movement mechanic, 2 = Add in OE movement mechanic.\n");
ConVar y_spt_stucksave("y_spt_stucksave", "", FCVAR_TAS_RESET, "Automatically saves when you get stuck in a prop.\n");
ConVar y_spt_piwsave("y_spt_piwsave", "", FCVAR_TAS_RESET, "Automatically saves when you push a prop into a wall.\n");
ConVar y_spt_pause_demo_on_tick(
    "y_spt_pause_demo_on_tick",
    "0",
    0,
    "Invokes the demo_pause command on the specified demo tick.\n"
    "Zero means do nothing.\n"
    "x > 0 means pause the demo at tick number x.\n"
    "x < 0 means pause the demo at <demo length> + x, so for example -1 will pause the demo at the last tick.\n\n"
    "Demos ending with changelevels report incorrect length; you can obtain the correct demo length using listdemo and then set this CVar to <demo length> - 1 manually.");
ConVar y_spt_on_slide_pause_for("y_spt_on_slide_pause_for",
                                "0",
                                0,
                                "Whenever sliding occurs in DMoMM, pause for this many ticks.");
ConVar y_spt_disable_tone_map_reset(
    "y_spt_disable_tone_map_reset",
    "0",
    0,
    "Prevents the tone map getting reset (during each load), useful for keeping colors the same between demos");
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

ConVar tas_pause("tas_pause", "0", 0, "Does a pause where you can look around when the game is paused.\n");
ConVar tas_log("tas_log", "0", 0, "If enabled, dumps a whole bunch of different stuff into the console.\n");
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
ConVar _y_spt_autojump_ensure_legit("_y_spt_autojump_ensure_legit", "1", FCVAR_CHEAT);
ConVar _y_spt_afterframes_reset_on_server_activate("_y_spt_afterframes_reset_on_server_activate", "1", FCVAR_ARCHIVE);
ConVar _y_spt_anglesetspeed(
    "_y_spt_anglesetspeed",
    "360",
    FCVAR_TAS_RESET,
    "Determines how fast the view angle can move per tick while doing _y_spt_setyaw/_y_spt_setpitch.\n");
ConVar _y_spt_pitchspeed("_y_spt_pitchspeed", "0", FCVAR_TAS_RESET);
ConVar _y_spt_yawspeed("_y_spt_yawspeed", "0", FCVAR_TAS_RESET);
ConVar _y_spt_force_fov("_y_spt_force_fov", "0");

ConVar y_spt_hud_velocity("y_spt_hud_velocity", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the velocity hud.\n");
ConVar y_spt_hud_flags("y_spt_hud_flags", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the flags hud.\n");
ConVar y_spt_hud_moveflags("y_spt_hud_moveflags", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the move type hud.\n");
ConVar y_spt_hud_movecollideflags("y_spt_hud_movecollideflags", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the move collide hud.\n");
ConVar y_spt_hud_collisionflags("y_spt_hud_collisionflags", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the collision group hud.\n");
ConVar y_spt_hud_accel("y_spt_hud_accel", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the acceleration hud.\n");
ConVar y_spt_hud_script_length("y_spt_hud_script_progress", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the script progress hud.\n");
ConVar y_spt_hud_portal_bubble("y_spt_hud_portal_bubble", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on portal bubble index hud.\n");
ConVar y_spt_hud_decimals("y_spt_hud_decimals",
                          "2",
                          FCVAR_CHEAT | FCVAR_SPT_HUD,
                          "Determines the number of decimals in the SPT HUD.\n");
ConVar y_spt_hud_vars("y_spt_hud_vars", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Turns on the movement vars HUD.\n");
ConVar y_spt_hud_ag_sg_tester("y_spt_hud_ag_sg_tester",
                              "0",
                              FCVAR_CHEAT | FCVAR_SPT_HUD,
                              "Tests if angle glitch will save glitch you.\n");
ConVar y_spt_hud_ent_info(
    "y_spt_hud_ent_info",
    "",
    FCVAR_CHEAT,
    "Display entity info on HUD. Format is \"[ent index],[prop regex],[prop regex],...,[prop regex];[ent index],...,[prop regex]\".\n");
ConVar y_spt_hud_left("y_spt_hud_left", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "When set to 1, displays SPT HUD on the left.\n");
ConVar y_spt_hud_oob("y_spt_hud_oob", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Is the player OoB?");
ConVar y_spt_hud_isg("y_spt_hud_isg", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "Is the ISG flag set?\n");
ConVar y_spt_hud_vec_velocity("y_spt_hud_vec_velocity", "0", FCVAR_CHEAT | FCVAR_SPT_HUD, "");
ConVar y_spt_prevent_vag_crash(
    "y_spt_prevent_vag_crash",
    "0",
    FCVAR_CHEAT | FCVAR_DONTRECORD,
    "Prevents the game from crashing from too many recursive teleports (useful when searching for vertical angle glitches).\n");
ConVar y_spt_hud_havok_velocity("y_spt_hud_havok_velocity", "0", FCVAR_CHEAT, "Show havok hitbox velocity");
ConVar y_spt_hud_enable("y_spt_hud_enable", "1", FCVAR_CHEAT, "Allow drawing hud elements");
ConVar _y_spt_overlay("_y_spt_overlay",
                      "0",
                      FCVAR_CHEAT,
                      "Enables the overlay camera in the top left of the screen.\n");
ConVar _y_spt_overlay_type(
    "_y_spt_overlay_type",
    "0",
    FCVAR_CHEAT,
    "Overlay type. 0 = save glitch body, 1 = angle glitch simulation, 2 = rear view mirror, 3 = havok view mirror.\n");
ConVar _y_spt_overlay_portal(
    "_y_spt_overlay_portal",
    "auto",
    FCVAR_CHEAT,
    "Chooses the portal for the overlay camera. Valid options are blue/orange/portal index. For the SG camera this is the portal you save glitch on, for angle glitch simulation this is the portal you enter.\n");
ConVar _y_spt_overlay_width("_y_spt_overlay_width",
                            "480",
                            FCVAR_CHEAT,
                            "Determines the width of the overlay. Height is determined automatically from width.\n",
                            true,
                            20.0f,
                            true,
                            3840.0f);
ConVar _y_spt_overlay_fov("_y_spt_overlay_fov",
                          "90",
                          FCVAR_CHEAT,
                          "Determines the FOV of the overlay.\n",
                          true,
                          5.0f,
                          true,
                          140.0f);
ConVar _y_spt_overlay_swap("_y_spt_overlay_swap", "0", FCVAR_CHEAT, "Swap alternate view and main screen?\n");

#ifdef OE
ConVar y_spt_gamedir(
    "y_spt_gamedir",
    "",
    0,
    "Sets the game directory, that is used for loading tas scripts and tests. Use the full path for the folder e.g. C:\\Steam\\steamapps\\sourcemods\\hl2oe\\\n");
#endif

#ifndef OE
void CC_Set_ISG(const CCommand& args)
{
	if (vphysicsDLL.isgFlagPtr)
		*vphysicsDLL.isgFlagPtr = args.ArgC() == 1 || atoi(args[1]);
	else
		Warning("y_spt_set_isg has no effect\n");
}

ConCommand y_spt_set_isg("y_spt_set_isg",
                         CC_Set_ISG,
                         "Sets the state of ISG in the game (1 or 0), no arguments means 1",
                         FCVAR_DONTRECORD | FCVAR_CHEAT);
#endif

ConVar* _viewmodel_fov = nullptr;
ConVar* _sv_accelerate = nullptr;
ConVar* _sv_airaccelerate = nullptr;
ConVar* _sv_friction = nullptr;
ConVar* _sv_maxspeed = nullptr;
ConVar* _sv_stopspeed = nullptr;
ConVar* _sv_stepsize = nullptr;
ConVar* _sv_gravity = nullptr;
ConVar* _sv_maxvelocity = nullptr;
ConVar* _sv_bounce = nullptr;
