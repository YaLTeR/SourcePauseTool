#pragma once

#include "../tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

extern const char* SPT_VERSION;

namespace player_trace
{
	class ITrWriter
	{
	public:
		template<typename T>
		bool Write(const T& o)
		{
			return Write(std::span{(const std::byte*)&o, sizeof o});
		}

		virtual bool Write(std::span<const std::byte> sp) = 0;
		virtual bool DoneWritingTrace() = 0; // notify writer to flush
	};

	class ITrReader
	{
	public:
		template<typename T>
		bool ReadTo(T& o, uint32_t at)
		{
			return ReadTo(std::span{(std::byte*)&o, sizeof o}, at);
		}

		// reading may not be in sequential chunks
		virtual bool ReadTo(std::span<std::byte> sp, uint32_t at) = 0;
	};

	class TrRestore
	{
	public:
		std::string errMsg;
		std::vector<std::string> warnings;

		// if false is returned, errMsg should have a message
		// if true is returned, the trace is valid and 'warnings' may have messages
		bool Restore(TrPlayerTrace& tr, ITrReader& rd);
	};

	// the writer interface is very simple and doesn't provide error message or warnings
	class TrWrite
	{
	public:
		bool Write(const TrPlayerTrace& tr, ITrWriter& wr);
	};

} // namespace player_trace

#endif
