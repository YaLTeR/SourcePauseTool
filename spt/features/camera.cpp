#include "stdafx.h"
#ifndef OE
#include "camera.hpp"
#include "playerio.hpp"
#include "interfaces.hpp"
#include "signals.hpp"
#include "command.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\cvars.hpp"

#include "usercmd.h"

#include <chrono>

Camera spt_camera;

ConVar y_spt_cam_control(
    "y_spt_cam_control",
    "0",
    FCVAR_CHEAT,
    "y_spt_cam_control <type> - Changes the camera control type.\n"
    "0 = Default\n"
    "1 = Drive mode\n"
    "    Camera is separated and can be controlled by user input.\n"
    "    Use +forward, +back, +moveright, +moveleft, +moveup, +movedown, +duck, +speed keys to drive camera.\n"
    "    In-game: Requires sv_cheats 1. Set y_spt_cam_drive 1 to enable drive mode.\n"
    "    Demo playback: Hold right mouse button to enable drive mode.\n"
    "2 = Cinematic mode\n"
    "    Camera is controlled by predefined path.\n"
    "    See commands y_spt_cam_path_");
ConVar y_spt_cam_drive("y_spt_cam_drive", "1", FCVAR_CHEAT, "Enables or disables camera drive mode in-game.");
ConVar y_spt_cam_path_draw("y_spt_cam_path_draw", "0", FCVAR_CHEAT, "Draws the current camera path.");

ConVar _y_spt_force_fov("_y_spt_force_fov", "0", 0, "Force FOV to some value.");

CON_COMMAND(y_spt_cam_setpos, "y_spt_cam_setpos <x> <y> <z> - Sets the camera position. (requires camera drive mode)")
{
	if (args.ArgC() != 4)
	{
		Msg("Usage: y_spt_cam_setpos <x> <y> <z>\n");
		return;
	}
	if (!y_spt_cam_control.GetInt() == 1 || !spt_camera.CanOverrideView())
	{
		Msg("Requires camera drive mode.\n");
		return;
	}
	Vector pos = {0.0, 0.0, 0.0};
	for (int i = 0; i < args.ArgC() - 1; i++)
	{
		pos[i] = atof(args.Arg(i + 1));
	}
	spt_camera.current_cam.origin = pos;
}

CON_COMMAND(y_spt_cam_setang,
            "y_spt_cam_setang <pitch> <yaw> [roll] - Sets the camera angles. (requires camera drive mode)")
{
	if (args.ArgC() != 3 && args.ArgC() != 4)
	{
		Msg("Usage: y_spt_cam_setang <pitch> <yaw> [roll]\n");
		return;
	}
	if (!y_spt_cam_control.GetInt() == 1 || !spt_camera.CanOverrideView())
	{
		Msg("Requires camera drive mode.\n");
		return;
	}
	QAngle ang = {0.0, 0.0, 0.0};
	for (int i = 0; i < args.ArgC() - 1; i++)
	{
		ang[i] = atof(args.Arg(i + 1));
	}
	spt_camera.current_cam.angles = ang;
}

CON_COMMAND(y_spt_cam_path_setkf,
            "y_spt_cam_path_setkf [frame] [x] [y] [z] [yaw] [pitch] [roll] [fov]- Sets a camera path keyframe.")
{
	if (!spt_demostuff.Demo_IsPlayingBack())
	{
		Msg("Cinematic mode requires demo playback.\n");
		return;
	}
	int argc = args.ArgC();
	Camera::CameraInfo info;
	int tick;
	if (argc == 1)
	{
		tick = spt_demostuff.Demo_GetPlaybackTick();
		info = spt_camera.current_cam;
	}
	else if (argc == 9)
	{
		tick = atoi(args.Arg(1));
		float nums[7];
		for (int i = 0; i < args.ArgC() - 2; i++)
		{
			nums[i] = std::stof(args.Arg(i + 2));
		}
		info.origin.x = nums[0];
		info.origin.y = nums[1];
		info.origin.z = nums[2];
		info.angles.x = nums[3];
		info.angles.y = nums[4];
		info.angles.z = nums[5];
		info.fov = nums[6];
	}
	else
	{
		Msg("Usage: y_spt_cam_path_setkf [frame] [x] [y] [z] [yaw] [pitch] [roll] [fov]\n");
		return;
	}
	spt_camera.keyframes[tick] = info;
	Msg("[%d]\n%s\n", tick, std::string(info).c_str());
	spt_camera.RecomputeInterpPath();
}

