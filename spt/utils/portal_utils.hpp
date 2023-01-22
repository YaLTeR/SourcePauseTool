#pragma once

#if !defined(OE) && !defined(BMS)

#define SPT_PORTAL_UTILS

#include <string>
#include "engine\iserverplugin.h"
#include "icliententity.h"
#include "tier2\tier2.h"

IClientEntity* getPortal(const char* arg, bool verbose);
bool invalidPortal(IClientEntity* portal);
IClientEntity* GetEnvironmentPortal();
void calculateAGPosition(Vector& new_player_origin, QAngle& new_player_angles);
void calculateAGOffsetPortal(IClientEntity* enter_portal,
                             IClientEntity* exit_portal,
                             Vector& new_player_origin,
                             QAngle& new_player_angles);
void transformThroughPortal(IClientEntity* saveglitch_portal,
                            const Vector& start_pos,
                            const QAngle start_angles,
                            Vector& transformed_origin,
                            QAngle& transformed_angles);
void calculateSGPosition(Vector& new_player_origin, QAngle& new_player_angles);
void calculateOffsetPlayer(IClientEntity* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles);
std::wstring calculateWillAGSG(Vector& new_player_origin, QAngle& new_player_angles);

#endif
