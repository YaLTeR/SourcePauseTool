#include "stdafx.hpp"

#include "hud.hpp"

#ifdef SPT_HUD_ENABLED

#include "..\feature.hpp"
#include <algorithm>
#include "convar.hpp"
#include "interfaces.hpp"
#include "math.hpp"
#include "signals.hpp"
#include "playerio.hpp"
#include "property_getter.hpp"

#undef min
#undef max

ConVar y_spt_jhud_hops("y_spt_jhud_hops", "0", FCVAR_CHEAT, "Turns on the hop practice Jump HUD.");
ConVar y_spt_jhud_ljstats("y_spt_jhud_ljstats", "0", FCVAR_CHEAT, "Turns on the LJ stats Jump HUD.");
ConVar y_spt_jhud_velocity("y_spt_jhud_velocity", "0", FCVAR_CHEAT, "Turns on the velocity Jump HUD.");
ConVar y_spt_jhud_x("y_spt_jhud_x", "0", FCVAR_CHEAT, "Jump HUD x offset.");
ConVar y_spt_jhud_y("y_spt_jhud_y", "100", FCVAR_CHEAT, "Jump HUD y offset.");

namespace ljstats
{
	enum class AccelDirection
	{
		Left,
		Forward,
		Right
	};

	struct SegmentStats
	{
		void Reset();
		float Sync() const;

		int ticks = 0;
		int accelTicks = 0;
		int gainTicks = 0;
		AccelDirection dir = AccelDirection::Left;
		float totalAccel = 0;
		float positiveAccel = 0;
		float negativeAccel = 0;
		float distanceCovered = 0;
		float startVel = 0;
	};

	struct JumpStats
	{
		int StrafeCount();
		int TotalTicks();
		float Sync();
		float AccelPerTick();
		float TotalAccel();
		float NegativeAccel();
		float PositiveAccel();
		float TimeAccelerating();
		float Length();
		float Prestrafe();
		float CurveLoss();

		void Reset();

		Vector endSpot;
		Vector jumpSpot;
		std::vector<SegmentStats> segments;
		float startVel = 0;
	};

	static Vector currentVelocity;
	static Vector previousPos;
	static AccelDirection prevAccelDir;
	static SegmentStats currentSegment;
	static JumpStats currentJump;
	static JumpStats lastJump;

	static bool inJump = false;
	static bool firstTick = false;
	static int groundTicks = 0;
	const float EPS = 0.001f;
	const int MIN_GROUND_TICKS = 10;

	AccelDirection GetDirection()
	{
		Vector newVel = spt_playerio.m_vecAbsVelocity.GetValue();
		Vector delta = newVel - currentVelocity;

		if (delta.Length2D() < EPS)
		{
			return AccelDirection::Forward;
		}

		QAngle oldAngle, newAngle;
		VectorAngles(currentVelocity, oldAngle);
		VectorAngles(newVel, newAngle);

		float yaw = static_cast<float>(utils::NormalizeDeg(newAngle.y - oldAngle.y));

		if (yaw < 0)
		{
			return AccelDirection::Right;
		}
		else if (yaw == 0)
		{
			return AccelDirection::Forward;
		}
		else
		{
			return AccelDirection::Left;
		}
	}

	void EndSegment()
	{
		currentJump.segments.push_back(currentSegment);
		currentSegment.Reset();
	}

	void EndJump()
	{
		inJump = false;
		currentJump.endSpot = spt_playerio.m_vecAbsOrigin.GetValue();
		EndSegment();
		lastJump = currentJump;
	}

	void OnJump()
	{
		if (!y_spt_jhud_ljstats.GetBool() || groundTicks < MIN_GROUND_TICKS)
			return;

		groundTicks = 0;
		currentJump.Reset();
		currentJump.jumpSpot = previousPos = spt_playerio.m_vecAbsOrigin.GetValue();
		prevAccelDir = AccelDirection::Forward;
		inJump = true;
		firstTick = true;
	}

