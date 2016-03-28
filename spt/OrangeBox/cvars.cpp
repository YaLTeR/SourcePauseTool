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

ConVar _y_spt_autojump_ensure_legit("_y_spt_autojump_ensure_legit", "1", FCVAR_ARCHIVE);
ConVar _y_spt_afterframes_reset_on_server_activate("_y_spt_afterframes_reset_on_server_activate", "1", FCVAR_ARCHIVE);
ConVar _y_spt_pitchspeed("_y_spt_pitchspeed", "0");
ConVar _y_spt_yawspeed("_y_spt_yawspeed", "0");
ConVar _y_spt_force_90fov("_y_spt_force_90fov", "0");

ConVar *_viewmodel_fov = nullptr;
