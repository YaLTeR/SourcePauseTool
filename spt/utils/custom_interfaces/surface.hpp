#pragma once

#include "VGuiMatSurface\IMatSystemSurface.h"
#ifdef OE
#include "VGuiMatSurface\IMatSystemSurfaceV4.h"
#endif

#include "Color.h"

class SurfaceWrapper
{
public:
	virtual ~SurfaceWrapper(){};

	virtual void DrawSetColor(Color col) = 0;
	virtual void DrawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawLine(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawOutlinedCircle(int x, int y, int radius, int segments) = 0;

	virtual void DrawSetTextColor(Color col) = 0;
	virtual void DrawSetTextPos(int x, int y) = 0;
	virtual void DrawSetTextFont(vgui::HFont font) = 0;
	virtual void DrawPrintText(const wchar_t* text,
	                           vgui::FontDrawType_t drawType = vgui::FontDrawType_t::FONT_DRAW_DEFAULT) = 0;
	virtual void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall) = 0;
	virtual int GetFontTall(vgui::HFont font) = 0;
};

template<class Surface>
class ISurfaceWrapperBase : public SurfaceWrapper
{
public:
	ISurfaceWrapperBase(Surface* surface) : surface(surface) {}

	virtual void DrawFilledRect(int x0, int y0, int x1, int y1)
	{
		surface->DrawFilledRect(x0, y0, x1, y1);
	}

	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		surface->DrawOutlinedRect(x0, y0, x1, y1);
	}

	virtual void DrawLine(int x0, int y0, int x1, int y1)
	{
		surface->DrawLine(x0, y0, x1, y1);
	}

	virtual void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		surface->DrawOutlinedCircle(x, y, radius, segments);
	}

	virtual void DrawSetTextPos(int x, int y)
	{
		surface->DrawSetTextPos(x, y);
	}

	virtual void DrawSetTextFont(vgui::HFont font)
	{
		surface->DrawSetTextFont(font);
	}

	virtual void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		surface->GetTextSize(font, text, wide, tall);
	}

	virtual int GetFontTall(vgui::HFont font)
	{
		return surface->GetFontTall(font);
	}

protected:
	Surface* surface;
};

template<class Surface>
class ISurfaceWrapper : public ISurfaceWrapperBase<Surface>
{
public:
	using ISurfaceWrapperBase<Surface>::ISurfaceWrapperBase;

	virtual void DrawSetColor(Color col)
	{
		this->surface->DrawSetColor(col);
	}

	virtual void DrawSetTextColor(Color col)
	{
		this->surface->DrawSetTextColor(col);
	}

	virtual void DrawPrintText(const wchar_t* text, vgui::FontDrawType_t drawType)
	{
		this->surface->DrawPrintText(text, wcslen(text), drawType);
	}
};

#ifdef OE
class ISurfaceWrapperV4 : public ISurfaceWrapperBase<IMatSystemSurfaceV4>
{
public:
	using ISurfaceWrapperBase<IMatSystemSurfaceV4>::ISurfaceWrapperBase;

	virtual void DrawSetColor(Color col)
	{
		this->surface->DrawSetColor(col.r(), col.g(), col.b(), col.a());
	}

	virtual void DrawSetTextColor(Color col)
	{
		this->surface->DrawSetTextColor(col.r(), col.g(), col.b(), col.a());
	}

	virtual void DrawPrintText(const wchar_t* text, vgui::FontDrawType_t drawType)
	{
		this->surface->DrawPrintText(text, drawType);
	}
};
#endif
