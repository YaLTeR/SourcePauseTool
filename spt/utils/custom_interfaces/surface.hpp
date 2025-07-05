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
		utils::vcall<void>(13, this, col);
	}

	void DrawFilledRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(15, this, x0, y0, x1, y1);
	}

	void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(17, this, x0, y0, x1, y1);
	}

	void DrawLine(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(18, this, x0, y0, x1, y1);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<void>(102, this, x, y, radius, segments);
	}

	void DrawSetTextFont(vgui::HFont font)
	{
		utils::vcall<void>(20, this, font);
	}

	void DrawSetTextColor(Color col)
	{
		utils::vcall<void>(21, this, col);
	}

	void DrawSetTextPos(int x, int y)
	{
		utils::vcall<void>(23, this, x, y);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<void>(25, this, text, textLen, drawType);
	}

	vgui::HFont CreateFont()
	{
		return utils::vcall<vgui::HFont>(70, this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<bool>(71, this, font, windowsFontName, tall, weight, blur, scanlines, flags, 0, 0);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<int>(73, this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<void>(79, this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<void>(42, this, &wide, &tall);
	}
};

class IMatSystemSurfaceBMSLatest : public IMatSystemSurfaceBMS
{
public:
	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<void>(104, this, x, y, radius, segments);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<void>(25, this, text, textLen, drawType, 0);
	}

	vgui::HFont CreateFont()
	{
		return utils::vcall<vgui::HFont>(72, this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<bool>(73, this, font, windowsFontName, tall, weight, blur, scanlines, flags, 0, 0);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<int>(75, this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<void>(81, this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<void>(44, this, &wide, &tall);
	}
};
#endif

#ifdef OE
class IMatSystemSurfaceV4
{
public:
	void DrawSetColor(Color col)
	{
		utils::vcall<void>(10, this, col);
	}

	void DrawFilledRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(12, this, x0, y0, x1, y1);
	}

	void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(14, this, x0, y0, x1, y1);
	}

	void DrawLine(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(15, this, x0, y0, x1, y1);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<void>(96, this, x, y, radius, segments);
	}

	void DrawSetTextFont(vgui::HFont font)
	{
		utils::vcall<void>(17, this, font);
	}

	void DrawSetTextColor(Color col)
	{
		utils::vcall<void>(18, this, col);
	}

	void DrawSetTextPos(int x, int y)
	{
		utils::vcall<void>(20, this, x, y);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<void>(22, this, text, textLen, drawType);
	}

	vgui::HFont CreateFont()
	{
		return utils::vcall<vgui::HFont>(64, this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<bool>(65, this, font, windowsFontName, tall, weight, blur, scanlines, flags);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<int>(67, this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<void>(72, this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<void>(37, this, &wide, &tall);
	}
};

class IMatSystemSurfaceDMoMM : public IMatSystemSurfaceV4
{
public:
	void DrawOutlinedRect(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(15, this, x0, y0, x1, y1);
	}

	void DrawLine(int x0, int y0, int x1, int y1)
	{
		utils::vcall<void>(16, this, x0, y0, x1, y1);
	}

	void DrawOutlinedCircle(int x, int y, int radius, int segments)
	{
		utils::vcall<void>(98, this, x, y, radius, segments);
	}

	void DrawSetTextFont(vgui::HFont font)
	{
		utils::vcall<void>(18, this, font);
	}

	void DrawSetTextColor(Color col)
	{
		utils::vcall<void>(19, this, col);
	}

	void DrawSetTextPos(int x, int y)
	{
		utils::vcall<void>(21, this, x, y);
	}

	void DrawPrintText(const wchar_t* text, int textLen, vgui::FontDrawType_t drawType)
	{
		utils::vcall<void>(23, this, 0, text, textLen, drawType, 0, 0);
	}

	vgui::HFont CreateFont()
	{
		return utils::vcall<vgui::HFont>(65, this);
	}

	bool SetFontGlyphSet(vgui::HFont font,
	                     const char* windowsFontName,
	                     int tall,
	                     int weight,
	                     int blur,
	                     int scanlines,
	                     int flags)
	{
		return utils::vcall<bool>(66, this, font, windowsFontName, tall, weight, blur, scanlines, flags);
	}

	int GetFontTall(vgui::HFont font)
	{
		return utils::vcall<int>(68, this, font);
	}

	void GetTextSize(vgui::HFont font, const wchar_t* text, int& wide, int& tall)
	{
		utils::vcall<void>(73, this, font, text, &wide, &tall);
	}

	void GetScreenSize(int& wide, int& tall)
	{
		utils::vcall<void>(38, this, &wide, &tall);
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
		surface->DrawSetColor(col);
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
		surface->DrawSetTextColor(col);
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
		return utils::vcall<bool>(65, surface, font, windowsFontName, tall, weight, blur, scanlines, flags);
	}
};
#endif
