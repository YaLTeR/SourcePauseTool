#include "stdafx.h"
#if defined(SSDK2007)
#include "ihud.hpp"

#include "..\cvars.hpp"
#include "command.hpp"
#include "interfaces.hpp"
#include "playerio.hpp"
#include "signals.hpp"
#include "property_getter.hpp"
#include "..\sptlib-wrapper.hpp"

InputHud spt_ihud;

ConVar y_spt_ihud("y_spt_ihud", "0", 0, "Draws movement inputs of client.\n");
ConVar y_spt_ihud_grid_size("y_spt_ihud_grid_size", "60", 0, "Grid size of input HUD.\n");
ConVar y_spt_ihud_grid_padding("y_spt_ihud_grid_padding", "2", 0, "Padding between grids of input HUD.");
ConVar y_spt_ihud_x("y_spt_ihud_x", "2", 0, "X position of input HUD.\n", true, 0.0f, true, 100.0f);
ConVar y_spt_ihud_y("y_spt_ihud_y", "2", 0, "Y position of input HUD.\n", true, 0.0f, true, 100.0f);

CON_COMMAND(
    y_spt_ihud_modify,
    "y_spt_ihud_modify <element|all> <param> <value> - Modifies parameters in given element.\nParams: enabled, text, font, x, y, width, height, background, highlight, textcolor, texthighlight.\n")
{
	if (args.ArgC() != 4)
	{
		Msg("Usage: y_spt_ihud_modify <element|all> <param> <value>\n");
		return;
	}
	const char* errorMsg =
	    "Invalid arguments.\nElements: all, angles, forward, back, moveright, moveleft, use, duck, jump, attack, attack2\nParams: enabled, text, font, x, y, width, height, background, highlight, textcolor, texthighlight.\n";

	const char* element = args.Arg(1);
	const char* param = args.Arg(2);
	const char* value = args.Arg(3);

	if (std::strcmp(element, "all") == 0)
	{
		for (const auto& kv : spt_ihud.buttonSettings)
		{
			if (!spt_ihud.ModifySetting(kv.first.c_str(), param, value))
			{
				Msg(errorMsg);
				return;
			}
		}
		if (std::strcmp(param, "text") != 0 && std::strcmp(param, "width") != 0
		    && std::strcmp(param, "height") != 0 && std::strcmp(param, "texthighlight") != 0)
		{
			spt_ihud.ModifySetting("angles", param, value);
		}
	}
	else if (!spt_ihud.ModifySetting(element, param, value))
	{
		Msg(errorMsg);
	}
}

