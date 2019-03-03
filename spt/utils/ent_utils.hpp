#pragma once

#include "icliententitylist.h"
#include "icliententity.h"

namespace utils
{
	void SetEntityList(IClientEntityList* list);
	IClientEntity* GetClientEntity(int index);
	void PrintAllClientEntities();
}
