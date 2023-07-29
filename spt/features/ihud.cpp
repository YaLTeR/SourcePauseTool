#include "stdafx.hpp"

#include "hud.hpp"

#if !defined(BMS) && defined(SPT_HUD_ENABLED)

#include "spt\feature.hpp"
#include "spt\features\playerio.hpp"
#include "spt\features\property_getter.hpp"
#include "spt\utils\convar.hpp"
#include "spt\utils\interfaces.hpp"
#include "spt\utils\signals.hpp"
#include "spt\utils\game_detection.hpp"

#include "basehandle.h"
#include "Color.h"

#ifdef OE
#include "..\game_shared\usercmd.h"
#else
#include "usercmd.h"
#endif

#include <cctype>
#include <algorithm>

#undef min
#undef max

// Input HUD
class InputHud : public FeatureWrapper<InputHud>
{
public:
	void DrawInputHud();
	void SetInputInfo(int button, Vector movement);
	bool ModifySetting(const char* element, const char* param, const char* value);

#ifndef OE
	void AddCustomKey(const char* key);
#endif

	struct Button
	{
		bool is_normal_key = false;
		bool enabled = false;
		std::wstring text;
		std::string font;
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
		Color background;
		Color highlight;
		Color textcolor;
		Color texthighlight;
		// used as button code if not normal key
		int mask = 0;

		std::string ToString() const;
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
	DECL_HOOK_THISCALL(void, DecodeUserCmdFromBuffer, void*, bf_read& buf, int sequence_number);
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
	void GetCurrentSize(int& x, int& y);

	IMatSystemSurface* surface = nullptr;

	int xOffset = 0;
	int yOffset = 0;
	int gridSize = 0;
	int padding = 0;

	Vector inputMovement;
	QAngle currentAng;
	QAngle previousAng;
	int buttonBits = 0;

	bool awaitingFrameDraw = false;