	void CollectStats(bool last)
	{
		float distance = (spt_playerio.m_vecAbsOrigin.GetValue() - previousPos).Length2D();

		if (firstTick)
		{
			currentVelocity = spt_playerio.m_vecAbsVelocity.GetValue();
			currentSegment.Reset();
			currentJump.startVel = currentVelocity.Length2D();
			firstTick = false;
		}
		else
		{
			Vector newVel = spt_playerio.m_vecAbsVelocity.GetValue();
			AccelDirection direction = GetDirection();

			if (direction != currentSegment.dir && direction != AccelDirection::Forward)
			{
				EndSegment();
				currentSegment.dir = direction;
			}

			float accel = newVel.Length2D() - currentVelocity.Length2D();
			currentSegment.totalAccel += accel;

			if (accel > EPS)
			{
				currentSegment.positiveAccel += accel;
				++currentSegment.gainTicks;
				++currentSegment.accelTicks;
			}
			else if (accel < -EPS)
			{
				currentSegment.negativeAccel -= accel;
				++currentSegment.accelTicks;
			}

			++currentSegment.ticks;
			if (!last)
			{
				currentVelocity = newVel;
				prevAccelDir = direction;
			}
		}

		currentSegment.distanceCovered += distance;
		previousPos = spt_playerio.m_vecAbsOrigin.GetValue();
	}

	void OnTick()
	{
		bool onground = (spt_playerio.m_fFlags.GetValue() & FL_ONGROUND) != 0;

		if (onground)
		{
			++groundTicks;
		}

		if (!inJump)
			return;

		Vector pos = spt_playerio.m_vecAbsOrigin.GetValue();
		bool last = pos.z <= currentJump.jumpSpot.z || onground;

		CollectStats(last);

		if (last)
		{
			EndJump();
			return;
		}
	}

	void SegmentStats::Reset()
	{
		ticks = 0;
		gainTicks = 0;
		accelTicks = 0;
		dir = AccelDirection::Forward;
		totalAccel = 0;
		positiveAccel = 0;
		negativeAccel = 0;
		distanceCovered = 0;
		startVel = spt_playerio.m_vecAbsVelocity.GetValue().Length2D();
	}

	float SegmentStats::Sync() const
	{
		return (static_cast<float>(gainTicks) / accelTicks) * 100;
	}

	int JumpStats::StrafeCount()
	{
		int count = 0;
		for (auto& strafe : segments)
		{
			if (strafe.dir != AccelDirection::Forward)
			{
				++count;
			}
		}

		return count;
	}

	int JumpStats::TotalTicks()
	{
		int ticks = 0;
		for (auto& strafe : segments)
		{
			ticks += strafe.ticks;
		}
		return ticks;
	}

	float JumpStats::Sync()
	{
		int gained = 0;
		int total = 0;

		for (auto& segment : segments)
		{
			gained += segment.gainTicks;
			total += segment.accelTicks;
		}

		if (total == 0)
		{
			return 0;
		}
		else
		{
			return (static_cast<float>(gained) / total) * 100;
		}
	}

	float JumpStats::AccelPerTick()
	{
		float accel = 0;
		int accelTicks = 0;

		for (auto& segment : segments)
		{
			accel += segment.totalAccel;
			accelTicks += segment.accelTicks;
		}

		if (accelTicks == 0)
		{
			return 0;
		}
		else
		{
			return accel / accelTicks;
		}
	}

	float JumpStats::TotalAccel()
	{
		float accel = 0;

		for (auto& segment : segments)
		{
			accel += segment.totalAccel;
		}

		return accel;
	}

	float JumpStats::NegativeAccel()
	{
		float accel = 0;

		for (auto& segment : segments)
		{
			accel += segment.negativeAccel;
		}

		return accel;
	}

	float JumpStats::PositiveAccel()
	{
		float accel = 0;

		for (auto& segment : segments)
		{
			accel += segment.positiveAccel;
		}

		return accel;
	}

