#include "stdafx.hpp"

#include "serialize.hpp"

#define LZMA_API_STATIC
#include "thirdparty/xz/include/lzma.h"

#include <format>

using namespace ser;

struct XzWriter::LzImpl
{
	lzma_stream strm = LZMA_STREAM_INIT;
	std::vector<uint8_t> writeBuf;
	bool alive = true;

	LzImpl(size_t bufSize) : writeBuf(bufSize) {}
};

XzWriter::XzWriter(IWriter& sink, uint32_t compressionLevel, size_t bufSize)
    : plzImpl{std::make_unique<LzImpl>(bufSize)}, sink{sink}
{
	if (bufSize == 0)
	{
		plzImpl->alive = false;
	}
	else
	{
		lzma_mt mt{
		    .threads = 1,
		    .timeout = 0,
		    .preset = compressionLevel,
		    .check = LZMA_CHECK_CRC64,
		};
		plzImpl->alive = lzma_stream_encoder_mt(&plzImpl->strm, &mt) == LZMA_OK;
	}
}

XzWriter::~XzWriter()
{
	lzma_end(&plzImpl->strm);
}

bool XzWriter::WriteInternal(std::span<const std::byte> sp)
{
	if (sp.empty())
		return true;
	plzImpl->strm.avail_in = sp.size_bytes();
	plzImpl->strm.next_in = (uint8_t*)sp.data();
	return WriteLzData(false);
}

bool XzWriter::FinishInternal()
{
	if (!WriteLzData(true))
		return false;
	if (sink.Finish())
		return true;
	Err(sink.GetStatus().errMsg);
	return false;
}

bool XzWriter::WriteLzData(bool finish)
{
	bool sinkAlive = true;
	lzma_ret ret = LZMA_OK;
	while (plzImpl->alive && sinkAlive)
	{
		plzImpl->strm.avail_out = plzImpl->writeBuf.size();
		plzImpl->strm.next_out = plzImpl->writeBuf.data();
		ret = lzma_code(&plzImpl->strm, finish ? LZMA_FINISH : LZMA_RUN);
		plzImpl->alive = ret == LZMA_OK || ret == LZMA_BUF_ERROR || ret == LZMA_STREAM_END;
		size_t nBytesToWrite = plzImpl->writeBuf.size() - plzImpl->strm.avail_out;
		if (plzImpl->alive && nBytesToWrite > 0)
			sinkAlive = sink.WriteSpan(std::span{plzImpl->writeBuf}.first(nBytesToWrite));
		if ((!finish && plzImpl->strm.avail_out > 0) || (finish && ret == LZMA_STREAM_END))
			break;
	}
	if (!sinkAlive)
		Err(sink.GetStatus().errMsg);
	else if (!plzImpl->alive)
		Err(std::format("lzma encoding failed, code: {}", (int)ret));
	return plzImpl->alive && sinkAlive;
}

void XzReader::Init(std::span<const std::byte> compressedSource, size_t decompressedMemLimit, size_t lzmaMemLimit)
{
	if (compressedSource.empty() || decompressedMemLimit == 0)
		return;

	bool decompressedSizeKnown = false;
	size_t compressedSize = compressedSource.size_bytes();
	size_t decompressedSize = 0;

	struct OldFooter
	{
		uint32_t numCompressedBytes;
		uint32_t numUncompressedBytes;
		char id[8];
		uint32_t version;
	};

	// backwards compat - old player trace format appended an unnecessary footer which messes with lzma
	if (compressedSource.size_bytes() >= sizeof(OldFooter))
	{
		size_t expectedCompressedSize = compressedSource.size_bytes() - sizeof(OldFooter);
		auto& oldFooter = *(const OldFooter*)&compressedSource[expectedCompressedSize];
		if (oldFooter.version == 1 && !strcmp(oldFooter.id, "omg_hi!")
		    && oldFooter.numCompressedBytes == expectedCompressedSize)
		{
			compressedSize = expectedCompressedSize;
			decompressedSize = oldFooter.numUncompressedBytes;
			decompressedSizeKnown = true;
		}
	}

	lzma_stream lzStream = LZMA_STREAM_INIT;
	lzma_ret ret = lzma_stream_decoder(&lzStream, lzmaMemLimit, 0);
	if (ret != LZMA_OK)
	{
		lzma_end(&lzStream);
		Err(std::format("lzma_stream_decoder failed, code: {}", (int)ret));
		return;
	}

	// decompress outChunkSize bytes at a time

	constexpr size_t outChunkSize = 4096;

	if (decompressedSizeKnown)
		decompressedBuf.resize(decompressedSize);
	else
		decompressedBuf.resize(std::min(outChunkSize, decompressedMemLimit));

	lzStream.next_in = (const uint8_t*)compressedSource.data();
	lzStream.avail_in = compressedSize;

	for (;;)
	{
		lzStream.next_out = (uint8_t*)decompressedBuf.data() + (size_t)lzStream.total_out;
		lzStream.avail_out = decompressedBuf.size() - (size_t)lzStream.total_out;
		ret = lzma_code(&lzStream, LZMA_FINISH);
		if (ret != LZMA_OK && ret != LZMA_BUF_ERROR)
			break;
		size_t newSize = decompressedBuf.size() + outChunkSize;
		if (newSize > decompressedMemLimit)
		{
			lzma_end(&lzStream);
			Err("decompressed size is too big");
			return;
		}
		decompressedBuf.resize(newSize);
	}

	lzma_end(&lzStream);
	if (ret != LZMA_STREAM_END)
	{
		Err(std::format("lzma decoding failed, code: {}", (int)ret));
		return;
	}

	MemReader::source = std::span{decompressedBuf}.first((size_t)lzStream.total_out);
}

XzReader::XzReader(std::span<const std::byte> compressedSource, size_t decompressedMemLimit, size_t lzmaMemLimit)
    : MemReader{std::span<char>{}}
{
	Init(compressedSource, decompressedMemLimit, lzmaMemLimit);
}

XzReader::XzReader(IReader& compressedSource, size_t decompressedMemLimit, size_t lzmaMemLimit)
    : MemReader{std::span<char>{}}
{
	std::vector<std::byte> compressedBuf(compressedSource.NumBytesLeft());
	if (!compressedSource.ReadSpan(std::span{compressedBuf}))
		Err(compressedSource.GetStatus().errMsg);
	else
		Init(compressedBuf, decompressedMemLimit, lzmaMemLimit);
}
