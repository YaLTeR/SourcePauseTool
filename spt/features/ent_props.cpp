#include "stdafx.hpp"
#include "..\feature.hpp"
#include "ent_props.hpp"
#include "convar.hpp"
#include "hud.hpp"
#include "playerio.hpp"
#include "game_detection.hpp"
#include "interfaces.hpp"
#include "ent_utils.hpp"
#include "string_utils.hpp"
#include "spt\utils\portal_utils.hpp"
#include "SPTLib\patterns.hpp"
#include <string>
#include <unordered_map>

ConVar y_spt_hud_portal_bubble("y_spt_hud_portal_bubble", "0", FCVAR_CHEAT, "Turns on portal bubble index hud.\n");
ConVar y_spt_hud_ent_info(
    "y_spt_hud_ent_info",
    "",
    FCVAR_CHEAT,
    "Display entity info on HUD. Format is \"[ent index],[prop regex],[prop regex],...,[prop regex];[ent index],...,[prop regex]\".\n");

static const int MAX_ENTRIES = 128;
static const int INFO_BUFFER_SIZE = 256;
static wchar INFO_ARRAY[MAX_ENTRIES * INFO_BUFFER_SIZE];
static const char ENT_SEPARATOR = ';';
static const char PROP_SEPARATOR = ',';

EntProps spt_entprops;

namespace patterns
{
	PATTERNS(Datamap,
	         "pattern1",
	         "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? B8",
	         "pattern2",
	         "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C3",
	         "pattern3",
	         "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? B8 ?? ?? ?? ?? C7 05");
}

void EntProps::InitHooks()
{
	AddMatchAllPattern(patterns::Datamap, "client", "Datamap", &clientPatterns);
	AddMatchAllPattern(patterns::Datamap, "server", "Datamap", &serverPatterns);
	tablesProcessed = false;
}

void EntProps::PreHook()
{
	ProcessTablesLazy();
}

void EntProps::UnloadFeature()
{
	for (auto* map : wrappers)
		delete map;
}

void EntProps::WalkDatamap(std::string key)
{
	ProcessTablesLazy();
	auto* result = GetDatamapWrapper(key);
	if (result)
		result->ExploreOffsets();
	else
		Msg("No datamap found with name \"%s\"\n", key.c_str());
}

void EntProps::PrintDatamaps()
{
	Msg("Printing all datamaps:\n");
	std::vector<std::string> names;
	names.reserve(wrappers.size());

	for (auto& pair : nameToMapWrapper)
		names.push_back(pair.first);

	std::sort(names.begin(), names.end());

	for (auto& name : names)
	{
		auto* map = nameToMapWrapper[name];

		Msg("\tDatamap: %s, server map %x, client map %x\n", name.c_str(), map->_serverMap, map->_clientMap);
	}
}

int EntProps::GetPlayerOffset(const std::string& key, bool server)
{
	ProcessTablesLazy();
	auto* playermap = GetPlayerDatamapWrapper();

	if (!playermap)
		return utils::INVALID_DATAMAP_OFFSET;

	if (server)
		return playermap->GetServerOffset(key);
	else
		return playermap->GetClientOffset(key);
}

void* EntProps::GetPlayer(bool server)
{
	if (server)
	{
		if (!interfaces::engine_server)
			return nullptr;

		auto edict = interfaces::engine_server->PEntityOfEntIndex(1);
		if (!edict || (uint32_t)edict == 0xFFFFFFFF)
			return nullptr;

		return edict->GetUnknown();
	}
	else
	{
		return interfaces::entList->GetClientEntity(1);
	}
}

int EntProps::GetFieldOffset(const std::string& mapKey, const std::string& key, bool server)
{
	auto map = GetDatamapWrapper(mapKey);
	if (!map)
	{
		return utils::INVALID_DATAMAP_OFFSET;
	}

	if (server)
		return map->GetServerOffset(key);
	else
		return map->GetClientOffset(key);
}

_InternalPlayerField EntProps::_GetPlayerField(const std::string& key, PropMode mode)
{
	_InternalPlayerField out;
	// Do nothing if we didn't load
	if (!startedLoading)
		return out;

	bool getServer = mode == PropMode::Server || mode == PropMode::PreferClient || mode == PropMode::PreferServer;
	bool getClient = mode == PropMode::Client || mode == PropMode::PreferClient || mode == PropMode::PreferServer;

	if (getServer)
	{
		out.serverOffset = GetPlayerOffset(key, true);
	}

	if (getClient)
	{
		out.clientOffset = GetPlayerOffset(key, false);
	}

	if (out.serverOffset == utils::INVALID_DATAMAP_OFFSET && out.clientOffset == utils::INVALID_DATAMAP_OFFSET)
	{
		DevWarning("Was unable to find field %s offsets for server or client!\n", key.c_str());
	}

	return out;
}

static bool IsAddressLegal(uint8_t* address, uint8_t* start, std::size_t length)
{
	return address >= start && address <= start + length;
}