	float JumpStats::TimeAccelerating()
	{
		int accelTicks = 0;
		int totalTicks = 0;

		for (auto& segment : segments)
		{
			accelTicks += segment.accelTicks;
			totalTicks += segment.ticks;
		}

		if (totalTicks == 0)
		{
			return 0;
		}
		else
		{
			return static_cast<double>(accelTicks) / totalTicks * 100;
		}
	}

	float JumpStats::Length()
	{
		Vector delta = endSpot - jumpSpot;
		const float COLLISION_HULL_WIDTH = 32.0f;

		return delta.Length2D() + COLLISION_HULL_WIDTH;
	}

	float JumpStats::Prestrafe()
	{
		return startVel;
	}

	float JumpStats::CurveLoss()
	{
		float totalDistance = 0;
		Vector delta = endSpot - jumpSpot;
		float length = delta.Length2D();

		for (auto& segment : segments)
		{
			totalDistance += segment.distanceCovered;
		}

		if (totalDistance < EPS)
		{
			return 0;
		}
		else
		{
			return totalDistance - length;
		}
	}

	void JumpStats::Reset()
	{
		endSpot.Init(0, 0, 0);
		jumpSpot = spt_playerio.m_vecAbsVelocity.GetValue();
		segments.clear();
		startVel = 0.0f;
	}
} // namespace ljstats

// Hops HUD
class HopsHud : public FeatureWrapper<HopsHud>
{
public:
	void OnTick();
	void OnJump();
	void OnGround(bool onground);
	void CalculateAbhVel();
	void DrawHopHud();
	bool ShouldDraw();
	void PrintStrafeCol(std::function<void(const ljstats::SegmentStats&, wchar_t* buffer, int x, int y)>,
	                    const wchar_t* fieldName,
	                    int x,
	                    int y,
	                    int MARGIN);

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	int sinceLanded = 0;
	bool velNotCalced = true;
	int lastHop = 0;
	int displayHop = 0;
	float percentage = 0.0f;
	float maxVel = 0.0f;
	float loss = 0.0f;

	// velocity hud
	float prevHopVel = 0.0f;
	float hopVel = 0.0f;
	float prevVel = 0.0f;
	float currVel = 0.0f;

	vgui::HFont hopsFont = 0;
};

static HopsHud spt_hops_hud;

bool HopsHud::ShouldLoadFeature()
{
	return spt_hud_feat.ShouldLoadFeature();
}

bool HopsHud::ShouldDraw()
{
	return y_spt_jhud_hops.GetBool() || y_spt_jhud_ljstats.GetBool() || y_spt_jhud_velocity.GetBool();
}

void HopsHud::PrintStrafeCol(std::function<void(const ljstats::SegmentStats&, wchar_t*, int, int)> func,
                             const wchar_t* fieldName,
                             int x,
                             int y,
                             int MARGIN)
{
	const int BUFFER_SIZE = 256;
	wchar_t buffer[BUFFER_SIZE];

	auto surface = interfaces::surface;
	auto fontTall = surface->GetFontTall(hopsFont);

	surface->DrawSetTextPos(x, y);
	surface->DrawPrintText(fieldName, wcslen(fieldName));
	y += (fontTall + MARGIN);

	for (auto& segment : ljstats::lastJump.segments)
	{
		if (segment.ticks == 0 || segment.dir == ljstats::AccelDirection::Forward)
		{
			continue;
		}

		// Leave space left for last line
		if (y + fontTall * 2 > spt_hud_feat.renderView->height)
		{
			break;
		}

		func(segment, buffer, x, y);
		surface->DrawSetTextPos(x, y);
		surface->DrawPrintText(buffer, wcslen(buffer));
		y += (fontTall + MARGIN);
	}
}

void HopsHud::InitHooks() {}

