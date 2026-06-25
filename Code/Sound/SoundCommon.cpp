#include "SoundCommon.h"

#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <math.h>

#include <DACOM.h>		// GENERAL_ERROR / diagnostics

/*
 * SoundCommon.cpp
 *
 * The XAudio2/X3DAudio backend behind ISoundCommon. Everything XAudio2-specific
 * lives here; the interface and its consumers never see these headers.
 *
 * The engine is a process-wide singleton shared by every SoundCommon instance.
 * Startup() and Shutdown() reference-count it, so the device is created once (by
 * the first consumer to start) and destroyed once (when the last consumer stops).
 */

namespace
{
	/*
	 * Clamps 'value' to the inclusive range [lo, hi].
	 */
	float ClampF(float value, float lo, float hi)
	{
		return value < lo ? lo : (value > hi ? hi : value);
	}

	/*
	 * Converts a DirectSound gain (hundredths of a decibel of attenuation) to the
	 * linear amplitude multiplier XAudio2 expects. -10000 (-100 dB) is treated as
	 * silence and 0 dB as full volume.
	 */
	float CentibelsToAmplitude(S32 centibels)
	{
		if (centibels <= -10000)
		{
			return 0.0f;
		}
		if (centibels >= 0)
		{
			return 1.0f;
		}
		return powf(10.0f, static_cast<float>(centibels) / 2000.0f);
	}

	/*
	 * Converts a backend-neutral SOUND_VECTOR to an X3DAudio vector.
	 */
	X3DAUDIO_VECTOR ToX3D(const SOUND_VECTOR& v)
	{
		X3DAUDIO_VECTOR out;
		out.x = v.x;
		out.y = v.y;
		out.z = v.z;
		return out;
	}
}

/*
 * A single sound voice. Each voice owns its own scratch matrix so that pan/3D
 * solutions on different voices can run concurrently (the SoundManager updates
 * from the game thread while the SoundStreamer pumps from its own thread).
 */
struct SoundVoice
{
	IXAudio2SourceVoice* voice = nullptr;
	float matrix[2 * 8] = {};
};

namespace
{
	/*
	 * AudioEngine
	 *
	 * The shared XAudio2 graph: the engine object, mastering voice, a mono reverb
	 * submix and an X3DAudio instance. A single instance (g_engine) is shared by
	 * every SoundCommon component and guarded by g_lock.
	 */
	struct AudioEngine
	{
		IXAudio2* xaudio2 = nullptr;
		IXAudio2MasteringVoice* masteringVoice = nullptr;
		IXAudio2SubmixVoice* reverbVoice = nullptr;	// Mono reverb return.
		X3DAUDIO_HANDLE x3dInstance = {};
		bool x3dReady = false;
		UINT32 outputChannels = 0;
		UINT32 channelMask = 0;
		LONG refCount = 0;

		bool Startup();
		void Shutdown();
		IXAudio2SourceVoice* CreateSourceVoice(const WAVEFORMATEX& format);
		void SetReverb(U32 environment, SINGLE volume, SINGLE decayScale, SINGLE damping);
	};

	AudioEngine g_engine;
	CRITICAL_SECTION g_lock;		// Guards Startup()/Shutdown() reference counting.

	/*
	 * Creates the XAudio2 device, mastering voice, X3DAudio instance and reverb
	 * submix. Returns true once the graph is ready (or already running); on
	 * failure it tears down any partial state and returns false.
	 */
	bool AudioEngine::Startup()
	{
		if (xaudio2 != nullptr)
		{
			return true;
		}

		if (FAILED(XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)) || xaudio2 == nullptr)
		{
			xaudio2 = nullptr;
			GENERAL_ERROR("SoundCommon: XAudio2Create failed.");
			return false;
		}

		if (FAILED(xaudio2->CreateMasteringVoice(&masteringVoice)) || masteringVoice == nullptr)
		{
			GENERAL_ERROR("SoundCommon: CreateMasteringVoice failed.");
			Shutdown();
			return false;
		}

		XAUDIO2_VOICE_DETAILS masterDetails = {};
		masteringVoice->GetVoiceDetails(&masterDetails);
		outputChannels = masterDetails.InputChannels;