static bool DoesMapLookValid(datamap_t* map, uint8_t* moduleStart, std::size_t length)
{
	if (!IsAddressLegal(reinterpret_cast<uint8_t*>(map->dataDesc), moduleStart, length))
		return false;

	int MAX_LEN = 64;
	char* name = const_cast<char*>(map->dataClassName);

	if (!IsAddressLegal(reinterpret_cast<uint8_t*>(name), moduleStart, length))
		return false;

	for (int i = 0; i < MAX_LEN; ++i)
	{
		if (name[i] == '\0')
		{
			return i > 0;
		}
	}

	return false;
}

PropMode EntProps::ResolveMode(PropMode mode)
{
	auto svplayer = GetPlayer(true);
	auto clplayer = GetPlayer(false);

	switch (mode)
	{
	case PropMode::Client:
		if (clplayer)
			return PropMode::Client;
		else
			return PropMode::None;
	case PropMode::Server:
		if (svplayer)
			return PropMode::Server;
		else
			return PropMode::None;
	case PropMode::PreferServer:
		if (svplayer)
			return PropMode::Server;
		else if (clplayer)
			return PropMode::Client;
		else
			return PropMode::None;
	case PropMode::PreferClient:
		if (clplayer)
			return PropMode::Client;
		else if (svplayer)
			return PropMode::Server;
		else
			return PropMode::None;
	default:
		return PropMode::None;
	}
}

void EntProps::AddMap(datamap_t* map, bool server)
{
	std::string name = map->dataClassName;

	// if this is a client map, get the associated server name
	if (!server)
	{
		if (!strcmp(map->dataClassName, "C_BaseHLPlayer"))
			name = "CHL2_Player";
		else if (strstr(map->dataClassName, "C_") == map->dataClassName)
			name = name.erase(1, 1);
	}

	auto result = nameToMapWrapper.find(name);
	utils::DatamapWrapper* ptr;

	if (result == nameToMapWrapper.end())
	{
		ptr = new utils::DatamapWrapper();
		wrappers.push_back(ptr);
		nameToMapWrapper[name] = ptr;
	}
	else
	{
		ptr = result->second;
	}

	if (server)
		ptr->_serverMap = map;
	else
		ptr->_clientMap = map;
}

utils::DatamapWrapper* EntProps::GetDatamapWrapper(const std::string& key)
{
	auto result = nameToMapWrapper.find(key);
	if (result != nameToMapWrapper.end())
		return result->second;
	else
		return nullptr;
}

utils::DatamapWrapper* EntProps::GetPlayerDatamapWrapper()
{
	// Grab cached result if exists, can also be NULL!
	if (playerDatamapSearched)
		return __playerdatamap;

	if (utils::DoesGameLookLikePortal())
	{
		__playerdatamap = GetDatamapWrapper("CPortal_Player");
	}
	else if (utils::DoesGameLookLikeDMoMM())
	{
		__playerdatamap = GetDatamapWrapper("CBasePlayer");
	}

	// Add any other game specific class names here
	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CHL2_Player");
	}

	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CBasePlayer");
	}

	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CBaseCombatCharacter");
	}

	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CBaseEntity");
	}

	playerDatamapSearched = true;
	return __playerdatamap;
}

static void GetDatamapInfo(patterns::MatchedPattern pattern, int& numfields, datamap_t**& pmap)
{
	switch (pattern.ptnIndex)
	{
	case 0:
		numfields = *reinterpret_cast<int*>(pattern.ptr + 6);
		pmap = reinterpret_cast<datamap_t**>(pattern.ptr + 12);
		break;
	case 1:
		numfields = *reinterpret_cast<int*>(pattern.ptr + 6);
		pmap = reinterpret_cast<datamap_t**>(pattern.ptr + 12);
		break;
	case 2:
		numfields = *reinterpret_cast<int*>(pattern.ptr + 6);
		pmap = reinterpret_cast<datamap_t**>(pattern.ptr + 17);
		break;
	}
}

void EntProps::ProcessTablesLazy()
{
	if (tablesProcessed)
		return;

	void* clhandle;
	void* clmoduleStart;
	size_t clmoduleSize;
	void* svhandle;
	void* svmoduleStart;
	size_t svmoduleSize;

	if (MemUtils::GetModuleInfo(L"server.dll", &svhandle, &svmoduleStart, &svmoduleSize))
	{
		for (auto serverPattern : serverPatterns)
		{
			int numfields;
			datamap_t** pmap;
			GetDatamapInfo(serverPattern, numfields, pmap);
			if (numfields > 0
			    && IsAddressLegal(reinterpret_cast<uint8_t*>(pmap),
			                      reinterpret_cast<uint8_t*>(svmoduleStart),
			                      svmoduleSize))
			{
				datamap_t* map = *pmap;
				if (DoesMapLookValid(map, reinterpret_cast<uint8_t*>(svmoduleStart), svmoduleSize))
					AddMap(map, true);
			}
		}
	}

	if (MemUtils::GetModuleInfo(L"client.dll", &clhandle, &clmoduleStart, &clmoduleSize))
	{
		for (auto& clientPattern : clientPatterns)
		{
			int numfields;
			datamap_t** pmap;
			GetDatamapInfo(clientPattern, numfields, pmap);
			if (numfields > 0
			    && IsAddressLegal(reinterpret_cast<uint8_t*>(pmap),
			                      reinterpret_cast<uint8_t*>(clmoduleStart),
			                      clmoduleSize))
			{
				datamap_t* map = *pmap;
				if (DoesMapLookValid(map, reinterpret_cast<uint8_t*>(clmoduleStart), clmoduleSize))
					AddMap(map, false);
			}
		}
	}

	clientPatterns.clear();
	serverPatterns.clear();
	tablesProcessed = true;
}

