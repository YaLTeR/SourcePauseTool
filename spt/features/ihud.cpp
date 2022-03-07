#include "stdafx.h"
#if defined(SSDK2007)
#include "ihud.hpp"

#include "..\cvars.hpp"
#include "playerio.hpp"
#include "signals.hpp"
#include "property_getter.hpp"
#include "..\sptlib-wrapper.hpp"
#include "Color.h"

InputHud spt_ihud;

ConVar y_spt_ihud("y_spt_ihud", "0", FCVAR_CHEAT, "Draws movement inputs of client.\n0 = Default,\n1 = tas.\n");
ConVar y_spt_ihud_button_color("y_spt_ihud_button_color",
                               "0 0 0 233",
                               FCVAR_CHEAT,
                               "RGBA button color of input HUD.\n");
ConVar y_spt_ihud_shadow_color("y_spt_ihud_shadow_color",
                               "0 0 0 66",
                               FCVAR_CHEAT,
                               "RGBA button shadow color of input HUD.\n");
ConVar y_spt_ihud_font_color("y_spt_ihud_font_color",
                             "255 255 255 233",
                             FCVAR_CHEAT,
                             "RGBA font color of input HUD.\n");
ConVar y_spt_ihud_shadow_font_color("y_spt_ihud_shadow_font_color",
                                    "255 255 255 66",
                                    FCVAR_CHEAT,
                                    "RGBA button shadow font color of input HUD.\n");
ConVar y_spt_ihud_grid_size("y_spt_ihud_grid_size", "60", FCVAR_CHEAT, "Grid size of input HUD.\n");
ConVar y_spt_ihud_grid_padding("y_spt_ihud_grid_padding", "2", FCVAR_CHEAT, "Padding between grids of input HUD.");
ConVar y_spt_ihud_font("y_spt_ihud_font", "Trebuchet20", FCVAR_CHEAT, "Font of input HUD.");
ConVar y_spt_ihud_x("y_spt_ihud_x", "2", FCVAR_CHEAT, "X offset of input HUD.\n");
ConVar y_spt_ihud_y("y_spt_ihud_y", "2", FCVAR_CHEAT, "Y offset of input HUD.\n");

bool InputHud::ShouldLoadFeature()
{
	return spt_hud.ShouldLoadFeature();
}

void InputHud::InitHooks()
{
	FIND_PATTERN(client, DecodeUserCmdFromBuffer);
	HOOK_FUNCTION(client, DecodeUserCmdFromBuffer);
}

void InputHud::LoadFeature()
{
	if (!loadingSuccessful)
		return;
	CreateMoveSignal.Connect(this, &InputHud::CreateMove);
		ihudFont = spt_hud.scheme->GetFont(y_spt_ihud_font.GetString(), false);

		bool result = AddHudCallback("ihud", std::bind(&InputHud::DrawInputHud, this), y_spt_ihud);
		if (result)
		{
			InitConcommandBase(y_spt_ihud_button_color);
			InitConcommandBase(y_spt_ihud_shadow_color);
			InitConcommandBase(y_spt_ihud_font_color);
			InitConcommandBase(y_spt_ihud_shadow_font_color);
			InitConcommandBase(y_spt_ihud_grid_size);
			InitConcommandBase(y_spt_ihud_grid_padding);
			InitConcommandBase(y_spt_ihud_font);
			InitConcommandBase(y_spt_ihud_x);
			InitConcommandBase(y_spt_ihud_y);
		}
	}
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