	bool loadingSuccessful = false;
};

InputHud spt_ihud;

ConVar y_spt_ihud("y_spt_ihud", "0", 0, "Draws movement inputs of client.");
ConVar y_spt_ihud_grid_size("y_spt_ihud_grid_size", "60", 0, "Grid size of input HUD.");
ConVar y_spt_ihud_grid_padding("y_spt_ihud_grid_padding", "2", 0, "Padding between grids of input HUD.");
ConVar y_spt_ihud_x("y_spt_ihud_x", "2", 0, "X position of input HUD.", true, 0.0f, true, 100.0f);
ConVar y_spt_ihud_y("y_spt_ihud_y", "2", 0, "Y position of input HUD.", true, 0.0f, true, 100.0f);

// default setting
static const Color background = {0, 0, 0, 233};
static const Color highlight = {255, 255, 255, 66};
static const Color textcolor = {255, 255, 255, 66};
static const Color texthighlight = {0, 0, 0, 233};
static const std::string font = FONT_Trebuchet20;

#ifdef SSDK2013
CON_COMMAND(y_spt_ihud_modify,
            "Modifies parameters in given element.\n"
            "spt_ihud_modify <button name|all> <enabled> [0|1]\n"
            "spt_ihud_modify <button name|all> <text|font> [value]\n"
            "spt_ihud_modify <button name|all> <x|y|pos|width|height> [value]\n"
            "spt_ihud_modify <element|all> <background|highlight|textcolor|texthighlight> [color]")
#else
static int IHudModifyFunc(AUTOCOMPLETION_FUNCTION_PARAMS)
{
	std::string s(partial);
	size_t pos = s.find_last_of(' ') + 1;
	std::string base = s.substr(0, pos);
	std::string incomplete = s.substr(pos);

	std::istringstream iss(base);
	std::vector<std::string> args{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

	switch (args.size())
	{
	case 0:
		return 0;
	case 1:
	{
		std::vector<std::string> suggestions;
		for (const auto& button : spt_ihud.buttonSettings)
		{
			suggestions.push_back(button.first);
		}
		suggestions.push_back("angles");
		suggestions.push_back("all");
		return AutoCompleteList::AutoCompleteSuggest(base, incomplete, suggestions, false, commands);
	}
	case 2:
	{
		if (args[1] == "all" || args[1] == "angles" || spt_ihud.buttonSettings.contains(args[1]))
			return AutoCompleteList::AutoCompleteSuggest(base,
			                                             incomplete,
			                                             {
			                                                 "enabled",
			                                                 "text",
			                                                 "font",
			                                                 "x",
			                                                 "y",
			                                                 "pos",
			                                                 "width",
			                                                 "height",
			                                                 "background",
			                                                 "highlight",
			                                                 "textcolor",
			                                                 "texthighlight",
			                                             },
			                                             false,
			                                             commands);
		return 0;
	}
	default:
		return 0;
	}
}

CON_COMMAND_F_COMPLETION(y_spt_ihud_modify,
                         "Modifies parameters in given element.\n"
                         "spt_ihud_modify <button name|all> <enabled> [0|1]\n"
                         "spt_ihud_modify <button name|all> <text|font> [value]\n"
                         "spt_ihud_modify <button name|all> <x|y|pos|width|height> [value]\n"
                         "spt_ihud_modify <element|all> <background|highlight|textcolor|texthighlight> [color]",
                         0,
                         IHudModifyFunc)
#endif
{
	if (args.ArgC() < 2)
	{
		for (const auto& button : spt_ihud.buttonSettings)
		{
			Msg("%s\n", button.first.c_str());
		}
		Msg("angles\n");
		return;
	}

	auto commandAction = [args](std::string name) -> bool
	{
		bool isAngles = (name == "angles");
		InputHud::Button& button = isAngles ? spt_ihud.anglesSetting : spt_ihud.buttonSettings[name];
		int argc = args.ArgC();
		if (argc == 2)
		{
			Msg("%s:\n"
			    "%s\n",
			    name.c_str(),
			    button.ToString().c_str());
			return true;
		}

		std::string param = args.Arg(2);
		if (param == "enabled")
		{
			if (argc == 3)
			{
				Msg("%s: enabled = %s\n", button.enabled ? "true" : "false");
			}
			else if (argc == 4)
			{
				button.enabled = !!std::atoi(args.Arg(3));
			}
			else
			{
				Msg("Usage: spt_ihud_modify <element|all> enable [value]\n");
				return false;
			}
		}
		else if (param == "text")
		{
			if (isAngles)
			{
				Msg("text for angles has no effect.\n");
				return true;
			}

			if (argc == 3)
			{
				std::string str(button.text.begin(), button.text.end());
				Msg("%s: text = \"%s\"", name.c_str(), str.c_str());
			}
			else if (argc == 4)
			{
				// only works for ASCII
				std::string str(args.Arg(3));
				std::wstring wstr(str.begin(), str.end());
				button.text = wstr;
			}
			else
			{
				Msg("Usage: spt_ihud_modify <element|all> text [text]\n");
				return false;
			}
		}
		else if (param == "font")
		{
			if (isAngles)
			{
				Msg("text for angles has no effect.\n");
				return true;
			}

			if (argc == 3)
			{
				Msg("%s: font = \"%s\"", name.c_str(), font.c_str());
			}
			else if (argc == 4)
			{
				button.font = args.Arg(3);
			}
			else
			{
				Msg("Usage: spt_ihud_modify <element|all> font [name]\n");
				return false;
			}
		}
		else if (param == "x" || param == "y" || param == "pos")
		{
			if (argc == 3)
			{
				Msg("%s: pos = (%d, %d)\n", name.c_str(), button.x, button.y);
			}
			else if (param == "pos")
			{
				if (argc != 5)
				{
					Msg("Usage: spt_ihud_modify <element|all> pos [<x> <y>]\n");
					return false;
				}
				button.x = std::atoi(args.Arg(3));
				button.y = std::atoi(args.Arg(4));
			}
			else
			{
				if (argc != 4)
				{
					Msg("Usage: spt_ihud_modify <element|all> <x|y> [value]\n");
					return false;
				}
				int value = std::atoi(args.Arg(3));
				if (param == "x")
					button.x = value;
				else
					button.y = value;
			}
		}
		else if (param == "width" || param == "height")
		{
			if (isAngles)
			{
				Msg("width and height for angles have no effect.\n");
				return true;
			}

			if (argc == 3)
			{
				Msg("%s: %s = %d\n",
				    name.c_str(),
				    param.c_str(),
				    param == "width" ? button.width : button.height);
			}
			else if (argc == 4)
			{
				int value = std::atoi(args.Arg(3));
				if (param == "width")
					button.width = value;
				else
					button.height = value;
			}
			else
			{
				Msg("Usage: spt_ihud_modify <element|all> <width|height> [value]\n");
				return false;
			}
		}
		else if (param == "background" || param == "highlight" || param == "textcolor"
		         || param == "texthighlight")
		{
			if (param == "texthighlight" && isAngles)
			{
				Msg("angles texthighlight has no effect\n");
				return true;
			}

			Color& elementColor = (param == "background")  ? button.background
			                      : (param == "highlight") ? button.highlight
			                      : (param == "textcolor") ? button.textcolor
			                                               : button.texthighlight;

			if (argc == 3)
			{
				Msg("%s: %s = %s\n", name.c_str(), param.c_str(), ColorToString(elementColor).c_str());
			}
			else if (argc == 4)
			{
				elementColor = StringToColor(args.Arg(3));
			}
			else
			{
				Msg("Usage: spt_ihud_modify <element|all> <background|highlight|textcolor|texthighlight> [color]\n");
				return false;
			}
		}
		else
		{
			Msg("Invalid arguments %s.\n"
			    "Params: enabled, text, font, x, y, pos, width, height, background, highlight, textcolor, texthighlight.\n",
			    param.c_str());
			return false;
		}
		return true;
	};

	std::string name = args.Arg(1);
	if (name == "all")
	{
		for (auto& kv : spt_ihud.buttonSettings)
		{
			if (!commandAction(kv.first))
				return;
		}
		commandAction("angles");
	}
	else if (spt_ihud.buttonSettings.contains(name) || name == "angles")
	{
		commandAction(name);
	}
	else
	{
		Msg("Button %s does not exist!\n", name.c_str());
	}
}

CON_COMMAND_AUTOCOMPLETE(y_spt_ihud_preset,
                         "Load ihud preset.\n"
                         "Valid options: normal, normal_mouse, tas.",
                         0,
                         ({"normal", "normal_mouse", "tas"}))
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: spt_ihud_preset <preset>\n"
		    "Presets: normal, normal_mouse, tas.\n");
	}
	const char* preset = args.Arg(1);
	if (std::strcmp(preset, "normal") == 0)
	{
		for (auto& kv : spt_ihud.buttonSettings)
			kv.second.enabled = false;
		spt_ihud.tasPreset = false;
		InputHud::Button* button = &spt_ihud.buttonSettings["forward"];
		button->enabled = true;
		button->x = 2;
		button->y = 0;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["use"];
		button->enabled = true;
		button->x = 3;
		button->y = 0;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["moveleft"];
		button->enabled = true;
		button->x = 1;
		button->y = 1;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["back"];
		button->enabled = true;
		button->x = 2;
		button->y = 1;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["moveright"];
		button->enabled = true;
		button->x = 3;
		button->y = 1;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["duck"];
		button->enabled = true;
		button->x = 0;
		button->y = 2;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["jump"];
		button->enabled = true;
		button->x = 1;
		button->y = 2;
		button->width = 3;
		button->height = 1;
		button = &spt_ihud.buttonSettings["attack"];
		button->enabled = true;
		button->x = 4;
		button->y = 2;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["attack2"];
		button->enabled = true;
		button->x = 5;
		button->y = 2;
		button->width = 1;
		button->height = 1;

		spt_ihud.anglesSetting.enabled = false;
	}
	else if (std::strcmp(preset, "normal_mouse") == 0)
	{
		for (auto& kv : spt_ihud.buttonSettings)
			kv.second.enabled = false;
		spt_ihud.tasPreset = false;
		InputHud::Button* button = &spt_ihud.buttonSettings["forward"];
		button->enabled = true;
		button->x = 1;
		button->y = 0;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["use"];
		button->enabled = true;
		button->x = 2;
		button->y = 0;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["moveleft"];
		button->enabled = true;
		button->x = 0;
		button->y = 1;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["back"];
		button->enabled = true;
		button->x = 1;
		button->y = 1;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["moveright"];
		button->enabled = true;
		button->x = 2;
		button->y = 1;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["duck"];
		button->enabled = true;
		button->x = 0;
		button->y = 2;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["jump"];
		button->enabled = true;
		button->x = 1;
		button->y = 2;
		button->width = 2;
		button->height = 1;
		button = &spt_ihud.buttonSettings["attack"];
		button->enabled = true;
		button->x = 4;
		button->y = 0;
		button->width = 1;
		button->height = 1;
		button = &spt_ihud.buttonSettings["attack2"];
		button->enabled = true;
		button->x = 5;
		button->y = 0;
		button->width = 1;
		button->height = 1;

		spt_ihud.anglesSetting.enabled = true;
		spt_ihud.anglesSetting.x = 4;
		spt_ihud.anglesSetting.y = 1;
	}
	else if (std::strcmp(preset, "tas") == 0)
	{
		spt_ihud.tasPreset = true;
	}
	else
	{
		Msg("Invalid preset.\nPresets: normal, normal_mouse, tas.\n");
	}
}

