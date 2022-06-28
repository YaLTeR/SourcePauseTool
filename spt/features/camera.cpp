#include "stdafx.h"
#include "camera.hpp"
#include "playerio.hpp"
#include "interfaces.hpp"
#include "ent_utils.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\cvars.hpp"

#ifdef OE
#include "..\game_shared\usercmd.h"
#else
#include "usercmd.h"
#endif

#include <chrono>

Camera spt_camera;

ConVar y_spt_cam_control("y_spt_cam_control",
                         "0",
                         FCVAR_CHEAT,
                         "Camera is separated and can be controlled by user input. (Require sv_cheats 1)\n");
ConVar y_spt_cam_drive("y_spt_cam_drive", "1", FCVAR_CHEAT, "Enables or disables camera drive mode.\n");

bool Camera::ShouldOverrideView() const
{
	return y_spt_cam_control.GetBool() && _sv_cheats->GetBool() && utils::playerEntityAvailable();
}

bool Camera::IsInDriveMode() const
{
	return ShouldOverrideView() && y_spt_cam_drive.GetBool() && !interfaces::engine_vgui->IsGameUIVisible();
}

void Camera::GetCurrentView()
{
	cam_origin = spt_playerio.GetPlayerEyePos();
	float ang[3];
	EngineGetViewAngles(ang);
	cam_angles = QAngle(ang[0], ang[1], 0);
}

void Camera::OverrideView(CViewSetup* view)
{
	if (ShouldOverrideView())
	{
		HandleInput(true);
		view->origin = cam_origin;
		view->angles = cam_angles;
	}
	else if (input_active)
	{
		HandleInput(false);
	}
}

void Camera::HandleInput(bool active)
{
	static std::chrono::time_point<std::chrono::steady_clock> last_frame;
	auto now = std::chrono::steady_clock::now();

	if (input_active ^ active)
	{
		if (input_active && !active)
		{
			// On camera control end
			interfaces::vgui_input->SetCursorPos(old_cursor[0], old_cursor[1]);
		}
		else
		{
			// On camera control start
			last_frame = now;
			GetCurrentView();
			interfaces::vgui_input->GetCursorPos(old_cursor[0], old_cursor[1]);
		}
	}

	float real_frame_time = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame).count();
	last_frame = now;

	if (active && IsInDriveMode())
	{
		float frame_time = real_frame_time;
		Vector movement(0.0f, 0.0f, 0.0f);

		bool shift_down = interfaces::inputSystem->IsButtonDown(KEY_LSHIFT)
		                  || interfaces::inputSystem->IsButtonDown(KEY_RSHIFT);
		bool ctrl_down = interfaces::inputSystem->IsButtonDown(KEY_LCONTROL)
		                 || interfaces::inputSystem->IsButtonDown(KEY_RCONTROL);
		float move_speed = shift_down ? 525.0f : (ctrl_down ? 60.0 : 175.0f);

		if (interfaces::inputSystem->IsButtonDown(KEY_W))
			movement.x += 1.0f;
		if (interfaces::inputSystem->IsButtonDown(KEY_S))
			movement.x -= 1.0f;
		if (interfaces::inputSystem->IsButtonDown(KEY_A))
			movement.y -= 1.0f;
		if (interfaces::inputSystem->IsButtonDown(KEY_D))
			movement.y += 1.0f;
		if (interfaces::inputSystem->IsButtonDown(KEY_X))
			movement.z += 1.0f;
		if (interfaces::inputSystem->IsButtonDown(KEY_Z))
			movement.z -= 1.0f;

		int mx, my;
		int dx, dy;

		interfaces::vgui_input->GetCursorPos(mx, my);

		dx = mx - old_cursor[0];
		dy = my - old_cursor[1];

		interfaces::vgui_input->SetCursorPos(old_cursor[0], old_cursor[1]);

		// Convert to pitch/yaw
		float pitch = (float)dy * 0.022f * sensitivity->GetFloat();
		float yaw = -(float)dx * 0.022f * sensitivity->GetFloat();

		// Apply mouse
		cam_angles.x += pitch;
		cam_angles.x = clamp(cam_angles.x, -89.0f, 89.0f);
		cam_angles.y += yaw;
		if (cam_angles.y > 180.0f)
			cam_angles.y -= 360.0f;
		else if (cam_angles.y < -180.0f)
			cam_angles.y += 360.0f;

		// Now apply forward, side, up

		Vector fwd, side, up;

		AngleVectors(cam_angles, &fwd, &side, &up);

		VectorNormalize(movement);
		movement *= move_speed * frame_time;

		cam_origin += fwd * movement.x;
		cam_origin += side * movement.y;
		cam_origin += up * movement.z;
	}
	input_active = active;
}