CON_COMMAND(y_spt_cam_path_showkfs, "y_spt_cam_path_showkfs - Prints all keyframes.")
{
	for (const auto& kv : spt_camera.keyframes)
	{
		Msg("[%d]\n%s\n", kv.first, std::string(kv.second).c_str());
	}
}

CON_COMMAND(y_spt_cam_path_getkfs, "y_spt_cam_path_getkfs - Prints all keyframes in commands.")
{
	for (const auto& kv : spt_camera.keyframes)
	{
		Camera::CameraInfo info = kv.second;
		Msg("y_spt_cam_path_setkf %d %f %f %f %f %f %f %f;\n",
		    kv.first,
		    info.origin.x,
		    info.origin.y,
		    info.origin.z,
		    info.angles.x,
		    info.angles.y,
		    info.angles.z,
		    info.fov);
	}
}

#ifdef SSDK2013
// autocomplete crashes on stemapipe :((
CON_COMMAND(y_spt_cam_path_rmkf, "y_spt_cam_path_rmkf <tick> - Removes a keyframe.")
#else
static int y_spt_cam_path_rmkf_CompletionFunc(
    const char* partial,
    char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	std::vector<std::string> completion;
	for (const auto& kv : spt_camera.keyframes)
	{
		completion.push_back(std::to_string(kv.first));
	}
	AutoCompleteList y_spt_cam_path_rmkf_Complete("y_spt_cam_path_rmkf", completion);
	return y_spt_cam_path_rmkf_Complete.AutoCompletionFunc(partial, commands);
}

CON_COMMAND_F_COMPLETION(y_spt_cam_path_rmkf,
                         "y_spt_cam_path_rmkf <tick> - Removes a keyframe.",
                         0,
                         y_spt_cam_path_rmkf_CompletionFunc)
#endif
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: y_spt_cam_path_rmkf <tick>\n");
		return;
	}
	if (!spt_demostuff.Demo_IsPlayingBack())
	{
		Msg("Cinematic mode requires demo playback.\n");
		return;
	}
	int tick = atoi(args.Arg(1));
	bool removed = spt_camera.keyframes.erase(tick);
	if (removed)
	{
		spt_camera.RecomputeInterpPath();
		Msg("Removed keyframe at frame %d.\n", tick);
	}
	else
		Msg("Keyframe at frame %d does not exist.\n", tick);
}

CON_COMMAND(y_spt_cam_path_clear, "y_spt_cam_path_clear - Removes all keyframes.")
{
	if (!spt_demostuff.Demo_IsPlayingBack())
	{
		Msg("Cinematic mode requires demo playback.\n");
		return;
	}
	spt_camera.keyframes.clear();
	spt_camera.RecomputeInterpPath();
	Msg("Removed all keyframes.\n");
}

