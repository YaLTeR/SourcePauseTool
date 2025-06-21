#pragma once

#include "dbg.h"
#undef min
#undef max

#include <algorithm>
#include <span>
#include <vector>
#include <string>
#include <iostream>
#include <optional>
#include <memory>

namespace ser
{
	struct Status
	{
		bool ok = true;
		std::string errMsg;
		std::vector<std::string> warnings;
	};

	class StatusTracker
	{
	protected:
		Status status;

	public:
		void Err(std::string s)
		{
			status.ok = false;
			if (status.errMsg.empty())
				status.errMsg = std::move(s);
		}

		void Warn(std::string s)
		{
			status.warnings.push_back(std::move(s));
		}

		const Status& GetStatus()
		{
			if (!status.ok && status.errMsg.empty())
			{
				AssertMsg(
				    0,
				    "SPT: the reader/writer or serialize/deserialize functions should log an error message on failure");
				status.errMsg = "an unspecified error occured";
			}
			return status;
		}

		bool Ok() const
		{
			return status.ok;
		}
	};

	/*
	* Abstract reader/writer interfaces. Example use case:
	* 
	* class MyClass {
	*   void Serialize(IWriter& wr);
	*   void Deserialize(IReader& rd);
	* };
	* 
	* Within these functions, you can call WritePod/WriteSpan or ReadPod/ReadSpan. You can either:
	* - exit early by checking the return of each call
	* - unconditionally do each call and do wr.Ok() or rd.Ok() checks to early exit
	* 
	* You can also write warnings & errors to the reader/writer using Warn() and Err(). If Err() is
	* called, Ok() will return false from then on and so will any read/write functions.
	* 
	* The caller of these functions should check the status of the reader/writer afterwards.
	* - in case Ok() returns false, you should ideally print the errMsg to console or whatever
	* - in case Ok() returns true, you can optionally print the warnings from the status
	* 
	* The writer must have Finish() called before destruction - call after MyClass::Serialize(),
	* but before checking the status.
	* 
	* When writing the serialize/deserialize functions, you can (and should) call Rebase() if you
	* use relative addresses anywhere in your format. This allows trivial concatenation of multiple
	* streams.
	*/

	class IWriter : public StatusTracker
	{
		size_t nWritten = 0;
		size_t absBase = 0;
		bool finished = false;

		bool Write(std::span<const std::byte> sp)
		{
			if (finished) [[unlikely]]
			{
				AssertMsg(0, "Finish() was already called on this writer!");
				return status.ok = false;
			}
			if (!status.ok) [[unlikely]]
				return false;
			size_t oldNWritten = nWritten;
			if (_addcarry_u32(0, oldNWritten, sp.size_bytes(), &nWritten)) [[unlikely]]
			{
				Err("writer overflowed");
				nWritten = oldNWritten;
				return false;
			}
			else
			{
				return status.ok = WriteInternal(sp);
			}
		}

	protected:
		IWriter() = default;
		IWriter(IWriter&) = delete;

		virtual bool WriteInternal(std::span<const std::byte> sp) = 0;
		virtual bool FinishInternal() = 0;

	public:
		virtual ~IWriter()
		{
			AssertMsg(!status.ok || finished, "SPT: Writer was not finished before destruction");
			// we can't call Finish() here because base classes have already had their dtor called
		}

		template<typename T>
		bool WritePod(const T& o)
		{
			return Write(std::span{(const std::byte*)&o, sizeof o});
		}

		template<typename T>
		bool WriteSpan(std::span<T> sp)
		{
			return Write(std::as_bytes(sp));
		}

		void Rebase()
		{
			absBase = nWritten;
		}

		size_t GetRelPos() const
		{
			return nWritten - absBase;
		}

		size_t GetAbsWritten() const
		{
			return nWritten;
		}

		size_t GetRelWritten() const
		{
			return nWritten - absBase;
		}

