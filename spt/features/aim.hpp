#pragma once

#include "..\feature.hpp"
#include "..\aim\aimstuff.hpp"

typedef struct
{
	float angle = 0.0f;
	bool set = false;
} angset_command_t;

class AimFeature : public FeatureWrapper<AimFeature>
{
public:
	aim::ViewState viewState;
	void HandleAiming(float* va, bool& yawChanged);
	bool DoAngleChange(float& angle, float target);
	void SetPitch(float pitch);
	void SetYaw(float yaw);
	void ResetPitchYawCommands();
	void SetJump();
	virtual void LoadFeature() override;

protected:
private:
	angset_command_t setPitch, setYaw;
};

extern AimFeature spt_aim;