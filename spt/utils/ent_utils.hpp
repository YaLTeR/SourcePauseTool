#pragma once

#include "cdll_int.h"
#include "client_class.h"
#include "engine\ivmodelinfo.h"
#include "icliententity.h"
#include "icliententitylist.h"
#include "trace.h"

namespace utils
{
#ifndef OE
	struct propValue
	{
		std::string name;
		std::string value;
		RecvProp* prop;

		int GetOffset();
		propValue(const char* name, const char* value, RecvProp* prop) : name(name), value(value), prop(prop) {}
	};

	void GetAllProps(RecvTable* table, void* ptr, std::vector<propValue>& props);
	void SetEntityList(IClientEntityList* list);
	void SetModelInfo(IVModelInfo* modelInfo);
	void SetClientDLL(IBaseClientDLL* interf);
	IClientEntity* GetClientEntity(int index);
	void PrintAllClientEntities();
	void PrintAllPortals();
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
#endif
	struct JBData
	{
		bool canJB;
		float landingHeight;
		int ticks;
	};

	JBData CanJB(float height);
	bool playerEntityAvailable();
	bool GetPunchAngleInformation(QAngle& punchAngle, QAngle& punchAngleVel);
	void FindClosestPlane(const trace_t& tr, trace_t& out, float maxDistSqr);
	bool TraceHit(const trace_t& tr, float maxDistSqr);
	bool TestSeamshot(const Vector& cameraPos,
	                  const Vector& seamPos,
	                  const cplane_t& plane1,
	                  const cplane_t& plane2,
	                  QAngle& seamAngle);
#if !defined(OE) && !defined(P2)
	void CheckPiwSave();
#endif
} // namespace utils
