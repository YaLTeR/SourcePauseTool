#pragma once

#ifndef OE

#ifdef P2
struct AudioState_t;
#endif

class IClientMode;

#include "inputsystem\buttoncode.h"
#include "vgui\ischeme.h"
#include "vgui_controls\controls.h"

namespace vgui
{
	IClientMode* GetClientMode();
	IScheme* GetScheme();
} // namespace vgui

#endif
