#pragma once

#ifndef OE
#include <string>
#include "engine\iserverplugin.h"
#include "icliententity.h"
#include "tier2\tier2.h"

bool invalidPortal(IClientEntity* portal);
IClientEntity* GetEnvironmentPortal();
void calculateAGPosition(Vector& new_player_origin, QAngle& new_player_angles);
void calculateAGOffsetPortal(IClientEntity* enter_portal,
                             IClientEntity* exit_portal,
                             Vector& new_player_origin,
                             QAngle& new_player_angles);
void calculateSGPosition(Vector& new_player_origin, QAngle& new_player_angles);
void calculateOffsetPlayer(IClientEntity* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles);
std::wstring calculateWillAGSG(Vector& new_player_origin, QAngle& new_player_angles);
#endif