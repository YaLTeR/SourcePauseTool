#pragma once

#ifdef SSDK2007

#include "engine\iserverplugin.h"
#include "tier2\tier2.h"


void calculate_ag_position(Vector& new_player_origin, QAngle& new_player_angles);
void calculate_offset_portal(edict_t* enter_portal, edict_t* exit_portal, Vector& new_player_origin, QAngle& new_player_angles);
void calculate_sg_position(Vector& new_player_origin, QAngle& new_player_angles);
void calculate_offset_player(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles);
int getPortal(const char* Arg, bool verbose);

#endif