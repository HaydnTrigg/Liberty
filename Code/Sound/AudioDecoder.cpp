#include "AudioDecoder.h"

#include <DACOM.h>		// GENERAL_ERROR / diagnostics

/*
 * AudioDecoder.cpp
 *
 * RIFF/WAVE parsing and PCM/MP3 decoding. See AudioDecoder.h for the contract.
 */

namespace
{
	/*
	 * Packs four characters into a little-endian FourCC, matching the byte order
	 * of RIFF chunk identifiers on disk.
	 */
	constexpr DWORD MakeFourCC(char a, char b, char c, char d)
	{
		return static_cast<DWORD>(static_cast<BYTE>(a))
			| (static_cast<DWORD>(static_cast<BYTE>(b)) << 8)
			| (static_cast<DWORD>(static_cast<BYTE>(c)) << 16)
			| (static_cast<DWORD>(static_cast<BYTE>(d)) << 24);
	}

	constexpr DWORD kFourCC_RIFF = MakeFourCC('R', 'I', 'F', 'F');
	constexpr DWORD kFourCC_WAVE = MakeFourCC('W', 'A', 'V', 'E');
	constexpr DWORD kFourCC_fmt = MakeFourCC('f', 'm', 't', ' ');
	constexpr DWORD kFourCC_data = MakeFourCC('d', 'a', 't', 'a');

	// Number of compressed bytes buffered between ACM conversions.
	constexpr DWORD kSourceBufferBytes = 16 * 1024;

	/*
	 * Locates and opens the installed MPEG Layer-3 ACM decoder (the Fraunhofer IIS
	 * codec on a standard Windows install). Returns NULL if no matching driver is
	 * found, in which case the caller lets ACM auto-select one.
	 */
	HACMDRIVER OpenMp3AcmDriver()
	{
		// Preferred driver long-names, most capable first.
		static const char* const kPreferredDrivers[] =
		{
			"Fraunhofer IIS MPEG Layer-3 Codec (advanced)",
			"Fraunhofer IIS MPEG Layer-3 Codec (decode only)",
		};

		struct EnumContext
		{
			const char* desiredName;
			HACMDRIVERID driverId;
		};

		auto enumProc = [](HACMDRIVERID hadid, DWORD_PTR dwInstance, DWORD /*fdwSupport*/) -> BOOL
		{
			EnumContext* context = reinterpret_cast<EnumContext*>(dwInstance);

			ACMDRIVERDETAILSA details = { sizeof(details) };
			if (acmDriverDetailsA(hadid, &details, 0) == MMSYSERR_NOERROR &&
				strcmp(context->desiredName, details.szLongName) == 0)
			{
				context->driverId = hadid;
				return FALSE;	// Stop enumeration; we found our driver.
			}
			return TRUE;		// Keep looking.
		};

		for (const char* name : kPreferredDrivers)
		{
			EnumContext context = { name, nullptr };
			if (acmDriverEnum(enumProc, reinterpret_cast<DWORD_PTR>(&context), 0) == MMSYSERR_NOERROR &&
				context.driverId != nullptr)
			{
				HACMDRIVER driver = nullptr;
				if (acmDriverOpen(&driver, context.driverId, 0) == MMSYSERR_NOERROR)
				{
					return driver;
				}
			}
		}

		return nullptr;
	}

	/*
	 * Returns the process-wide MP3 ACM decoder, opening it on first use.
	 * Function-local static initialization is thread-safe under C++11 and later.
	 */
	HACMDRIVER GetMp3AcmDriver()
	{
		static HACMDRIVER driver = OpenMp3AcmDriver();
		return driver;
	}
}

AudioDecoder::~AudioDecoder()
{
	Close();
}

/*
 * Parses the WAVE header from 'source' and sets up the decode path: a direct
 * passthrough for PCM, or an ACM conversion stream for MP3. Returns true on
 * success, leaving the decoder positioned at the start of the audio data.
 */
