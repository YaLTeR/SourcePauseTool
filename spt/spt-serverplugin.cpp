#include "stdafx.hpp"

#include <chrono>
#include <functional>
#include <sstream>
#include <time.h>

#include "vstdlib/IKeyValuesSystem.h"
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
#include "ienginevgui.h"
#include "toolframework\ienginetool.h"
#include "inputsystem\iinputsystem.h"
#include "SDK\hl_movedata.h"
#include "SDK\igamemovement.h"
#include "interfaces.hpp"
#include "signals.hpp"

#if SSDK2007
#include "mathlib\vmatrix.h"
#endif

#include "SPTLib\sptlib.hpp"
#include "tier0\memdbgoff.h" // YaLTeR - switch off the memory debugging.
using namespace std::literals;

namespace interfaces
{
	std::unique_ptr<EngineClientWrapper> engine;
	IVEngineServer* engine_server = nullptr;
	IVEngineClient* engine_client = nullptr;
	IMatSystemSurface* surface = nullptr;
	vgui::ISchemeManager* scheme = nullptr;
	vgui::IInput* vgui_input = nullptr;
	IEngineVGui* engine_vgui = nullptr;
	IEngineTool* engine_tool = nullptr;
	IVDebugOverlay* debugOverlay = nullptr;
	IMaterialSystem* materialSystem = nullptr;
	IInputSystem* inputSystem = nullptr;
	IGameMovement* gm = nullptr;
	IClientEntityList* entList;
	IVModelInfo* modelInfo;
	IBaseClientDLL* clientInterface;
	IEngineTrace* engineTraceClient = nullptr;
	IEngineTrace* engineTraceServer = nullptr;
	IServerPluginHelpers* pluginHelpers = nullptr;
	IPhysicsCollision* physicsCollision = nullptr;
	IStaticPropMgrServer* staticpropmgr = nullptr;
	IShaderDevice* shaderDevice = nullptr;
} // namespace interfaces

ConVar* _viewmodel_fov = nullptr;
ConCommand* _record = nullptr;
ConCommand* _stop = nullptr;
ConVar* _sv_accelerate = nullptr;
ConVar* _sv_airaccelerate = nullptr;
ConVar* _sv_friction = nullptr;
ConVar* _sv_maxspeed = nullptr;
ConVar* _sv_stopspeed = nullptr;
ConVar* _sv_stepsize = nullptr;
ConVar* _sv_gravity = nullptr;
ConVar* _sv_maxvelocity = nullptr;
ConVar* _sv_bounce = nullptr;
ConVar* _sv_cheats = nullptr;

extern ConVar tas_force_airaccelerate;
extern ConVar tas_force_wishspeed_cap;
extern ConVar tas_reset_surface_friction;

