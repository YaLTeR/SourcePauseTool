#pragma once

#include "engine\iserverplugin.h"
#include "tier2\tier2.h"


void calculate_sg_position(Vector& new_player_origin, QAngle& new_player_angles);
void calculate_offset_player(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles);
int getPortal(const char* Arg, bool verbose);
