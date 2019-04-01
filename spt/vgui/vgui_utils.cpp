#include "stdafx.h"
#include "vgui_utils.hpp"
#include "..\OrangeBox\modules\ClientDLL.hpp"
#include "..\OrangeBox\modules.hpp"
#include "tier3\tier3.h"
#include "tier2\tier2.h"
#include "tier1\tier1.h"

namespace vgui
{
#ifndef OE
	IClientMode* GetClientMode()
	{
		return (IClientMode*)clientDLL.ORIG_GetClientModeNormal();
	}
	IScheme * GetScheme()
	{
		return scheme()->GetIScheme(scheme()->GetDefaultScheme());
	}
#endif
}