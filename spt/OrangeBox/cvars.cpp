#include "convar.h"

ConVar y_spt_pause("y_spt_pause", "1", FCVAR_ARCHIVE);
ConVar y_spt_motion_blur_fix("y_spt_motion_blur_fix", "0");
ConVar y_spt_autojump("y_spt_autojump", "0", FCVAR_ARCHIVE);
ConVar y_spt_additional_jumpboost("y_spt_additional_jumpboost", "0", 0, "1 = Add in ABH movement mechanic, 2 = Add in OE movement mechanic.\n");
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
ConVar y_spt_on_slide_pause_for("y_spt_on_slide_pause_for", "0", 0, "Whenever sliding occurs in DMoMM, pause for this many ticks.");

ConVar tas_strafe("tas_strafe", "0");
ConVar tas_strafe_type("tas_strafe_type", "0", 0, "TAS strafe types:\n\t0 - Max acceleration strafing,\n\t1 - Max angle strafing.\n");
ConVar tas_strafe_dir("tas_strafe_dir", "3", 0, "TAS strafe dirs:\n\t0 - to the left,\n\t1 - to the right,\n\t3 - to the yaw given in tas_strafe_yaw.");
ConVar tas_strafe_yaw("tas_strafe_yaw", "", 0, "Yaw to strafe to with tas_strafe_dir = 3.");
ConVar tas_strafe_buttons("tas_strafe_buttons", "", 0, "Sets the strafing buttons. The format is 4 digits: \"<AirLeft> <AirRight> <GroundLeft> <GroundRight>\". The default (auto-detect) is empty string: \"\".\n"
															   "Table of buttons:\n\t0 - W\n\t1 - WA\n\t2 - A\n\t3 - SA\n\t4 - S\n\t5 - SD\n\t6 - D\n\t7 - WD\n");
ConVar tas_strafe_vectorial("tas_strafe_vectorial", "0", 0, "Determines if strafing uses vectorial strafing\n");
ConVar tas_strafe_vectorial_increment("tas_strafe_vectorial_increment", "2.5", 0, "Determines how fast the player yaw angle moves towards the target yaw angle. 0 for no movement, 180 for instant snapping. Has no effect on strafing speed.\n");
ConVar tas_strafe_vectorial_offset("tas_strafe_vectorial_offset", "0", 0, "Determines the target view angle offset from tas_strafe_yaw\n");
ConVar tas_strafe_vectorial_snap("tas_strafe_vectorial_snap", "170", 0, "Determines when the yaw angle snaps to the target yaw. Mainly used to prevent ABHing from resetting the yaw angle to the back on every jump.\n");

ConVar tas_force_airaccelerate("tas_force_airaccelerate", "", 0, "Sets the value of airaccelerate used in TAS calculations. If empty, uses the value of sv_airaccelerate.\n\nShould be set to 15 for Portal.\n");
ConVar tas_force_wishspeed_cap("tas_force_wishspeed_cap", "", 0, "Sets the value of the wishspeed cap used in TAS calculations. If empty, uses the default value: 30.\n\nShould be set to 60 for Portal.\n");
ConVar tas_reset_surface_friction("tas_reset_surface_friction", "1", 0, "If enabled, the surface friction is assumed to be reset in the beginning of CategorizePosition().\n\nShould be set to 0 for Portal.\n");

ConVar tas_force_onground("tas_force_onground", "0", 0, "If enabled, strafing assumes the player is on ground regardless of what the prediction indicates. Useful for save glitch in Portal where the prediction always reports the player being in the air.\n");

ConVar tas_log("tas_log", "0", 0, "If enabled, dumps a whole bunch of different stuff into the console.\n");
ConVar tas_strafe_lgagst("tas_strafe_lgagst", "0", 0, "If enabled, jumps automatically when it's faster to move in the air than on ground. Incomplete, intended use is for tas_strafe_glitchless only.\n");
ConVar tas_strafe_lgagst_minspeed("tas_strafe_lgagst_minspeed", "30", 0, "Prevents LGAGST from triggering when the player speed is below this value. The default should be fine.");
ConVar tas_strafe_lgagst_fullmaxspeed("tas_strafe_lgagst_fullmaxspeed", "0", 0, "If enabled, LGAGST assumes the player is standing regardless of the actual ducking state. Useful for when you land while crouching but intend to stand up immediately.\n");
ConVar tas_strafe_jumptype("tas_strafe_jumptype", "1", 0, "TAS jump strafe types:\n\t0 - Does nothing,\n\t1 - Looks directly opposite to desired direction (for games with ABH),\n\t2 - Looks in desired direction (games with speed boost upon jumping but no ABH),\n\t3 - Looks in direction that results in greatest speed loss (for glitchless TASes on game with ABH).\n");
	
ConVar _y_spt_autojump_ensure_legit("_y_spt_autojump_ensure_legit", "1", FCVAR_ARCHIVE);
ConVar _y_spt_afterframes_reset_on_server_activate("_y_spt_afterframes_reset_on_server_activate", "1", FCVAR_ARCHIVE);
ConVar _y_spt_pitchspeed("_y_spt_pitchspeed", "0");
ConVar _y_spt_yawspeed("_y_spt_yawspeed", "0");
ConVar _y_spt_force_90fov("_y_spt_force_90fov", "0");
ConVar _y_spt_camera_sg("_y_spt_camera_sg", "0", 0, "Enables the save glitch camera in the top left of the screen.\n");
ConVar _y_spt_camera_portal("_y_spt_camera_portal", "blue", 0, "Chooses the portal for the camera. Valid options are blue/orange/portal index\n");

ConVar *_viewmodel_fov = nullptr;
ConVar *_sv_accelerate = nullptr;
ConVar *_sv_airaccelerate = nullptr;
ConVar *_sv_friction = nullptr;
ConVar *_sv_maxspeed = nullptr;
ConVar *_sv_stopspeed = nullptr;