bool AudioDecoder::Open(StreamSource* source)
{
	Close();

	if (source == nullptr || !source->IsOpen())
	{
		return false;
	}

	m_source = source;

	if (!ParseWaveHeader())
	{
		GENERAL_ERROR("SoundStreamer: file is not a valid RIFF/WAVE container.");
		m_source = nullptr;
		return false;
	}

	const WORD formatTag = m_sourceFormat->wFormatTag;
	if (formatTag == WAVE_FORMAT_PCM || formatTag == WAVE_FORMAT_IEEE_FLOAT)
	{
		// Uncompressed: stream the data chunk straight to the device.
		m_pcmFormat = *m_sourceFormat;
		m_pcmFormat.cbSize = 0;
		m_needsDecode = false;
	}
	else
	{
		// Compressed (MP3): set up an ACM conversion stream to linear PCM.
		HACMDRIVER driver = GetMp3AcmDriver();

		ZeroMemory(&m_pcmFormat, sizeof(m_pcmFormat));
		m_pcmFormat.wFormatTag = WAVE_FORMAT_PCM;
		if (acmFormatSuggest(driver, m_sourceFormat, &m_pcmFormat, sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG) != MMSYSERR_NOERROR)
		{
			GENERAL_ERROR("SoundStreamer: no PCM format suggestion for the source audio.");
			m_source = nullptr;
			return false;
		}

		MMRESULT result = acmStreamOpen(&m_acmStream, driver, m_sourceFormat, &m_pcmFormat, nullptr, 0, 0, ACM_STREAMOPENF_NONREALTIME);
		if (result != MMSYSERR_NOERROR && driver != nullptr)
		{
			// The preferred driver could not service this format; let ACM choose.
			result = acmStreamOpen(&m_acmStream, nullptr, m_sourceFormat, &m_pcmFormat, nullptr, 0, 0, ACM_STREAMOPENF_NONREALTIME);
		}
		if (result != MMSYSERR_NOERROR)
		{
			GENERAL_ERROR("SoundStreamer: failed to open an ACM decode stream (missing MP3 codec?).");
			m_acmStream = nullptr;
			m_source = nullptr;
			return false;
		}

		// Size the destination buffer to the worst case for our source block.
		m_srcCapacity = kSourceBufferBytes;
		if (acmStreamSize(m_acmStream, m_srcCapacity, &m_pcmCapacity, ACM_STREAMSIZEF_SOURCE) != MMSYSERR_NOERROR || m_pcmCapacity == 0)
		{
			// Fall back to roughly one second of PCM.
			m_pcmCapacity = m_pcmFormat.nAvgBytesPerSec != 0 ? m_pcmFormat.nAvgBytesPerSec : (m_srcCapacity * 16);
		}

		m_srcBuffer = new BYTE[m_srcCapacity];
		m_pcmBuffer = new BYTE[m_pcmCapacity];

		ZeroMemory(&m_acmHeader, sizeof(m_acmHeader));
		m_acmHeader.cbStruct = sizeof(m_acmHeader);
		m_acmHeader.pbSrc = m_srcBuffer;
		m_acmHeader.cbSrcLength = m_srcCapacity;
		m_acmHeader.pbDst = m_pcmBuffer;
		m_acmHeader.cbDstLength = m_pcmCapacity;
		if (acmStreamPrepareHeader(m_acmStream, &m_acmHeader, 0) != MMSYSERR_NOERROR)
		{
			GENERAL_ERROR("SoundStreamer: failed to prepare the ACM decode header.");
			Close();
			return false;
		}
		m_headerPrepared = true;
		m_needsDecode = true;
	}

	SeekToStart();
	return true;
}

/*
 * Releases the ACM stream and buffers and resets all decode state.
 */
