#pragma once

#include <memory>
#include "custom_interfaces.hpp"
#include "eiface.h"
#include "tier3\tier3.h"
#include "engine\ivdebugoverlay.h"
#include "vgui\IScheme.h"
#include "vgui\IInput.h"
#include "ienginevgui.h"
#include "toolframework\ienginetool.h"
#include "inputsystem\iinputsystem.h"
#include "cdll_int.h"
#include "icliententitylist.h"
#include "engine\ivmodelinfo.h"
#include "engine\IEngineTrace.h"

namespace interfaces
{
	extern std::unique_ptr<EngineClientWrapper> engine;
	extern IVEngineServer* engine_server;
	extern IVEngineClient* engine_client;
	extern IMatSystemSurface* surface;
	extern vgui::ISchemeManager* scheme;
	extern vgui::IInput* vgui_input;
	extern IEngineVGui* engine_vgui;
	extern IEngineTool* engine_tool;
	extern IVDebugOverlay* debugOverlay;
	extern IMaterialSystem* materialSystem;
	extern IInputSystem* inputSystem;
	extern ICvar* g_pCVar;
	extern void* gm;
	extern IClientEntityList* entList;
	extern IVModelInfo* modelInfo;
	extern IBaseClientDLL* clientInterface;
	extern IEngineTrace* engineTraceClient;
	extern IServerPluginHelpers* pluginHelpers;
} // namespace interfaces