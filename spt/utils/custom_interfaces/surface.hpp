#pragma once
#include "vcall.hpp"
#include "VGuiMatSurface\IMatSystemSurface.h"
#ifdef OE
#include "VGuiMatSurface\IMatSystemSurfaceV4.h"
#endif

#include "Color.h"

class SurfaceWrapper
{
public:
	virtual ~SurfaceWrapper() {};

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

#ifdef BMS
class IMatSystemSurfaceBMS
{
public:
	void DrawSetColor(Color col)
	{
		utils::vcall<0, void>(this, col);
	}

	void DrawFilledRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<0, void>(this, x0, y0, x1, y1);
	}

	void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<0, void>(this, x0, y0, x1, y1);
	}

	void DrawLine(int x0, int y0, int x1, int y1)
	{
		utils::vcall<0, void>(this, x0, y0, x1, y1);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<0, void>(this, x, y, radius, segments);
	}

	void DrawSetTextColor(Color col)
	{
		utils::vcall<0, void>(this, col);
	}

	void DrawSetTextPos(int x, int y)
	{
		utils::vcall<0, void>(this, x, y);
	}

	void DrawSetTextFont(vgui::HFont font)
	{
		utils::vcall<0, void>(this, font);
	}

	void DrawPrintText(const wchar_t* text, vgui::FontDrawType_t drawType)
	{
		utils::vcall<0, void>(this, text, (int)wcslen(text), drawType);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<0, void>(this, font, text, wide, tall);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<0, int>(this, font);
	}
};

class IMatSystemSurfaceBMSLatest : public IMatSystemSurfaceBMS
{
public:
	void DrawPrintText(const wchar_t* text, vgui::FontDrawType_t drawType)
	{
		utils::vcall<0, void>(this, text, (int)wcslen(text), drawType, 0);
	}
};
#endif

/**
 * Wrapper for an interface similar to IMatSystemSurface.
 */
template<class Surface>
class ISurfaceWrapperBase : public SurfaceWrapper
{
public:
	ISurfaceWrapperBase(Surface* surface) : surface(surface) {}

	virtual void DrawFilledRect(int x0, int y0, int x1, int y1) override
	{
		surface->DrawFilledRect(x0, y0, x1, y1);
	}

	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1) override
	{
		surface->DrawOutlinedRect(x0, y0, x1, y1);
	}

	virtual void DrawLine(int x0, int y0, int x1, int y1) override
	{
		surface->DrawLine(x0, y0, x1, y1);
	}

	virtual void DrawOutlinedCircle(int x, int y, int radius, int segments) override
	{
		surface->DrawOutlinedCircle(x, y, radius, segments);
	}

	virtual void DrawSetTextPos(int x, int y) override
	{
		surface->DrawSetTextPos(x, y);
	}

	virtual void DrawSetTextFont(vgui::HFont font) override
	{
		surface->DrawSetTextFont(font);
	}

	virtual void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall) override
	{
		surface->GetTextSize(font, text, wide, tall);
	}

	virtual int GetFontTall(vgui::HFont font) override
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

	virtual void DrawSetColor(Color col) override
	{
		this->surface->DrawSetColor(col);
	}

	virtual void DrawSetTextColor(Color col) override
	{
		this->surface->DrawSetTextColor(col);
	}

	virtual void DrawPrintText(const wchar_t* text, vgui::FontDrawType_t drawType) override
	{
		this->surface->DrawPrintText(text, wcslen(text), drawType);
	}
};

#ifdef OE
class ISurfaceWrapperV4 : public ISurfaceWrapperBase<IMatSystemSurfaceV4>
{
public:
	using ISurfaceWrapperBase<IMatSystemSurfaceV4>::ISurfaceWrapperBase;

	virtual void DrawSetColor(Color col) override
	{
		this->surface->DrawSetColor(col.r(), col.g(), col.b(), col.a());
	}

	virtual void DrawSetTextColor(Color col) override
	{
		this->surface->DrawSetTextColor(col.r(), col.g(), col.b(), col.a());
	}

	virtual void DrawPrintText(const wchar_t* text, vgui::FontDrawType_t drawType) override
	{
		this->surface->DrawPrintText(text, drawType);
	}
};
#endif
