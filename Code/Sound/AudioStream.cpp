#include "AudioStream.h"

/*
 * AudioStream.cpp
 *
 * Implementation of a single streamed sound. See AudioStream.h for the design.
 * All audio backend work is delegated to the shared SoundCommon engine through
 * the ISoundCommon interface; this file owns only the decode/stream logic.
 */

AudioStream::AudioStream(ISoundCommon* common) :
	m_common(common)
{
}

AudioStream::~AudioStream()
{
	Shutdown();
}

/*
 * Opens the source file, prepares the decoder, creates a voice for the decoded
 * PCM format and allocates the ring of streaming buffers. Starts playback if
 * STRMFL_PLAY was requested. Returns true on success.
 */
bool AudioStream::Initialize(const char* filename, IFileSystem* parent, DWORD flags, float bufferSeconds)
{
	if (m_common == nullptr || filename == nullptr)
	{
		return false;
	}

	m_looping = (flags & STRMFL_LOOPING) != 0;

	if (!m_source.Open(filename, parent))
	{
		return false;
	}
	if (!m_decoder.Open(&m_source))
	{
		return false;
	}

	m_format = m_decoder.PcmFormat();

	m_voice = m_common->CreateVoice(&m_format);
	if (m_voice == nullptr)
	{
		return false;
	}

	// Size each buffer to 'bufferSeconds' of audio, rounded down to a whole number
	// of sample frames.
	DWORD bytesPerSecond = m_format.nAvgBytesPerSec;
	if (bytesPerSecond == 0)
	{
		bytesPerSecond = m_format.nSamplesPerSec * m_format.nBlockAlign;
	}

	DWORD bufferBytes = static_cast<DWORD>(static_cast<float>(bytesPerSecond) * bufferSeconds);
	if (m_format.nBlockAlign != 0)
	{
		bufferBytes -= bufferBytes % m_format.nBlockAlign;
	}
	if (bufferBytes == 0)
	{
		bufferBytes = bytesPerSecond / 4;
		if (bufferBytes == 0)
		{
			bufferBytes = 4096;
		}
	}
	m_bufferBytes = bufferBytes;

	for (int i = 0; i < kBufferCount; ++i)
	{
		m_buffers[i] = new BYTE[m_bufferBytes];
	}

	ApplyVolume();
	ApplyPan();

	if (flags & STRMFL_PLAY)
	{
		Start();
	}

	return true;
}

/*
 * Destroys the voice and releases the decoder, file and buffers.
 */
void AudioStream::Shutdown()
{
	if (m_voice != nullptr && m_common != nullptr)
	{
		m_common->DestroyVoice(m_voice);
		m_voice = nullptr;
	}

	m_decoder.Close();
	m_source.Close();

	for (int i = 0; i < kBufferCount; ++i)
	{
		delete[] m_buffers[i];
		m_buffers[i] = nullptr;
	}
}

/*
 * Starts (or resumes) playback and primes the buffer queue immediately.
 */
void AudioStream::Start()
{
	if (m_voice == nullptr)
	{
		return;
	}

	m_stopped = false;
	m_started = true;
	m_common->Start(m_voice);
	Pump();		// Prime the buffer queue right away.
}

/*
 * Pauses playback, leaving the queued buffers in place.
 */
void AudioStream::Stop()
{
	if (m_voice == nullptr)
	{
		return;
	}

	m_common->Stop(m_voice);
	m_stopped = true;
}

/*
 * Rewinds the decoder and restarts playback from the beginning of the file.
 */
void AudioStream::Restart()
{
	if (m_voice == nullptr)
	{
		return;
	}

	m_common->Stop(m_voice);
	m_common->Flush(m_voice);

	m_decoder.SeekToStart();
	m_decoderEof = false;
	m_ringHead = 0;
	m_stopped = false;
	m_started = true;

	m_common->Start(m_voice);
	Pump();
}