CON_COMMAND_AUTOCOMPLETE(y_spt_ihud_preset,
                         "Load ihud preset.\nValid options: normal, normal_mouse, tas.\n",
                         0,
                         ({"normal", "normal_mouse", "tas"}))
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: y_spt_ihud_preset <preset>\nPresets: normal, normal_mouse, tas.\n");
	}
	const char* preset = args.Arg(1);
	if (std::strcmp(preset, "normal") == 0)
	{
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

namespace patterns::client
{
	PATTERNS(DecodeUserCmdFromBuffer, "5135", "83 EC 54 33 C0 D9 EE 89 44 24 ?? D9 54 24 ?? 89 44 24 ??");
}

bool InputHud::ShouldLoadFeature()
{
	return spt_hud.ShouldLoadFeature();
}

void InputHud::InitHooks()
{
	HOOK_FUNCTION(client, DecodeUserCmdFromBuffer);
}

void InputHud::LoadFeature()
{
	if (!loadingSuccessful)
		return;
	if (CreateMoveSignal.Works)
		CreateMoveSignal.Connect(this, &InputHud::CreateMove);

	bool result = AddHudCallback("ihud", std::bind(&InputHud::DrawInputHud, this), y_spt_ihud);
	if (result)
	{
		InitCommand(y_spt_ihud_modify);
		InitCommand(y_spt_ihud_preset);
		InitConcommandBase(y_spt_ihud_grid_size);
		InitConcommandBase(y_spt_ihud_grid_padding);
		InitConcommandBase(y_spt_ihud_x);
		InitConcommandBase(y_spt_ihud_y);
	}

	Color background = {0, 0, 0, 233};
	Color highlight = {255, 255, 255, 66};
	Color textcolor = {255, 255, 255, 66};
	Color texthighlight = {0, 0, 0, 233};
	const std::string font = FONT_Trebuchet20;

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
	    {true, L"W", font, 2, 0, 1, 1, background, highlight, textcolor, texthighlight, IN_FORWARD};
	buttonSettings["use"] = {true, L"E", font, 3, 0, 1, 1, background, highlight, textcolor, texthighlight, IN_USE};
	buttonSettings["moveleft"] =
	    {true, L"A", font, 1, 1, 1, 1, background, highlight, textcolor, texthighlight, IN_MOVELEFT};
	buttonSettings["back"] =
	    {true, L"S", font, 2, 1, 1, 1, background, highlight, textcolor, texthighlight, IN_BACK};
	buttonSettings["moveright"] =
	    {true, L"D", font, 3, 1, 1, 1, background, highlight, textcolor, texthighlight, IN_MOVERIGHT};
	buttonSettings["duck"] =
	    {true, L"Duck", font, 0, 2, 1, 1, background, highlight, textcolor, texthighlight, IN_DUCK};
	buttonSettings["jump"] =
	    {true, L"Jump", font, 1, 2, 3, 1, background, highlight, textcolor, texthighlight, IN_JUMP};
	buttonSettings["attack"] =
	    {true, L"L", font, 4, 2, 1, 1, background, highlight, textcolor, texthighlight, IN_ATTACK};
	buttonSettings["attack2"] =
	    {true, L"R", font, 5, 2, 1, 1, background, highlight, textcolor, texthighlight, IN_ATTACK2};
	anglesSetting = {false, L"", font, 4, 1, 0, 0, background, highlight, textcolor, texthighlight, 0};
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
	if (std::strcmp(element, "angles") == 0)
	{
		angle = true;
		target = &anglesSetting;
	}
	else if (buttonSettings.find(element) != buttonSettings.end())
	{
		target = &buttonSettings[element];
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
		target->text = wstr;
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

Color InputHud::StringToColor(const char* color)
{
	int len = std::strlen(color);
	if (len != 6 && len != 8)
	{
		return Color(0, 0, 0);
	}
	int shift = len == 6 ? 16 : 24;
	unsigned int hex = strtoul(color, NULL, 16);
	int r, g, b, a = 0xff;
	r = (hex >> shift) & 0xff;
	shift -= 8;
	g = (hex >> shift) & 0xff;
	shift -= 8;
	b = (hex >> shift) & 0xff;
	if (len == 8)
		a = hex & 0xff;
	return Color(r, g, b, a);
}

void InputHud::GetCurrentSize(int& x, int& y)
{
	int gridWidth = 0;
	int gridHeight = 0;

	int gridSize = y_spt_ihud_grid_size.GetInt();
	int padding = y_spt_ihud_grid_padding.GetInt();

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

	if (!spt_hud.GetFont(anglesSetting.font, anglesSettingFont))
	{
		return;
	}

	if (awaitingFrameDraw)
	{
		previousAng = currentAng;
		float va[3];
		EngineGetViewAngles(va);
		currentAng = Vector(va[0], va[1], va[3]);
		awaitingFrameDraw = false;
	}

	surface = interfaces::surface;

	// get offset from percentage
	int ihudSizeX, ihudSizeY;
	GetCurrentSize(ihudSizeX, ihudSizeY);
	xOffset = (spt_hud.screen->width - ihudSizeX) * y_spt_ihud_x.GetFloat() * 0.01f;
	yOffset = (spt_hud.screen->height - ihudSizeY) * y_spt_ihud_y.GetFloat() * 0.01f;

	gridSize = y_spt_ihud_grid_size.GetInt();
	padding = y_spt_ihud_grid_padding.GetInt();

	if (y_spt_ihud.GetBool())
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
			float maxSpeed = utils::GetProperty<float>(0, "m_flMaxspeed");
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
			wchar_t* text = L"move analog";
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
	vgui::HFont font;

	if (!spt_hud.GetFont(fontName, font))
	{
		return;
	}

	surface->DrawSetColor(buttonColor);
	surface->DrawFilledRect(x0, y0, x1, y1);

	int tw, th;
	surface->GetTextSize(font, text, tw, th);
	int xc = x0 + ((x1 - x0) / 2);
	int yc = y0 + ((y1 - y0) / 2);
	surface->DrawSetTextFont(font);
	surface->DrawSetTextColor(textColor);
	surface->DrawSetTextPos(xc - (tw / 2), yc - (th / 2));
	surface->DrawPrintText(text, wcslen(text));
}

void InputHud::DrawButton(Button button)
{
	if (!button.enabled)
		return;
	bool state = buttonBits & button.mask;
	DrawRectAndCenterTxt(state ? button.highlight : button.background,
	                     xOffset + button.x * (gridSize + padding),
	                     yOffset + button.y * (gridSize + padding),
	                     xOffset + (button.x + button.width) * (gridSize + padding) - padding,
	                     yOffset + (button.y + button.height) * (gridSize + padding) - padding,
	                     button.font,
	                     state ? button.texthighlight : button.textcolor,
	                     button.text.c_str());
}

void __fastcall InputHud::HOOKED_DecodeUserCmdFromBuffer(void* thisptr, int edx, bf_read& buf, int sequence_number)
{
	spt_ihud.ORIG_DecodeUserCmdFromBuffer(thisptr, edx, buf, sequence_number);

	auto m_pCommands =
	    *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(thisptr) + spt_playerio.offM_pCommands);
	auto pCmd = m_pCommands + spt_playerio.sizeofCUserCmd * (sequence_number % 90);
	auto cmd = reinterpret_cast<CUserCmd*>(pCmd);
	spt_ihud.SetInputInfo(cmd->buttons, Vector(cmd->sidemove, cmd->forwardmove, cmd->upmove));
	pCmd = 0;
}

void InputHud::CreateMove(uintptr_t pCmd)
{
	auto cmd = reinterpret_cast<CUserCmd*>(pCmd);
	spt_ihud.SetInputInfo(cmd->buttons, Vector(cmd->sidemove, cmd->forwardmove, cmd->upmove));
}

#endif