#ifndef OE
CON_COMMAND(y_spt_ihud_add_key, "Add custom key to ihud.")
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: spt_ihud_add_key <key>\n");
		return;
	}
	spt_ihud.AddCustomKey(args.Arg(1));
}
#endif

namespace patterns
{
	PATTERNS(
	    DecodeUserCmdFromBuffer,
	    "5135",
	    "83 EC 54 33 C0 D9 EE 89 44 24 ?? D9 54 24 ?? 89 44 24 ??",
	    "7197370",
	    "55 8B EC 83 EC 54 56 8B F1 C7 45 ?? ?? ?? ?? ?? 8D 4D ?? C7 45 ?? 00 00 00 00 C7 45 ?? 00 00 00 00 C7 45 ?? 00 00 00 00 C7 45 ?? 00 00 00 00 C7 45 ?? 00 00 00 00 E8 ?? ?? ?? ?? 8B 4D ??",
	    "4044",
	    "83 EC 54 53 57 8D 44 24 ?? 50 8B 44 24 ?? 99");
}

void InputHud::InitHooks()
{
	HOOK_FUNCTION(client, DecodeUserCmdFromBuffer);
}

bool InputHud::ShouldLoadFeature()
{
	return spt_hud_feat.ShouldLoadFeature();
}

void InputHud::LoadFeature()
{
	if (!loadingSuccessful)
		return;
	if (CreateMoveSignal.Works)
		CreateMoveSignal.Connect(this, &InputHud::CreateMove);

	bool result = spt_hud_feat.AddHudDefaultGroup(HudCallback(
	    std::bind(&InputHud::DrawInputHud, this), []() { return y_spt_ihud.GetBool(); }, false));
	if (result)
	{
		InitCommand(y_spt_ihud_modify);
		InitCommand(y_spt_ihud_preset);

#ifndef OE
		InitCommand(y_spt_ihud_add_key);
#endif

		InitConcommandBase(y_spt_ihud);
		InitConcommandBase(y_spt_ihud_grid_size);
		InitConcommandBase(y_spt_ihud_grid_padding);
		InitConcommandBase(y_spt_ihud_x);
		InitConcommandBase(y_spt_ihud_y);
	}

	const int IN_ATTACK = (1 << 0);
	const int IN_JUMP = (1 << 1);
	const int IN_DUCK = (1 << 2);
	const int IN_FORWARD = (1 << 3);
	const int IN_BACK = (1 << 4);
	const int IN_USE = (1 << 5);
	const int IN_MOVELEFT = (1 << 9);
	const int IN_MOVERIGHT = (1 << 10);
	const int IN_ATTACK2 = (1 << 11);

	buttonSettings["forward"] =
	    {true, true, L"W", font, 2, 0, 1, 1, background, highlight, textcolor, texthighlight, IN_FORWARD};
	buttonSettings["use"] =
	    {true, true, L"E", font, 3, 0, 1, 1, background, highlight, textcolor, texthighlight, IN_USE};
	buttonSettings["moveleft"] =
	    {true, true, L"A", font, 1, 1, 1, 1, background, highlight, textcolor, texthighlight, IN_MOVELEFT};
	buttonSettings["back"] =
	    {true, true, L"S", font, 2, 1, 1, 1, background, highlight, textcolor, texthighlight, IN_BACK};
	buttonSettings["moveright"] =
	    {true, true, L"D", font, 3, 1, 1, 1, background, highlight, textcolor, texthighlight, IN_MOVERIGHT};
	buttonSettings["duck"] =
	    {true, true, L"Duck", font, 0, 2, 1, 1, background, highlight, textcolor, texthighlight, IN_DUCK};
	buttonSettings["jump"] =
	    {true, true, L"Jump", font, 1, 2, 3, 1, background, highlight, textcolor, texthighlight, IN_JUMP};
	buttonSettings["attack"] =
	    {true, true, L"L", font, 4, 2, 1, 1, background, highlight, textcolor, texthighlight, IN_ATTACK};
	buttonSettings["attack2"] =
	    {true, true, L"R", font, 5, 2, 1, 1, background, highlight, textcolor, texthighlight, IN_ATTACK2};
	anglesSetting = {true, false, L"", font, 4, 1, 0, 0, background, highlight, textcolor, texthighlight, 0};
}

