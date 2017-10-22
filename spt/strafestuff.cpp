#include "stdafx.hpp"

#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <algorithm>
#include <sstream>
#include <iomanip>

#include "OrangeBox/cvars.hpp"
#include "strafestuff.hpp"

// This code is a messed up version of hlstrafe,
// go take a look at that instead:
// https://github.com/HLTAS/hlstrafe

static const constexpr double SAFEGUARD_THETA_DIFFERENCE_RAD = M_PI / 65536;

// Return angle in [-Pi; Pi).
inline double NormalizeRad(double a)
{
	a = std::fmod(a, M_PI * 2);
	if (a >= M_PI)
		a -= 2 * M_PI;
	else if (a < -M_PI)
		a += 2 * M_PI;
	return a;
}

inline double NormalizeDeg(double a)
{
	a = std::fmod(a, 360.0);
	if (a >= 180.0)
		a -= 360.0;
	else if (a < -180.0)
		a += 360.0;
	return a;
}

// Convert both arguments to doubles.
inline double Atan2(double a, double b)
{
	return std::atan2(a, b);
}

double MaxAccelTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed)
{
	double accel = onground ? vars.Accelerate : vars.Airaccelerate;
	double accelspeed = accel * wishspeed * vars.EntFriction * vars.Frametime;
	if (accelspeed <= 0.0)
		return M_PI;

	if (player.Velocity.AsVector2D().IsZero(0))
		return 0.0;

	double wishspeed_capped = onground ? wishspeed : vars.WishspeedCap;
	double tmp = wishspeed_capped - accelspeed;
	if (tmp <= 0.0)
		return M_PI / 2;

	double speed = player.Velocity.Length2D();
	if (tmp < speed)
		return std::acos(tmp / speed);

	return 0.0;
}

double MaxAccelIntoYawTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, double vel_yaw, double yaw)
{
	if (!player.Velocity.AsVector2D().IsZero(0))
		vel_yaw = Atan2(player.Velocity.y, player.Velocity.x);

	double theta = MaxAccelTheta(player, vars, onground, wishspeed);
	if (theta == 0.0 || theta == M_PI)
		return NormalizeRad(yaw - vel_yaw + theta);
	return std::copysign(theta, NormalizeRad(yaw - vel_yaw));
}

double MaxAngleTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, bool& safeguard_yaw)
{
	safeguard_yaw = false;
	double speed = player.Velocity.Length2D();
	double accel = onground ? vars.Accelerate : vars.Airaccelerate;
	double accelspeed = accel * wishspeed * vars.EntFriction * vars.Frametime;

	if (accelspeed <= 0.0) {
		double wishspeed_capped = onground ? wishspeed : vars.WishspeedCap;
		accelspeed *= -1;
		if (accelspeed >= speed) {
			if (wishspeed_capped >= speed)
				return 0.0;
			else {
				safeguard_yaw = true;
				return std::acos(wishspeed_capped / speed); // The actual angle needs to be _less_ than this.
			}
		} else {
			if (wishspeed_capped >= speed)
				return std::acos(accelspeed / speed);
			else {
				safeguard_yaw = (wishspeed_capped <= accelspeed);
				return std::acos(std::min(accelspeed, wishspeed_capped) / speed); // The actual angle needs to be _less_ than this if wishspeed_capped <= accelspeed.
			}
		}
	} else {
		if (accelspeed >= speed)
			return M_PI;
		else
			return std::acos(-1 * accelspeed / speed);
	}
}

void VectorFME(PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, const Vector2D& a)
{
	double wishspeed_capped = onground ? wishspeed : vars.WishspeedCap;
	double tmp = wishspeed_capped - player.Velocity.AsVector2D().Dot(a);
	if (tmp <= 0.0)
		return;

	double accel = onground ? vars.Accelerate : vars.Airaccelerate;
	double accelspeed = accel * wishspeed * vars.EntFriction * vars.Frametime;
	if (accelspeed <= tmp)
		tmp = accelspeed;

	player.Velocity.x += static_cast<float>(a.x * tmp);
	player.Velocity.y += static_cast<float>(a.y * tmp);
}

double ButtonsPhi(Button button)
{
	switch (button) {
	case Button::FORWARD: return 0;
	case Button::FORWARD_LEFT: return M_PI / 4;
	case Button::LEFT: return M_PI / 2;
	case Button::BACK_LEFT: return 3 * M_PI / 4;
	case Button::BACK: return -M_PI;
	case Button::BACK_RIGHT: return -3 * M_PI / 4;
	case Button::RIGHT: return -M_PI / 2;
	case Button::FORWARD_RIGHT: return -M_PI / 4;
	default: return 0;
	}
}