void InputHud::DrawInputHud()
{
	if (awaitingFrameDraw)
	{
		previousAng = currentAng;
		float va[3];
		EngineGetViewAngles(va);
		currentAng = Vector(va[0], va[1], va[3]);
		awaitingFrameDraw = false;
	}
	auto surface = spt_hud.surface;
	auto scheme = spt_hud.scheme;
	auto screen = spt_hud.screen;
	ihudFont = scheme->GetFont(y_spt_ihud_font.GetString(), false);
	const int IN_ATTACK = (1 << 0);
	const int IN_JUMP = (1 << 1);
	const int IN_DUCK = (1 << 2);
	const int IN_FORWARD = (1 << 3);
	const int IN_BACK = (1 << 4);
	const int IN_USE = (1 << 5);
	const int IN_MOVELEFT = (1 << 9);
	const int IN_MOVERIGHT = (1 << 10);
	const int IN_ATTACK2 = (1 << 11);

	int xOffset = y_spt_ihud_x.GetInt();
	int yOffset = y_spt_ihud_y.GetInt();
	int gridSize = y_spt_ihud_grid_size.GetInt();
	int padding = y_spt_ihud_grid_padding.GetInt();

	Color buttonColor;
	Color shadowColor;
	Color buttonFontColor;
	Color shadowFontColor;

#define GET_COLOR(cmd, color) \
	{ \
		static int r = 0, g = 0, b = 0, a = 0; \
		if (strcmp("", cmd.GetString()) != 0) \
		{ \
			sscanf(cmd.GetString(), "%d %d %d %d", &r, &g, &b, &a); \
		} \
		color = Color(r, g, b, a); \
	}

	GET_COLOR(y_spt_ihud_button_color, buttonColor);
	GET_COLOR(y_spt_ihud_shadow_color, shadowColor);
	GET_COLOR(y_spt_ihud_font_color, buttonFontColor);
	GET_COLOR(y_spt_ihud_shadow_font_color, shadowFontColor);

#define DRAW_BUTTON(button, col, row, size, colSize, rowSize, text) \
	{ \
		Color btnColor = (buttonBits & (button)) ? buttonColor : shadowColor; \
		Color fontColor = (buttonBits & (button)) ? buttonFontColor : shadowFontColor; \
		DrawRectAndCenterTxt(btnColor, \
		                     xOffset + col * (size + padding), \
		                     yOffset + row * (size + padding), \
		                     xOffset + (col + colSize) * (size + padding) - padding, \
		                     yOffset + (row + rowSize) * (size + padding) - padding, \
		                     ihudFont, \
		                     fontColor, \
		                     surface, \
		                     text); \
	}

	if (y_spt_ihud.GetInt() == 2)
	{
		gridSize /= 2;

		// Angle
		Vector ang = currentAng - previousAng;
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

		// Movement
		Vector movement = inputMovement;
		int movementSpeed = movement.Length();
		float maxSpeed = utils::GetProperty<float>(0, "m_flMaxspeed");
		movement /= movementSpeed > maxSpeed ? movementSpeed : maxSpeed;

		// Draw
		int r = 2 * gridSize;
		int cX1 = (xOffset + xOffset + 5 * (gridSize + padding) - padding) / 2;
		int cY1 = (yOffset + yOffset + 5 * (gridSize + padding) - padding) / 2;
		int cX2 = (xOffset + 5 * (gridSize + padding) + xOffset + 10 * (gridSize + padding) - padding) / 2;
		int cY2 = (yOffset + yOffset + 5 * (gridSize + padding) - padding) / 2;
		int movX = cX1 + r * movement.x;
		int movY = cY1 - r * movement.y;
		int angX = cX2 + r * ang.y;
		int angY = cY2 + r * ang.x;
		surface->DrawSetColor(buttonColor);
		surface->DrawFilledRect(xOffset,
		                        yOffset,
		                        xOffset + 5 * (gridSize + padding) - padding,
		                        yOffset + 5 * (gridSize + padding) - padding);
		surface->DrawFilledRect(xOffset + 5 * (gridSize + padding),
		                        yOffset,
		                        xOffset + 10 * (gridSize + padding) - padding,
		                        yOffset + 5 * (gridSize + padding) - padding);

		int th = surface->GetFontTall(ihudFont);
		surface->DrawSetTextFont(ihudFont);
		surface->DrawSetTextColor(buttonFontColor);
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

		DRAW_BUTTON(IN_DUCK, 0, 5, gridSize, 2, 1, L"Duck");
		DRAW_BUTTON(IN_USE, 2, 5, gridSize, 2, 1, L"Use");
		DRAW_BUTTON(IN_JUMP, 4, 5, gridSize, 2, 1, L"Jump");
		DRAW_BUTTON(IN_ATTACK, 6, 5, gridSize, 2, 1, L"Blue");
		DRAW_BUTTON(IN_ATTACK2, 8, 5, gridSize, 2, 1, L"Orange");
	}
	else
	{
		DRAW_BUTTON(IN_FORWARD, 2, 0, gridSize, 1, 1, L"W");
		DRAW_BUTTON(IN_USE, 3, 0, gridSize, 1, 1, L"E");
		DRAW_BUTTON(IN_MOVELEFT, 1, 1, gridSize, 1, 1, L"A");
		DRAW_BUTTON(IN_BACK, 2, 1, gridSize, 1, 1, L"S");
		DRAW_BUTTON(IN_MOVERIGHT, 3, 1, gridSize, 1, 1, L"D");
		DRAW_BUTTON(IN_DUCK, 0, 2, gridSize, 1, 1, L"Duck");
		DRAW_BUTTON(IN_JUMP, 1, 2, gridSize, 3, 1, L"Jump");
		DRAW_BUTTON(IN_ATTACK, 4, 2, gridSize, 1, 1, L"L");
		DRAW_BUTTON(IN_ATTACK2, 5, 2, gridSize, 1, 1, L"R");
	}
}

void InputHud::DrawRectAndCenterTxt(Color buttonColor,
                                    int x0,
                                    int y0,
                                    int x1,
                                    int y1,
                                    vgui::HFont font,
                                    Color textColor,
                                    IMatSystemSurface* surface,
                                    const wchar_t* text)
{
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