		DWORD mask = 0;
		masteringVoice->GetChannelMask(&mask);
		channelMask = mask != 0 ? mask : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);

		if (FAILED(X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3dInstance)))
		{
			GENERAL_ERROR("SoundCommon: X3DAudioInitialize failed.");
			Shutdown();
			return false;
		}
		x3dReady = true;

		// Create a mono reverb return; it pairs naturally with X3DAudio's single
		// reverb send. Reverb is optional: if it cannot be created, voices simply
		// run dry.
		IUnknown* reverbEffect = nullptr;
		if (SUCCEEDED(XAudio2CreateReverb(&reverbEffect)))
		{
			XAUDIO2_EFFECT_DESCRIPTOR effectDescriptor = { reverbEffect, TRUE, 1 };
			XAUDIO2_EFFECT_CHAIN effectChain = { 1, &effectDescriptor };
			xaudio2->CreateSubmixVoice(&reverbVoice, 1, masterDetails.InputSampleRate, 0, 0, nullptr, &effectChain);
			reverbEffect->Release();

			if (reverbVoice != nullptr)
			{
				SetReverb(0, 0.0f, 1.0f, 1.0f);
			}
		}

		return true;
	}

	/*
	 * Destroys the reverb submix, mastering voice and XAudio2 device, and resets
	 * the cached device state. Safe to call on a partially constructed engine.
	 */
	void AudioEngine::Shutdown()
	{
		if (reverbVoice != nullptr)
		{
			reverbVoice->DestroyVoice();
			reverbVoice = nullptr;
		}
		if (masteringVoice != nullptr)
		{
			masteringVoice->DestroyVoice();
			masteringVoice = nullptr;
		}
		if (xaudio2 != nullptr)
		{
			xaudio2->Release();
			xaudio2 = nullptr;
		}

		x3dReady = false;
		outputChannels = 0;
		channelMask = 0;
	}

	/*
	 * Creates an XAudio2 source voice for the given PCM format, routed to both the
	 * dry (mastering) and wet (reverb) paths. Returns null on failure.
	 */
	IXAudio2SourceVoice* AudioEngine::CreateSourceVoice(const WAVEFORMATEX& format)
	{
		if (xaudio2 == nullptr)
		{
			return nullptr;
		}

		XAUDIO2_SEND_DESCRIPTOR sends[2];
		UINT32 sendCount = 0;
		sends[sendCount++] = { 0, masteringVoice };
		if (reverbVoice != nullptr)
		{
			sends[sendCount++] = { 0, reverbVoice };
		}
		XAUDIO2_VOICE_SENDS sendList = { sendCount, sends };

		IXAudio2SourceVoice* voice = nullptr;
		// Allow pitch head-room above 1.0 for Doppler on top of any app pitch.
		if (FAILED(xaudio2->CreateSourceVoice(&voice, &format, 0, 4.0f, nullptr, &sendList, nullptr)))
		{
			return nullptr;
		}
		return voice;
	}

	/*
	 * Configures the reverb submix from the engine-neutral reverb parameters.
	 * 'environment' selects a room size, 'volume' the reverb return level,
	 * 'decayScale' scales the decay time and 'damping' the high-frequency
	 * absorption. Does nothing if reverb is unavailable.
	 */
	void AudioEngine::SetReverb(U32 environment, SINGLE volume, SINGLE decayScale, SINGLE damping)
	{
		if (reverbVoice == nullptr)
		{
			return;
		}

		XAUDIO2FX_REVERB_PARAMETERS params = {};
		params.WetDryMix = 100.0f;		// The submix output is the wet signal.
		params.ReflectionsDelay = XAUDIO2FX_REVERB_DEFAULT_REFLECTIONS_DELAY;
		params.ReverbDelay = XAUDIO2FX_REVERB_DEFAULT_REVERB_DELAY;
		params.RearDelay = XAUDIO2FX_REVERB_DEFAULT_REAR_DELAY;
		params.PositionLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION;
		params.PositionRight = XAUDIO2FX_REVERB_DEFAULT_POSITION;
		params.PositionMatrixLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
		params.PositionMatrixRight = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
		params.EarlyDiffusion = XAUDIO2FX_REVERB_DEFAULT_EARLY_DIFFUSION;
		params.LateDiffusion = XAUDIO2FX_REVERB_DEFAULT_LATE_DIFFUSION;
		params.LowEQGain = XAUDIO2FX_REVERB_DEFAULT_LOW_EQ_GAIN;
		params.LowEQCutoff = XAUDIO2FX_REVERB_DEFAULT_LOW_EQ_CUTOFF;
		params.HighEQCutoff = XAUDIO2FX_REVERB_DEFAULT_HIGH_EQ_CUTOFF;
		params.RoomFilterFreq = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_FREQ;
		params.RoomFilterMain = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_MAIN;
		params.RoomFilterHF = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_HF;
		params.ReflectionsGain = XAUDIO2FX_REVERB_DEFAULT_REFLECTIONS_GAIN;
		params.ReverbGain = XAUDIO2FX_REVERB_DEFAULT_REVERB_GAIN;
		params.Density = XAUDIO2FX_REVERB_DEFAULT_DENSITY;

		// Larger environments map to larger rooms; the 26 EAX environments span the
		// room-size range.
		const float environmentFraction = ClampF(static_cast<float>(environment) / 25.0f, 0.0f, 1.0f);
		params.RoomSize = ClampF(20.0f + environmentFraction * 80.0f, 1.0f, 100.0f);

		const float decay = decayScale > 0.0f ? decayScale : 1.0f;
		params.DecayTime = ClampF(XAUDIO2FX_REVERB_DEFAULT_DECAY_TIME * decay, 0.1f, 20.0f);

		// Higher damping -> more high-frequency absorption -> lower high EQ gain.
		const int highGain = static_cast<int>(8.0f - damping * 2.0f + 0.5f);
		params.HighEQGain = static_cast<BYTE>(highGain < 0 ? 0 : (highGain > 15 ? 15 : highGain));

		reverbVoice->SetEffectParameters(0, &params, sizeof(params));
		reverbVoice->SetVolume(ClampF(volume, 0.0f, 1.0f));
	}
}

