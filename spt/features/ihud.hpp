#pragma once

#if defined(SSDK2007)
#include "..\feature.hpp"
#include "hud.hpp"
#include "basehandle.h"
#include "Color.h"

#ifdef OE
#include "..\game_shared\usercmd.h"
#else
#include "usercmd.h"
#endif

// Input HUD
class InputHud : public FeatureWrapper<InputHud>
{
public:
	void DrawInputHud();
	void SetInputInfo(int button, Vector movement);
	bool ModifySetting(const char* element, const char* param, const char* value);
	void AddCustomKey(const char* key);

	struct Button
	{
		bool is_normal_key;
		bool enabled;
		std::wstring text;
		std::string font;
		int x;
		int y;
		int width;
		int height;
		Color background;
		Color highlight;
		Color textcolor;
		Color texthighlight;
		// used as button code if not normal key
		int mask;
	};
	bool tasPreset = false;
	std::map<std::string, Button> buttonSettings;
	Button anglesSetting;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void PreHook() override;

	virtual void UnloadFeature() override;

private:
	DECL_HOOK_THISCALL(void, DecodeUserCmdFromBuffer, bf_read& buf, int sequence_number);
	void CreateMove(uintptr_t pCmd);
	void DrawRectAndCenterTxt(Color buttonColor,
	                          int x0,
	                          int y0,
	                          int x1,
	                          int y1,
	                          const std::string& fontName,
	                          Color textColor,
	                          const wchar_t* text);
	void DrawButton(Button button);
	Color StringToColor(const char* hex);
	void GetCurrentSize(int& x, int& y);

	IMatSystemSurface* surface;

	int xOffset;
	int yOffset;
	int gridSize;
	int padding;

	Vector inputMovement;
	Vector currentAng;
	Vector previousAng;
	int buttonBits;

	bool awaitingFrameDraw;

	bool loadingSuccessful = false;
};

extern InputHud spt_ihud;

#endif