#pragma once

#include <memory>
#include "custom_interfaces.hpp"
#include "eiface.h"
#include "tier3\tier3.h"
#include "engine\ivdebugoverlay.h"
#include "vgui\IScheme.h"
#include "cdll_int.h"
#include "icliententitylist.h"
#include "engine\ivmodelinfo.h"

namespace interfaces
{
	extern std::unique_ptr<EngineClientWrapper> engine;
	extern IVEngineServer* engine_server;
	extern IMatSystemSurface* surface;
	extern vgui::ISchemeManager* scheme;
	extern IVDebugOverlay* debugOverlay;
	extern IMaterialSystem* materialSystem;
	extern ICvar* g_pCVar;
	extern void* gm;
	extern IClientEntityList* entList;
	extern IVModelInfo* modelInfo;
	extern IBaseClientDLL* clientInterface;
} // namespace interfaces