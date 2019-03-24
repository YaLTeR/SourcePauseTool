#pragma once

#ifndef OE

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