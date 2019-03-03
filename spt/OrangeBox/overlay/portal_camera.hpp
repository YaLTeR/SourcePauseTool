#pragma once

#ifdef SSDK2007
#include "engine\iserverplugin.h"
#include "tier2\tier2.h"
#include <string>
#include "icliententity.h"

bool invalidPortal(IClientEntity* portal);
IClientEntity* GetEnviromentPortal();
void calculateAGPosition(Vector& new_player_origin, QAngle& new_player_angles);
void calculateAGOffsetPortal(IClientEntity* enter_portal, IClientEntity* exit_portal, Vector& new_player_origin, QAngle& new_player_angles);
void calculateSGPosition(Vector& new_player_origin, QAngle& new_player_angles);
void calculateOffsetPlayer(IClientEntity* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles);
std::wstring calculateWillAGSG(Vector& new_player_origin, QAngle& new_player_angles);
#endif