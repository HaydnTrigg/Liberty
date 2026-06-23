#pragma once

#ifndef __STREAMER_H__
#define __STREAMER_H__

#include <DACOM.h>

/*
 * Streamer.h
 *
 * Public interface for the SoundStreamer component. The SoundStreamer plays
 * large, streamed audio files (music, ambient beds and movie audio) that are too
 * big to load into memory in their entirety. Callers obtain an IStreamer2 pointer
 * through the DACOM component manager (class id "SoundStreamer"), call Init()
 * once, and then Open() each file they wish to play.
 *
 * The interface is intentionally identical to the original Digital Anvil
 * component so the rest of the engine (the SoundManager and the executable
 * itself) continues to bind to it unchanged. Only the implementation behind the
 * interface has been rewritten on top of XAudio2; see SoundStreamer.cpp.
 */

/*
 * Opaque handle to a single open stream. The concrete type (AudioStream) is an
 * implementation detail of the Sound DLL and is never dereferenced by callers.
 */
typedef struct AudioStream* HSTREAM;

/*
 * Initialization descriptor passed to IStreamer::Init(). The structure is
 * self-sizing so future extensions stay binary compatible: Init() validates
 * 'size' against sizeof(STREAMERDESC).
 */
struct STREAMERDESC
{
	U32 size;						// Size of this structure, in bytes.
	struct IDirectSound* lpDSound;	// Legacy DirectSound device (unused by the XAudio2 backend; retained for ABI).
	HWND hMainWindow;				// Main application window (unused by the XAudio2 backend).
	UINT uMsg;						// Notification message id (unused by the XAudio2 backend).
	SINGLE readBufferTime;			// File read-ahead, in seconds. 0 selects the default (4.0 is reasonable).
	SINGLE soundBufferTime;			// Length of each streamed playback buffer, in seconds. 0 selects the default (0.25 is reasonable).

	STREAMERDESC(void) :
		size(sizeof(*this)),
		lpDSound(nullptr),
		hMainWindow(nullptr),
		uMsg(0),
		readBufferTime(0.0f),
		soundBufferTime(0.0f)
	{}
};

// Flags accepted by IStreamer::Open().
#define STRMFL_PLAY			0x00000001			// Begin playback immediately once the stream is opened.
#define STRMFL_LOOPING		0x00000002			// Loop the entire stream continuously until closed.

#define IID_IStreamer DACOM_MAKE_IID("IStreamer")
#define IID_IStreamer2 DACOM_MAKE_IID("IStreamer2")

/*
 * IStreamer
 *
 * The original streaming playback interface. The method order and signatures
 * define the DACOM vtable and must not be reordered or changed.
 */
struct DACOM_NO_VTABLE IStreamer : IDAComponent
{
	// Playback status of a single stream, returned by GetStatus().
	enum STATUS
	{
		INVALID = 0,		// The handle is not a valid, open stream.
		PLAYING,			// Audio is currently being delivered to the device.
		STOPPED,			// Playback has been stopped by the caller.
		EOFREACHED,			// The decoder reached end-of-file; queued audio is still draining.
		COMPLETED,			// Playback finished and all queued audio has been consumed.
		INITSUCCESS			// The stream opened successfully but has not started playing.
	};

	/*
	 * Performs one-time initialization of the streamer: spins up the audio engine
	 * and the background streaming thread. Returns TRUE on success.
	 */
	DACOM_DEFMETHOD_(BOOL32, Init) (STREAMERDESC* desc) = 0;

	/*
	 * Opens 'filename' (a RIFF/WAVE file containing PCM or MP3 data) for
	 * streaming. 'parent', when non-null, is the file system the file is read
	 * through; otherwise the default file system is used. Returns a stream handle,
	 * or null on failure.
	 */
	DACOM_DEFMETHOD_(HSTREAM, Open) (const char* filename, struct IFileSystem* parent, DWORD flags = STRMFL_PLAY) = 0;

	/*
	 * Stops playback and releases all resources owned by the stream. The handle is
	 * invalid once this returns.
	 */
	DACOM_DEFMETHOD_(BOOL32, CloseHandle) (HSTREAM hStream) = 0;

	// Stops playback of the stream without closing it.
	DACOM_DEFMETHOD_(BOOL32, Stop) (HSTREAM hStream) = 0;

	// Restarts playback of the stream from the beginning.
	DACOM_DEFMETHOD_(BOOL32, Restart) (HSTREAM hStream) = 0;

	/*
	 * Sets/gets the stream volume, in hundredths of a decibel of attenuation
	 * (DirectSound convention: 0 is full volume, -10000 is silence).
	 */
	DACOM_DEFMETHOD_(BOOL32, SetVolume) (HSTREAM hStream, S32 volume) = 0;
	DACOM_DEFMETHOD_(BOOL32, GetVolume) (HSTREAM hStream, S32* volume) const = 0;

	// Returns the current playback status of the stream.
	DACOM_DEFMETHOD_(STATUS, GetStatus) (HSTREAM hStream) const = 0;
};

/*
 * IStreamer2
 *
 * Extends IStreamer with panning and a pair of legacy query accessors. The
 * method order and signatures define the DACOM vtable and must not be changed.
 */
struct DACOM_NO_VTABLE IStreamer2 : IStreamer
{
	// Legacy diagnostic accessors retained for ABI compatibility.
	DACOM_DEFMETHOD_(DWORD, GetSomethingA) () = 0;
	DACOM_DEFMETHOD_(DWORD, GetSomethingB) () = 0;

	/*
	 * Sets/gets the stream stereo pan, in hundredths of a decibel (DirectSound
	 * convention: 0 is centered, -10000 is full left, 10000 is full right).
	 */
	DACOM_DEFMETHOD_(BOOL32, SetPan) (HSTREAM hStream, S32 pan) = 0;
	DACOM_DEFMETHOD_(BOOL32, GetPan) (HSTREAM hStream, S32* pan) const = 0;
};

#endif // __STREAMER_H__