void AudioDecoder::Close()
{
	if (m_acmStream != nullptr)
	{
		if (m_headerPrepared)
		{
			acmStreamUnprepareHeader(m_acmStream, &m_acmHeader, 0);
			m_headerPrepared = false;
		}
		acmStreamClose(m_acmStream, 0);
		m_acmStream = nullptr;
	}

	delete[] m_srcBuffer;
	m_srcBuffer = nullptr;
	delete[] m_pcmBuffer;
	m_pcmBuffer = nullptr;

	m_source = nullptr;
	m_sourceFormat = nullptr;
	m_needsDecode = false;
	m_srcCapacity = m_srcLength = 0;
	m_pcmCapacity = m_pcmHead = m_pcmAvailable = 0;
	m_srcExhausted = false;
	m_restartConversion = true;
}

/*
 * Walks the RIFF/WAVE chunk list, capturing the "fmt " chunk and locating the
 * "data" chunk. Returns true once the data chunk's offset and size are known.
 */
bool AudioDecoder::ParseWaveHeader()
{
	// RIFF header: "RIFF" <fileSize> "WAVE".
	DWORD riffHeader[3] = {};
	if (!m_source->Seek(0) || m_source->Read(riffHeader, sizeof(riffHeader)) != sizeof(riffHeader))
	{
		return false;
	}
	if (riffHeader[0] != kFourCC_RIFF || riffHeader[2] != kFourCC_WAVE)
	{
		return false;
	}

	bool haveFormat = false;
	LONG position = static_cast<LONG>(sizeof(riffHeader));

	for (;;)
	{
		struct { DWORD id; DWORD size; } chunk = {};
		if (m_source->Read(&chunk, sizeof(chunk)) != sizeof(chunk))
		{
			break;
		}
		position += static_cast<LONG>(sizeof(chunk));

		const LONG bodyStart = position;
		const DWORD paddedSize = chunk.size + (chunk.size & 1);	// RIFF chunks are word-aligned.

		if (chunk.id == kFourCC_fmt)
		{
			const DWORD toRead = __min(chunk.size, static_cast<DWORD>(sizeof(m_sourceFormatStorage)));
			if (m_source->Read(m_sourceFormatStorage, toRead) != toRead)
			{
				return false;
			}
			m_sourceFormat = reinterpret_cast<WAVEFORMATEX*>(m_sourceFormatStorage);
			if (chunk.size <= 16)
			{
				// PCMWAVEFORMAT has no cbSize field on disk.
				m_sourceFormat->cbSize = 0;
			}
			haveFormat = true;

			position = bodyStart + static_cast<LONG>(paddedSize);
			if (!m_source->Seek(position))
			{
				break;
			}
		}
		else if (chunk.id == kFourCC_data)
		{
			// The format chunk always precedes the data chunk in a valid file.
			if (!haveFormat)
			{
				return false;
			}
			m_dataOffset = static_cast<DWORD>(bodyStart);
			m_dataSize = chunk.size;
			return true;
		}
		else
		{
			position = bodyStart + static_cast<LONG>(paddedSize);
			if (!m_source->Seek(position))
			{
				break;
			}
		}
	}

	return false;
}

/*
 * Reads more source bytes from the data chunk into m_srcBuffer, marking the
 * source exhausted once the data chunk has been fully consumed.
 */
void AudioDecoder::FillSourceBuffer()
{
	if (m_srcExhausted)
	{
		return;
	}

	const DWORD space = m_srcCapacity - m_srcLength;
	if (space == 0)
	{
		return;
	}

	const DWORD want = __min(space, m_dataRemaining);
	if (want == 0)
	{
		m_srcExhausted = true;
		return;
	}

	const DWORD got = m_source->Read(m_srcBuffer + m_srcLength, want);
	m_srcLength += got;
	m_dataRemaining -= got;
	if (got < want || m_dataRemaining == 0)
	{
		m_srcExhausted = true;
	}
}

/*
 * Runs one ACM conversion pass over the buffered source bytes, leaving the
 * decoded PCM in m_pcmBuffer and carrying any unconsumed source to the front of
 * m_srcBuffer. Returns false if the conversion fails.
 */
