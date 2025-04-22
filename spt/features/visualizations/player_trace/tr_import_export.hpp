#pragma once

#include "tr_structs.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

#define LZMA_API_STATIC
#include "thirdparty/xz/include/lzma.h"

#define TR_FILE_EXTENSION ".sptr.xz"

namespace player_trace
{
	constexpr size_t TR_MAX_SPT_VERSION_LEN = 32;

	struct TrLump_v1;

	class ITrWriter
	{
	public:
		bool WriteTrace(const TrPlayerTrace& tr);

	protected:
		virtual bool Write(std::span<const char> sp) = 0;
		virtual void DoneWritingTrace() = 0;

		bool Write(const void* what, size_t nBytes)
		{
			return Write(std::span<const char>{(char*)what, nBytes});
		}

		template<typename T>
		bool Write(const T& o)
		{
			return Write(&o, sizeof o);
		}

	private:
		template<typename T>
		bool WriteLumpHeader(const std::vector<T>& vec, uint32_t& fileOff, uint32_t& lumpDataFileOff);

		template<typename T>
		bool WriteLumpData(const std::vector<T>& vec, uint32_t& fileOff);
	};

	class ITrReader
	{
	public:
		// if false is returned, errMsg may have warnings
		bool ReadTrace(TrPlayerTrace& tr, char sptVersionOut[TR_MAX_SPT_VERSION_LEN], std::string& errMsg);

	protected:
		virtual bool ReadTo(std::span<char> sp, uint32_t at) = 0;
		virtual void DoneReadingTrace() = 0;

		template<typename T>
		bool ReadTo(T& o, uint32_t at)
		{
			return ReadTo(std::span<char>{(char*)&o, sizeof o}, at);
		}

	private:
		template<typename T>
		bool ReadLumpData(std::vector<T>& vec,
		                  std::unordered_map<std::string_view, const TrLump_v1*>& lumpMap,
		                  std::string& errMsg);
	};

	class TrXzFileWriter : public ITrWriter
	{
		lzma_stream lzma_strm;
		bool alive;
		std::vector<uint8_t> outBuf;
		std::ostream& oStream;

	public:
		TrXzFileWriter(std::ostream& oStream, uint32_t compressionLevel = 1);
		~TrXzFileWriter();

	protected:
		virtual bool Write(std::span<const char> sp);
		virtual void DoneWritingTrace();

	private:
		bool LzmaToOfStream(bool finish);
	};

	class TrXzFileReader : public ITrReader
	{
		/*
		* Since lzma streams don't support seeking and we can't guarantee that we will read data
		* sequentially, I'm going with the naive approach of decompressing the whole thing at once.
		*/
		std::vector<uint8_t> outBuf;

	public:
		/*
		* If an error happens while decoding the lzma stream, it will be logged here and all ReadTo
		* calls (and therefore ReadTrace) will return false. Prioritize reading this message over
		* the message returned by ITrReader::ReadTrace
		*/
		std::string errMsg; 

		TrXzFileReader(std::istream& iStream);

	protected:
		virtual bool ReadTo(std::span<char> sp, uint32_t at);
		virtual void DoneReadingTrace() {};
	};

} // namespace player_trace

#endif