//--------------------------------------------------------------------------//
// One-time critical-section management (driven from DllMain).
//--------------------------------------------------------------------------//

/*
 * Initializes the engine reference-counting lock. Called once on DLL attach.
 */
void SoundCommon_InitLock()
{
	InitializeCriticalSection(&g_lock);
}

/*
 * Destroys the engine reference-counting lock. Called once on DLL detach.
 */
void SoundCommon_DeleteLock()
{
	DeleteCriticalSection(&g_lock);
}

//--------------------------------------------------------------------------//
// ISoundCommon implementation (forwards to the shared engine).
//--------------------------------------------------------------------------//

/*
 * DACOM factory hook, called immediately after construction. Nothing to do here:
 * the engine is created lazily on the first Startup().
 */
GENRESULT SoundCommon::init(AGGDESC* /*desc*/)
{
	return GR_OK;
}

/*
 * IAggregateComponent initialization hook. No per-instance state to set up.
 */
GENRESULT SoundCommon::Initialize(void)
{
	return GR_OK;
}

/*
 * Adds a reference to the shared engine, creating it on the first call. Returns
 * TRUE if the engine is running on return.
 */
BOOL32 SoundCommon::Startup()
{
	EnterCriticalSection(&g_lock);
	bool ok = true;
	if (g_engine.refCount == 0)
	{
		ok = g_engine.Startup();
	}
	if (ok)
	{
		++g_engine.refCount;
	}
	LeaveCriticalSection(&g_lock);
	return ok ? TRUE : FALSE;
}

/*
 * Releases this consumer's reference to the engine, destroying it when the last
 * reference goes away.
 */
void SoundCommon::Shutdown()
{
	EnterCriticalSection(&g_lock);
	if (g_engine.refCount > 0)
	{
		if (--g_engine.refCount == 0)
		{
			g_engine.Shutdown();
		}
	}
	LeaveCriticalSection(&g_lock);
}

/*
 * Returns the number of output channels on the device (0 until Startup()).
 */
U32 SoundCommon::GetOutputChannels()
{
	return g_engine.outputChannels;
}

/*
 * Creates a voice wrapping an XAudio2 source voice for the given PCM format.
 * Returns null if 'format' is null or the source voice cannot be created.
 */
HSOUNDVOICE SoundCommon::CreateVoice(const WAVEFORMATEX* format)
{
	if (format == nullptr)
	{
		return nullptr;
	}

	IXAudio2SourceVoice* xaVoice = g_engine.CreateSourceVoice(*format);
	if (xaVoice == nullptr)
	{
		return nullptr;
	}

	SoundVoice* voice = new SoundVoice();
	voice->voice = xaVoice;
	return voice;
}

/*
 * Stops, flushes and destroys a voice and its backing XAudio2 source voice.
 */
void SoundCommon::DestroyVoice(HSOUNDVOICE voice)
{
	if (voice == nullptr)
	{
		return;
	}
	if (voice->voice != nullptr)
	{
		voice->voice->Stop(0);
		voice->voice->FlushSourceBuffers();
		voice->voice->DestroyVoice();
	}
	delete voice;
}

/*
 * Queues a SOUND_BUFFER on a voice, translating it to an XAUDIO2_BUFFER. Returns
 * TRUE if the buffer was accepted.
 */
