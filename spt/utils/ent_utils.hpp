#pragma once

#include <string>
#include "cdll_int.h"
#include "client_class.h"
#include "engine\ivmodelinfo.h"
#include "icliententity.h"
#include "icliententitylist.h"
#include "iserverunknown.h"
#include "trace.h"

#define INDEX_MASK (MAX_EDICTS - 1)

namespace utils
{
	struct propValue
	{
		std::string name;
		std::string value;
		RecvProp* prop;

		int GetOffset();
		propValue(const char* name, const char* value, RecvProp* prop) : name(name), value(value), prop(prop) {}
	};

	void GetAllProps(RecvTable* table, void* ptr, std::vector<propValue>& props);
	IClientEntity* GetClientEntity(int index);
	void PrintAllClientEntities();
	IClientEntity* GetPlayer();
	const char* GetModelName(IClientEntity* ent);
	void PrintAllProps(int index);
	Vector GetPortalPosition(IClientEntity* ent);
	QAngle GetPortalAngles(IClientEntity* ent);
	Vector GetPlayerEyePosition();
	QAngle GetPlayerEyeAngles();
	IClientEntity* FindLinkedPortal(IClientEntity* ent);
	int FillInfoArray(std::string argString, wchar* arr, int maxEntries, int bufferSize, char sep, char entSep);
	void SimulateFrames(int frames);
	int GetIndex(void* ent);
	IServerUnknown* GetServerPlayer();
	struct JBData
	{
		bool canJB;
		float landingHeight;
		int ticks;
	};

	JBData CanJB(float height);
	bool playerEntityAvailable();
	bool GetPunchAngleInformation(QAngle& punchAngle, QAngle& punchAngleVel);
#if !defined(OE)
	void CheckPiwSave(bool simulating);
#endif
} // namespace utils
