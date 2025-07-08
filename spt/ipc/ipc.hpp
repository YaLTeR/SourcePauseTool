#include <unordered_map>
#include <vector>

#include "thirdparty\json_spt_include.hpp"

namespace ipc
{
	typedef void (*PrintFunc)(const char* msg);
	typedef void (*MsgCallback)(const nlohmann::json& msg);

	void Print(const char* msg, ...);
	void Shutdown_IPC();
	bool Winsock_Initialized();
	void InitWinsock();
	void AddPrintFunc(PrintFunc func);

	class IPCServer
	{
	public:
		IPCServer();
		void StartListening(const char* port);
		void CloseConnections();
		void Loop();
		bool BlockForMessages(const std::string& msg, int timeoutMsec);
		void AddCallback(std::string type, MsgCallback callback, bool blocking);
		void SendMsg(const nlohmann::json& msg);
		bool ClientConnected();
		~IPCServer();

	private:
		void ReadMessages();
		void CheckForConnections();
		void DispatchMessages();
		void DispatchMessages(const std::string& type);
		int listenSocket;
		int clientSocket;
		char* RECV_BUFFER;

		std::unordered_map<std::string, MsgCallback> callbacks;
		std::unordered_map<std::string, bool> blockingMap;
		std::unordered_map<std::string, std::vector<nlohmann::json>> msgQueue;
	};

} // namespace ipc