namespace patterns
{
	PATTERNS(ClientModeShared__OverrideView,
	         "5135",
	         "83 EC 58 E8 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 8B 10 56 8B 74 24 ?? 8B C8",
	         "3420",
	         "83 EC 40 E8 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 8B 10 56 8B 74 24 ?? 8B C8",
	         "1910503",
	         "55 8B EC 83 EC 58 E8 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 8B 10 56 8B 75 ?? 8B C8",
	         "7197370",
	         "55 8B EC 83 EC 58 E8 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 8B 10 8B C8 56");
	PATTERNS(ClientModeShared__CreateMove,
	         "5135",
	         "E8 ?? ?? ?? ?? 85 C0 75 ?? B0 01 C2 08 00 8B 4C 24 ?? D9 44 24 ?? 8B 10",
	         "1910503",
	         "55 8B EC E8 ?? ?? ?? ?? 85 C0 75 ?? B0 01 5D C2 08 00 8B 4D ?? 8B 10",
	         "7197370",
	         "55 8B EC E8 ?? ?? ?? ?? 8B C8 85 C9 75 ?? B0 01 5D C2 08 00 8B 01");
	PATTERNS(
	    C_BasePlayer__ShouldDrawLocalPlayer,
	    "5135",
	    "8B 0D ?? ?? ?? ?? 8B 01 8B 50 ?? FF D2 85 C0 75 ?? E8 ?? ?? ?? ?? 84 C0 74 ?? E8 ?? ?? ?? ?? 84 C0 75 ?? 33 C0",
	    "7197370",
	    "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 01 8B 40 ?? FF D0 84 C0 74 ?? A1 ?? ?? ?? ?? A8 01");
	PATTERNS(
	    CInput__MouseMove,
	    "5135",
	    "83 EC 14 56 8B F1 8B 0D ?? ?? ?? ?? 8B 01 8B 40 ?? 8D 54 24 ?? 52 FF D0 8B CE E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 11",
	    "7197370",
	    "55 8B EC 83 EC 1C 8D 55 ?? 56 8B F1 8B 0D ?? ?? ?? ?? 52 8B 01 FF 50 ?? 8B CE E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 8B 01");
	PATTERNS(C_BasePlayer__ShouldDrawThisPlayer,
	         "7197370",
	         "E8 ?? ?? ?? ?? 84 C0 75 ?? B0 01 C3 8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 01");

} // namespace patterns

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

bool Camera::CanOverrideView() const
{
	// Requires sv_cheats if not playing demo
	return interfaces::engine_client->IsInGame() && _sv_cheats->GetBool() || spt_demostuff.Demo_IsPlayingBack();
}

bool Camera::CanInput() const
{
	// Is in drive mode and y_spt_cam_drive
	// If playing demo automatically enables input and need right click to drive
	if (!CanOverrideView() || y_spt_cam_control.GetInt() != 1)
		return false;
	bool can_input;
	if (spt_demostuff.Demo_IsPlayingBack())
		can_input = interfaces::inputSystem->IsButtonDown(MOUSE_RIGHT);
	else
		can_input = y_spt_cam_drive.GetBool();
	return can_input && !interfaces::engine_vgui->IsGameUIVisible();
}

void Camera::GetCurrentView()
{
	current_cam.origin = spt_playerio.GetPlayerEyePos();
	float ang[3];
	EngineGetViewAngles(ang);
	current_cam.angles = QAngle(ang[0], ang[1], 0);
}

void Camera::RefreshTimeOffset()
{
	time_offset = interfaces::engine_tool->ClientTime() - spt_demostuff.Demo_GetPlaybackTick() / 66.6666f;
}

void Camera::OverrideView(CViewSetup* view)
{
	int control_type = CanOverrideView() ? y_spt_cam_control.GetInt() : 0;
	if (_y_spt_force_fov.GetBool())
		current_cam.fov = _y_spt_force_fov.GetFloat();
	else
		current_cam.fov = view->fov;
	HandleDriveMode(control_type == 1);
	HandleCinematicMode(control_type == 2 && spt_demostuff.Demo_IsPlayingBack());
	if (control_type)
	{
		view->origin = current_cam.origin;
		view->angles = current_cam.angles;
	}
	view->fov = current_cam.fov;
}

void Camera::HandleDriveMode(bool active)
{
	static bool drive_active = false;
	if (drive_active ^ active)
	{
		if (drive_active && !active)
		{
			// On drive mode end
			HandleInput(false);
		}
		else
		{
			// On drive mode start
			GetCurrentView();
		}
	}
	HandleInput(active && CanInput());
	drive_active = active;
}

static bool isBindDown(const char* bind)
{
	const char* key = interfaces::engine_client->Key_LookupBinding(bind);
	if (key)
	{
		ButtonCode_t code = interfaces::inputSystem->StringToButtonCode(key);
		return interfaces::inputSystem->IsButtonDown(code);
	}
	return false;
}