// useful helper func
inline bool FStrEq(const char* sz1, const char* sz2)
{
	return (stricmp(sz1, sz2) == 0);
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

CSourcePauseTool g_SourcePauseTool;

extern "C" __declspec(dllexport) const void* CreateInterface(const char* pName, int* pReturnCode) {
	if (pReturnCode) {
		*pReturnCode = 2007;
	}

	return &g_SourcePauseTool;
}


class SDKVersion
{
public:
	enum Value : uint8_t
	{
		SDK_UNKNOWN,
		SDK_2007,
		SDK_2013,
		SDK_BMS,
		SDK_OE,
	};

	SDKVersion() = default;

	constexpr SDKVersion(Value version) : value(version) {}

	constexpr operator Value() const
	{
		return value;
	}

	explicit operator bool() const = delete;

	const char* GetSPTName()
	{
		switch (value)
		{
		case SDK_2007:
			return "spt.dll";
		case SDK_2013:
			return "spt-2013.dll";
		case SDK_BMS:
			return "spt-bms.dll";
		case SDK_OE:
			return "spt-oe.dll";
		default:
			return "";
		}
	}

private:
	Value value;
};

static SDKVersion sptSDKVersion =
#if defined(SSDK2007)
SDKVersion::SDK_2007
#elif defined(SSDK2013)
SDKVersion::SDK_2013
#elif defined(BMS)
SDKVersion::SDK_BMS
#elif defined(OE)
SDKVersion::SDK_OE
#else
#error "Unknown SDK version"
#endif
;

static SDKVersion CheckSDKVersion(CreateInterfaceFn interfaceFactory)
{
	void* icvar = interfaceFactory("VEngineCvar003", NULL);
	if (icvar)
		return SDKVersion::SDK_OE;
	icvar = interfaceFactory("VEngineCvar004", NULL);
	if (!icvar)
		return SDKVersion::SDK_UNKNOWN;

	typedef void* (__thiscall* FindCommandBase_func)(void* thisptr, const char* name);

	// GENIUS HACK (BUT STILL BAD) (from SST): BMS has everything in ICvar shifted
	// down 3 places due to the extra stuff in IAppSystem. This means that
	// if we look up the BMS-specific cvar using FindCommandBase, it
	// *actually* calls the const-overloaded FindVar on other branches,
	// which just happens to still work fine.

	auto find_commandbase_bms = (*(FindCommandBase_func**)(icvar))[13];
	if (find_commandbase_bms(icvar, "kill"))
		return SDKVersion::SDK_BMS;

	if (interfaceFactory("VEngineClient014", NULL))
		return SDKVersion::SDK_2013;

	return SDKVersion::SDK_2007;
}

static void SetFuncIfFound(void** pTarget, void* func, bool critical = false) {
	if (func) {
		*pTarget = func;
	}
	else if (critical) {
		DebugBreak();
	}
}

static void GrabTier0Stuff() {
	HMODULE moduleHandle = GetModuleHandleA("tier0.dll");
	HMODULE moduleHandleVstdlib = GetModuleHandleA("vstdlib.dll");
	if (moduleHandle != NULL) {
		SetFuncIfFound((void**)&ConColorMsg, GetProcAddress(moduleHandle, "ConColorMsg"));
		SetFuncIfFound((void**)&ConMsg, GetProcAddress(moduleHandle, "?ConMsg@@YAXPBDZZ"));
		SetFuncIfFound((void**)&DevMsg, GetProcAddress(moduleHandle, "?DevMsg@@YAXPBDZZ"));
		SetFuncIfFound((void**)&DevWarning, GetProcAddress(moduleHandle, "?DevWarning@@YAXPBDZZ"));
		SetFuncIfFound((void**)&Msg, GetProcAddress(moduleHandle, "Msg"));
		SetFuncIfFound((void**)&Warning, GetProcAddress(moduleHandle, "Warning"));
	}
	if (moduleHandleVstdlib != NULL) {
		SetFuncIfFound((void**)&KeyValuesSystem_impl, GetProcAddress(moduleHandleVstdlib, "KeyValuesSystem"), true);
	}
}


bool CSourcePauseTool::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	auto startTime = std::chrono::high_resolution_clock::now();
	GrabTier0Stuff();

	SDKVersion detectedVersion = CheckSDKVersion(interfaceFactory);
	if (detectedVersion != sptSDKVersion)
	{
		if (detectedVersion == SDKVersion::SDK_UNKNOWN)
		{
			Warning("This game is not supported by SPT.\n");
		}
		else
		{
			Warning(
				"You might be using the wrong SPT build (%s)\n"
				"The correct SPT build seems to be %s\n",
				sptSDKVersion.GetSPTName(),
				detectedVersion.GetSPTName());
		}

		skipUnload = true;
		return false;
	}

	if (pluginLoaded)
	{
		Warning("Trying to load SPT when SPT is already loaded.\n");
		// Failure to load causes immediate call to Unload, make sure we do nothing there
		skipUnload = true;
		return false;
	}
	pluginLoaded = true;

	interfaces::gm = (IGameMovement*)gameServerFactory(INTERFACENAME_GAMEMOVEMENT, NULL);
	g_pCVar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, NULL);
	interfaces::engine_server = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
#ifdef BMS
	interfaces::engine_client = (IVEngineClient*)interfaceFactory("VEngineClient015", NULL);
#else
	interfaces::engine_client = (IVEngineClient*)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
