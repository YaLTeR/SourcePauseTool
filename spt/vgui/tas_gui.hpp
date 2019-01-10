#pragma once
#include "networkvar.h"
#include "vgui_controls\frame.h"
#include <vgui_controls\Panel.h>

namespace vgui
{
	class CHudImport : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(CHudImport, Panel);

	public:
		CHudImport(const char *pElementName);

	protected:
		virtual void Paint();
	};

	void OpenTASGUI();

}