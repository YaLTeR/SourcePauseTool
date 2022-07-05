#include "stdafx.h"

#include "..\features\generic.hpp"
#include "interfaces.hpp"
#include "vgui_utils.hpp"

namespace vgui
{
#ifndef OE
	IClientMode* GetClientMode()
	{
		if (spt_generic.ORIG_GetClientModeNormal)
			return (IClientMode*)spt_generic.ORIG_GetClientModeNormal();
		else
			return nullptr;
	}
	IScheme* GetScheme()
	{
		return interfaces::scheme->GetIScheme(interfaces::scheme->GetDefaultScheme());
	}
#endif
} // namespace vgui