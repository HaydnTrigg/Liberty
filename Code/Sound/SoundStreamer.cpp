#include "SoundStreamer.h"

#include <DACOM.h>
#include <algorithm>

/*
 * SoundStreamer.cpp
 *
 * Implementation of the SoundStreamer DACOM component on top of the shared
 * SoundCommon engine. See SoundStreamer.h and Streamer.h for the interface and
 * design notes.
 */

// The component is a process-wide singleton, mirroring the original Digital
// Anvil implementation. init() publishes the instance here so the DLL's detach
// handler can shut it down even if the host never released it.
static SoundStreamer* g_soundStreamer = nullptr;

// Default length of each streamed buffer when the caller does not specify one.
// kBufferCount of these are queued per stream, giving a comfortable margin
// against the streaming thread's polling interval.
static const float kDefaultBufferSeconds = 0.5f;

//--------------------------------------------------------------------------//
// Background streaming thread
//--------------------------------------------------------------------------//

/*
 * Win32 thread entry thunk: forwards to the SoundStreamer's RunStreamingThread().
 */
static DWORD WINAPI StreamingThreadThunk(LPVOID parameter)
{
	return static_cast<DWORD>(reinterpret_cast<SoundStreamer*>(parameter)->RunStreamingThread());
}

/*
 * Background thread loop. Periodically pumps every open stream so its voice never
 * runs dry, until a stop is requested. Returns 0 on exit.
 */
int SoundStreamer::RunStreamingThread()
{
	// Let Init() know the thread is up and running.
	SetEvent(m_threadReady);

	while (m_stopRequested == 0)
	{
		EnterCriticalSection(&m_lock);
		for (AudioStream* stream : m_streams)
		{
			stream->Pump();
		}
		LeaveCriticalSection(&m_lock);

		Sleep(20);
	}

	return 0;
}

//--------------------------------------------------------------------------//
// Construction / teardown
//--------------------------------------------------------------------------//

SoundStreamer::SoundStreamer()
{
	InitializeCriticalSection(&m_lock);
}

SoundStreamer::~SoundStreamer()
{
	Quiesce();
	DeleteCriticalSection(&m_lock);

	if (g_soundStreamer == this)
	{
		g_soundStreamer = nullptr;
	}
}

/*
 * Stops and joins the streaming thread (waiting up to two seconds) and closes its
 * handles. Safe to call when no thread is running.
 */
void SoundStreamer::StopStreamingThread()
{
	if (m_thread != nullptr)
	{
		InterlockedExchange(&m_stopRequested, 1);
		WaitForSingleObject(m_thread, 2000);
		::CloseHandle(m_thread);
		m_thread = nullptr;
	}

	if (m_threadReady != nullptr)
	{
		::CloseHandle(m_threadReady);
		m_threadReady = nullptr;
	}
}

/*
 * Tears the component down: stops the thread, deletes every open stream and
 * releases the shared engine. Idempotent.
 */
void SoundStreamer::Quiesce()
{
	StopStreamingThread();

	// The thread is gone, so the stream list can be torn down without racing it.
	EnterCriticalSection(&m_lock);
	for (AudioStream* stream : m_streams)
	{
		delete stream;
	}
	m_streams.clear();
	LeaveCriticalSection(&m_lock);

	if (m_common != nullptr)
	{
		m_common->Shutdown();
		m_common->Release();
		m_common = nullptr;
	}
}

//--------------------------------------------------------------------------//
// DACOM lifecycle
//--------------------------------------------------------------------------//

/*
 * DACOM factory hook, called immediately after construction. Records the
 * singleton instance pointer; fails if a streamer already exists.
 */
GENRESULT SoundStreamer::init(AGGDESC* /*desc*/)
{
	if (g_soundStreamer != nullptr)
	{
		return GR_GENERIC;	// Only one streamer instance is supported.
	}

	g_soundStreamer = this;
	Initialize();
	return GR_OK;
}

/*
 * IAggregateComponent initialization hook. The audio engine and streaming thread
 * are created later, in Init(), once the streaming parameters are known.
 */