void InputHud::PreHook()
{
	loadingSuccessful = !!(ORIG_DecodeUserCmdFromBuffer);
}

void InputHud::UnloadFeature() {}

void InputHud::SetInputInfo(int button, Vector movement)
{
	if (awaitingFrameDraw)
	{
		buttonBits |= button;
	}
	else
	{
		buttonBits = button;
	}
	inputMovement = movement;
	awaitingFrameDraw = true;
}

bool InputHud::ModifySetting(const char* element, const char* param, const char* value)
{
	InputHud::Button* target;
	bool angle = false;
	std::string key(element);
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
	if (key == "angles")
	{
		angle = true;
		target = &anglesSetting;
	}
	else if (buttonSettings.find(key) != buttonSettings.end())
	{
		target = &buttonSettings[key];
	}
	else
	{
		return false;
	}

	if (std::strcmp(param, "enabled") == 0)
	{
		target->enabled = std::atoi(value);
	}
	else if (std::strcmp(param, "text") == 0)
	{
		if (angle)
		{
			Msg("angles font has no effect\n");
			return true;
		}
		// only works for ASCII
		std::string str(value);
		std::wstring wstr(str.begin(), str.end());
		target->text = std::move(wstr);
	}
	else if (std::strcmp(param, "font") == 0)
	{
		target->font = value;
	}
	else if (std::strcmp(param, "x") == 0)
	{
		target->x = std::atoi(value);
	}
	else if (std::strcmp(param, "y") == 0)
	{
		target->y = std::atoi(value);
	}
	else if (std::strcmp(param, "width") == 0)
	{
		if (angle)
		{
			Msg("angles width has no effect\n");
			return true;
		}
		target->width = std::atoi(value);
	}
	else if (std::strcmp(param, "height") == 0)
	{
		if (angle)
		{
			Msg("angles height has no effect\n");
			return true;
		}
		target->height = std::atoi(value);
	}
	else if (std::strcmp(param, "background") == 0)
	{
		target->background = StringToColor(value);
	}
	else if (std::strcmp(param, "highlight") == 0)
	{
		target->highlight = StringToColor(value);
	}
	else if (std::strcmp(param, "textcolor") == 0)
	{
		target->textcolor = StringToColor(value);
	}
	else if (std::strcmp(param, "texthighlight") == 0)
	{
		if (angle)
		{
			Msg("angles texthighlight has no effect\n");
			return true;
		}
		target->texthighlight = StringToColor(value);
	}
	else
	{
		return false;
	}
	return true;
}

