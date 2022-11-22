#pragma once

class IClientMode;

#include "inputsystem\buttoncode.h"
#include "vgui\ischeme.h"
#include "vgui_controls\controls.h"

namespace vgui
{
	IClientMode* GetClientMode();
	IScheme* GetScheme();
} // namespace vgui
