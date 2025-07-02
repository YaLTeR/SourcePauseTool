#pragma once

namespace utils
{
	template<size_t index, typename T, typename... Args>
	__forceinline T vcall(void* thisptr, Args... args)
	{
		using fn_t = T(__fastcall*)(void*, int, Args...);

		auto fn = (*reinterpret_cast<fn_t**>(thisptr))[index];
		return fn(thisptr, 0, args...);
	}
} // namespace utils