void __fastcall Camera::HOOKED_ClientModeShared__OverrideView(void* thisptr, int edx, CViewSetup* view)
{
	spt_camera.OverrideView(view);
	spt_camera.ORIG_ClientModeShared__OverrideView(thisptr, edx, view);
}

bool __fastcall Camera::HOOKED_ClientModeShared__CreateMove(void* thisptr, int edx, float flInputSampleTime, void* cmd)
{
	if (spt_camera.IsInDriveMode())
	{
		// Block all inputs
		auto usercmd = reinterpret_cast<CUserCmd*>(cmd);
		usercmd->buttons = 0;
		usercmd->forwardmove = 0;
		usercmd->sidemove = 0;
		usercmd->upmove = 0;
	}
	return spt_camera.ORIG_ClientModeShared__CreateMove(thisptr, edx, flInputSampleTime, cmd);
}

bool __fastcall Camera::HOOKED_C_BasePlayer__ShouldDrawLocalPlayer(void* thisptr, int edx)
{
	if (spt_camera.ShouldOverrideView())
	{
		return true;
	}
	return spt_camera.ORIG_C_BasePlayer__ShouldDrawLocalPlayer(thisptr, edx);
}

#if defined(SSDK2013)
bool __fastcall Camera::HOOKED_C_BasePlayer__ShouldDrawThisPlayer(void* thisptr, int edx)
{
	// ShouldDrawLocalPlayer only decides draw view model or weapon model in steampipe
	// We need ShouldDrawThisPlayer to make player model draw
	if (spt_camera.ShouldOverrideView())
	{
		return true;
	}
	return spt_camera.ORIG_C_BasePlayer__ShouldDrawThisPlayer(thisptr, edx);
}
#endif

void __fastcall Camera::HOOKED_CInput__MouseMove(void* thisptr, int edx, void* cmd)
{
	// Block mouse inputs and stop the game from resetting cursor pos
	if (spt_camera.IsInDriveMode())
		return;
	spt_camera.ORIG_CInput__MouseMove(thisptr, edx, cmd);
}

bool Camera::ShouldLoadFeature()
{
	return interfaces::engine_vgui != nullptr && interfaces::vgui_input != nullptr
	       && interfaces::inputSystem != nullptr;
}

void Camera::InitHooks()
{
	HOOK_FUNCTION(client, ClientModeShared__OverrideView);
	HOOK_FUNCTION(client, ClientModeShared__CreateMove);
	HOOK_FUNCTION(client, CInput__MouseMove);
	HOOK_FUNCTION(client, C_BasePlayer__ShouldDrawLocalPlayer);
#if defined(SSDK2013)
	// This function only exist in steampipe
	HOOK_FUNCTION(client, C_BasePlayer__ShouldDrawThisPlayer);
#endif
}

void Camera::PreHook()
{
	loadingSuccessful = ORIG_ClientModeShared__OverrideView && ORIG_ClientModeShared__CreateMove
	                    && ORIG_CInput__MouseMove && ORIG_C_BasePlayer__ShouldDrawLocalPlayer;
#if defined(SSDK2013)
	loadingSuccessful &= !!ORIG_C_BasePlayer__ShouldDrawThisPlayer;
#endif
}

void Camera::LoadFeature()
{
	if (loadingSuccessful)
	{
		InitConcommandBase(y_spt_cam_control);
		InitConcommandBase(y_spt_cam_drive);

		sensitivity = interfaces::g_pCVar->FindVar("sensitivity");
	}
}

void Camera::UnloadFeature() {}