#endif
	interfaces::debugOverlay = (IVDebugOverlay*)interfaceFactory(VDEBUG_OVERLAY_INTERFACE_VERSION, NULL);
	interfaces::materialSystem = (IMaterialSystem*)interfaceFactory(MATERIAL_SYSTEM_INTERFACE_VERSION, NULL);
	interfaces::engine_vgui = (IEngineVGui*)interfaceFactory(VENGINE_VGUI_VERSION, NULL);
	interfaces::engine_tool = (IEngineTool*)interfaceFactory(VENGINETOOL_INTERFACE_VERSION, NULL);
	interfaces::vgui_input = (vgui::IInput*)interfaceFactory(VGUI_INPUT_INTERFACE_VERSION, NULL);
	interfaces::surface = (IMatSystemSurface*)interfaceFactory(MAT_SYSTEM_SURFACE_INTERFACE_VERSION, NULL);
	interfaces::scheme = (vgui::ISchemeManager*)interfaceFactory(VGUI_SCHEME_INTERFACE_VERSION, NULL);

	interfaces::inputSystem = (IInputSystem*)interfaceFactory(INPUTSYSTEM_INTERFACE_VERSION, NULL);

	auto clientFactory = Sys_GetFactory("client");
	interfaces::entList = (IClientEntityList*)clientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);
	interfaces::modelInfo = (IVModelInfo*)interfaceFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL);
	interfaces::clientInterface = (IBaseClientDLL*)clientFactory(CLIENT_DLL_INTERFACE_VERSION, NULL);
	interfaces::engineTraceClient = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_CLIENT, NULL);
	interfaces::engineTraceServer = (IEngineTrace*)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
	interfaces::pluginHelpers =
		(IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
	interfaces::physicsCollision = (IPhysicsCollision*)interfaceFactory(VPHYSICS_COLLISION_INTERFACE_VERSION, NULL);
	interfaces::staticpropmgr =
		(IStaticPropMgrServer*)interfaceFactory(INTERFACEVERSION_STATICPROPMGR_SERVER, NULL);
	interfaces::shaderDevice = (IShaderDevice*)interfaceFactory(SHADER_DEVICE_INTERFACE_VERSION, NULL);

	if (interfaces::gm)
	{
		DevMsg("SPT: Found IGameMovement at %p.\n", interfaces::gm);
	}
	else
	{
		DevWarning("SPT: Could not find IGameMovement.\n");
		DevWarning("SPT: ProcessMovement logging with spt_tas_log is unavailable.\n");
	}

	if (g_pCVar)
	{
#define GETCVAR(x) _##x = g_pCVar->FindVar(#x);
#ifdef OE
		GETCVAR(viewmodel_fov);
#endif
		GETCVAR(sv_airaccelerate);
		GETCVAR(sv_accelerate);
		GETCVAR(sv_friction);
		GETCVAR(sv_maxspeed);
		GETCVAR(sv_stopspeed);
		GETCVAR(sv_stepsize);
		GETCVAR(sv_gravity);
		GETCVAR(sv_maxvelocity);
		GETCVAR(sv_bounce);
		GETCVAR(sv_cheats);

#ifndef OE
#define GETCMD(x) _##x = g_pCVar->FindCommand(#x);
		GETCMD(record);
		GETCMD(stop);
#endif
	}

#ifdef SSDK2013
	ReplaceAutoCompleteSuggest();
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
		Warning("SPT: spt_afterframes has no effect.\n");
		Warning("SPT: spt_setpitch and spt_setyaw have no effect.\n");
		Warning("SPT: spt_pitchspeed and spt_yawspeed have no effect.\n");
		Warning("SPT: spt_stucksave has no effect.\n");
	}

	if (utils::DoesGameLookLikePortal())
	{
		DevMsg("SPT: This game looks like portal. Setting the spt_tas_* cvars appropriately.\n");

		tas_force_airaccelerate.SetValue(15);
		tas_force_wishspeed_cap.SetValue(60);
		tas_reset_surface_friction.SetValue(0);
	}

	if (!interfaces::engine_server)
	{
		DevWarning("SPT: Failed to get the IVEngineServer interface.\n");
	}

	if (!interfaces::engine_client)
	{
		DevWarning("SPT: Failed to get the IVEngineClient interface.\n");
	}

	if (!interfaces::debugOverlay)
	{
		DevWarning("SPT: Failed to get the debug overlay interface.\n");
		Warning("Seam visualization has no effect.\n");
	}

	if (!interfaces::materialSystem)
		DevWarning("SPT: Failed to get the material system interface.\n");

	if (!interfaces::engine_vgui)
		DevWarning("SPT: Failed to get the engine vgui interface.\n");

	if (!interfaces::engine_tool)
		DevWarning("SPT: Failed to get the engine tool interface.\n");

	if (!interfaces::vgui_input)
		DevWarning("SPT: Failed to get the vgui input interface.\n");

	if (!interfaces::inputSystem)
		DevWarning("SPT: Failed to get the input system interface.\n");

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

	// Start build number search for build number specific feature stuff
	utils::StartBuildNumberSearch();
	TickSignal.Works = true;
	LevelInitSignal.Works = true;
	ServerActivateSignal.Works = true;
	LevelShutdownSignal.Works = true;
	OnEdictAllocatedSignal.Works = true;
	ClientPutInServerSignal.Works = true;
	ClientDisconnectSignal.Works = true;
	ClientSettingsChangedSignal.Works = true;
	Feature::LoadFeatures();
	Cvar_RegisterSPTCvars();

	auto loadTime =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
		.count();
	std::ostringstream out;
	out << "SourcePauseTool version " << SPT_VERSION << " was loaded in " << loadTime << "ms.\n";

	Msg("%s", std::string(out.str()).c_str());

	return true;
}

