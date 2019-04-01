#pragma once

#ifndef OE

#ifdef P2
struct AudioState_t;
#endif

#include "inputsystem\buttoncode.h"

#include "iclientmode.h"
#include "vgui\ischeme.h"
#include "vgui_controls\controls.h"

namespace vgui
{
	IClientMode* GetClientMode();
	IScheme* GetScheme();
} // namespace vgui

#endif