BOOL32 SoundCommon::SubmitBuffer(HSOUNDVOICE voice, const SOUND_BUFFER* buffer)
{
	if (voice == nullptr || voice->voice == nullptr || buffer == nullptr)
	{
		return FALSE;
	}

	XAUDIO2_BUFFER xb = {};
	xb.pAudioData = reinterpret_cast<const BYTE*>(buffer->data);
	xb.AudioBytes = buffer->byteCount;
	xb.PlayBegin = buffer->playBeginSample;
	xb.LoopBegin = buffer->loopBeginSample;
	xb.LoopLength = buffer->loopLengthSamples;
	xb.LoopCount = (buffer->loopCount >= SOUND_LOOP_INFINITE) ? XAUDIO2_LOOP_INFINITE : buffer->loopCount;
	if (buffer->endOfStream)
	{
		xb.Flags = XAUDIO2_END_OF_STREAM;
	}
	xb.pContext = voice;

	return SUCCEEDED(voice->voice->SubmitSourceBuffer(&xb)) ? TRUE : FALSE;
}

/*
 * Starts playback on a voice.
 */
void SoundCommon::Start(HSOUNDVOICE voice)
{
	if (voice != nullptr && voice->voice != nullptr)
	{
		voice->voice->Start(0);
	}
}

/*
 * Stops playback on a voice without discarding its queued buffers.
 */
void SoundCommon::Stop(HSOUNDVOICE voice)
{
	if (voice != nullptr && voice->voice != nullptr)
	{
		voice->voice->Stop(0);
	}
}

/*
 * Discards a voice's queued buffers.
 */
void SoundCommon::Flush(HSOUNDVOICE voice)
{
	if (voice != nullptr && voice->voice != nullptr)
	{
		voice->voice->FlushSourceBuffers();
	}
}

/*
 * Returns the number of buffers still queued on a voice.
 */
U32 SoundCommon::GetBuffersQueued(HSOUNDVOICE voice)
{
	if (voice == nullptr || voice->voice == nullptr)
	{
		return 0;
	}
	XAUDIO2_VOICE_STATE state = {};
	voice->voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	return state.BuffersQueued;
}

/*
 * Sets a voice's volume from a DirectSound centibel attenuation.
 */
void SoundCommon::SetVolume(HSOUNDVOICE voice, S32 centibels)
{
	if (voice != nullptr && voice->voice != nullptr)
	{
		voice->voice->SetVolume(CentibelsToAmplitude(centibels));
	}
}

/*
 * Sets a voice's pitch as a multiplier of its source sample rate.
 */
void SoundCommon::SetFrequencyRatio(HSOUNDVOICE voice, SINGLE ratio)
{
	if (voice != nullptr && voice->voice != nullptr)
	{
		voice->voice->SetFrequencyRatio(ratio);
	}
}

/*
 * Applies a stereo pan and reverb send to a voice by building its output matrix.
 * A positive pan attenuates the left channel and a negative pan the right; the
 * source's channels are folded down to the device's channel count.
 */
void SoundCommon::SetPan(HSOUNDVOICE voice, S32 panCentibels, U32 sourceChannels, SINGLE reverbSend)
{
	if (voice == nullptr || voice->voice == nullptr)
	{
		return;
	}

	if (panCentibels < -10000) panCentibels = -10000;
	if (panCentibels > 10000) panCentibels = 10000;
	const float leftGain = panCentibels > 0 ? CentibelsToAmplitude(-panCentibels) : 1.0f;
	const float rightGain = panCentibels < 0 ? CentibelsToAmplitude(panCentibels) : 1.0f;

	const UINT32 src = sourceChannels;
	const UINT32 dst = g_engine.outputChannels;
	for (UINT32 i = 0; i < src * dst; ++i)
	{
		voice->matrix[i] = 0.0f;
	}

	auto setCoef = [&](UINT32 srcCh, UINT32 dstCh, float gain)
	{
		if (dstCh < dst)
		{
			voice->matrix[dstCh * src + srcCh] = gain;
		}
	};

	if (src == 1)
	{
		setCoef(0, 0, leftGain);
		setCoef(0, dst > 1 ? 1 : 0, rightGain);
	}
	else
	{
		setCoef(0, 0, leftGain);
		setCoef(1, dst > 1 ? 1 : 0, rightGain);
	}

	voice->voice->SetOutputMatrix(g_engine.masteringVoice, src, dst, voice->matrix);

	if (g_engine.reverbVoice != nullptr)
	{
		const float level = ClampF(reverbSend, 0.0f, 1.0f);
		float reverbMatrix[2] = { level, level };
		voice->voice->SetOutputMatrix(g_engine.reverbVoice, src, 1, reverbMatrix);
	}
}

