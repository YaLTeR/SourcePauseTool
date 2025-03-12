#pragma once

#if !defined(OE) && !defined(BMS)

#define SPT_PORTAL_UTILS

#include <string>
#include "engine\iserverplugin.h"
#include "icliententity.h"
#include "tier2\tier2.h"
#include "spt\utils\ent_list.hpp"

void calculateAGPosition(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles);
void calculateAGOffsetPortal(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles);
void transformThroughPortal(const utils::PortalInfo* portal,
                            const Vector& start_pos,
                            const QAngle start_angles,
                            Vector& transformed_origin,
                            QAngle& transformed_angles);
void calculateSGPosition(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles);
void calculateOffsetPlayer(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles);
std::wstring calculateWillAGSG(const utils::PortalInfo* portal, Vector& new_player_origin, QAngle& new_player_angles);

/*
* getPortal should be used for all portal selection cvars (e.g. spt_overlay_type). Before, all such
* features used their own logic and behaved differently. This is especially annoying from a ImGui
* perspective - ideally all the logic shoud be the same so that there can be a single function for
* a portal selection widget. The macros here should be used for the description of these cvars.
* 
* Allowed options are: blue/orange/auto/env/overlay/<index>.
*/

// prepend to the rest of the description if getPortal(allowAuto=true) is used
#define SPT_PORTAL_SELECT_DESCRIPTION_AUTO_PREFIX \
	"  - auto: prioritize the player's portal environment, otherwise uses last drawn portal\n"

// prepend to the rest of the description if getPortal("overlay") is allowed
#define SPT_PORTAL_SELECT_DESCRIPTION_OVERLAY_PREFIX "  - overlay: uses the same portal as spt_overlay_portal\n"

#define SPT_PORTAL_SELECT_DESCRIPTION \
	"  - env: draw using the player's portal environment\n" \
	"  - blue/orange: use a specific portal color\n" \
	"  - <index>: specify portal entity index"

const utils::PortalInfo* getPortal(const char* arg, bool onlyOpen, bool allowAuto = true);

#endif
