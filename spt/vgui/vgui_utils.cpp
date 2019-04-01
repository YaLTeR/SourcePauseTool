#include "stdafx.h"

#include "..\OrangeBox\modules.hpp"
#include "..\OrangeBox\modules\ClientDLL.hpp"
#include "tier1\tier1.h"
#include "tier2\tier2.h"
#include "tier3\tier3.h"
#include "vgui_utils.hpp"

namespace vgui
{
#ifndef OE
	IClientMode* GetClientMode()
	{
		return (IClientMode*)clientDLL.ORIG_GetClientModeNormal();
	}
	IScheme* GetScheme()
	{
		return scheme()->GetIScheme(scheme()->GetDefaultScheme());
	}
#endif
} // namespace vgui