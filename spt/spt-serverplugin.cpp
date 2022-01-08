#include "stdafx.h"
#include "..\stdafx.hpp"

#include <chrono>
#include <functional>
#include <sstream>
#include <time.h>

#include <SPTLib\Hooks.hpp>
#include "spt-serverplugin.hpp"
#include "..\sptlib-wrapper.hpp"
#include "ent_utils.hpp"
#include "math.hpp"
#include "string_utils.hpp"
#include "game_detection.hpp"
#include "..\features\generic.hpp"
#include "..\features\playerio.hpp"
#include "custom_interfaces.hpp"
#include "cvars.hpp"
#include "scripts\srctas_reader.hpp"
#include "..\feature.hpp"
#include "vstdlib\random.h"

#include "cdll_int.h"
#include "eiface.h"
#include "engine\iserverplugin.h"
#include "icliententitylist.h"
#include "tier2\tier2.h"
#include "tier3\tier3.h"
#include "vgui\iinput.h"
#include "vgui\isystem.h"
#include "vgui\ivgui.h"
#include "SDK\hl_movedata.h"
#include "SDK\igamemovement.h"
#include "interfaces.hpp"
#include "signals.hpp"

#if SSDK2007
#include "mathlib\vmatrix.h"
#endif

#include "SPTLib\sptlib.hpp"
#include "overlay\overlay-renderer.hpp"
#include "overlay\overlays.hpp"
#include "tier0\memdbgoff.h" // YaLTeR - switch off the memory debugging.
using namespace std::literals;

namespace interfaces
{
	std::unique_ptr<EngineClientWrapper> engine;
	IVEngineServer* engine_server = nullptr;
	IMatSystemSurface* surface = nullptr;
	vgui::ISchemeManager* scheme = nullptr;
	IVDebugOverlay* debugOverlay = nullptr;
	ICvar* g_pCVar = nullptr;
	void* gm = nullptr;
	IClientEntityList* entList;
	IVModelInfo* modelInfo;
	IBaseClientDLL* clientInterface;
} // namespace interfaces

ConVar* _viewmodel_fov = nullptr;
ConVar* _sv_accelerate = nullptr;
ConVar* _sv_airaccelerate = nullptr;
ConVar* _sv_friction = nullptr;
ConVar* _sv_maxspeed = nullptr;
ConVar* _sv_stopspeed = nullptr;
ConVar* _sv_stepsize = nullptr;
ConVar* _sv_gravity = nullptr;
ConVar* _sv_maxvelocity = nullptr;
ConVar* _sv_bounce = nullptr;

// useful helper func
inline bool FStrEq(const char* sz1, const char* sz2)
{
	return (Q_stricmp(sz1, sz2) == 0);
}

void CallServerCommand(const char* cmd)
{
	if (interfaces::engine)
		interfaces::engine->ClientCmd(cmd);
}

void GetViewAngles(float viewangles[3])
{
	if (interfaces::engine)
	{
		QAngle va;
		interfaces::engine->GetViewAngles(va);

		viewangles[0] = va.x;
		viewangles[1] = va.y;
		viewangles[2] = va.z;
	}
}

void SetViewAngles(const float viewangles[3])
{
	if (interfaces::engine)
	{
		QAngle va(viewangles[0], viewangles[1], viewangles[2]);
		interfaces::engine->SetViewAngles(va);
	}
}

void DefaultFOVChangeCallback(ConVar* var, char const* pOldString)
{
	if (FStrEq(var->GetString(), "75") && FStrEq(pOldString, "90"))
	{
		//Msg("Attempted to change default_fov from 90 to 75. Preventing.\n");
		var->SetValue("90");
	}
}

//
// The plugin is a static singleton that is exported as an interface
//
#ifdef OE
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSourcePauseTool,
                                  IServerPluginCallbacks,
                                  INTERFACEVERSION_ISERVERPLUGINCALLBACKS_VERSION_1,
                                  g_SourcePauseTool);
#else
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSourcePauseTool,
                                  IServerPluginCallbacks,
                                  INTERFACEVERSION_ISERVERPLUGINCALLBACKS,
                                  g_SourcePauseTool);
#endif

