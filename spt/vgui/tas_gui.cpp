#include "stdafx.h"
#include "tas_gui.hpp"
#include "hud_element_helper.h"
#include "tier0/memdbgon.h"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "vgui_utils.hpp"
#include <exception>
#include "vgui\ivgui.h"
#include "vgui_controls\controls.h"

namespace vgui
{
	void OpenTASGUI()
	{
		try
		{
			Frame* fr = new Frame(GetClientMode()->GetViewport(), "test");
			fr->SetScheme(scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "SourceScheme"));
			fr->SetPos(100, 100);
			fr->SetSize(100, 100);
			fr->SetVisible(true);
			fr->SetTitle("My First Frame", true);
			fr->Activate();
		}
		catch (const std::exception& ex)
		{
			Msg("%s\n", ex.what());
		}
		catch (...)
		{
			Msg("Uncaught exception.\n");
		}
	}

	CHudImport::CHudImport(const char * pElementName)
	{
		auto parent = GetClientMode()->GetViewport();
		SetParent(parent);
		SetVisible(true);
		SetAlpha(255);
	}
	void CHudImport::Paint()
	{
		auto s = surface();
		s->DrawSetColor(255, 255, 255, 255);
		s->DrawTexturedRect(20, 20, 200, 200);
	}
}