CON_COMMAND(y_spt_canjb, "Tests if player can jumpbug on a given height, with the current position and speed.")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: spt_canjb [height]\n");
		return;
	}

	float height = std::stof(args.Arg(1));
	auto can = utils::CanJB(height);

	if (can.canJB)
		Msg("Yes, projected landing height %.6f in %d ticks\n", can.landingHeight - height, can.ticks);
	else
		Msg("No, missed by %.6f in %d ticks.\n", can.landingHeight - height, can.ticks);
}

CON_COMMAND(y_spt_print_ents, "Prints all client entity indices and their corresponding classes.")
{
	utils::PrintAllClientEntities();
}

#ifdef SPT_PORTAL_UTILS
CON_COMMAND(y_spt_print_portals, "Prints all portal indexes, their position and angles.")
{
	utils::PrintAllPortals();
}
#endif

CON_COMMAND(y_spt_print_ent_props, "Prints all props for a given entity index.")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: spt_print_ent_props [index]\n");
	}
	else
	{
		utils::PrintAllProps(std::stoi(args.Arg(1)));
	}
}

CON_COMMAND(_y_spt_datamap_print, "Prints all datamaps.")
{
	spt_entprops.PrintDatamaps();
}

CON_COMMAND(_y_spt_datamap_walk, "Walk through a datamap and print all offsets.")
{
	spt_entprops.WalkDatamap(args.Arg(1));
}

void EntProps::LoadFeature()
{
	InitCommand(_y_spt_datamap_print);
	InitCommand(_y_spt_datamap_walk);
	InitCommand(y_spt_canjb);
	InitCommand(y_spt_print_ents);
	InitCommand(y_spt_print_ent_props);
#ifdef SPT_PORTAL_UTILS
	if (utils::DoesGameLookLikePortal())
		InitCommand(y_spt_print_portals);
#endif
#if defined(SPT_HUD_ENABLED) && defined(SPT_PORTAL_UTILS)
	if (utils::DoesGameLookLikePortal())
	{
		AddHudCallback(
		    "portal_bubble",
		    [this](std::string)
		    {
			    int in_bubble = GetEnvironmentPortal() != NULL;
			    spt_hud_feat.DrawTopHudElement(L"portal bubble: %d", in_bubble);
		    },
		    y_spt_hud_portal_bubble);
	}
#endif

	bool result = spt_hud_feat.AddHudCallback("ent_info",
	                                          HudCallback(
	                                              [](std::string args)
	                                              {
		                                              std::string info;
		                                              if (args == "")
			                                              info = y_spt_hud_ent_info.GetString();
		                                              else
			                                              info = args;
		                                              if (!whiteSpacesOnly(info))
		                                              {
			                                              int entries =
			                                                  utils::FillInfoArray(info,
			                                                                       INFO_ARRAY,
			                                                                       MAX_ENTRIES,
			                                                                       INFO_BUFFER_SIZE,
			                                                                       PROP_SEPARATOR,
			                                                                       ENT_SEPARATOR);
			                                              for (int i = 0; i < entries; ++i)
			                                              {
				                                              spt_hud_feat.DrawTopHudElement(
				                                                  INFO_ARRAY + i * INFO_BUFFER_SIZE);
			                                              }
		                                              }
	                                              },
	                                              []() { return true; },
	                                              false));

	if (result)
	{
		InitConcommandBase(y_spt_hud_ent_info);
	}
}

void** _InternalPlayerField::GetServerPtr() const
{
	auto serverplayer = reinterpret_cast<uintptr_t>(spt_entprops.GetPlayer(true));
	if (serverplayer && serverOffset != utils::INVALID_DATAMAP_OFFSET)
		return reinterpret_cast<void**>(serverplayer + serverOffset);
	else
		return nullptr;
}

void** _InternalPlayerField::GetClientPtr() const
{
	auto clientPlayer = reinterpret_cast<uintptr_t>(spt_entprops.GetPlayer(false));
	if (clientPlayer && clientOffset != utils::INVALID_DATAMAP_OFFSET)
		return reinterpret_cast<void**>(clientPlayer + clientOffset);
	else
		return nullptr;
}

bool _InternalPlayerField::ClientOffsetFound() const
{
	return clientOffset != utils::INVALID_DATAMAP_OFFSET;
}

bool _InternalPlayerField::ServerOffsetFound() const
{
	return serverOffset != utils::INVALID_DATAMAP_OFFSET;
}