void Camera::HandleInput(bool active)
{
	static bool input_active = false;

	static std::chrono::time_point<std::chrono::steady_clock> last_frame;

	if (input_active ^ active)
	{
		if (input_active && !active)
		{
			// On input mode end
			interfaces::vgui_input->SetCursorPos(old_cursor[0], old_cursor[1]);
		}
		else
		{
			// On input mode start
			last_frame = std::chrono::steady_clock::now();
			interfaces::vgui_input->GetCursorPos(old_cursor[0], old_cursor[1]);
		}
	}

	if (active)
	{
		auto now = std::chrono::steady_clock::now();
		float real_frame_time =
		    std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame).count();
		last_frame = now;

		float frame_time = real_frame_time;
		Vector movement(0.0f, 0.0f, 0.0f);

		float move_speed = isBindDown("+speed") ? 525.0f : (isBindDown("+duck") ? 60.0 : 175.0f);

		if (isBindDown("+forward"))
			movement.x += 1.0f;
		if (isBindDown("+back"))
			movement.x -= 1.0f;
		if (isBindDown("+moveleft"))
			movement.y -= 1.0f;
		if (isBindDown("+moveright"))
			movement.y += 1.0f;
		if (isBindDown("+moveup"))
			movement.z += 1.0f;
		if (isBindDown("+movedown"))
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
		QAngle angles = current_cam.angles;
		angles.x += pitch;
		angles.x = clamp(angles.x, -89.0f, 89.0f);
		angles.y += yaw;
		if (angles.y > 180.0f)
			angles.y -= 360.0f;
		else if (angles.y < -180.0f)
			angles.y += 360.0f;
		current_cam.angles = angles;

		// Now apply forward, side, up

		Vector fwd, side, up;

		AngleVectors(angles, &fwd, &side, &up);

		VectorNormalize(movement);
		movement *= move_speed * frame_time;

		current_cam.origin += fwd * movement.x;
		current_cam.origin += side * movement.y;
		current_cam.origin += up * movement.z;
	}
	input_active = active;
}

// Canera path interp code from SourceAutoRecord
// https://github.com/p2sr/SourceAutoRecord/blob/master/src/Features/Camera.cpp
static float InterpCurve(std::vector<Vector> points, float x, bool is_angles = false)
{
	enum
	{
		FIRST,
		PREV,
		NEXT,
		LAST
	};

	if (is_angles)
	{
		float old_y = 0;
		for (int i = 0; i < 4; i++)
		{
			float ang_diff = points[i].y - old_y;
			ang_diff += (ang_diff > 180) ? -360 : (ang_diff < -180) ? 360 : 0;
			points[i].y = old_y += ang_diff;
			old_y = points[i].y;
		}
	}

	if (x <= points[PREV].x)
		return points[PREV].y;
	if (x >= points[NEXT].x)
		return points[NEXT].y;

	float t = (x - points[PREV].x) / (points[NEXT].x - points[PREV].x);

	float t2 = t * t, t3 = t * t * t;
	float x0 = (points[FIRST].x - points[PREV].x) / (points[NEXT].x - points[PREV].x);
	float x1 = 0, x2 = 1;
	float x3 = (points[LAST].x - points[PREV].x) / (points[NEXT].x - points[PREV].x);
	float m1 = ((points[NEXT].y - points[PREV].y) / (x2 - x1) + (points[PREV].y - points[FIRST].y) / (x1 - x0)) / 2;
	float m2 = ((points[LAST].y - points[NEXT].y) / (x3 - x2) + (points[NEXT].y - points[PREV].y) / (x2 - x1)) / 2;

	return (2 * t3 - 3 * t2 + 1) * points[PREV].y + (t3 - 2 * t2 + t) * m1 + (-2 * t3 + 3 * t2) * points[NEXT].y
	       + (t3 - t2) * m2;
}

std::vector<Vector> Camera::CameraInfoToPoints(float* x, CameraInfo* y, CameraInfoParameter param)
{
	std::vector<Vector> points;
	for (int i = 0; i < 4; i++)
	{
		Vector v;
		v.x = x[i];
		CameraInfo cs = y[i];
		switch (param)
		{
		case ORIGIN_X:
			v.y = cs.origin.x;
			break;
		case ORIGIN_Y:
			v.y = cs.origin.y;
			break;
		case ORIGIN_Z:
			v.y = cs.origin.z;
			break;
		case ANGLES_X:
			v.y = cs.angles.x;
			break;
		case ANGLES_Y:
			v.y = cs.angles.y;
			break;
		case ANGLES_Z:
			v.y = cs.angles.z;
			break;
		case FOV:
			v.y = cs.fov;
			break;
		}
		points.push_back(v);
	}
	return points;
}