/*
 * Runs an X3DAudio solution for a voice and applies the resulting output matrix,
 * Doppler-adjusted pitch and reverb level. Does nothing until the engine and its
 * X3DAudio instance are ready.
 */
void SoundCommon::Apply3D(HSOUNDVOICE voice, const SOUND_LISTENER* listener, const SOUND_EMITTER* emitter, SINGLE frequencyRatio, SINGLE reverbSend)
{
	if (voice == nullptr || voice->voice == nullptr || listener == nullptr || emitter == nullptr || !g_engine.x3dReady)
	{
		return;
	}

	X3DAUDIO_LISTENER x3dListener = {};
	x3dListener.OrientFront = ToX3D(listener->front);
	x3dListener.OrientTop = ToX3D(listener->top);
	x3dListener.Position = ToX3D(listener->position);
	x3dListener.Velocity = ToX3D(listener->velocity);

	X3DAUDIO_EMITTER x3dEmitter = {};
	static FLOAT32 s_channelAzimuths[2] = { 0.0f, 0.0f };
	x3dEmitter.ChannelCount = emitter->channelCount;
	if (x3dEmitter.ChannelCount > 1)
	{
		x3dEmitter.pChannelAzimuths = s_channelAzimuths;
		x3dEmitter.ChannelRadius = 1.0f;
	}
	x3dEmitter.OrientFront = ToX3D(emitter->front);
	x3dEmitter.OrientTop = ToX3D(emitter->top);
	x3dEmitter.Position = ToX3D(emitter->position);
	x3dEmitter.Velocity = ToX3D(emitter->velocity);
	x3dEmitter.InnerRadius = emitter->innerRadius;
	x3dEmitter.CurveDistanceScaler = emitter->curveDistanceScaler > 0.0f ? emitter->curveDistanceScaler : 1.0f;
	x3dEmitter.DopplerScaler = emitter->dopplerScaler;

	X3DAUDIO_CONE cone = {};
	if (emitter->hasCone)
	{
		cone.InnerAngle = emitter->coneInnerAngle;
		cone.OuterAngle = emitter->coneOuterAngle;
		cone.InnerVolume = emitter->coneInnerVolume;
		cone.OuterVolume = emitter->coneOuterVolume;
		cone.InnerLPF = 0.0f;
		cone.OuterLPF = 1.0f;
		cone.InnerReverb = 1.0f;
		cone.OuterReverb = 1.0f;
		x3dEmitter.pCone = &cone;
	}

	X3DAUDIO_DSP_SETTINGS dsp = {};
	dsp.SrcChannelCount = x3dEmitter.ChannelCount;
	dsp.DstChannelCount = g_engine.outputChannels;
	dsp.pMatrixCoefficients = voice->matrix;

	UINT32 flags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER;
	if (g_engine.reverbVoice != nullptr)
	{
		flags |= X3DAUDIO_CALCULATE_REVERB;
	}
	X3DAudioCalculate(g_engine.x3dInstance, &x3dListener, &x3dEmitter, flags, &dsp);

	voice->voice->SetOutputMatrix(g_engine.masteringVoice, dsp.SrcChannelCount, dsp.DstChannelCount, voice->matrix);
	voice->voice->SetFrequencyRatio(frequencyRatio * dsp.DopplerFactor);

	if (g_engine.reverbVoice != nullptr)
	{
		const float level = dsp.ReverbLevel * reverbSend;
		float reverbMatrix[2] = { level, level };
		voice->voice->SetOutputMatrix(g_engine.reverbVoice, dsp.SrcChannelCount, 1, reverbMatrix);
	}
}

/*
 * Updates the shared environmental reverb. Forwards to the engine.
 */
void SoundCommon::SetReverb(U32 environment, SINGLE volume, SINGLE decayScale, SINGLE damping)
{
	g_engine.SetReverb(environment, volume, decayScale, damping);
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//
//
// DllMain (Sound/DllMain.cpp) calls these registration hooks.

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_soundCommonFactory = nullptr;

extern "C"
{
	/*
	 * Registers the SoundCommon component factory with the component manager.
	 */
	void Register_SoundCommon()
	{
		g_soundCommonFactory = RegisterComponentFactory<DAComponentAggregate<SoundCommon>>(DACOM_LIBRARY_NAME, CLSID_SoundCommon, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the SoundCommon component factory.
	 */
	void Shutdown_SoundCommon()
	{
		if (g_soundCommonFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_soundCommonFactory, CLSID_SoundCommon);
			g_soundCommonFactory = nullptr;
		}
	}
}