Button GetBestButtons(double theta, bool right)
{
	if (theta < M_PI / 8)
		return Button::FORWARD;
	else if (theta < 3 * M_PI / 8)
		return right ? Button::FORWARD_RIGHT : Button::FORWARD_LEFT;
	else if (theta < 5 * M_PI / 8)
		return right ? Button::RIGHT : Button::LEFT;
	else if (theta < 7 * M_PI / 8)
		return right ? Button::BACK_RIGHT : Button::BACK_LEFT;
	else
		return Button::BACK;
}

void SideStrafeGeneral(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed,
	const StrafeButtons& strafeButtons, bool useGivenButtons, Button& usedButton, double vel_yaw, double theta, bool right, Vector2D& velocity, double& yaw)
{
	if (useGivenButtons) {
		if (!onground) {
			if (right)
				usedButton = strafeButtons.AirRight;
			else
				usedButton = strafeButtons.AirLeft;
		} else {
			if (right)
				usedButton = strafeButtons.GroundRight;
			else
				usedButton = strafeButtons.GroundLeft;
		}
	} else {
		usedButton = GetBestButtons(theta, right);
	}
	double phi = ButtonsPhi(usedButton);
	theta = right ? -theta : theta;

	if (!player.Velocity.AsVector2D().IsZero(0))
		vel_yaw = Atan2(player.Velocity.y, player.Velocity.x);

	yaw = NormalizeRad(vel_yaw - phi + theta);

	Vector2D avec(std::cos(yaw + phi), std::sin(yaw + phi));
	PlayerData pl = player;
	VectorFME(pl, vars, onground, wishspeed, avec);
	velocity = pl.Velocity.AsVector2D();
}

double YawStrafeMaxAccel(PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons, Button& usedButton,
	double vel_yaw, double yaw)
{
	double resulting_yaw;
	double theta = MaxAccelIntoYawTheta(player, vars, onground, wishspeed, vel_yaw, yaw);
	Vector2D newvel;
	SideStrafeGeneral(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw, std::fabs(theta), (theta < 0), newvel, resulting_yaw);
	player.Velocity.AsVector2D() = newvel;

	return resulting_yaw;
}

double YawStrafeMaxAngle(PlayerData& player, const MovementVars& vars, bool onground, double wishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons, Button& usedButton,
	double vel_yaw, double yaw)
{
	bool safeguard_yaw;
	double theta = MaxAngleTheta(player, vars, onground, wishspeed, safeguard_yaw);
	if (!player.Velocity.AsVector2D().IsZero(0.0f))
		vel_yaw = Atan2(player.Velocity[1], player.Velocity[0]);

	Vector2D newvel;
	double resulting_yaw;
	SideStrafeGeneral(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw, theta, (NormalizeRad(yaw - vel_yaw) < 0), newvel, resulting_yaw);

	if (safeguard_yaw) {
		Vector2D test_vel1, test_vel2;
		double test_yaw1, test_yaw2;

		SideStrafeGeneral(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw, std::min(theta - SAFEGUARD_THETA_DIFFERENCE_RAD, 0.0), (NormalizeRad(yaw - vel_yaw) < 0), test_vel1, test_yaw1);
		SideStrafeGeneral(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw, std::max(theta + SAFEGUARD_THETA_DIFFERENCE_RAD, 0.0), (NormalizeRad(yaw - vel_yaw) < 0), test_vel2, test_yaw2);

		double cos_test1 = test_vel1.Dot(player.Velocity.AsVector2D()) / (player.Velocity.Length2D() * test_vel1.Length());
		double cos_test2 = test_vel2.Dot(player.Velocity.AsVector2D()) / (player.Velocity.Length2D() * test_vel2.Length());
		double cos_newvel = newvel.Dot(player.Velocity.AsVector2D()) / (player.Velocity.Length2D() * newvel.Length());

		//DevMsg("cos_newvel = %.8f; cos_test1 = %.8f; cos_test2 = %.8f\n", cos_newvel, cos_test1, cos_test2);

		if (cos_test1 < cos_newvel) {
			if (cos_test2 < cos_test1) {
				newvel = test_vel2;
				resulting_yaw = test_yaw2;
				cos_newvel = cos_test2;
			} else {
				newvel = test_vel1;
				resulting_yaw = test_yaw1;
				cos_newvel = cos_test1;
			}
		} else if (cos_test2 < cos_newvel) {
			newvel = test_vel2;
			resulting_yaw = test_yaw2;
			cos_newvel = cos_test2;
		}
	} else {
		//DevMsg("theta = %.08f, yaw = %.08f, vel_yaw = %.08f, speed = %.08f\n", theta, yaw, vel_yaw, player.Velocity.Length2D());
	}

	player.Velocity.AsVector2D() = newvel;
	return resulting_yaw;
}