GENRESULT SoundStreamer::Initialize(void)
{
	return GR_OK;
}

/*
 * Performs the one-time setup the host requests through IStreamer: validates the
 * descriptor, acquires and starts the shared engine, and launches the background
 * streaming thread. Returns FALSE (cleaning up any partial state) on failure.
 */
BOOL32 SoundStreamer::Init(STREAMERDESC* desc)
{
	if (desc == nullptr || desc->size != sizeof(*desc))
	{
		GENERAL_ERROR("SoundStreamer: invalid STREAMERDESC.");
		return FALSE;
	}

	// lpDSound, hMainWindow and uMsg are part of the legacy DirectSound contract
	// and are intentionally ignored by the XAudio2 backend.
	m_bufferSeconds = desc->soundBufferTime > 0.0f ? desc->soundBufferTime : kDefaultBufferSeconds;

	// Acquire the shared audio engine through DACOM and start it.
	AGGDESC commonDesc(CLSID_SoundCommon);
	ICOManager* manager = DACOM_Acquire();
	if (manager == nullptr || manager->CreateInstance(&commonDesc, reinterpret_cast<void**>(&m_common)) != GR_OK || m_common == nullptr)
	{
		m_common = nullptr;
		GENERAL_ERROR("SoundStreamer: failed to acquire the SoundCommon engine.");
		return FALSE;
	}
	if (!m_common->Startup())
	{
		m_common->Release();
		m_common = nullptr;
		return FALSE;
	}

	m_stopRequested = 0;
	m_threadReady = CreateEventA(nullptr, TRUE, FALSE, nullptr);
	if (m_threadReady == nullptr)
	{
		m_common->Shutdown();
		m_common->Release();
		m_common = nullptr;
		return FALSE;
	}

	m_thread = CreateThread(nullptr, 0, &StreamingThreadThunk, this, 0, nullptr);
	if (m_thread == nullptr)
	{
		::CloseHandle(m_threadReady);
		m_threadReady = nullptr;
		m_common->Shutdown();
		m_common->Release();
		m_common = nullptr;
		return FALSE;
	}

	// Wait for the thread to come up before returning.
	WaitForSingleObject(m_threadReady, 5000);
	return TRUE;
}

//--------------------------------------------------------------------------//
// Stream management
//--------------------------------------------------------------------------//

/*
 * Returns true if 'stream' is currently one of this component's open streams.
 * The caller must hold m_lock.
 */
bool SoundStreamer::IsValidStream(HSTREAM stream) const
{
	if (stream == nullptr)
	{
		return false;
	}
	return std::find(m_streams.begin(), m_streams.end(), stream) != m_streams.end();
}

/*
 * Opens a file for streaming and adds it to the open-stream list. Returns the new
 * stream handle, or null on failure.
 */
HSTREAM SoundStreamer::Open(const char* filename, IFileSystem* parent, DWORD flags)
{
	if (m_common == nullptr || filename == nullptr)
	{
		return nullptr;
	}

	AudioStream* stream = new AudioStream(m_common);
	if (!stream->Initialize(filename, parent, flags, m_bufferSeconds))
	{
		delete stream;
		return nullptr;
	}

	EnterCriticalSection(&m_lock);
	m_streams.push_back(stream);
	LeaveCriticalSection(&m_lock);

	return stream;
}

/*
 * Closes a stream and frees its resources. Returns FALSE if the handle is not an
 * open stream.
 */
BOOL32 SoundStreamer::CloseHandle(HSTREAM hStream)
{
	EnterCriticalSection(&m_lock);
	auto it = std::find(m_streams.begin(), m_streams.end(), hStream);
	const bool found = it != m_streams.end();
	if (found)
	{
		m_streams.erase(it);
	}
	LeaveCriticalSection(&m_lock);

	if (!found)
	{
		return FALSE;
	}

	// Destroy outside the lock: DestroyVoice() can block, and the stream is no
	// longer reachable by the streaming thread.
	delete hStream;
	return TRUE;
}