#ifndef OE
void InputHud::AddCustomKey(const char* key)
{
	ButtonCode_t code = interfaces::inputSystem->StringToButtonCode(key);
	if (code == -1)
	{
		Msg("Key <%s> does not exist.\n", key);
		return;
	}
	std::string str(key);
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
	std::wstring wstr(str.begin(), str.end());
	buttonSettings[str] =
	    {false, true, wstr, font, 0, 0, 1, 1, background, highlight, textcolor, texthighlight, code};
}
#endif

void InputHud::GetCurrentSize(int& x, int& y)
{
	int gridWidth = 0;
	int gridHeight = 0;

	gridSize = y_spt_ihud_grid_size.GetInt();
	padding = y_spt_ihud_grid_padding.GetInt();

	if (tasPreset)
	{
		gridWidth = 10;
		gridHeight = 6;
		gridSize /= 2;
	}
	else
	{
		for (const auto& kv : buttonSettings)
		{
			auto button = kv.second;
			if (!button.enabled)
				continue;
			gridWidth = std::max(gridWidth, button.x + button.width);
			gridHeight = std::max(gridHeight, button.y + button.height);
		}
		if (anglesSetting.enabled)
		{
			gridWidth = std::max(gridWidth, anglesSetting.x + 2);
			gridHeight = std::max(gridHeight, anglesSetting.y + 2);
		}
	}
	x = gridWidth * gridSize + std::max(0, gridWidth - 1) * padding;
	y = gridHeight * gridSize + std::max(0, gridHeight - 1) * padding;
}