bool Strafe(PlayerData& player, const MovementVars& vars, bool onground, bool jumped, bool ducking, StrafeType type, StrafeDir dir, double target_yaw, double vel_yaw, ProcessedFrame& out, bool reduceWishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons)
{
	//DevMsg("[Strafing] ducking = %d\n", (int)ducking);
	if (jumped && player.Velocity.Length2D() >= vars.Maxspeed * ((ducking || (vars.Maxspeed == 320)) ? 0.1 : 0.5)) {
		if (tas_strafe_glitchless.GetBool()) {
			const Vector vel = player.Velocity;
			out.Yaw = NormalizeRad(atan2(vel.y, vel.x));
		}
		
		out.Forward = false;
		out.Back = false;
		out.Right = false;
		out.Left = false;

		//Vector vecForward;
		//AngleVectors(QAngle(0, out.Yaw, 0), &vecForward);
		//vecForward.z = 0;
		//VectorNormalize(vecForward);

		//// We give a certain percentage of the current forward movement as a bonus to the jump speed.  That bonus is clipped
		//// to not accumulate over time.
		//float flSpeedBoostPerc = ((vars.Maxspeed != 320) && !ducking) ? 0.5f : 0.1f;
		//float flSpeedAddition = fabs(0 * flSpeedBoostPerc);
		//float flMaxSpeed = vars.Maxspeed + (vars.Maxspeed * flSpeedBoostPerc);
		//float flNewSpeed = (flSpeedAddition + player.Velocity.Length2D());

		//// If we're over the maximum, we want to only boost as much as will get us to the goal speed
		//if (flNewSpeed > flMaxSpeed)
		//{
		//	flSpeedAddition -= flNewSpeed - flMaxSpeed;
		//}

		//if (0 < 0.0f)
		//	flSpeedAddition *= -1.0f;

		//// Add it on
		//VectorAdd((vecForward*flSpeedAddition), player.Velocity, player.Velocity);

		return onground;
	}

	double wishspeed = vars.Maxspeed;
	if (reduceWishspeed)
		wishspeed *= 0.33333333f;

	Button usedButton;
	bool strafed;
	//if (frame.Strafe) {
		strafed = true;

		switch (dir) {
		//case StrafeDir::LEFT:
		//	if (type == StrafeType::MAXACCEL)
		//		out.Yaw = static_cast<float>(SideStrafeMaxAccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, false) * M_RAD2DEG);
		//	else if (type == StrafeType::MAXANGLE)
		//		out.Yaw = static_cast<float>(SideStrafeMaxAngle(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, false) * M_RAD2DEG);
		//	//else if (type == StrafeType::CONSTSPEED)
		//	//	out.Yaw = static_cast<float>(SideStrafeConstSpeed(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, false) * M_RAD2DEG);
		//	//else if (type == StrafeType::MAXDECCEL) {
		//	//	auto yaw = static_cast<float>(SideStrafeMaxDeccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, false, strafed) * M_RAD2DEG);
		//	//	if (strafed)
		//	//		out.Yaw = yaw;
		//	//}
		//	break;

		//case StrafeDir::RIGHT:
		//	if (type == StrafeType::MAXACCEL)
		//		out.Yaw = static_cast<float>(SideStrafeMaxAccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, true) * M_RAD2DEG);
		//	else if (type == StrafeType::MAXANGLE)
		//		out.Yaw = static_cast<float>(SideStrafeMaxAngle(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, true) * M_RAD2DEG);
		//	//else if (type == StrafeType::CONSTSPEED)
		//	//	out.Yaw = static_cast<float>(SideStrafeConstSpeed(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, true) * M_RAD2DEG);
		//	//else if (type == StrafeType::MAXDECCEL) {
		//	//	auto yaw = static_cast<float>(SideStrafeMaxDeccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, true, strafed) * M_RAD2DEG);
		//	//	if (strafed)
		//	//		out.Yaw = yaw;
		//	//}
		//	break;

		//case StrafeDir::BEST:
		//	if (type == StrafeType::MAXACCEL)
		//		out.Yaw = static_cast<float>(BestStrafeMaxAccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD) * M_RAD2DEG);
		//	else if (type == StrafeType::MAXANGLE)
		//		out.Yaw = static_cast<float>(BestStrafeMaxAngle(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD) * M_RAD2DEG);
		//	else if (type == StrafeType::CONSTSPEED)
		//		out.Yaw = static_cast<float>(BestStrafeConstSpeed(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD) * M_RAD2DEG);
		//	else if (type == StrafeType::MAXDECCEL) {
		//		auto yaw = static_cast<float>(BestStrafeMaxDeccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, strafed) * M_RAD2DEG);
		//		if (strafed)
		//			out.Yaw = yaw;
		//	}
		//	break;

		case StrafeDir::YAW:
			if (type == StrafeType::MAXACCEL)
				out.Yaw = static_cast<float>(YawStrafeMaxAccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, target_yaw * M_DEG2RAD) * M_RAD2DEG);
			else if (type == StrafeType::MAXANGLE)
				out.Yaw = static_cast<float>(YawStrafeMaxAngle(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, target_yaw * M_DEG2RAD) * M_RAD2DEG);
			//else if (type == StrafeType::CONSTSPEED)
			//	out.Yaw = static_cast<float>(YawStrafeConstSpeed(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, target_yaw * M_DEG2RAD) * M_RAD2DEG);
			//else if (type == StrafeType::MAXDECCEL) {
			//	auto yaw = static_cast<float>(YawStrafeMaxDeccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, target_yaw * M_DEG2RAD, strafed) * M_RAD2DEG);
			//	if (strafed)
			//		out.Yaw = yaw;
			//}
			break;

		//case StrafeDir::POINT:
		//{
		//	double point[] = { frame.GetX(), frame.GetY() };
		//	auto yaw = static_cast<float>(PointStrafe(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, type, point, strafed) * M_RAD2DEG);
		//	if (strafed)
		//		out.Yaw = yaw;
		//}
		//break;

		default:
			strafed = false;
			break;
		}
	//}

	//std::ostringstream o;
	//o << std::setprecision(8) << std::fixed << "[Strafing] wishspeed = " << wishspeed << "; speed = " << player.Velocity.Length2D();

	//out.Yaw = static_cast<float>(YawStrafeMaxAccel(player, vars, onground, wishspeed, strafeButtons, useGivenButtons, usedButton, vel_yaw * M_DEG2RAD, target_yaw * M_DEG2RAD) * M_RAD2DEG);

	if (strafed) {
		out.Forward = (usedButton == Button::FORWARD || usedButton == Button::FORWARD_LEFT || usedButton == Button::FORWARD_RIGHT);
		out.Back = (usedButton == Button::BACK || usedButton == Button::BACK_LEFT || usedButton == Button::BACK_RIGHT);
		out.Right = (usedButton == Button::RIGHT || usedButton == Button::FORWARD_RIGHT || usedButton == Button::BACK_RIGHT);
		out.Left = (usedButton == Button::LEFT || usedButton == Button::FORWARD_LEFT || usedButton == Button::BACK_LEFT);
	}

	//o << "; resulting yaw = " << out.Yaw << "; predicted speed = " << player.Velocity.Length2D() << "\n";
	//DevMsg("%s", o.str().c_str());

	return onground;
}

