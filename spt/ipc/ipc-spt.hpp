#include "thirdparty\json.hpp"

namespace ipc
{
	void Init();
	bool IsActive();
	void Loop();
	void Send(const nlohmann::json& msg);
} // namespace ipc