extern "C" IMAGE_DOS_HEADER __ImageBase;

void CSourcePauseTool::Unload(void)
{
	if (skipUnload)
	{
		// Preventing double load of plugin, do nothing on unload
		skipUnload = false; // Enable unloading again
		return;
	}

#ifdef SSDK2007
	// Replace the plugin handler with the real handle right before the engine tries to unload.
	// This allows it to actually do so.
	// In newer branches (after 3420) this is redundant but doesn't do any harm.
	auto& plugins = *(CUtlVector<void*>*)((uint32_t)interfaces::pluginHelpers + 4);
	extern CSourcePauseTool g_SourcePauseTool;
	for (int i = 0; i < plugins.Count(); i++)
	{
		// m_pPlugin
		if (*(IServerPluginCallbacks**)((uint32_t)plugins[i] + 132) == &g_SourcePauseTool)
		{
			// m_pPluginModule
			*(void**)((uint32_t)plugins[i] + 140) = ((void*)&__ImageBase);
			break;
		}
	}
#endif

	Cvar_UnregisterSPTCvars();
	Feature::UnloadFeatures();
	Hooks::Free();
	pluginLoaded = false;
}

const char* CSourcePauseTool::GetPluginDescription(void)
{
	extern const char* SPT_DESCRIPTION;
	return SPT_DESCRIPTION;
}

void CSourcePauseTool::GameFrame(bool simulating)
{
	if (simulating)
		TickSignal();
}

void CSourcePauseTool::LevelInit(char const* pMapName)
{
	LevelInitSignal(pMapName);
}

void CSourcePauseTool::ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
	ServerActivateSignal(pEdictList, edictCount, clientMax);
}

void CSourcePauseTool::LevelShutdown()
{
	LevelShutdownSignal();
}

#ifndef OE
void CSourcePauseTool::OnEdictAllocated(edict_t* edict)
{
	OnEdictAllocatedSignal(edict);
}
#endif

void CSourcePauseTool::ClientPutInServer(edict_t* pEntity, char const* playername)
{
	ClientPutInServerSignal(pEntity, playername);
}

void CSourcePauseTool::ClientActive(edict_t* pEntity)
{
	ClientActiveSignal(pEntity);
}

void CSourcePauseTool::ClientDisconnect(edict_t* pEntity)
{
	ClientDisconnectSignal(pEntity);
}

void CSourcePauseTool::ClientSettingsChanged(edict_t* pEdict)
{
	ClientSettingsChangedSignal(pEdict);
}

// override new/delete operators to use the game's allocator
// These news and deletes start happening before any of the normal code is ran because of globals, so we gotta always check if we have memalloc
static bool s_hasMemAlloc = false;

static void GetMemAlloc() {
	if (s_hasMemAlloc)
		return;
	s_hasMemAlloc = true;
	HMODULE moduleHandle = GetModuleHandleA("tier0.dll");
	// We get the address of the pointer via GetProcAddress, so have to dereference it
	g_pMemAlloc = *(IMemAlloc**)GetProcAddress(moduleHandle, "g_pMemAlloc");
}


void* __cdecl operator new(unsigned int nSize)
{
	GetMemAlloc();
	return g_pMemAlloc->Alloc(nSize);
}

void* __cdecl operator new[](unsigned int nSize)
{
	GetMemAlloc();
	return g_pMemAlloc->Alloc(nSize);
}

void* __cdecl operator new(unsigned int nSize, int nBlockUse, const char* pFileName, int nLine)
{
	GetMemAlloc();
	return g_pMemAlloc->Alloc(nSize, pFileName, nLine);
}

void* __cdecl operator new[](unsigned int nSize, int nBlockUse, const char* pFileName, int nLine)
{
	GetMemAlloc();
	return g_pMemAlloc->Alloc(nSize, pFileName, nLine);
}

void __cdecl operator delete(void* pMem)
{
	GetMemAlloc();
	g_pMemAlloc->Free(pMem);
}

void __cdecl operator delete[](void* pMem)
{
	GetMemAlloc();
	g_pMemAlloc->Free(pMem);
}
