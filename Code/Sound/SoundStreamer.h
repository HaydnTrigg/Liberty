#pragma once

#ifndef __SOUNDSTREAMER_H__
#define __SOUNDSTREAMER_H__

#include "Streamer.h"
#include "AudioStream.h"

#include <ISoundCommon.h>
#include <vector>

/*
 * SoundStreamer.h
 *
 * The SoundStreamer DACOM component. It implements IStreamer2 on top of the
 * shared SoundCommon engine: every open stream owns a sound voice, and a single
 * background thread keeps each stream's voice fed with freshly decoded PCM.
 *
 * Lifetime:
 *   - The component is created by the DACOM factory (see SoundStreamer.cpp),
 *     which calls init() immediately after construction.
 *   - Init() performs the heavy one-time setup: it acquires the audio engine and
 *     starts the streaming thread.
 *   - Streams are created with Open() and torn down with CloseHandle(); the
 *     component owns every open stream until then.
 */

#define CLSID_SoundStreamer "SoundStreamer"

struct DACOM_NO_VTABLE SoundStreamer : IStreamer2, IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(SoundStreamer)
		DACOM_INTERFACE_ENTRY(IStreamer)
		DACOM_INTERFACE_ENTRY(IStreamer2)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_IStreamer, IStreamer)
		DACOM_INTERFACE_ENTRY2(IID_IStreamer2, IStreamer2)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent, IAggregateComponent)
		END_DACOM_MAP()

	SoundStreamer();
	~SoundStreamer();

	// *** IStreamer methods ***

	DACOM_DEFMETHOD_(BOOL32, Init) (STREAMERDESC* desc) override;
	DACOM_DEFMETHOD_(HSTREAM, Open) (const char* filename, IFileSystem* parent, DWORD flags = STRMFL_PLAY) override;
	DACOM_DEFMETHOD_(BOOL32, CloseHandle) (HSTREAM hStream) override;
	DACOM_DEFMETHOD_(BOOL32, Stop) (HSTREAM hStream) override;
	DACOM_DEFMETHOD_(BOOL32, Restart) (HSTREAM hStream) override;
	DACOM_DEFMETHOD_(BOOL32, SetVolume) (HSTREAM hStream, S32 volume) override;
	DACOM_DEFMETHOD_(BOOL32, GetVolume) (HSTREAM hStream, S32* volume) const override;
	DACOM_DEFMETHOD_(STATUS, GetStatus) (HSTREAM hStream) const override;

	// *** IStreamer2 methods ***

	DACOM_DEFMETHOD_(DWORD, GetSomethingA) () override;
	DACOM_DEFMETHOD_(DWORD, GetSomethingB) () override;
	DACOM_DEFMETHOD_(BOOL32, SetPan) (HSTREAM hStream, S32 pan) override;
	DACOM_DEFMETHOD_(BOOL32, GetPan) (HSTREAM hStream, S32* pan) const override;

	// *** IAggregateComponent methods ***

	DACOM_DEFMETHOD(Initialize) (void) override;

	/*
	 * Called by the DACOM factory after construction. Establishes the singleton
	 * instance pointer.
	 */
	GENRESULT init(AGGDESC* desc);

	/*
	 * Entry point for the background streaming thread. Not part of the public
	 * interface.
	 */
	int RunStreamingThread();

	/*
	 * Idempotent teardown: stops the thread, frees all streams and shuts the
	 * engine down. Safe to call more than once; used by both the destructor and
	 * the DLL detach handler.
	 */
	void Quiesce();

private:
	/*
	 * Returns true if 'stream' is a live stream owned by this component. Guards
	 * every public call against stale or foreign handles. The caller must hold
	 * m_lock.
	 */
	bool IsValidStream(HSTREAM stream) const;

	// Stops and joins the streaming thread, if running.
	void StopStreamingThread();

	ISoundCommon* m_common = nullptr;		// Shared audio engine (acquired via DACOM).
	std::vector<AudioStream*> m_streams;	// All currently open streams.
	mutable CRITICAL_SECTION m_lock;		// Serializes access to m_streams and the streams themselves.

	HANDLE m_thread = nullptr;				// Background streaming thread.
	HANDLE m_threadReady = nullptr;			// Signaled once the thread has started.
	volatile LONG m_stopRequested = 0;		// Set to ask the streaming thread to exit.

	float m_bufferSeconds = 0.0f;			// Length of each streamed buffer.
};

/*
 * Component registration hooks, invoked from the merged binary's single entry
 * point (DllMain.cpp). SOUND_DEC is the host module's export/import decoration.
 */
extern "C"
{
	SOUND_DEC void Register_SoundStreamer();
	SOUND_DEC void Shutdown_SoundStreamer();
}

#endif // __SOUNDSTREAMER_H__
