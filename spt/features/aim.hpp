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
	ANGCHANGETYPE type;
	bool doYaw;
	float yawValue;
	bool doPitch;
	float pitchValue;
	_angchange_command() : 
		yawValue(0.0f), doYaw(false),
		pitchValue(0.0f), doPitch(false),
		type(SET) {}
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
	void SetAngles(float pitch, float yaw);
	void TurnAngles(float pitch, float yaw);
	void ResetPitchYawCommands();
	void SetJump();
	virtual void LoadFeature() override;
	std::deque<angchange_command_t> angChanges;

protected:
private:
	void AddChange(angchange_command_t change);
	bool DoAngleChange(float& angle, float target);
	float DoAngleTurn(float& angle, float left);
	ConVar* cl_pitchup;
	ConVar* cl_pitchdown;
	bool IsPitchWithinLimits(float pitch);
	float BoundPitch(float pitch);
};

extern AimFeature spt_aim;