void InputHud::DrawInputHud()
{
	vgui::HFont anglesSettingFont;

	if (!spt_hud_feat.GetFont(anglesSetting.font, anglesSettingFont))
	{
		return;
	}

	if (awaitingFrameDraw)
	{
		previousAng = currentAng;
		currentAng = utils::GetPlayerEyeAngles();
		awaitingFrameDraw = false;
	}

	surface = interfaces::surface;

	// get offset from percentage
	int ihudSizeX, ihudSizeY;
	GetCurrentSize(ihudSizeX, ihudSizeY);
	xOffset = (spt_hud_feat.renderView->width - ihudSizeX) * y_spt_ihud_x.GetFloat() * 0.01f;
	yOffset = (spt_hud_feat.renderView->height - ihudSizeY) * y_spt_ihud_y.GetFloat() * 0.01f;

	gridSize = y_spt_ihud_grid_size.GetInt();
	padding = y_spt_ihud_grid_padding.GetInt();

	if (y_spt_ihud.GetBool() && !interfaces::engine_vgui->IsGameUIVisible())
	{
		Vector ang;
		if (tasPreset || anglesSetting.enabled)
		{
			ang = {previousAng.y - currentAng.y, previousAng.x - currentAng.x, 0};
			while (ang.x < -180.0f)
				ang.x += 360.0f;
			if (ang.x > 180.0f)
				ang.x -= 360.0f;

			float len = ang.Length();
			if (len > 0)
			{
				ang /= len;
				ang *= pow(len / 180.0f, 0.2);
				ang.y *= -1;
			}
		}
		if (tasPreset)
		{
			gridSize /= 2;

			// Movement
			Vector movement = inputMovement;
			int movementSpeed = movement.Length();
			float maxSpeed = spt_propertyGetter.GetProperty<float>(0, "m_flMaxspeed");
			movement /= movementSpeed > maxSpeed ? movementSpeed : maxSpeed;

			// Draw
			int r = 2 * gridSize;
			int cX1 = (xOffset + xOffset + 5 * (gridSize + padding) - padding) / 2;
			int cY1 = (yOffset + yOffset + 5 * (gridSize + padding) - padding) / 2;
			int cX2 =
			    (xOffset + 5 * (gridSize + padding) + xOffset + 10 * (gridSize + padding) - padding) / 2;
			int cY2 = (yOffset + yOffset + 5 * (gridSize + padding) - padding) / 2;
			int movX = cX1 + r * movement.x;
			int movY = cY1 - r * movement.y;
			int angX = cX2 + r * ang.x;
			int angY = cY2 + r * ang.y;
			surface->DrawSetColor(anglesSetting.background);
			surface->DrawFilledRect(xOffset,
			                        yOffset,
			                        xOffset + 5 * (gridSize + padding) - padding,
			                        yOffset + 5 * (gridSize + padding) - padding);
			surface->DrawFilledRect(xOffset + 5 * (gridSize + padding),
			                        yOffset,
			                        xOffset + 10 * (gridSize + padding) - padding,
			                        yOffset + 5 * (gridSize + padding) - padding);

			int th = surface->GetFontTall(anglesSettingFont);
			surface->DrawSetTextFont(anglesSettingFont);
			surface->DrawSetTextColor(anglesSetting.textcolor);
			const wchar_t* text = L"move analog";
			surface->DrawSetTextPos(cX1 - r, cY1 - r - th);
			surface->DrawPrintText(text, wcslen(text));
			text = L"view analog";
			surface->DrawSetTextPos(cX2 - r, cY2 - r - th);
			surface->DrawPrintText(text, wcslen(text));

			int joyStickSize = gridSize / 5;
			surface->DrawSetColor(255, 200, 100, 100); // oragne
			surface->DrawOutlinedCircle(cX1, cY1, r, 64);
			surface->DrawOutlinedRect(cX1 - r, cY1 - r, cX1 + r, cY1 + r);
			surface->DrawOutlinedCircle(movX, movY, joyStickSize, 16);
			surface->DrawLine(cX1 - r, cY1, cX1 + r, cY1);
			surface->DrawLine(cX1, cY1 - r, cX1, cY1 + r);
			surface->DrawLine(cX1, cY1, movX, movY);
			surface->DrawSetColor(100, 200, 255, 100); // blue
			surface->DrawOutlinedCircle(cX2, cY2, r, 64);
			surface->DrawOutlinedRect(cX2 - r, cY2 - r, cX2 + r, cY2 + r);
			surface->DrawOutlinedCircle(angX, angY, joyStickSize, 16);
			surface->DrawLine(cX2 - r, cY2, cX2 + r, cY2);
			surface->DrawLine(cX2, cY2 - r, cX2, cY2 + r);
			surface->DrawLine(cX2, cY2, angX, angY);

			Button button = buttonSettings["duck"];
			button.enabled = true;
			button.x = 0;
			button.y = 5;
			button.width = 2;
			button.height = 1;
			button.text = L"Duck";
			DrawButton(button);
			button = buttonSettings["use"];
			button.enabled = true;
			button.x = 2;
			button.y = 5;
			button.width = 2;
			button.height = 1;
			button.text = L"Use";
			DrawButton(button);
			button = buttonSettings["jump"];
			button.enabled = true;
			button.x = 4;
			button.y = 5;
			button.width = 2;
			button.height = 1;
			button.text = L"Jump";
			DrawButton(button);
			button = buttonSettings["attack"];
			button.enabled = true;
			button.x = 6;
			button.y = 5;
			button.width = 2;
			button.height = 1;
			button.text = L"Blue";
			DrawButton(button);
			button = buttonSettings["attack2"];
			button.enabled = true;
			button.x = 8;
			button.y = 5;
			button.width = 2;
			button.height = 1;
			button.text = L"Orange";
			DrawButton(button);
		}
		else
		{
			for (const auto& kv : buttonSettings)
			{
				DrawButton(kv.second);
			}

			// mouse
			if (anglesSetting.enabled)
			{
				int r = gridSize;
				int cX = xOffset + (anglesSetting.x + 1) * (gridSize + padding) - padding / 2;
				int cY = yOffset + (anglesSetting.y + 1) * (gridSize + padding) - padding / 2;
				int angX = cX + r * ang.x;
				int angY = cY + r * ang.y;
				surface->DrawSetColor(anglesSetting.background);
				surface->DrawFilledRect(xOffset + anglesSetting.x * (gridSize + padding),
				                        yOffset + anglesSetting.y * (gridSize + padding),
				                        xOffset + (anglesSetting.x + 2) * (gridSize + padding)
				                            - padding,
				                        yOffset + (anglesSetting.y + 2) * (gridSize + padding)
				                            - padding);

				int joyStickSize = gridSize / 10;
				surface->DrawSetColor(anglesSetting.highlight);
				surface->DrawOutlinedCircle(cX, cY, r, 64);
				surface->DrawOutlinedCircle(angX, angY, joyStickSize, 16);
				surface->DrawLine(cX - r, cY, cX + r, cY);
				surface->DrawLine(cX, cY - r, cX, cY + r);
				surface->DrawLine(cX, cY, angX, angY);
			}
		}
	}
}

