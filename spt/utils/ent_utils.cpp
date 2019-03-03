#include "stdafx.h"
#include "ent_utils.hpp"
#include "client_class.h"

static IClientEntityList* entList;

namespace utils
{
	void SetEntityList(IClientEntityList* list)
	{
		entList = list;
	}

	IClientEntity * GetClientEntity(int index)
	{
		return entList->GetClientEntity(index+1);
	}

	void PrintAllClientEntities()
	{
		int maxIndex = entList->GetHighestEntityIndex();

		for (int i = 0; i <= maxIndex; ++i)
		{
			auto ent = GetClientEntity(i);
			if (ent)
			{
				Msg("%d : %s\n", i, ent->GetClientClass()->m_pNetworkName);
			}
		}

	}

}