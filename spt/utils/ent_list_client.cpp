#include <stdafx.hpp>

#include "ent_list.hpp"
#include "spt/features/ent_props.hpp"
#include "spt/features/property_getter.hpp"
#include "interfaces.hpp"

template<>
void utils::SptEntListClient::CheckRebuildLists(bool force)
{
	if (!interfaces::engine_tool || !interfaces::entListClient)
		return;
	int newFrame = interfaces::engine_tool->HostFrameCount();
	if (newFrame == lastUpdatedAt && !force)
		return;
	lastUpdatedAt = newFrame;
	_entList.clear();
	_portalList.clear();
	/*
	* On newer versions you can do a a couple static casts:
	* IClientEntityList -> CClienEntityList -> CBaseEntityList
	* 
	* This would give a nice linked list that can be used for client entity iteration, but this
	* doesn't work on OE, so we just use the IClientEntityList directly.
	*/
	for (int i = 0; i < interfaces::entListClient->GetHighestEntityIndex(); i++)
	{
		IClientEntity* ent = GetEnt(i);
		if (!ent)
			continue;
		_entList.push_back(ent);
		if (EntIsPortal(ent))
		{
			_portalList.emplace_back();
			FillPortalInfo(ent, _portalList.back());
		}
	}
}

template<>
auto utils::SptEntListClient::GetEnt(int i) -> ent_type
{
	// the engine does not check the upper bound for us :/
	if (!interfaces::entListClient || i >= NUM_ENT_ENTRIES)
		return nullptr;
	return interfaces::entListClient->GetClientEntity(i);
}

template<>
const char* utils::SptEntListClient::NetworkClassName(ent_type ent)
{
	return ent ? ent->GetClientClass()->GetName() : nullptr;
}

template<>
void utils::SptEntListClient::FillPortalInfo(ent_type ent, utils::PortalInfo& info)
{
	Assert(EntIsPortal(ent));
	if (!interfaces::modelInfo)
	{
		Assert(0);
		return;
	}

	CBaseHandle handle = ent->GetRefEHandle();
	CBaseHandle linked = spt_propertyGetter.GetProperty<CBaseHandle>(handle.GetEntryIndex(), "m_hLinkedPortal");
	bool activated = spt_propertyGetter.GetProperty<bool>(handle.GetEntryIndex(), "m_bActivated");
	auto linkedEnt = linked.IsValid() ? GetEnt(linked.GetEntryIndex()) : nullptr;

	info.pEnt = ent;
	info.handle = handle;
	info.linkedHandle = linked;
	info.pos = ent->GetAbsOrigin();
	info.linkedPos = linkedEnt ? linkedEnt->GetAbsOrigin() : Vector{NAN, NAN, NAN};
	info.ang = ent->GetAbsAngles();
	info.linkedAng = linkedEnt ? linkedEnt->GetAbsAngles() : QAngle{NAN, NAN, NAN};
	info.isOrange = !!strstr(interfaces::modelInfo->GetModelName(ent->GetModel()), "portal2");
	info.isActivated = activated;
	info.isOpen = linked.IsValid();
}

template<>
const utils::PortalInfo* utils::SptEntListClient::GetEnvironmentPortal()
{
	CBaseHandle handle = spt_propertyGetter.GetProperty<CBaseHandle>(1, "m_hPortalEnvironment");
	return handle.IsValid() ? GetPortalAtIndex(handle.GetEntryIndex()) : nullptr;
}
