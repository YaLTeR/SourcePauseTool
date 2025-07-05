#pragma once
#include "vcall.hpp"
#include "cdll_int.h"

class EngineClientWrapper
{
public:
	virtual ~EngineClientWrapper() {};
	virtual void ClientCmd(const char* command) = 0;
	virtual void GetViewAngles(QAngle& viewangles) = 0;
	virtual void SetViewAngles(QAngle& viewangles) = 0;

#ifdef OE
	virtual int Cmd_Argc() = 0;
	virtual const char* Cmd_Argv(int arg) = 0;
#endif
	virtual const char* GetGameDirectory() = 0;
};

#ifdef OE
class IVEngineClientDMoMM
{
public:
	void ClientCmd(const char* command)
	{
		utils::vcall<9, void>(this, command);
	}

	void GetViewAngles(QAngle& viewangles)
	{
		utils::vcall<21, void>(this, &viewangles);
	}

	void SetViewAngles(QAngle& viewangles)
	{
		utils::vcall<22, void>(this, &viewangles);
	}

	int Cmd_Argc()
	{
		return utils::vcall<34, int>(this);
	}

	const char* Cmd_Argv(int arg)
	{
		return utils::vcall<35, const char*>(this, arg);
	}
};
#endif

/**
 * Wrapper for an interface similar to IVEngineClient.
 */
template<class EngineClient>
class IVEngineClientWrapper : public EngineClientWrapper
{
public:
	IVEngineClientWrapper(EngineClient* engine) : engine(engine) {}

	virtual void ClientCmd(const char* command) override
	{
		engine->ClientCmd(command);
	}

	virtual void GetViewAngles(QAngle& viewangles) override
	{
		engine->GetViewAngles(viewangles);
	}

	virtual void SetViewAngles(QAngle& viewangles) override
	{
		engine->SetViewAngles(viewangles);
	}

#ifdef OE
	virtual int Cmd_Argc() override
	{
		return engine->Cmd_Argc();
	}

	virtual const char* Cmd_Argv(int arg) override
	{
		return engine->Cmd_Argv(arg);
	}

	virtual const char* GetGameDirectory() override
	{
		extern const char* GetGameDirectoryOE();
		return GetGameDirectoryOE();
	}
#else
	virtual const char* GetGameDirectory() override
	{
		return engine->GetGameDirectory();
	}
#endif

private:
	EngineClient* engine;
};