/*
 * Keeps the voice's queue topped up: while it holds fewer than kBufferCount
 * buffers, decode the next chunk into the oldest free buffer and submit it. Stops
 * when the decoder reaches end-of-stream.
 */
void AudioStream::Pump()
{
	if (m_voice == nullptr || !m_started || m_stopped || m_decoderEof)
	{
		return;
	}

	int queued = static_cast<int>(m_common->GetBuffersQueued(m_voice));
	while (queued < kBufferCount && !m_decoderEof)
	{
		BYTE* buffer = m_buffers[m_ringHead];

		bool streamEof = false;
		const DWORD filled = FillBuffer(buffer, m_bufferBytes, streamEof);
		if (filled == 0)
		{
			if (streamEof)
			{
				m_decoderEof = true;
			}
			break;
		}

		SOUND_BUFFER soundBuffer = {};
		soundBuffer.data = buffer;
		soundBuffer.byteCount = filled;
		soundBuffer.endOfStream = streamEof ? TRUE : FALSE;
		if (streamEof)
		{
			m_decoderEof = true;
		}

		if (!m_common->SubmitBuffer(m_voice, &soundBuffer))
		{
			break;
		}

		m_ringHead = (m_ringHead + 1) % kBufferCount;
		++queued;
	}
}

/*
 * Decodes up to 'want' bytes into 'dst'. For a looping stream the source is
 * rewound and reading continues; for a non-looping stream 'streamEof' is set once
 * the source is exhausted. Returns the number of bytes written.
 */
DWORD AudioStream::FillBuffer(BYTE* dst, DWORD want, bool& streamEof)
{
	streamEof = false;

	DWORD total = 0;
	while (total < want)
	{
		bool eof = false;
		const DWORD got = m_decoder.ReadPcm(dst + total, want - total, eof);
		total += got;

		if (!eof)
		{
			if (got == 0)
			{
				break;		// No progress without EOF; stop to avoid spinning.
			}
			continue;
		}

		if (!m_looping)
		{
			streamEof = true;
			break;
		}

		// Looping: rewind and keep filling. Guard against a zero-length source.
		if (got == 0 && total == 0)
		{
			streamEof = true;
			break;
		}
		m_decoder.SeekToStart();
	}

	return total;
}

/*
 * Stores and applies a new volume.
 */
void AudioStream::SetVolume(S32 centibels)
{
	m_volumeCentibels = centibels;
	ApplyVolume();
}

/*
 * Stores and applies a new pan.
 */
void AudioStream::SetPan(S32 centibels)
{
	m_panCentibels = centibels;
	ApplyPan();
}

/*
 * Pushes the stored volume to the voice.
 */
void AudioStream::ApplyVolume()
{
	if (m_voice != nullptr)
	{
		m_common->SetVolume(m_voice, m_volumeCentibels);
	}
}

/*
 * Pushes the stored pan to the voice. Music is not spatialized: pan only, with no
 * reverb send.
 */
void AudioStream::ApplyPan()
{
	if (m_voice != nullptr)
	{
		m_common->SetPan(m_voice, m_panCentibels, m_format.nChannels, 0.0f);
	}
}

/*
 * Maps the stream's internal state and queue depth onto an IStreamer::STATUS.
 */
IStreamer::STATUS AudioStream::GetStatus() const
{
	if (m_voice == nullptr)
	{
		return IStreamer::INVALID;
	}
	if (m_stopped)
	{
		return IStreamer::STOPPED;
	}
	if (!m_started)
	{
		return IStreamer::INITSUCCESS;
	}

	const U32 queued = m_common->GetBuffersQueued(m_voice);
	if (queued > 0)
	{
		return m_decoderEof ? IStreamer::EOFREACHED : IStreamer::PLAYING;
	}

	if (m_decoderEof)
	{
		return IStreamer::COMPLETED;
	}

	// A transient underrun; the streaming thread will refill shortly.
	return IStreamer::PLAYING;
}
