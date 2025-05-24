#include "stdafx.hpp"

#include "tr_binary_compress.hpp"

#ifdef SPT_PLAYER_TRACE_ENABLED

using namespace player_trace;

constexpr char TR_XZ_FILE_ID[] = "omg_hi!";
constexpr uint32_t TR_XZ_FILE_VERSION = 1;

struct TrXzFooter
{
	uint32_t numCompressedBytes;
	uint32_t numUncompressedBytes;
	char id[sizeof TR_XZ_FILE_ID];
	uint32_t version;
};

TrXzFileWriter::TrXzFileWriter(std::ostream& oStream, uint32_t compressionLevel)
    : oStream{oStream}, lzma_strm{lzma_stream LZMA_STREAM_INIT}, alive{true}
{
	lzma_mt mt{
	    .threads = std::thread::hardware_concurrency(),
	    .timeout = 0,
	    .preset = compressionLevel,
	    .check = LZMA_CHECK_CRC64,
	};
	alive = lzma_stream_encoder_mt(&lzma_strm, &mt) == LZMA_OK;
	if (alive)
		outBuf.resize(1 << 16);
}

TrXzFileWriter::~TrXzFileWriter()
{
	lzma_end(&lzma_strm);
}

bool TrXzFileWriter::Write(std::span<const std::byte> sp)
{
	if (!alive)
		return false;
	lzma_strm.avail_in = sp.size_bytes();
	lzma_strm.next_in = (uint8_t*)sp.data();
	return LzmaToOfStream(false);
}

bool TrXzFileWriter::DoneWritingTrace()
{
	alive = LzmaToOfStream(true);
	if (alive)
	{
		TrXzFooter footer{
		    .numCompressedBytes = (uint32_t)lzma_strm.total_out,
		    .numUncompressedBytes = (uint32_t)lzma_strm.total_in,
		    .version = TR_XZ_FILE_VERSION,
		};
		memcpy(footer.id, TR_XZ_FILE_ID, sizeof footer.id);
		alive = oStream.write((char*)&footer, sizeof footer).good();
	}
	if (alive)
		alive = oStream.flush().good();
	return alive;
}

bool TrXzFileWriter::LzmaToOfStream(bool finish)
{
	while (alive)
	{
		lzma_strm.avail_out = outBuf.size();
		lzma_strm.next_out = outBuf.data();
		lzma_ret ret = lzma_code(&lzma_strm, finish ? LZMA_FINISH : LZMA_RUN);
		alive = ret == LZMA_OK || ret == LZMA_BUF_ERROR || ret == LZMA_STREAM_END;
		size_t nBytesToWrite = outBuf.size() - lzma_strm.avail_out;
		if (alive && nBytesToWrite > 0)
			alive = oStream.write((char*)outBuf.data(), nBytesToWrite).good();
		if ((!finish && lzma_strm.avail_out > 0) || (finish && ret == LZMA_STREAM_END))
			break;
	}
	return alive;
}

TrXzFileReader::TrXzFileReader(std::istream& iStream)
{
	TrXzFooter footer;
	bool alive = iStream.seekg(-(int)sizeof(TrXzFooter), std::ios_base::end)
	                 .read((char*)&footer, sizeof footer)
	                 .seekg(0)
	                 .good();
	if (!alive)
	{
		errMsg = "failed to read footer";
		return;
	}
	if (memcmp(footer.id, TR_XZ_FILE_ID, sizeof footer.id) || footer.version != TR_XZ_FILE_VERSION)
	{
		errMsg = "invalid footer";
		return;
	}

	lzma_stream lzma_strm LZMA_STREAM_INIT;

	lzma_mt mt{
	    .flags = LZMA_FAIL_FAST,
	    .threads = std::thread::hardware_concurrency(),
	    .timeout = 0,
	    .memlimit_threading = 1 << 29,
	    .memlimit_stop = 1 << 29,
	};
	lzma_ret ret = lzma_stream_decoder_mt(&lzma_strm, &mt);
	alive = ret == LZMA_OK;
	if (!alive)
	{
		errMsg = std::format("failed to initialize lzma decoder (error {})", (int)ret);
		return;
	}

	outBuf.resize(footer.numUncompressedBytes);
	lzma_strm.avail_out = outBuf.size();
	lzma_strm.next_out = outBuf.data();

	std::vector<uint8_t> inBuf(1 << 16);
	do
	{
		uint32_t nBytesToRead = MIN(inBuf.size(), footer.numCompressedBytes - lzma_strm.total_in);
		if (nBytesToRead == 0)
			break;
		alive = iStream.read((char*)inBuf.data(), nBytesToRead).good();
		if (alive)
		{
			lzma_strm.avail_in = inBuf.size();
			lzma_strm.next_in = inBuf.data();
			ret = lzma_code(&lzma_strm, LZMA_RUN);
			alive = ret == LZMA_OK || ret == LZMA_STREAM_END;
			if (!alive)
				errMsg = std::format("lzma_code error: {}", (int)ret);
		}
		else
		{
			errMsg = "file read error";
		}
	} while (alive && lzma_strm.avail_out > 0 && ret != LZMA_STREAM_END);

	if (alive && lzma_strm.total_out != outBuf.size())
	{
		errMsg = "stream ran out of bytes";
		alive = false;
	}

	if (!alive)
		outBuf = std::move(std::vector<uint8_t>{});

	lzma_end(&lzma_strm);
}

bool TrXzFileReader::ReadTo(std::span<std::byte> sp, uint32_t at)
{
	if (at + sp.size() <= outBuf.size())
	{
		memcpy(sp.data(), outBuf.data() + at, sp.size());
		return true;
	}
	return false;
}

#endif
