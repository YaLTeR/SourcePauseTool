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
