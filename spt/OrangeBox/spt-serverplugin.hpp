#pragma once

#define SPT_VERSION __DATE__ " " __TIME__

#if defined(OE)
#include "cdll_int.h"
#endif

#include "custom_interfaces.hpp"
#include "eiface.h"
#include "engine\iserverplugin.h"
#include "engine\ivdebugoverlay.h"
#include "icvar.h"
#include "vgui\ischeme.h"
#include "vguimatsurface\imatsystemsurface.h"
#include "thirdparty\Signal.h"

// glorious hack cause p2 doesn't have conwarning for some reason
#ifdef P2
#define ConWarning(arg1, arg2) Warning(arg2)
#endif

//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class CSourcePauseTool : public IServerPluginCallbacks
{
public:
	CSourcePauseTool(){};
	~CSourcePauseTool(){};

	// IServerPluginCallbacks methods
	virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void Unload(void);
	virtual void Pause(void){};
	virtual void UnPause(void){};
	virtual const char* GetPluginDescription(void);
	virtual void LevelInit(char const* pMapName){};
	virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax){};
	virtual void GameFrame(bool simulating);
	virtual void LevelShutdown(void){};
	virtual void ClientActive(edict_t* pEntity){};
	virtual void ClientDisconnect(edict_t* pEntity){};
	virtual void ClientPutInServer(edict_t* pEntity, char const* playername){};
	virtual void SetCommandClient(int index){};
	virtual void ClientSettingsChanged(edict_t* pEdict){};

#if defined(OE)
	virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect,
	                                    edict_t* pEntity,
	                                    const char* pszName,
	                                    const char* pszAddress,
	                                    char* reject,
	                                    int maxrejectlen)
	{
		return PLUGIN_CONTINUE;
	};
	virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity)
	{
		return PLUGIN_CONTINUE;
	};
	virtual PLUGIN_RESULT NetworkIDValidated(const char* pszUserName, const char* pszNetworkID)
	{
		return PLUGIN_CONTINUE;
	};
#else
	virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect,
	                                    edict_t* pEntity,
	                                    const char* pszName,
	                                    const char* pszAddress,
	                                    char* reject,
	                                    int maxrejectlen)
	{
		return PLUGIN_CONTINUE;
	};
	virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity, const CCommand& args)
	{
		return PLUGIN_CONTINUE;
	};
	virtual PLUGIN_RESULT NetworkIDValidated(const char* pszUserName, const char* pszNetworkID)
	{
		return PLUGIN_CONTINUE;
	};
	virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie,
	                                      edict_t* pPlayerEntity,
	                                      EQueryCvarValueStatus eStatus,
	                                      const char* pCvarName,
	                                      const char* pCvarValue){};

	// added with version 3 of the interface.
	virtual void OnEdictAllocated(edict_t* edict){};
	virtual void OnEdictFreed(const edict_t* edict){};

#endif

	Gallant::Signal0<void> ServerGameFrameSignal;

#ifdef P2
	virtual void ClientFullyConnect(edict_t* pEntity){};
#endif
};

IServerUnknown* GetServerPlayer();
IVEngineServer* GetEngine();
IVDebugOverlay* GetDebugOverlay();
void* GetGamemovement();
ICvar* GetCvarInterface();
std::string GetGameDir();
bool DoesGameLookLikeHDTF();
bool DoesGameLookLikePortal();
bool DoesGameLookLikeHLS();

#if defined(OE)
struct ArgsWrapper
{
	EngineClientWrapper* engine_pointer;

	ArgsWrapper(EngineClientWrapper* engine)
	{
		this->engine_pointer = engine;
	};

	ArgsWrapper();

	int ArgC()
	{
		return engine_pointer->Cmd_Argc();
	};

	const char* Arg(int arg)
	{
		return engine_pointer->Cmd_Argv(arg);
	};
};
#endif
