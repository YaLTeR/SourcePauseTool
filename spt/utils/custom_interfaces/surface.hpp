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
	virtual vgui::HFont CreateFont() = 0;
	virtual bool SetFontGlyphSet(vgui::HFont font,
	                             const char* windowsFontName,
	                             int tall,
	                             int weight,
	                             int blur,
	                             int scanlines,
	                             int flags) = 0;
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

	vgui::HFont CreateFont()
	{
		return utils::vcall<70, vgui::HFont>(this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<71, bool>(this, font, windowsFontName, tall, weight, blur, scanlines, flags, 0, 0);
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
		utils::vcall<42, void>(this, &wide, &tall);
	}
};

class IMatSystemSurfaceBMSLatest : public IMatSystemSurfaceBMS
{
public:
	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<104, void>(this, x, y, radius, segments);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<25, void>(this, text, textLen, drawType, 0);
	}

	vgui::HFont CreateFont()
	{
		return utils::vcall<72, vgui::HFont>(this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<73, bool>(this, font, windowsFontName, tall, weight, blur, scanlines, flags, 0, 0);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<75, int>(this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<81, void>(this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<44, void>(this, &wide, &tall);
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

	vgui::HFont CreateFont()
	{
		return utils::vcall<64, vgui::HFont>(this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<65, bool>(this, font, windowsFontName, tall, weight, blur, scanlines, flags);
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

	vgui::HFont CreateFont()
	{
		return utils::vcall<65, vgui::HFont>(this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<66, bool>(this, font, windowsFontName, tall, weight, blur, scanlines, flags);
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
class ISurfaceWrapper : public SurfaceWrapper
{
public:
	ISurfaceWrapper(Surface* surface) : surface(surface) {}

	virtual void DrawSetColor(Color col) override
	{
		this->surface->DrawSetColor(col);
	}

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

	virtual void DrawSetTextColor(Color col) override
	{
		this->surface->DrawSetTextColor(col);
	}

	virtual void DrawSetTextFont(vgui::HFont font) override
	{
		surface->DrawSetTextFont(font);
	}

	virtual void DrawSetTextPos(int x, int y) override
	{
		surface->DrawSetTextPos(x, y);
	}

	virtual void DrawPrintText(const wchar_t* text, vgui::FontDrawType_t drawType) override
	{
		surface->DrawPrintText(text, wcslen(text), drawType);
	}

	virtual vgui::HFont CreateFont()
	{
		return surface->CreateFont();
	}

	virtual bool SetFontGlyphSet(vgui::HFont font,
	                             const char* windowsFontName,
	                             int tall,
	                             int weight,
	                             int blur,
	                             int scanlines,
	                             int flags)
	{
		return surface->SetFontGlyphSet(font, windowsFontName, tall, weight, blur, scanlines, flags);
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

#ifdef SSDK2007
class SurfaceWrapper2007 : public ISurfaceWrapper<IMatSystemSurface>
{
public:
	using ISurfaceWrapper<IMatSystemSurface>::ISurfaceWrapper;

	virtual bool SetFontGlyphSet(vgui::HFont font,
	                             const char* windowsFontName,
	                             int tall,
	                             int weight,
	                             int blur,
	                             int scanlines,
	                             int flags) override
	{
		return utils::vcall<65, bool>(surface, font, windowsFontName, tall, weight, blur, scanlines, flags);
	}
};
#endif
