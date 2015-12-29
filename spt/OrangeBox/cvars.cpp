#include "convar.h"

ConVar y_spt_pause("y_spt_pause", "1", FCVAR_ARCHIVE);
ConVar y_spt_motion_blur_fix("y_spt_motion_blur_fix", "0");
ConVar y_spt_autojump("y_spt_autojump", "0", FCVAR_ARCHIVE);
ConVar y_spt_additional_abh("y_spt_additional_abh", "0", FCVAR_ARCHIVE);
ConVar y_spt_stucksave("y_spt_stucksave", "");
ConVar tas_strafe("tas_strafe", "0");
ConVar tas_strafe_yaw("tas_strafe_yaw", "");
ConVar tas_strafe_buttons("tas_strafe_buttons", "", 0, "Sets the strafing buttons. The format is 4 digits: \"<AirLeft> <AirRight> <GroundLeft> <GroundRight>\". The default (auto-detect) is empty string: \"\".\n"
															   "Table of buttons:\n\t0 - W\n\t1 - WA\n\t2 - A\n\t3 - SA\n\t4 - S\n\t5 - SD\n\t6 - D\n\t7 - WD\n");
//ConVar tas_strafe_lgagst("tas_strafe_lgagst", "1");
//ConVar tas_strafe_lgagst_minspeed("tas_strafe_lgagst_minspeed", "30");
//ConVar tas_strafe_lgagst_fullmaxspeed("tas_strafe_lgagst_fullmaxspeed", "0");

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
