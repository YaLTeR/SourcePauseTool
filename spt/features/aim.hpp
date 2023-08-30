#pragma once

#include "..\feature.hpp"
#include "..\aim\aimstuff.hpp"
#include "..\strafe\strafestuff.hpp"

typedef struct _angset_command
{
	float angle;
	bool set;
	_angset_command() : angle(0.0f), set(false) {}
} angset_command_t;

typedef struct _angturn_command
{
	float old; // degrees to snap
	float left; // degrees left in turn
	_angturn_command() : left(0.0f) {}
} angturn_command_t;

class AimFeature : public FeatureWrapper<AimFeature>
{
public:
	aim::ViewState viewState;
	void HandleAiming(float* va, bool& yawChanged, const Strafe::StrafeInput& input);
	bool DoAngleChange(float& angle, float target);
	float DoAngleTurn(float& angle, float left);
	void SetPitch(float pitch);
	void TurnPitch(float add);
	void SetYaw(float yaw);
	void TurnYaw(float yaw);
	void ResetPitchYawCommands();
	void SetJump();
	virtual void LoadFeature() override;

protected:
private:
	angset_command_t setPitch, setYaw;
	angturn_command_t turnPitch, turnYaw;
};

extern AimFeature spt_aim;