bool CSourcePauseTool::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	auto startTime = std::chrono::high_resolution_clock::now();

	if (pluginLoaded)
	{
		Warning("Trying to load SPT when SPT is already loaded.\n");
		// Failure to load causes immediate call to Unload, make sure we do nothing there
		skipUnload = true;
		return false;
	}
	pluginLoaded = true;

	ConnectTier1Libraries(&interfaceFactory, 1);
	ConnectTier3Libraries(&interfaceFactory, 1);

	interfaces::gm = gameServerFactory(INTERFACENAME_GAMEMOVEMENT, NULL);
	interfaces::g_pCVar = g_pCVar;
	interfaces::engine_server = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	interfaces::debugOverlay = (IVDebugOverlay*)interfaceFactory(VDEBUG_OVERLAY_INTERFACE_VERSION, NULL);

	auto clientFactory = Sys_GetFactory("client");
	interfaces::entList = (IClientEntityList*)clientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);
	interfaces::modelInfo = (IVModelInfo*)interfaceFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL);
	interfaces::clientInterface = (IBaseClientDLL*)clientFactory(CLIENT_DLL_INTERFACE_VERSION, NULL);

	if (interfaces::gm)
	{
		DevMsg("SPT: Found IGameMovement at %p.\n", interfaces::gm);
	}
	else
	{
		DevWarning("SPT: Could not find IGameMovement.\n");
		DevWarning("SPT: ProcessMovement logging with tas_log is unavailable.\n");
	}

	if (g_pCVar)
#if defined(OE)
	{
		_viewmodel_fov = g_pCVar->FindVar("viewmodel_fov");
	}
#else
	{
#define GETCVAR(x) _##x = g_pCVar->FindVar(#x);

		GETCVAR(sv_airaccelerate);
		GETCVAR(sv_accelerate);
		GETCVAR(sv_friction);
		GETCVAR(sv_maxspeed);
		GETCVAR(sv_stopspeed);
		GETCVAR(sv_stepsize);
		GETCVAR(sv_gravity);
		GETCVAR(sv_maxvelocity);
		GETCVAR(sv_bounce);
	}
#endif

#if !defined(BMS)
	auto ptr = interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
#else
	auto ptr = interfaceFactory("VEngineClient015", NULL);
#endif

	if (ptr)
	{
#ifdef OE
		if (utils::DoesGameLookLikeDMoMM())
			interfaces::engine = std::make_unique<IVEngineClientWrapper<IVEngineClientDMoMM>>(
			    reinterpret_cast<IVEngineClientDMoMM*>(ptr));
#endif

		// Check if we assigned it in the ifdef above.
		if (!interfaces::engine)
			interfaces::engine = std::make_unique<IVEngineClientWrapper<IVEngineClient>>(
			    reinterpret_cast<IVEngineClient*>(ptr));
	}

	if (!interfaces::engine)
	{
		DevWarning("SPT: Failed to get the IVEngineClient interface.\n");
		Warning("SPT: y_spt_afterframes has no effect.\n");
		Warning("SPT: _y_spt_setpitch and _y_spt_setyaw have no effect.\n");
		Warning("SPT: _y_spt_pitchspeed and _y_spt_yawspeed have no effect.\n");
		Warning("SPT: y_spt_stucksave has no effect.\n");
	}

	if (utils::DoesGameLookLikePortal())
	{
		DevMsg("SPT: This game looks like portal. Setting the tas_* cvars appropriately.\n");

		tas_force_airaccelerate.SetValue(15);
		tas_force_wishspeed_cap.SetValue(60);
		tas_reset_surface_friction.SetValue(0);
	}

	if (!interfaces::engine_server)
	{
		DevWarning("SPT: Failed to get the IVEngineServer interface.\n");
	}

	if (!interfaces::debugOverlay)
	{
		DevWarning("SPT: Failed to get the debug overlay interface.\n");
		Warning("Seam visualization has no effect.\n");
	}

	if (!interfaces::entList)
		DevWarning("Unable to retrieve entitylist interface.\n");

	if (!interfaces::modelInfo)
		DevWarning("Unable to retrieve the model info interface.\n");

	if (!interfaces::clientInterface)
		DevWarning("Unable to retrieve the client DLL interface.\n");

	EngineConCmd = CallServerCommand;
	EngineGetViewAngles = GetViewAngles;
	EngineSetViewAngles = SetViewAngles;

	_EngineMsg = Msg;
	_EngineDevMsg = DevMsg;
	_EngineWarning = Warning;
	_EngineDevWarning = DevWarning;

	TickSignal.Works = true;
	Feature::LoadFeatures();
	Cvar_RegisterSPTCvars();

	auto loadTime =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	std::ostringstream out;
	out << "SourcePauseTool version " SPT_VERSION " was loaded in " << loadTime << "ms.\n";

	Msg("%s", std::string(out.str()).c_str());

	return true;
}

void CSourcePauseTool::Unload(void)
{
	if (skipUnload)
	{
		// Preventing double load of plugin, do nothing on unload
		skipUnload = false; // Enable unloading again
		return;
	}

	Cvar_UnregisterSPTCvars();
	DisconnectTier1Libraries();
	DisconnectTier3Libraries();
	Feature::UnloadFeatures();
	Hooks::Free();
	pluginLoaded = false;
}

const char* CSourcePauseTool::GetPluginDescription(void)
{
	return "SourcePauseTool v" SPT_VERSION ", Ivan \"YaLTeR\" Molodetskikh";
}

void CSourcePauseTool::GameFrame(bool simulating)
{
	if (simulating)
		TickSignal();
}
