#pragma once

#include "cmodel.h"

// This code is a messed up version of hlstrafe,
// go take a look at that instead:
// https://github.com/HLTAS/hlstrafe

namespace Strafe
{
	struct TraceResult
	{
		bool AllSolid;
		bool StartSolid;
		float Fraction;
		Vector EndPos;
		Vector PlaneNormal;
		int Entity;
	};

	struct MovementVars
	{
		float Scale;
		float Accelerate;
		float Airaccelerate;
		float EntFriction;
		float Frametime;
		float Friction;
		float Maxspeed;
		float Stopspeed;
		float WishspeedCap;

		float EntGravity;
		float Maxvelocity;
		float Gravity;
		float Stepsize;
		float Bounce;

		bool OnGround;
		bool CantJump;
		bool ReduceWishspeed;
	};

	struct PlayerData
	{
		Vector UnduckedOrigin;
		Vector Velocity;
		Vector Basevelocity;
		bool Ducking;
		bool DuckPressed;
	};

	enum class Button : unsigned char
	{
		FORWARD = 0,
		FORWARD_LEFT,
		LEFT,
		BACK_LEFT,
		BACK,
		BACK_RIGHT,
		RIGHT,
		FORWARD_RIGHT
	};

	struct StrafeButtons
	{
		StrafeButtons()
		    : AirLeft(Button::FORWARD)
		    , AirRight(Button::FORWARD)
		    , GroundLeft(Button::FORWARD)
		    , GroundRight(Button::FORWARD)
		{
		}

		Button AirLeft;
		Button AirRight;
		Button GroundLeft;
		Button GroundRight;
	};

	struct ProcessedFrame
	{
		bool Processed; // Should apply strafing in ClientDLL?
		bool Forward;
		bool Back;
		bool Right;
		bool Left;
		bool Jump;
		bool ForceUnduck;

		double Yaw;
		float ForwardSpeed;
		float SideSpeed;

		ProcessedFrame()
		    : Processed(false)
		    , Forward(false)
		    , Back(false)
		    , Right(false)
		    , Left(false)
		    , Jump(false)
		    , Yaw(0)
		    , ForwardSpeed(0)
		    , SideSpeed(0)
		    , ForceUnduck(false)
		{
		}
	};

	struct CurrentState
	{
		float LgagstMinSpeed;
		bool LgagstFullMaxspeed;
	};

	enum class StrafeType
	{
		MAXACCEL = 0,
		MAXANGLE = 1,
		CAPPED = 2,
		DIRECTION = 3
	};

	enum class StrafeDir
	{
		LEFT = 0,
		RIGHT = 1,
		YAW = 3
	};

	enum class PositionType
	{
		GROUND = 0,
		AIR,
		WATER
	};

	enum class HullType : int
	{
		NORMAL = 0,
		DUCKED = 1,
		POINT = 2
	};

	void TracePlayer(trace_t& trace, const Vector& start, const Vector& end, HullType hull);
	void Trace(trace_t& trace, const Vector& start, const Vector& end);

	bool CanUnduck(const PlayerData& player);

	PositionType GetPositionType(PlayerData& player, HullType hull);

	PositionType Move(PlayerData& player, const MovementVars& vars);

	// Convert both arguments to doubles.
	double Atan2(double a, double b);

	double MaxAccelTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed);

	double MaxAccelIntoYawTheta(const PlayerData& player,
	                            const MovementVars& vars,
	                            bool onground,
	                            double wishspeed,
	                            double vel_yaw,
	                            double yaw);

	double MaxAngleTheta(const PlayerData& player,
	                     const MovementVars& vars,
	                     bool onground,
	                     double wishspeed,
	                     bool& safeguard_yaw);

	void VectorFME(PlayerData& player,
	               const MovementVars& vars,
	               bool onground,
	               double wishspeed,
	               const Vector2D& a);

	double ButtonsPhi(Button button);

	Button GetBestButtons(double theta, bool right);

	void SideStrafeGeneral(const PlayerData& player,
	                       const MovementVars& vars,
	                       bool onground,
	                       double wishspeed,
	                       const StrafeButtons& strafeButtons,
	                       bool useGivenButtons,
	                       Button& usedButton,
	                       double vel_yaw,
	                       double theta,
	                       bool right,
	                       Vector2D& velocity,
	                       double& yaw);

	double YawStrafeMaxAccel(PlayerData& player,
	                         const MovementVars& vars,
	                         bool onground,
	                         double wishspeed,
	                         const StrafeButtons& strafeButtons,
	                         bool useGivenButtons,
	                         Button& usedButton,
	                         double vel_yaw,
	                         double yaw);

	double YawStrafeMaxAngle(PlayerData& player,
	                         const MovementVars& vars,
	                         bool onground,
	                         double wishspeed,
	                         const StrafeButtons& strafeButtons,
	                         bool useGivenButtons,
	                         Button& usedButton,
	                         double vel_yaw,
	                         double yaw);

	void StrafeVectorial(PlayerData& player,
	                     const MovementVars& vars,
	                     bool jumped,
	                     StrafeType type,
	                     StrafeDir dir,
	                     double target_yaw,
	                     double vel_yaw,
	                     ProcessedFrame& out,
	                     bool lockCamera);

	bool Strafe(PlayerData& player,
	            const MovementVars& vars,
	            bool jumped,
	            StrafeType type,
	            StrafeDir dir,
	            double target_yaw,
	            double vel_yaw,
	            ProcessedFrame& out,
	            const StrafeButtons& strafeButtons,
	            bool useGivenButtons);

	void Friction(PlayerData& player, bool onground, const MovementVars& vars);

	bool LgagstJump(PlayerData& player, const MovementVars& vars);
} // namespace Strafe