		// must be called BEFORE destruction
		bool Finish()
		{
			if (status.ok && finished)
				return true;
			if (!status.ok)
				return false;
			return finished = status.ok = FinishInternal();
		}
	};

	class IReader : public StatusTracker
	{
		size_t nextAbsReadPos = 0;
		size_t absBase = 0;
		std::optional<size_t> len;

		bool Read(std::span<std::byte> to)
		{
			if (!status.ok) [[unlikely]]
				return false;
			size_t oldAbsReadPos = nextAbsReadPos;
			if (_addcarry_u32(0, oldAbsReadPos, to.size_bytes(), &nextAbsReadPos)) [[unlikely]]
			{
				Err("reader overflowed");
				nextAbsReadPos = oldAbsReadPos;
				return false;
			}
			else
			{
				return status.ok = ReadInternal(to, oldAbsReadPos);
			}
		}

	protected:
		IReader() = default;
		IReader(IReader&) = delete;

		virtual bool ReadInternal(std::span<std::byte> to, size_t absOff) = 0;
		virtual size_t TotalNumBytes() = 0; // only called once

		size_t GetAbsReadPos() const
		{
			return nextAbsReadPos;
		}

	public:
		virtual ~IReader() {}

		size_t NumBytesLeft()
		{
			if (status.ok && !len.has_value()) [[unlikely]]
				len = TotalNumBytes();
			return status.ok && nextAbsReadPos <= len.value() ? len.value() - nextAbsReadPos : 0;
		}

		size_t GetPos() const
		{
			return nextAbsReadPos - absBase;
		}

		bool SetPos(size_t relToBase)
		{
			size_t oldAbsNextReadPos = nextAbsReadPos;
			if (_addcarry_u32(0, absBase, relToBase, &nextAbsReadPos)) [[unlikely]]
			{
				Err("reader overflowed");
				nextAbsReadPos = oldAbsNextReadPos;
				return false;
			}
			return true;
		}

		void Rebase()
		{
			absBase = nextAbsReadPos;
		}

		bool Skip(size_t nBytes)
		{
			if (NumBytesLeft() < nBytes) [[unlikely]]
			{
				Err("reader overflowed");
				return false;
			}
			nextAbsReadPos += nBytes;
			return true;
		}

		// read immediately after the last read thing (or at the place we last seeked to)

		template<typename T>
		bool ReadPod(T& o)
		{
			return Read({(std::byte*)&o, sizeof(T)});
		}

		template<typename T>
		bool ReadSpan(std::span<T> sp)
		{
			return Read(std::as_writable_bytes(sp));
		}
	};

	// basic iostream wrappers

	class StreamWriter : public IWriter
	{
	protected:
		std::ostream& ostream;

		bool CheckErr()
		{
			if (ostream.fail())
			{
				Err("failed to write to output stream");
				return false;
			}
			return true;
		}

		virtual bool WriteInternal(std::span<const std::byte> sp) override
		{
			if (sp.empty())
				return true;
			ostream.write((const char*)sp.data(), sp.size_bytes());
			return CheckErr();
		}

		virtual bool FinishInternal() override
		{
			ostream.flush();
			return CheckErr();
		}

	public:
		StreamWriter(std::ostream& ostream) : ostream{ostream} {}
	};

	class StreamReader : public IReader
	{
	protected:
		std::istream& istream;

		bool CheckErr()
		{
			if (istream.fail())
			{
				Err("failed to read from input stream");
				return false;
			}
			return true;
		}

		virtual bool ReadInternal(std::span<std::byte> to, size_t absOff) override
		{
			istream.seekg(absOff).read((char*)to.data(), to.size_bytes());
			return CheckErr();
		}

		virtual size_t TotalNumBytes() override
		{
			return (size_t)istream.seekg(0, std::ios_base::end).tellg();
		}

	public:
		StreamReader(std::istream& istream) : istream{istream} {}
	};

	// memory reader/writer :)

	class MemReader : public IReader
	{
	protected:
		std::span<const std::byte> source;

