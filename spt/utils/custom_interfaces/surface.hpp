#pragma once
#include "vcall.hpp"
#include "Color.h"
#include "VGuiMatSurface\IMatSystemSurface.h"
#ifdef OE
#define MAT_SYSTEM_SURFACE_INTERFACE_VERSION_4 "MatSystemSurface004"
#endif

class SurfaceWrapper
{
public:
	virtual ~SurfaceWrapper() {};

	virtual void DrawSetColor(Color col) = 0;
	virtual void DrawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawLine(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawOutlinedCircle(int x, int y, int radius, int segments) = 0;

	virtual void DrawSetTextFont(vgui::HFont font) = 0;
	virtual void DrawSetTextColor(Color col) = 0;
	virtual void DrawSetTextPos(int x, int y) = 0;
	virtual void DrawPrintText(const wchar_t* text,
	                           vgui::FontDrawType_t drawType = vgui::FontDrawType_t::FONT_DRAW_DEFAULT) = 0;
	virtual int GetFontTall(vgui::HFont font) = 0;
	virtual void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall) = 0;

	virtual void GetScreenSize(int& wide, int& tall) = 0;
};

#ifdef BMS
class IMatSystemSurfaceBMS
{
public:
	void DrawSetColor(Color col)
	{
		utils::vcall<13, void>(this, col);
	}

	void DrawFilledRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<15, void>(this, x0, y0, x1, y1);
	}

	void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<17, void>(this, x0, y0, x1, y1);
	}

	void DrawLine(int x0, int y0, int x1, int y1)
	{
		utils::vcall<18, void>(this, x0, y0, x1, y1);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<102, void>(this, x, y, radius, segments);
	}

	void DrawSetTextFont(vgui::HFont font)
	{
		utils::vcall<20, void>(this, font);
	}

	void DrawSetTextColor(Color col)
	{
		utils::vcall<21, void>(this, col);
	}

	void DrawSetTextPos(int x, int y)
	{
		utils::vcall<23, void>(this, x, y);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<25, void>(this, text, textLen, drawType);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<73, int>(this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<79, void>(this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<40, void>(this, &wide, &tall);
	}
};

class IMatSystemSurfaceBMSLatest : public IMatSystemSurfaceBMS
{
public:
	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<25, void>(this, text, textLen, drawType, 0);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<75, int>(this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<81, void>(this, font, text, &wide, &tall);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<104, void>(this, x, y, radius, segments);
	}
};
#endif

#ifdef OE
class IMatSystemSurfaceV4
{
public:
	void DrawSetColor(Color col)
	{
		utils::vcall<10, void>(this, col);
	}

	void DrawFilledRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<12, void>(this, x0, y0, x1, y1);
	}

	void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<14, void>(this, x0, y0, x1, y1);
	}

	void DrawLine(int x0, int y0, int x1, int y1)
	{
		utils::vcall<15, void>(this, x0, y0, x1, y1);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<96, void>(this, x, y, radius, segments);
	}

	void DrawSetTextFont(vgui::HFont font)
	{
		utils::vcall<17, void>(this, font);
	}

	void DrawSetTextColor(Color col)
	{
		utils::vcall<18, void>(this, col);
	}

	void DrawSetTextPos(int x, int y)
	{
		utils::vcall<20, void>(this, x, y);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<22, void>(this, text, textLen, drawType);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<67, int>(this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<72, void>(this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<37, void>(this, &wide, &tall);
	}
};

class IMatSystemSurfaceDMoMM : public IMatSystemSurfaceV4
{
public:
	void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<15, void>(this, x0, y0, x1, y1);
	}

	void DrawLine(int x0, int y0, int x1, int y1)
	{
		utils::vcall<16, void>(this, x0, y0, x1, y1);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<98, void>(this, x, y, radius, segments);
	}

	void DrawSetTextFont(vgui::HFont font)
	{
		utils::vcall<18, void>(this, font);
	}

	void DrawSetTextColor(Color col)
	{
		utils::vcall<19, void>(this, col);
	}

	void DrawSetTextPos(int x, int y)
	{
		utils::vcall<21, void>(this, x, y);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<23, void>(this, 0, text, textLen, drawType, 0, 0);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<68, int>(this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<73, void>(this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<38, void>(this, &wide, &tall);
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

	virtual void DrawSetTextFont(vgui::HFont font) override
	{
		surface->DrawSetTextFont(font);
	}

	virtual void DrawSetTextPos(int x, int y) override
	{
		surface->DrawSetTextPos(x, y);
	}

	virtual int GetFontTall(vgui::HFont font) override
	{
		return surface->GetFontTall(font);
	}

	virtual void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall) override
	{
		surface->GetTextSize(font, text, wide, tall);
	}

	virtual void GetScreenSize(int& wide, int& tall) override
	{
		surface->GetScreenSize(wide, tall);
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
