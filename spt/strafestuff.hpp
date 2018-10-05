#pragma once

// This code is a messed up version of hlstrafe,
// go take a look at that instead:
// https://github.com/HLTAS/hlstrafe

struct MovementVars {
	float Accelerate;
	float Airaccelerate;
	float EntFriction;
	float Frametime;
	float Friction;
	float Maxspeed;
	float Stopspeed;
	float WishspeedCap;
};

struct PlayerData {
	Vector Velocity;
};

enum class Button : unsigned char {
	FORWARD = 0,
	FORWARD_LEFT,
	LEFT,
	BACK_LEFT,
	BACK,
	BACK_RIGHT,
	RIGHT,
	FORWARD_RIGHT
};

struct StrafeButtons {
	StrafeButtons() :
		AirLeft(Button::FORWARD),
		AirRight(Button::FORWARD),
		GroundLeft(Button::FORWARD),
		GroundRight(Button::FORWARD) {}

	Button AirLeft;
	Button AirRight;
	Button GroundLeft;
	Button GroundRight;
};

struct ProcessedFrame {
	bool Forward;
	bool Back;
	bool Right;
	bool Left;
	bool Jump;

	double Yaw;
	float ForwardSpeed;
	float SideSpeed;

	ProcessedFrame()
		: Forward(false)
		, Back(false)
		, Right(false)
		, Left(false)
		, Jump(false)
		, Yaw(0)
		, ForwardSpeed(0)
		, SideSpeed(0)
	{
	}
};

struct CurrentState {
	float LgagstMinSpeed;
	bool LgagstFullMaxspeed;
};

enum class StrafeType {
	MAXACCEL = 0,
	MAXANGLE = 1
};

enum class StrafeDir {
	LEFT = 0,
	RIGHT = 1,
	YAW = 3
};

// Convert both arguments to doubles.
double Atan2(double a, double b);

double MaxAccelTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed);

double MaxAccelIntoYawTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, double vel_yaw, double yaw);

double MaxAngleTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, bool& safeguard_yaw);

void VectorFME(PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, const Vector2D& a);

double ButtonsPhi(Button button);

Button GetBestButtons(double theta, bool right);

void SideStrafeGeneral(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed,
	const StrafeButtons& strafeButtons, bool useGivenButtons, Button& usedButton, double vel_yaw, double theta, bool right, Vector2D& velocity, double& yaw);

double YawStrafeMaxAccel(PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons, Button& usedButton,
	double vel_yaw, double yaw);

double YawStrafeMaxAngle(PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons, Button& usedButton,
	double vel_yaw, double yaw);

void StrafeVectorial(PlayerData& player, const MovementVars& vars, bool onground, bool jumped, bool ducking, StrafeType type, StrafeDir dir, double target_yaw, double vel_yaw, ProcessedFrame& out, bool reduceWishspeed);

bool Strafe(PlayerData& player, const MovementVars& vars, bool onground, bool jumped, bool ducking, StrafeType type, StrafeDir dir, double target_yaw, double vel_yaw, ProcessedFrame& out, bool reduceWishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons);

void Friction(PlayerData& player, bool onground, const MovementVars& vars);

void LgagstJump(const PlayerData& player, const MovementVars& vars, const CurrentState& curState, bool onground, bool ducking, StrafeType type, StrafeDir dir, double target_yaw, double vel_yaw, ProcessedFrame& out, bool reduceWishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons);