void InputHud::DrawRectAndCenterTxt(Color buttonColor,
                                    int x0,
                                    int y0,
                                    int x1,
                                    int y1,
                                    const std::string& fontName,
                                    Color textColor,
                                    const wchar_t* text)
{
	vgui::HFont rectFont;

	if (!spt_hud_feat.GetFont(fontName, rectFont))
	{
		return;
	}

	surface->DrawSetColor(buttonColor);
	surface->DrawFilledRect(x0, y0, x1, y1);

	int tw, th;
	surface->GetTextSize(rectFont, text, tw, th);
	int xc = x0 + ((x1 - x0) / 2);
	int yc = y0 + ((y1 - y0) / 2);
	surface->DrawSetTextFont(rectFont);
	surface->DrawSetTextColor(textColor);
	surface->DrawSetTextPos(xc - (tw / 2), yc - (th / 2));
	surface->DrawPrintText(text, wcslen(text));
}

void InputHud::DrawButton(Button button)
{
	if (!button.enabled)
		return;
	bool state;
	if (button.is_normal_key)
		state = buttonBits & button.mask;
	else
		state = interfaces::inputSystem->IsButtonDown((ButtonCode_t)button.mask);
	DrawRectAndCenterTxt(state ? button.highlight : button.background,
	                     xOffset + button.x * (gridSize + padding),
	                     yOffset + button.y * (gridSize + padding),
	                     xOffset + (button.x + button.width) * (gridSize + padding) - padding,
	                     yOffset + (button.y + button.height) * (gridSize + padding) - padding,
	                     button.font,
	                     state ? button.texthighlight : button.textcolor,
	                     button.text.c_str());
}

