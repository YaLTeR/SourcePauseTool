#pragma once



#include "icliententitylist.h"
#include "icliententity.h"
#include "engine\ivmodelinfo.h"
#include "cdll_int.h"

namespace utils
{
#ifndef OE
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
#endif
	bool serverActive();
}