bool AudioDecoder::ConvertBlock()
{
	m_acmHeader.cbSrcLength = m_srcLength;
	m_acmHeader.cbSrcLengthUsed = 0;
	m_acmHeader.cbDstLengthUsed = 0;

	DWORD flags = 0;
	if (m_restartConversion)
	{
		flags |= ACM_STREAMCONVERTF_START;
		m_restartConversion = false;
	}
	// While more source is coming, only convert whole blocks and carry the
	// remainder. Once the data chunk is exhausted, flush the final partial block.
	if (!m_srcExhausted)
	{
		flags |= ACM_STREAMCONVERTF_BLOCKALIGN;
	}

	if (acmStreamConvert(m_acmStream, &m_acmHeader, flags) != MMSYSERR_NOERROR)
	{
		return false;
	}

	const DWORD srcUsed = m_acmHeader.cbSrcLengthUsed;
	m_pcmHead = 0;
	m_pcmAvailable = m_acmHeader.cbDstLengthUsed;

	// Carry any unconverted source bytes to the front of the buffer.
	if (srcUsed < m_srcLength)
	{
		memmove(m_srcBuffer, m_srcBuffer + srcUsed, m_srcLength - srcUsed);
	}
	m_srcLength -= srcUsed;

	return true;
}

/*
 * Delivers decoded PCM to the caller. PCM sources are copied straight from the
 * data chunk; MP3 sources are decoded block by block, draining leftover PCM from
 * the previous conversion first. Sets 'endOfStream' when no more audio remains.
 */
DWORD AudioDecoder::ReadPcm(BYTE* dst, DWORD dstBytes, bool& endOfStream)
{
	endOfStream = false;

	if (dst == nullptr || dstBytes == 0)
	{
		return 0;
	}

	// Uncompressed passthrough.
	if (!m_needsDecode)
	{
		if (m_dataRemaining == 0)
		{
			endOfStream = true;
			return 0;
		}

		const DWORD want = __min(dstBytes, m_dataRemaining);
		const DWORD got = m_source->Read(dst, want);
		m_dataRemaining -= got;
		if (got < want || m_dataRemaining == 0)
		{
			endOfStream = true;
		}
		return got;
	}

	// MP3 decode.
	DWORD produced = 0;
	while (produced < dstBytes)
	{
		// Deliver any PCM left over from the previous conversion first.
		if (m_pcmAvailable > 0)
		{
			const DWORD n = __min(dstBytes - produced, m_pcmAvailable);
			memcpy(dst + produced, m_pcmBuffer + m_pcmHead, n);
			m_pcmHead += n;
			m_pcmAvailable -= n;
			produced += n;
			continue;
		}

		if (m_srcExhausted && m_srcLength == 0)
		{
			endOfStream = true;
			break;
		}

		FillSourceBuffer();
		if (m_srcLength == 0)
		{
			endOfStream = true;
			break;
		}

		const DWORD srcBefore = m_srcLength;
		if (!ConvertBlock())
		{
			endOfStream = true;
			break;
		}

		// Guard against a stall: the converter produced nothing and consumed
		// nothing. If we cannot supply more source, give up; otherwise loop to top
		// up the buffer and try again.
		if (m_pcmAvailable == 0 && m_srcLength == srcBefore)
		{
			if (m_srcExhausted || m_srcLength == m_srcCapacity)
			{
				endOfStream = true;
				break;
			}
		}
	}

	return produced;
}

/*
 * Rewinds to the first byte of the audio data and clears the decode buffers so
 * playback can restart or loop.
 */
void AudioDecoder::SeekToStart()
{
	if (m_source != nullptr)
	{
		m_source->Seek(static_cast<LONG>(m_dataOffset));
	}

	m_dataRemaining = m_dataSize;
	m_srcLength = 0;
	m_srcExhausted = false;
	m_pcmHead = 0;
	m_pcmAvailable = 0;
	m_restartConversion = true;
}