void HopsHud::LoadFeature()
{
	bool result = spt_hud_feat.AddHudDefaultGroup(
	    HudCallback(std::bind(&HopsHud::DrawHopHud, this), std::bind(&HopsHud::ShouldDraw, this), false));

	if (!result)
		return;

	if (JumpSignal.Works)
	{
		if (TickSignal.Works)
		{
			TickSignal.Connect(this, &HopsHud::OnTick);
			InitConcommandBase(y_spt_jhud_velocity);
		}

		if (OngroundSignal.Works)
		{
			OngroundSignal.Connect(this, &HopsHud::OnGround);
			InitConcommandBase(y_spt_jhud_hops);
		}
		
		if (TickSignal.Works || OngroundSignal.Works)
		{
			JumpSignal.Connect(this, &HopsHud::OnJump);
			InitConcommandBase(y_spt_jhud_x);
			InitConcommandBase(y_spt_jhud_y);
		}
	}

	// ljstats
	if (JumpSignal.Works && TickSignal.Works)
	{
		JumpSignal.Connect(ljstats::OnJump);
		TickSignal.Connect(ljstats::OnTick);
		InitConcommandBase(y_spt_jhud_ljstats);
	}
}

void HopsHud::UnloadFeature() {}

void HopsHud::DrawHopHud()
{
#define DrawBuffer() \
	do \
	{ \
		int tw, th; \
		surface->GetTextSize(hopsFont, buffer, tw, th); \
		surface->DrawSetTextPos(x - tw / 2, y); \
		surface->DrawPrintText(buffer, wcslen(buffer)); \
		y += (th + MARGIN); \
	} while (0)

#define DrawValue(fmt, value) \
	swprintf_s(buffer, BUFFER_SIZE, fmt, value); \
	DrawBuffer();

	if (hopsFont == 0 && !spt_hud_feat.GetFont(FONT_Trebuchet24, hopsFont))
	{
		return;
	}

	const Color blue(64, 160, 255, 255);
	const Color orange(255, 160, 32, 255);
	const Color white(255, 255, 255, 255);

	auto surface = interfaces::surface;
	auto renderView = spt_hud_feat.renderView;

	surface->DrawSetTextFont(hopsFont);
	surface->DrawSetTextColor(white);
	surface->DrawSetTexture(0);

	const int MARGIN = 2;
	const int BUFFER_SIZE = 256;
	wchar_t buffer[BUFFER_SIZE];
	int x = renderView->width / 2 + y_spt_jhud_x.GetFloat();
	int y = renderView->height / 2 + y_spt_jhud_y.GetFloat();

	if (y_spt_jhud_hops.GetBool())
	{
		swprintf_s(buffer, BUFFER_SIZE, L"Timing: %d", displayHop);
		DrawBuffer();

		swprintf_s(buffer, BUFFER_SIZE, L"Speed loss: %.3f", loss);
		DrawBuffer();

		swprintf_s(buffer, BUFFER_SIZE, L"Percentage: %.3f", percentage);
		DrawBuffer();
	}

	if (y_spt_jhud_velocity.GetBool())
	{
		if (fabs(currVel - prevVel) < 0.01)
			surface->DrawSetTextColor(white);
		else if (currVel > prevVel)
			surface->DrawSetTextColor(blue);
		else
			surface->DrawSetTextColor(orange);

		swprintf_s(buffer, BUFFER_SIZE, L"%d", (int)currVel);
		DrawBuffer();

		surface->DrawSetTextColor(white);
		swprintf_s(buffer, BUFFER_SIZE, L"%d(%+d)", (int)hopVel, (int)(hopVel - prevHopVel));
		DrawBuffer();
	}

	if (y_spt_jhud_ljstats.GetBool())
	{
		// Main stats
		DrawValue(L"Length: %.3f", ljstats::lastJump.Length());
		DrawValue(L"Prestrafe: %.3f", ljstats::lastJump.Prestrafe());
		DrawValue(L"Strafes: %d", ljstats::lastJump.StrafeCount());
		DrawValue(L"Sync: %.3f%%", ljstats::lastJump.Sync());
		DrawValue(L"Time accelerating: %.3f%%", ljstats::lastJump.TimeAccelerating());
		DrawValue(L"Accel per tick: %.3f", ljstats::lastJump.AccelPerTick());
		DrawValue(L"Curve loss: %.3f", ljstats::lastJump.CurveLoss());

		// Individual strafes
		surface->DrawSetTextFont(hopsFont);
		surface->DrawSetTextColor(white);
		surface->DrawSetTexture(0);
		int ticks = ljstats::lastJump.TotalTicks();
		const int COL_WIDTH = 75;

		x = 6;
		y = renderView->height / 3;

		PrintStrafeCol([](const ljstats::SegmentStats& segment, wchar_t* buffer, int x, int y)
		               { swprintf(buffer, L"%.3f", segment.positiveAccel); },
		               L"Gain",
		               x,
		               y,
		               MARGIN);

		x += COL_WIDTH;

		PrintStrafeCol([](const ljstats::SegmentStats& segment, wchar_t* buffer, int x, int y)
		               { swprintf(buffer, L"%.3f", segment.negativeAccel); },
		               L"Loss",
		               x,
		               y,
		               MARGIN);

		x += COL_WIDTH;

		PrintStrafeCol(
		    [ticks](const ljstats::SegmentStats& segment, wchar_t* buffer, int x, int y)
		    {
			    float weight = segment.ticks / (float)ticks;
			    swprintf(buffer, L"%.3f", weight);
		    },
		    L"Time",
		    x,
		    y,
		    MARGIN);

		x += COL_WIDTH;

		PrintStrafeCol([](const ljstats::SegmentStats& segment, wchar_t* buffer, int x, int y)
		               { swprintf(buffer, L"%.3f", segment.Sync()); },
		               L"Sync",
		               x,
		               y,
		               MARGIN);
	}
}

