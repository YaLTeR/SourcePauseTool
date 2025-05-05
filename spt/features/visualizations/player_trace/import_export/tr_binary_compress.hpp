#pragma once

#include "tr_binary.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

#define LZMA_API_STATIC
#include "thirdparty/xz/include/lzma.h"

#define TR_COMPRESSED_FILE_EXT ".sptr.xz"

namespace player_trace
{
	class TrXzFileWriter : public ITrWriter
	{
		lzma_stream lzma_strm;
		bool alive;
		std::vector<uint8_t> outBuf;
		std::ostream& oStream;

	public:
		TrXzFileWriter(std::ostream& oStream, uint32_t compressionLevel = 1);
		~TrXzFileWriter();

		virtual bool Write(std::span<const std::byte> sp);
		virtual bool DoneWritingTrace();

	private:
		bool LzmaToOfStream(bool finish);
	};

	class TrXzFileReader : public ITrReader
	{
		/*
		* Since lzma streams don't support seeking and we can't guarantee that we will read data
		* sequentially, I'm going with the simple approach of decompressing the whole thing at once.
		*/
		std::vector<uint8_t> outBuf;

	public:
		/*
		* If an error occurs while decoding the lzma stream, it will be logged here and all ReadTo
		* calls will (probably) return false.
		*/
		std::string errMsg;

		TrXzFileReader(std::istream& iStream);

		virtual bool ReadTo(std::span<std::byte> sp, uint32_t at);
	};
} // namespace player_trace

#endif
