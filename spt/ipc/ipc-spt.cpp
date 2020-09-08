#include "stdafx.h"

#include "ipc-spt.hpp"

#include "..\sptlib-wrapper.hpp"
#include "convar.h"
#include "ipc.hpp"

namespace ipc
{
	static ipc::IPCServer server;
#ifdef OE
	void IPC_Changed(ConVar* var, char const* pOldString);
#else
	void IPC_Changed(IConVar* var, const char* pOldValue, float flOldValue);
#endif
	ConVar y_spt_ipc("y_spt_ipc", "0", 0, "Enables IPC.", IPC_Changed);
	ConVar y_spt_ipc_port("y_spt_ipc_port", "27182", 0, "Port used for IPC.");

	void StartIPC()
	{
		if (!ipc::Winsock_Initialized())
		{
			ipc::InitWinsock();
			server.StartListening(y_spt_ipc_port.GetString());
		}
	}

	void ShutdownIPC()
	{
		if (ipc::Winsock_Initialized())
		{
			server.CloseConnections();
			ipc::Shutdown_IPC();
		}
	}

	void CmdCallback(const nlohmann::json& msg)
	{
		if (msg.find("cmd") != msg.end())
		{
			std::string cmd = msg["cmd"];
			EngineConCmd(cmd.c_str());
		}
		else
		{
			Msg("Message contained no cmd field!\n");
		}
	}

	void MsgWrapper(const char* msg)
	{
		Msg(msg);
	}

	void Init()
	{
		ipc::AddPrintFunc(MsgWrapper);
		if (y_spt_ipc.GetBool())
		{
			StartIPC();
		}
		server.AddCallback("cmd", CmdCallback, false);
	}

	bool IsActive()
	{
		return ipc::Winsock_Initialized() && server.ClientConnected();
	}

	void Loop()
	{
		if (ipc::Winsock_Initialized())
		{
			server.Loop();
		}
	}

	void Send(const nlohmann::json& msg)
	{
		if (ipc::IsActive())
		{
			server.SendMsg(msg);
		}
	}

#ifdef OE
	void IPC_Changed(ConVar* var, char const* pOldString)
#else
	void IPC_Changed(IConVar* var, const char* pOldValue, float flOldValue)
#endif
	{
		if (y_spt_ipc.GetBool())
		{
			StartIPC();
		}
		else
		{
			ipc::ShutdownIPC();
		}
	}

} // namespace ipc