void HopsHud::OnTick()
{
	if (!y_spt_jhud_velocity.GetBool())
		return;

	prevVel = currVel;
	currVel = spt_playerio.GetPlayerVelocity().Length2D();
}

void HopsHud::OnJump()
{
	if (!y_spt_jhud_hops.GetBool() && !y_spt_jhud_velocity.GetBool())
		return;

	if (sinceLanded == 0)
		CalculateAbhVel();

	// Don't count very delayed hops
	if (sinceLanded > 15)
		prevHopVel = 0;
	else
		prevHopVel = hopVel;

	hopVel = spt_playerio.GetPlayerVelocity().Length2D();

	velNotCalced = true;
	lastHop = sinceLanded;
}

void HopsHud::OnGround(bool onground)
{
	if (!y_spt_jhud_hops.GetBool())
		return;

	if (!onground)
	{
		sinceLanded = 0;

		if (velNotCalced)
		{
			velNotCalced = false;

			// Don't count very delayed hops
			if (lastHop > 15)
				return;

			auto vel = spt_playerio.GetPlayerVelocity().Length2D();
			loss = maxVel - vel;
			percentage = (vel / maxVel) * 100;
			displayHop = lastHop - 1;
			displayHop = std::max(0, displayHop);
		}
	}
	else
	{
		if (sinceLanded == 0)
		{
			CalculateAbhVel();
		}
		++sinceLanded;
	}
}

void HopsHud::CalculateAbhVel()
{
	auto vel = spt_playerio.GetPlayerVelocity().Length2D();
	auto ducked = spt_playerio.m_fFlags.GetValue() & FL_DUCKING;
	auto sprinting = spt_propertyGetter.GetProperty<bool>(0, "m_fIsSprinting");
	auto vars = spt_playerio.GetMovementVars();

	float modifier;

	if (ducked)
		modifier = 0.1;
	else if (sprinting)
		modifier = 0.5;
	else
		modifier = 1;

	float jspeed = vars.Maxspeed + (vars.Maxspeed * modifier);

	maxVel = vel + (vel - jspeed);
	maxVel = std::max(maxVel, jspeed);
}
#endif