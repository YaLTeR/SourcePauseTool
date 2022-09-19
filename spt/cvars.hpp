#pragma once

#include "convar.hpp"

#define FCVAR_TAS_RESET (1 << 31)

void Cvar_RegisterSPTCvars();
void Cvar_UnregisterSPTCvars();
void Cvar_InitConCommandBase(ConCommandBase& concommand, void* owner);
void FormatConCmd(const char* fmt, ...);

extern ConVar y_spt_pause;
extern ConVar y_spt_motion_blur_fix;
extern ConVar y_spt_autojump;
extern ConVar y_spt_jumpboost;
extern ConVar y_spt_aircontrol;
extern ConVar y_spt_focus_nosleep;
extern ConVar y_spt_stucksave;
extern ConVar y_spt_piwsave;
extern ConVar y_spt_pause_demo_on_tick;
extern ConVar y_spt_block_demo_cmd;
extern ConVar y_spt_on_slide_pause_for;
extern ConVar y_spt_prevent_vag_crash;
extern ConVar y_spt_disable_tone_map_reset;
extern ConVar y_spt_draw_seams;
extern ConVar y_spt_vag_search_portal;
extern ConVar y_spt_vag_trace;
extern ConVar y_spt_vag_target;
extern ConVar y_spt_vag_trace_portal;
extern ConVar y_spt_cam_control;
extern ConVar y_spt_cam_drive;
extern ConVar y_spt_cam_path_draw;

extern ConVar tas_strafe;
extern ConVar tas_strafe_type;
extern ConVar tas_strafe_dir;
extern ConVar tas_strafe_scale;
extern ConVar tas_strafe_yaw;
extern ConVar tas_strafe_buttons;
extern ConVar tas_strafe_afh;
extern ConVar tas_strafe_afh_length;

extern ConVar tas_strafe_vectorial;
extern ConVar tas_strafe_vectorial_increment;
extern ConVar tas_strafe_vectorial_offset;
extern ConVar tas_strafe_vectorial_snap;
extern ConVar tas_strafe_allow_jump_override;

extern ConVar tas_force_airaccelerate;
extern ConVar tas_force_wishspeed_cap;
extern ConVar tas_reset_surface_friction;

extern ConVar tas_force_onground;

extern ConVar tas_anglespeed;
extern ConVar tas_pause;
extern ConVar tas_log;
extern ConVar tas_strafe_lgagst;
extern ConVar tas_strafe_lgagst_max;
extern ConVar tas_strafe_lgagst_min;
extern ConVar tas_strafe_lgagst_minspeed;
extern ConVar tas_strafe_lgagst_fullmaxspeed;
extern ConVar tas_strafe_jumptype;
extern ConVar tas_strafe_capped_limit;
extern ConVar tas_strafe_hull_is_line;
extern ConVar tas_strafe_use_tracing;
extern ConVar tas_strafe_autojb;
extern ConVar tas_strafe_version;

extern ConVar tas_script_printvars;
extern ConVar tas_script_savestates;
extern ConVar tas_script_onsuccess;

extern ConVar _y_spt_autojump_ensure_legit;
extern ConVar _y_spt_afterframes_reset_on_server_activate;
extern ConVar _y_spt_anglesetspeed;
extern ConVar _y_spt_pitchspeed;
extern ConVar _y_spt_yawspeed;
extern ConVar _y_spt_force_fov;

extern ConVar y_spt_hud_velocity;
extern ConVar y_spt_hud_flags;
extern ConVar y_spt_hud_moveflags;
extern ConVar y_spt_hud_movecollideflags;
extern ConVar y_spt_hud_collisionflags;
extern ConVar y_spt_hud_accel;
extern ConVar y_spt_hud_script_length;
extern ConVar y_spt_hud_portal_bubble;
extern ConVar y_spt_hud_decimals;
extern ConVar y_spt_hud_vars;
extern ConVar y_spt_hud_ag_sg_tester;
extern ConVar y_spt_hud_ent_info;
extern ConVar y_spt_hud_left;
extern ConVar y_spt_hud_oob;
extern ConVar y_spt_hud_isg;

extern ConVar y_spt_ihud;
extern ConVar y_spt_ihud_grid_size;
extern ConVar y_spt_ihud_grid_padding;
extern ConVar y_spt_ihud_x;
extern ConVar y_spt_ihud_y;

extern ConVar _y_spt_overlay;
extern ConVar _y_spt_overlay_type;
extern ConVar _y_spt_overlay_portal;
extern ConVar _y_spt_overlay_width;
extern ConVar _y_spt_overlay_fov;
extern ConVar _y_spt_overlay_swap;

#ifdef OE
extern ConVar y_spt_gamedir;
#endif

#if defined(SSDK2007) || defined(SSDK2013)
extern ConCommand y_spt_set_isg;
#endif

extern ConVar* _viewmodel_fov;
extern ConCommand* _record;
extern ConCommand* _stop;
extern ConVar* _sv_accelerate;
extern ConVar* _sv_airaccelerate;
extern ConVar* _sv_friction;
extern ConVar* _sv_maxspeed;
extern ConVar* _sv_stopspeed;
extern ConVar* _sv_stepsize;
extern ConVar* _sv_gravity;
extern ConVar* _sv_maxvelocity;
extern ConVar* _sv_bounce;
extern ConVar* _sv_cheats;