Camera::CameraInfo Camera::InterpPath(float time)
{
	enum
	{
		FIRST,
		PREV,
		NEXT,
		LAST
	};

	// reading 4 frames closest to time
	float frame_time = time * 66.6666f;
	int frames[4] = {INT_MIN, INT_MIN, INT_MAX, INT_MAX};
	for (auto const& state : keyframes)
	{
		int state_time = state.first;
		if (frame_time - state_time >= 0 && frame_time - state_time < frame_time - frames[PREV])
		{
			frames[FIRST] = frames[PREV];
			frames[PREV] = state_time;
		}
		if (state_time - frame_time >= 0 && state_time - frame_time < frames[NEXT] - frame_time)
		{
			frames[LAST] = frames[NEXT];
			frames[NEXT] = state_time;
		}
		if (state_time > frames[FIRST] && state_time < frames[PREV])
		{
			frames[FIRST] = state_time;
		}
		if (state_time > frames[NEXT] && state_time < frames[LAST])
		{
			frames[LAST] = state_time;
		}
	}

	// x values for interpolation
	float x[4];
	for (int i = 0; i < 4; i++)
		x[i] = (float)frames[i];

	// making sure all X values are correct before reading Y values,
	// "frames" fixed for read, "x" fixed for interpolation.
	// if there's at least one cam state in the map, none of these should be MIN or MAX after this.
	if (frames[PREV] == INT_MIN)
	{
		x[PREV] = (float)frames[NEXT];
		frames[PREV] = frames[NEXT];
	}
	if (frames[NEXT] == INT_MAX)
	{
		x[NEXT] = (float)frames[PREV];
		frames[NEXT] = frames[PREV];
	}
	if (frames[FIRST] == INT_MIN)
	{
		x[FIRST] = (float)(2 * frames[PREV] - frames[NEXT]);
		frames[FIRST] = frames[PREV];
	}
	if (frames[LAST] == INT_MAX)
	{
		x[LAST] = (float)(2 * frames[NEXT] - frames[PREV]);
		frames[LAST] = frames[NEXT];
	}

	// filling Y values
	CameraInfo y[4];
	for (int i = 0; i < 4; i++)
	{
		y[i] = keyframes[frames[i]];
	}

	// interpolating each parameter
	CameraInfo interp;
	interp.origin.x = InterpCurve(CameraInfoToPoints(x, y, ORIGIN_X), frame_time);
	interp.origin.y = InterpCurve(CameraInfoToPoints(x, y, ORIGIN_Y), frame_time);
	interp.origin.z = InterpCurve(CameraInfoToPoints(x, y, ORIGIN_Z), frame_time);
	interp.angles.x = InterpCurve(CameraInfoToPoints(x, y, ANGLES_X), frame_time, true);
	interp.angles.y = InterpCurve(CameraInfoToPoints(x, y, ANGLES_Y), frame_time, true);
	interp.angles.z = InterpCurve(CameraInfoToPoints(x, y, ANGLES_Z), frame_time, true);
	interp.fov = InterpCurve(CameraInfoToPoints(x, y, FOV), frame_time);

	return interp;
}

void Camera::HandleCinematicMode(bool active)
{
	if (!active)
		return;
	if (!keyframes.size())
		return;
	float current_time = interfaces::engine_tool->ClientTime() - time_offset;
	current_cam = InterpPath(current_time);
}

void Camera::RecomputeInterpPath()
{
	interp_path_cache.clear();
	if (!keyframes.size())
		return;
	int start = keyframes.begin()->first;
	int end = keyframes.rbegin()->first;
	for (int i = start; i <= end; i++)
	{
		interp_path_cache.push_back(InterpPath(i / 66.6666f));
	}
}

