#include "convar.h"

ConVar y_spt_pause("y_spt_pause", "1", FCVAR_ARCHIVE);
ConVar y_spt_motion_blur_fix("y_spt_motion_blur_fix", "0");
ConVar y_spt_autojump("y_spt_autojump", "0", FCVAR_ARCHIVE);
ConVar y_spt_additional_abh("y_spt_additional_abh", "0", FCVAR_ARCHIVE);
ConVar y_spt_stucksave("y_spt_stucksave", "");
ConVar y_spt_pause_demo_on_tick(
	"y_spt_pause_demo_on_tick",
	"0",
	0,
	"Invokes the demo_pause command on the specified demo tick.\n"
	"Zero means do nothing.\n"
	"x > 0 means pause the demo at tick number x.\n"
	"x < 0 means pause the demo at <demo length> + x, so for example -1 will pause the demo at the last tick.\n\n"
	"Demos ending with changelevels report incorrect length; you can obtain the correct demo length using listdemo and then set this CVar to <demo length> - 1 manually.");

ConVar tas_strafe("tas_strafe", "0");
ConVar tas_strafe_type("tas_strafe_type", "0", 0, "TAS strafe types:\n\t0 - Max acceleration strafing,\n\t1 - Max angle strafing.\n");
ConVar tas_strafe_dir("tas_strafe_dir", "3", 0, "TAS strafe dirs:\n\t0 - to the left,\n\t1 - to the right,\n\t3 - to the yaw given in tas_strafe_yaw.");
ConVar tas_strafe_yaw("tas_strafe_yaw", "", 0, "Yaw to strafe to with tas_strafe_dir = 3.");
ConVar tas_strafe_buttons("tas_strafe_buttons", "", 0, "Sets the strafing buttons. The format is 4 digits: \"<AirLeft> <AirRight> <GroundLeft> <GroundRight>\". The default (auto-detect) is empty string: \"\".\n"
															   "Table of buttons:\n\t0 - W\n\t1 - WA\n\t2 - A\n\t3 - SA\n\t4 - S\n\t5 - SD\n\t6 - D\n\t7 - WD\n");

ConVar tas_force_airaccelerate("tas_force_airaccelerate", "", 0, "Sets the value of airaccelerate used in TAS calculations. If empty, uses the value of sv_airaccelerate.\n\nShould be set to 15 for Portal.\n");
ConVar tas_force_wishspeed_cap("tas_force_wishspeed_cap", "", 0, "Sets the value of the wishspeed cap used in TAS calculations. If empty, uses the default value: 30.\n\nShould be set to 60 for Portal.\n");
ConVar tas_reset_surface_friction("tas_reset_surface_friction", "1", 0, "If enabled, the surface friction is assumed to be reset in the beginning of CategorizePosition().\n\nShould be set to 0 for Portal.\n");

ConVar tas_force_onground("tas_force_onground", "0", 0, "If enabled, strafing assumes the player is on ground regardless of what the prediction indicates. Useful for save glitch in Portal where the prediction always reports the player being in the air.\n");

ConVar tas_log("tas_log", "0", 0, "If enabled, dumps a whole bunch of different stuff into the console.\n");
ConVar tas_strafe_lgagst("tas_strafe_lgagst", "0", 0, "If enabled, only jumped from the ground when it's faster to move in the air. Only recommended in OrangeBox Engine games when _y_spt_glitchless_bhop_enabled = 1. /n");
ConVar tas_strafe_lgagst_minspeed("tas_strafe_lgagst_minspeed", "30", 0, "Prevents lgagst from calculating below this speed. Not reccommended to change beyond the player's crouching speed. \n");
ConVar tas_strafe_lgagst_fullmaxspeed("tas_strafe_lgagst_fullmaxspeed", "0", 0, "When enabled, uses the maxspeed to determine if the player is touching the ground. Useful for areas where you land ducked but want to unduck and continue groundstrafing while still standing. /n");
ConVar tas_strafe_glitchless("tas_strafe_glitchless", "0", 0, "When set to 1, disables ABH and looks in the exact direction of velocity to prevent any accidental speed gain. Only recommended for use in OrangeBox engine games. /n");
	
ConVar _y_spt_autojump_ensure_legit("_y_spt_autojump_ensure_legit", "1", FCVAR_ARCHIVE);
ConVar _y_spt_afterframes_reset_on_server_activate("_y_spt_afterframes_reset_on_server_activate", "1", FCVAR_ARCHIVE);
ConVar _y_spt_pitchspeed("_y_spt_pitchspeed", "0");
ConVar _y_spt_yawspeed("_y_spt_yawspeed", "0");
ConVar _y_spt_force_90fov("_y_spt_force_90fov", "0");

ConVar *_viewmodel_fov = nullptr;
ConVar *_sv_accelerate = nullptr;
ConVar *_sv_airaccelerate = nullptr;
ConVar *_sv_friction = nullptr;
ConVar *_sv_maxspeed = nullptr;
ConVar *_sv_stopspeed = nullptr;
