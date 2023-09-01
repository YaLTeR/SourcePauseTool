#pragma once

#include "..\feature.hpp"
#include "..\aim\aimstuff.hpp"
#include "..\strafe\strafestuff.hpp"
#include <deque>

typedef int ANGCOMPONENT;
enum ANGCHANGETYPE
{
	SET,
	TURN
};

typedef struct _angchange_command
{
public:
	ANGCOMPONENT component;
	ANGCHANGETYPE type;
	float value;
	_angchange_command() {}
} angchange_command_t;


class AimFeature : public FeatureWrapper<AimFeature>
{
public:
	aim::ViewState viewState;
	void HandleAiming(float* va, bool& yawChanged, const Strafe::StrafeInput& input);
	void SetPitch(float pitch);
	void TurnPitch(float add);
	void SetYaw(float yaw);
	void TurnYaw(float yaw);
	void ResetPitchYawCommands();
	void SetJump();
	virtual void LoadFeature() override;

protected:
private:
	void AddChange(angchange_command_t change);
	bool DoAngleChange(float& angle, float target);
	float DoAngleTurn(float& angle, float left);
	ConVar* cl_pitchup;
	ConVar* cl_pitchdown;
	bool IsPitchWithinLimits(float pitch);
	float BoundPitch(float pitch);
	std::deque<angchange_command_t> angChanges;
};

extern AimFeature spt_aim;