/*
 * Stops playback of a stream without closing it.
 */
BOOL32 SoundStreamer::Stop(HSTREAM hStream)
{
	BOOL32 result = FALSE;
	EnterCriticalSection(&m_lock);
	if (IsValidStream(hStream))
	{
		hStream->Stop();
		result = TRUE;
	}
	LeaveCriticalSection(&m_lock);
	return result;
}

/*
 * Restarts a stream from the beginning.
 */
BOOL32 SoundStreamer::Restart(HSTREAM hStream)
{
	BOOL32 result = FALSE;
	EnterCriticalSection(&m_lock);
	if (IsValidStream(hStream))
	{
		hStream->Restart();
		result = TRUE;
	}
	LeaveCriticalSection(&m_lock);
	return result;
}

/*
 * Sets a stream's volume (DirectSound centibels).
 */
BOOL32 SoundStreamer::SetVolume(HSTREAM hStream, S32 volume)
{
	BOOL32 result = FALSE;
	EnterCriticalSection(&m_lock);
	if (IsValidStream(hStream))
	{
		hStream->SetVolume(volume);
		result = TRUE;
	}
	LeaveCriticalSection(&m_lock);
	return result;
}

/*
 * Retrieves a stream's volume. Returns FALSE on a null pointer or invalid handle.
 */
BOOL32 SoundStreamer::GetVolume(HSTREAM hStream, S32* volume) const
{
	BOOL32 result = FALSE;
	EnterCriticalSection(&m_lock);
	if (volume != nullptr && IsValidStream(hStream))
	{
		*volume = hStream->GetVolume();
		result = TRUE;
	}
	LeaveCriticalSection(&m_lock);
	return result;
}

/*
 * Returns a stream's current playback status, or INVALID for an unknown handle.
 */
SoundStreamer::STATUS SoundStreamer::GetStatus(HSTREAM hStream) const
{
	STATUS status = INVALID;
	EnterCriticalSection(&m_lock);
	if (IsValidStream(hStream))
	{
		status = hStream->GetStatus();
	}
	LeaveCriticalSection(&m_lock);
	return status;
}

/*
 * Sets a stream's stereo pan (DirectSound centibels).
 */
BOOL32 SoundStreamer::SetPan(HSTREAM hStream, S32 pan)
{
	BOOL32 result = FALSE;
	EnterCriticalSection(&m_lock);
	if (IsValidStream(hStream))
	{
		hStream->SetPan(pan);
		result = TRUE;
	}
	LeaveCriticalSection(&m_lock);
	return result;
}

/*
 * Retrieves a stream's stereo pan. Returns FALSE on a null pointer or invalid
 * handle.
 */
BOOL32 SoundStreamer::GetPan(HSTREAM hStream, S32* pan) const
{
	BOOL32 result = FALSE;
	EnterCriticalSection(&m_lock);
	if (pan != nullptr && IsValidStream(hStream))
	{
		*pan = hStream->GetPan();
		result = TRUE;
	}
	LeaveCriticalSection(&m_lock);
	return result;
}

/*
 * Legacy diagnostic accessor; no meaningful value under the XAudio2 backend.
 */
DWORD SoundStreamer::GetSomethingA()
{
	return 0;
}

/*
 * Legacy diagnostic accessor; no meaningful value under the XAudio2 backend.
 */
DWORD SoundStreamer::GetSomethingB()
{
	return 0;
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_soundStreamerFactory = nullptr;

extern "C"
{
	/*
	 * Registers the SoundStreamer component factory with the component manager.
	 */
	void Register_SoundStreamer()
	{
		g_soundStreamerFactory = RegisterComponentFactory<DAComponentAggregate<SoundStreamer>>(DACOM_LIBRARY_NAME, CLSID_SoundStreamer, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the SoundStreamer component factory.
	 */
	void Shutdown_SoundStreamer()
	{
		if (g_soundStreamerFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_soundStreamerFactory, CLSID_SoundStreamer);
			g_soundStreamerFactory = nullptr;
		}
	}
}
