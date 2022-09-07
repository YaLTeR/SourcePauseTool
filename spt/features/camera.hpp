#pragma once
#include "..\feature.hpp"
#include "view_shared.h"
#include "demo.hpp"

#include <sstream>
#include <map>

// Camera stuff
class Camera : public FeatureWrapper<Camera>
{
public:
	enum CameraInfoParameter
	{
		ORIGIN_X,
		ORIGIN_Y,
		ORIGIN_Z,
		ANGLES_X,
		ANGLES_Y,
		ANGLES_Z,
		FOV
	};
	struct CameraInfo
	{
		Vector origin = Vector();
		QAngle angles = QAngle();
		float fov = 75.0f;
		operator std::string() const
		{
			std::ostringstream s;
			s << "pos: " << origin.x << " " << origin.y << " " << origin.z << '\n';
			s << "ang: " << angles.x << " " << angles.y << " " << angles.z << '\n';
			s << "fov: " << fov;
			return s.str();
		}
	};
	CameraInfo current_cam;
	std::map<int, CameraInfo> keyframes;

	bool CanOverrideView() const;
	bool CanInput() const;
	void RecomputeInterpPath();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	DECL_HOOK_THISCALL(void, ClientModeShared__OverrideView, CViewSetup* view);
	DECL_HOOK_THISCALL(bool, ClientModeShared__CreateMove, float flInputSampleTime, void* cmd);
	DECL_HOOK_THISCALL(bool, C_BasePlayer__ShouldDrawLocalPlayer);
	DECL_HOOK_THISCALL(void, CInput__MouseMove, void* cmd);
#if defined(SSDK2013)
	DECL_HOOK_THISCALL(bool, C_BasePlayer__ShouldDrawThisPlayer);
#endif

	void OverrideView(CViewSetup* view);
	void HandleDriveMode(bool active);
	void HandleInput(bool active);
	void HandleCinematicMode(bool active);
	static std::vector<Vector> CameraInfoToPoints(float* x, CameraInfo* y, CameraInfoParameter param);
	CameraInfo InterpPath(float time);
	void GetCurrentView();
	void RefreshTimeOffset();
	void DrawPath();

	bool loadingSuccessful = false;

	int old_cursor[2] = {0, 0};
	float time_offset = 0.0f;
	std::vector<CameraInfo> interp_path_cache;

	const ConVar* sensitivity;
};

extern Camera spt_camera;