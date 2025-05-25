#pragma once

#include "tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

namespace player_trace::tr_imgui
{
	void PlayerTabCallback(tr_tick activeTick);
	void EntityTabCallback(tr_tick activeTick);
	void PortalTabCallback(tr_tick activeTick);

} // namespace player_trace::tr_imgui

#endif