		virtual bool ReadInternal(std::span<std::byte> to, size_t absOff) override
		{
			if (absOff + to.size() <= source.size()) [[likely]]
			{
				memcpy(to.data(), source.data() + absOff, to.size());
				return true;
			}
			Err("attempted to read too many bytes from mem reader");
			return false;
		}

		virtual size_t TotalNumBytes() override
		{
			return source.size_bytes();
		}

	public:
		template<typename T>
		        requires(sizeof(T) == 1)
		MemReader(std::span<T> mem) : source{std::as_bytes(mem)}
		{
		}

		// get span for remaining data, does not increment read position
		auto GetRemaining() const
		{
			return Ok() ? source.subspan(GetAbsReadPos()) : std::span<const std::byte>{};
		}
	};

	// use MakeMemWriter, see below
	template<typename WriteElem, typename SinkIt>
	        requires(sizeof(WriteElem) == 1)
	class MemWriter : public IWriter
	{
	protected:
		SinkIt sinkIt;
		size_t nWritten = 0;
		const size_t maxBytes;
		bool overflowed = false;

		virtual bool WriteInternal(std::span<const std::byte> sp) override
		{
			size_t nNew = sp.size_bytes();
			if (nWritten > (size_t)-1 - nNew || nWritten + nNew > maxBytes) [[unlikely]]
			{
				overflowed = true;
				Err("attempted to write too many bytes to mem writer");
				return false;
			}
			sinkIt = std::copy((WriteElem*)sp.data(), (WriteElem*)sp.data() + nNew, sinkIt);
			nWritten += nNew;
			return true;
		}

		virtual bool FinishInternal() override
		{
			return true;
		};

	public:
		MemWriter(SinkIt outIt, size_t maxBytes) : sinkIt{outIt}, maxBytes{maxBytes} {}
	};

	/*
	* Apparently C++ only supports class template partial specialization if you give zero template
	* arguments, but functions support partial specialization deduction. Also, output iterators
	* don't have an output value_type, so we need to provide the type we're *writing to* as a
	* separate template. Usage:
	* 
	* std::vector<char> vec;
	* auto wr = MakeMemWriter<char>(std::back_inserter(vec));
	* 
	* std::array<char, 32> arr;
	* auto wr = MakeMemWriter<char>(arr.begin(), arr.size());
	*/
	template<typename WriteElem, typename SinkIt>
	MemWriter<WriteElem, SinkIt> MakeMemWriter(SinkIt sinkIt, size_t maxBytes = (size_t)-1)
	{
		return MemWriter<WriteElem, SinkIt>{sinkIt, maxBytes};
	}

	// reader/writer wrappers that create/read a single xz stream from/to another reader/writer

	class XzWriter : public IWriter
	{
	private:
		// PImpl idiom to hide lzma details
		struct LzImpl;
		std::unique_ptr<LzImpl> plzImpl;
		IWriter& sink;

	protected:
		virtual bool WriteInternal(std::span<const std::byte> sp) override;
		virtual bool FinishInternal() override;

	public:
		XzWriter(IWriter& sink, uint32_t compressionLevel = 1, size_t bufSize = 1 << 16);
		virtual ~XzWriter() override;

	protected:
		bool WriteLzData(bool finish);
	};

	class XzReader : public MemReader
	{
	private:
		/*
		* Since lzma streams don't support seeking and we can't guarantee that we will read data
		* sequentially, I'm just decompressing the whole thing into this vector.
		*/
		std::vector<std::byte> decompressedBuf;

		void Init(std::span<const std::byte> compressedSource,
		          size_t decompressedMemLimit,
		          size_t lzmaMemLimit);

	public:
		XzReader(std::span<const std::byte> compressedSource,
		         size_t decompressedMemLimit = 1 << 29,
		         size_t lzmaMemLimit = 1 << 29);

		XzReader(IReader& compressedSource,
		         size_t decompressedMemLimit = 1 << 29,
		         size_t lzmaMemLimit = 1 << 29);
	};

} // namespace ser
