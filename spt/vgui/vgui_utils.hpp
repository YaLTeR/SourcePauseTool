#pragma once

#ifndef OE

#ifdef P2
struct AudioState_t;
#endif

#include "inputsystem\buttoncode.h"
#include "iclientmode.h"
#include "vgui_controls\controls.h"
#include "vgui\ischeme.h"

namespace vgui
{
	IClientMode* GetClientMode();
	IScheme* GetScheme();
}

#endif