#pragma once

#include "convar.hpp"

void Cvar_RegisterSPTCvars();
void Cvar_UnregisterSPTCvars();
void Cvar_InitConCommandBase(ConCommandBase& concommand, void* owner);
void FormatConCmd(const char* fmt, ...);

#ifdef SSDK2013
void ReplaceAutoCompleteSuggest();
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

/*
* Converts legacy SPT command names to new ones. Uses either a temporary string that is only
* valid until the next call, or allocates a new string that must be deallocated manually.
* 
* Examples:
* spt_hud_position -> spt_hud_position  (allocated=false)
* y_spt_hud_isg    -> spt_hud_isg       (allocated=false)
* +spt_duckspam    -> +spt_duckspam     (allocated=false)
* tas_pause        -> spt_tas_pause     (allocated=true)
* +y_spt_duckspam  -> +spt_duckspam     (allocated=true)
*/
const char* WrangleLegacyCommandName(const char* name, bool useTempStorage, bool* allocated);