IMPL_HOOK_THISCALL(InputHud, void, DecodeUserCmdFromBuffer, void*, bf_read& buf, int sequence_number)
{
	spt_ihud.ORIG_DecodeUserCmdFromBuffer(thisptr, buf, sequence_number);

	auto m_pCommands =
	    *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(thisptr) + spt_playerio.offM_pCommands);
	auto pCmd = m_pCommands + spt_playerio.sizeofCUserCmd * (sequence_number % 90);
	auto cmd = reinterpret_cast<CUserCmd*>(pCmd);
	spt_ihud.SetInputInfo(cmd->buttons, Vector(cmd->sidemove, cmd->forwardmove, cmd->upmove));
}

void InputHud::CreateMove(uintptr_t pCmd)
{
	auto cmd = reinterpret_cast<CUserCmd*>(pCmd);
	spt_ihud.SetInputInfo(cmd->buttons, Vector(cmd->sidemove, cmd->forwardmove, cmd->upmove));
}

std::string InputHud::Button::ToString() const
{
	std::stringstream ss;

	ss << "enabled: " << (enabled ? "true" : "false") << "\n";

	std::string textStr(text.begin(), text.end());
	ss << "text: " << textStr << "\n";

	ss << "pos: (" << x << ", " << y << ")\n";
	ss << "width: " << width << "\n";
	ss << "height: " << height << "\n";

	ss << "font: " << font << "\n";

	ss << "background: " << ColorToString(background) << "\n";
	ss << "highlight: " << ColorToString(highlight) << "\n";
	ss << "textcolor: " << ColorToString(textcolor) << "\n";
	ss << "texthighlight: " << ColorToString(texthighlight) << "\n";

	return ss.str();
}

#endif