void Camera::DrawPath()
{
	if (!y_spt_cam_path_draw.GetBool() || !spt_demostuff.Demo_IsPlayingBack())
		return;
	if (!interp_path_cache.size())
		return;
	auto last = interp_path_cache[0];
	int start = keyframes.begin()->first;

#ifdef OE
#define NDEBUG_PERSIST_TILL_NEXT_SERVER 0.01023f
#endif

	for (size_t i = 0; i < interp_path_cache.size(); i++)
	{
		auto current = interp_path_cache[i];
		Vector forward;
		AngleVectors(current.angles, &forward);
		interfaces::debugOverlay->AddLineOverlay(current.origin,
		                                         last.origin,
		                                         255,
		                                         255,
		                                         255,
		                                         false,
		                                         NDEBUG_PERSIST_TILL_NEXT_SERVER);
		if (keyframes.find(start + i) != keyframes.end())
		{
			interfaces::debugOverlay->AddLineOverlay(current.origin,
			                                         current.origin + forward * 50.0f,
			                                         0,
			                                         255,
			                                         0,
			                                         false,
			                                         NDEBUG_PERSIST_TILL_NEXT_SERVER);
			interfaces::debugOverlay->AddTextOverlay(current.origin,
			                                         NDEBUG_PERSIST_TILL_NEXT_SERVER,
			                                         "[%d]",
			                                         start + i);
		}
		else
			interfaces::debugOverlay->AddLineOverlay(current.origin,
			                                         current.origin + forward * 25.0f,
			                                         255,
			                                         255,
			                                         0,
			                                         false,
			                                         NDEBUG_PERSIST_TILL_NEXT_SERVER);
		last = current;
	}
}

HOOK_THISCALL(void, Camera, ClientModeShared__OverrideView, CViewSetup* view)
{
	spt_camera.ORIG_ClientModeShared__OverrideView(thisptr, edx, view);
	spt_camera.OverrideView(view);
}

HOOK_THISCALL(bool, Camera, ClientModeShared__CreateMove, float flInputSampleTime, void* cmd)
{
	if (spt_camera.CanInput())
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

HOOK_THISCALL(bool, Camera, C_BasePlayer__ShouldDrawLocalPlayer)
{
	if (spt_camera.CanOverrideView() && y_spt_cam_control.GetBool())
	{
		return true;
	}
	return spt_camera.ORIG_C_BasePlayer__ShouldDrawLocalPlayer(thisptr, edx);
}

#if defined(SSDK2013)
HOOK_THISCALL(bool, Camera, C_BasePlayer__ShouldDrawThisPlayer)
{
	// ShouldDrawLocalPlayer only decides draw view model or weapon model in steampipe
	// We need ShouldDrawThisPlayer to make player model draw
	if (spt_camera.CanOverrideView() && y_spt_cam_control.GetBool())
	{
		return true;
	}
	return spt_camera.ORIG_C_BasePlayer__ShouldDrawThisPlayer(thisptr, edx);
}
#endif

void __fastcall Camera::HOOKED_CInput__MouseMove(void* thisptr, int edx, void* cmd)
{
	// Block mouse inputs and stop the game from resetting cursor pos
	if (spt_camera.CanInput())
		return;
	spt_camera.ORIG_CInput__MouseMove(thisptr, edx, cmd);
}

bool Camera::ShouldLoadFeature()
{
	return interfaces::engine_client != nullptr && interfaces::engine_vgui != nullptr
	       && interfaces::vgui_input != nullptr && interfaces::inputSystem != nullptr;
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
		InitConcommandBase(_y_spt_force_fov);
		InitCommand(y_spt_cam_setpos);
		InitCommand(y_spt_cam_setang);

		if (spt_demostuff.Demo_IsDemoPlayerAvailable() && DemoStartPlaybackSignal.Works
		    && interfaces::engine_tool)
		{
			DemoStartPlaybackSignal.Connect(this, &Camera::RefreshTimeOffset);
			InitCommand(y_spt_cam_path_setkf);
			InitCommand(y_spt_cam_path_showkfs);
			InitCommand(y_spt_cam_path_getkfs);
			InitCommand(y_spt_cam_path_rmkf);
			InitCommand(y_spt_cam_path_clear);
			if (interfaces::debugOverlay && FrameSignal.Works)
			{
				FrameSignal.Connect(this, &Camera::DrawPath);
				InitConcommandBase(y_spt_cam_path_draw);
			}
		}

		sensitivity = interfaces::g_pCVar->FindVar("sensitivity");
	}
}

void Camera::UnloadFeature() {}

#endif