void Friction(PlayerData& player, bool onground, const MovementVars& vars)
{
	if (!onground)
		return;

	// Doing all this in floats, mismatch is too real otherwise.
	auto speed = player.Velocity.Length();
	if (speed < 0.1)
		return;

	auto friction = float{ vars.Friction * vars.EntFriction };
	auto control = (speed < vars.Stopspeed) ? vars.Stopspeed : speed;
	auto drop = control * friction * vars.Frametime;
	auto newspeed = std::max(speed - drop, 0.f);
	player.Velocity *= (newspeed / speed);
}

void LgagstJump(const PlayerData& player, const MovementVars& vars, const CurrentState& curState, bool onground, bool ducking, StrafeType type, StrafeDir dir, double target_yaw, double vel_yaw, ProcessedFrame& out, bool reduceWishspeed, const StrafeButtons& strafeButtons, bool useGivenButtons)
{
	if (player.Velocity.Length2D() < curState.LgagstMinSpeed)
		return;

	auto ground = PlayerData(player);
	Friction(ground, onground, vars);
	auto out_temp = ProcessedFrame(out);
	Strafe(ground, vars, onground, false, ducking, type, dir, target_yaw, vel_yaw, out_temp, reduceWishspeed && !curState.LgagstFullMaxspeed, strafeButtons, useGivenButtons);

	auto air = PlayerData(player);
	out_temp = ProcessedFrame(out);
	out_temp.Jump = true;
	onground = false;
	Strafe(air, vars, onground, true, ducking, type, dir, target_yaw, vel_yaw, out_temp, reduceWishspeed && !curState.LgagstFullMaxspeed, strafeButtons, useGivenButtons);

	auto l_gr = ground.Velocity.Length2D();
	auto l_air = air.Velocity.Length2D();
	if (l_air > l_gr) {
		out.Jump = true;
	}
}
