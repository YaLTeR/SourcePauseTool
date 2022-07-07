#include "..\feature.hpp"
#include "view_shared.h"

typedef void(__fastcall* _ClientModeShared__OverrideView)(void* thisptr, int edx, CViewSetup* view);
typedef bool(__fastcall* _ClientModeShared__CreateMove)(void* thisptr, int edx, float flInputSampleTime, void* cmd);
typedef bool(__fastcall* _C_BasePlayer__ShouldDrawLocalPlayer)(void* thisptr, int edx);
typedef bool(__fastcall* _C_BasePlayer__ShouldDrawThisPlayer)(void* thisptr, int edx);
typedef void(__fastcall* _CInput__MouseMove)(void* thisptr, int edx, void* cmd);

// Camera stuff
class Camera : public FeatureWrapper<Camera>
{
public:
	bool ShouldOverrideView() const;
	bool IsInDriveMode() const;
	Vector cam_origin;
	QAngle cam_angles;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	static void __fastcall HOOKED_ClientModeShared__OverrideView(void* thisptr, int edx, CViewSetup* view);
	static bool __fastcall HOOKED_ClientModeShared__CreateMove(void* thisptr,
	                                                           int edx,
	                                                           float flInputSampleTime,
	                                                           void* cmd);
	static bool __fastcall HOOKED_C_BasePlayer__ShouldDrawLocalPlayer(void* thisptr, int edx);
	static void __fastcall HOOKED_CInput__MouseMove(void* thisptr, int edx, void* cmd);
	void OverrideView(CViewSetup* view);
	void HandleInput(bool active);
	void GetCurrentView();

	_ClientModeShared__OverrideView ORIG_ClientModeShared__OverrideView = nullptr;
	_ClientModeShared__CreateMove ORIG_ClientModeShared__CreateMove = nullptr;
	_C_BasePlayer__ShouldDrawLocalPlayer ORIG_C_BasePlayer__ShouldDrawLocalPlayer = nullptr;
	_CInput__MouseMove ORIG_CInput__MouseMove = nullptr;

	bool loadingSuccessful = false;

	bool input_active = false;
	int old_cursor[2] = {0, 0};

	const ConVar* sensitivity;

#if defined(SSDK2013)
	static bool __fastcall HOOKED_C_BasePlayer__ShouldDrawThisPlayer(void* thisptr, int edx);
	_C_BasePlayer__ShouldDrawThisPlayer ORIG_C_BasePlayer__ShouldDrawThisPlayer = nullptr;
#endif
};

extern Camera spt_camera;
