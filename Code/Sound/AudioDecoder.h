#pragma once

#ifndef __AUDIODECODER_H__
#define __AUDIODECODER_H__

#include <Windows.h>
#include <mmreg.h>
#include <msacm.h>

#include "StreamSource.h"

/*
 * AudioDecoder.h
 *
 * Parses a RIFF/WAVE container and produces a continuous stream of linear PCM
 * suitable for submission to an XAudio2 source voice.
 *
 * Two source encodings are supported:
 *   - Uncompressed PCM (WAVE_FORMAT_PCM) is passed straight through.
 *   - MPEG Layer-3 (WAVE_FORMAT_MPEGLAYER3) is decoded with the Audio
 *     Compression Manager, which on a standard Windows install resolves to the
 *     Fraunhofer IIS MP3 decoder.
 *
 * The decoder owns no playback state; it simply turns "the next region of the
 * data chunk" into PCM on demand. AudioStream drives it and handles looping by
 * calling SeekToStart().
 */
class AudioDecoder
{
public:
	AudioDecoder() = default;
	~AudioDecoder();

	AudioDecoder(const AudioDecoder&) = delete;
	AudioDecoder& operator=(const AudioDecoder&) = delete;

	/*
	 * Parses the WAVE header read from 'source' and prepares the decode path.
	 * 'source' must outlive the decoder and is not owned by it. Returns true on
	 * success, after which PcmFormat() describes the output samples.
	 */
	bool Open(StreamSource* source);

	// Releases the ACM stream and intermediate buffers.
	void Close();

	/*
	 * Fills up to 'dstBytes' bytes of 'dst' with decoded PCM. Returns the number
	 * of bytes written. Sets 'endOfStream' to true once the entire data chunk has
	 * been decoded and no further PCM is available.
	 */
	DWORD ReadPcm(BYTE* dst, DWORD dstBytes, bool& endOfStream);

	// Rewinds to the start of the audio data so playback can loop.
	void SeekToStart();

	// The format of the PCM produced by ReadPcm(). Valid after a successful Open().
	const WAVEFORMATEX& PcmFormat() const { return m_pcmFormat; }

private:
	// Reads more compressed/source bytes from the data chunk into m_srcBuffer.
	void FillSourceBuffer();

	// Runs one ACM conversion pass over the buffered source bytes.
	bool ConvertBlock();

	/*
	 * Reads the RIFF/WAVE chunk layout, capturing the format and locating the data
	 * chunk. Returns true on success.
	 */
	bool ParseWaveHeader();

	StreamSource* m_source = nullptr;	// Backing file (not owned).

	// WAVE layout.
	DWORD m_dataOffset = 0;				// Byte offset of the audio data chunk within the file.
	DWORD m_dataSize = 0;				// Size of the audio data chunk, in bytes.
	DWORD m_dataRemaining = 0;			// Unread bytes left in the data chunk.

	// Format description. m_sourceFormat aliases m_sourceFormatStorage so that
	// codec-specific extension fields (e.g. MPEGLAYER3WAVEFORMAT) are preserved.
	BYTE m_sourceFormatStorage[64] = {};
	WAVEFORMATEX* m_sourceFormat = nullptr;
	WAVEFORMATEX m_pcmFormat = {};
	bool m_needsDecode = false;			// True for MP3 (ACM); false for PCM passthrough.

	// ACM decode state (only used when m_needsDecode is true).
	HACMSTREAM m_acmStream = nullptr;
	ACMSTREAMHEADER m_acmHeader = {};
	bool m_headerPrepared = false;

	BYTE* m_srcBuffer = nullptr;		// Holds compressed bytes awaiting conversion.
	DWORD m_srcCapacity = 0;
	DWORD m_srcLength = 0;				// Valid bytes currently in m_srcBuffer.
	bool m_srcExhausted = false;		// True once the data chunk has been fully read.

	BYTE* m_pcmBuffer = nullptr;		// Receives PCM from the most recent conversion.
	DWORD m_pcmCapacity = 0;
	DWORD m_pcmHead = 0;				// Offset of the next undelivered PCM byte.
	DWORD m_pcmAvailable = 0;			// Undelivered PCM bytes in m_pcmBuffer.

	bool m_restartConversion = true;	// Emit ACM_STREAMCONVERTF_START on the next conversion.
};

#endif // __AUDIODECODER_H__
