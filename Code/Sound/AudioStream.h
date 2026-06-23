#pragma once

#ifndef __AUDIOSTREAM_H__
#define __AUDIOSTREAM_H__

#include "Streamer.h"
#include "StreamSource.h"
#include "AudioDecoder.h"

#include <ISoundCommon.h>

/*
 * AudioStream.h
 *
 * A single open stream. This is the concrete type referred to by the opaque
 * HSTREAM handle in the public interface.
 *
 * An AudioStream owns the file, its decoder, and one sound voice obtained from
 * the shared SoundCommon engine. The SoundStreamer's background thread calls
 * Pump() periodically to keep the voice supplied with freshly decoded PCM. A
 * small ring of buffers is recycled as the engine finishes playing them; because
 * buffers are consumed in FIFO order, the oldest buffer is always the next one
 * free to refill.
 *
 * All public methods are expected to be called while holding the SoundStreamer
 * critical section, so the stream needs no internal locking of its own.
 */
struct AudioStream
{
	explicit AudioStream(ISoundCommon* common);
	~AudioStream();

	AudioStream(const AudioStream&) = delete;
	AudioStream& operator=(const AudioStream&) = delete;

	/*
	 * Opens the file, prepares the decoder and creates the sound voice.
	 * 'bufferSeconds' is the length of each streamed buffer. When STRMFL_PLAY is
	 * set, playback starts immediately. Returns true on success.
	 */
	bool Initialize(const char* filename, IFileSystem* parent, DWORD flags, float bufferSeconds);

	// Releases the voice, decoder, file and buffers. Called from the destructor.
	void Shutdown();

	// Playback control.
	void Start();
	void Stop();
	void Restart();

	/*
	 * Tops up the voice's buffer queue with newly decoded audio. Called from the
	 * streaming thread.
	 */
	void Pump();

	// Volume/pan are expressed in hundredths of a decibel (DirectSound units).
	void SetVolume(S32 centibels);
	S32 GetVolume() const { return m_volumeCentibels; }
	void SetPan(S32 centibels);
	S32 GetPan() const { return m_panCentibels; }

	// Returns the current playback status.
	IStreamer::STATUS GetStatus() const;

private:
	// Applies the stored volume/pan values to the voice.
	void ApplyVolume();
	void ApplyPan();

	/*
	 * Fills 'want' bytes of 'dst' with PCM, transparently looping the source when
	 * STRMFL_LOOPING is set. Sets 'streamEof' when the (non-looping) stream has no
	 * more audio. Returns the number of bytes written.
	 */
	DWORD FillBuffer(BYTE* dst, DWORD want, bool& streamEof);

	static const int kBufferCount = 3;	// Number of buffers cycled through the voice.

	ISoundCommon* m_common = nullptr;	// Shared audio engine (not owned).
	StreamSource m_source;				// Backing file.
	AudioDecoder m_decoder;				// PCM/MP3 decoder over the file.
	HSOUNDVOICE m_voice = nullptr;
	WAVEFORMATEX m_format = {};			// PCM format delivered to the voice.

	BYTE* m_buffers[kBufferCount] = {};	// Recycled PCM buffers.
	DWORD m_bufferBytes = 0;			// Size of each buffer.
	int m_ringHead = 0;					// Index of the next buffer to fill and submit.

	bool m_looping = false;				// STRMFL_LOOPING was requested.
	bool m_started = false;				// Playback has been started at least once.
	bool m_stopped = false;				// Playback is currently stopped by the caller.
	bool m_decoderEof = false;			// The decoder has produced its final buffer.

	S32 m_volumeCentibels = 0;			// 0 = full volume.
	S32 m_panCentibels = 0;				// 0 = centered.
};

#endif // __AUDIOSTREAM_H__
