#pragma once

#ifndef ISOUNDCOMMON_H
#define ISOUNDCOMMON_H

#include <DACOM.h>
#include <windows.h>
#include <mmreg.h>		// WAVEFORMATEX (Windows multimedia; not an XAudio2 header)

/*
 * ISoundCommon.h
 *
 * Public interface for the SoundCommon component, the shared audio backend used
 * by every other sound component in the engine. SoundCommon owns the process
 * audio device (currently XAudio2 + X3DAudio) and exposes it through a small,
 * backend-neutral DACOM interface. Neither this header nor any consumer of it
 * references XAudio2 directly; the implementation keeps those headers private.
 *
 * Both the SoundManager (3D sound effects) and the SoundStreamer (streamed
 * music) acquire ISoundCommon from the component manager and drive their audio
 * through opaque "sound voices" (HSOUNDVOICE handles):
 *
 *     AGGDESC desc(CLSID_SoundCommon);
 *     ISoundCommon* common = nullptr;
 *     DACOM_Acquire()->CreateInstance(&desc, (void**)&common);
 *     common->Startup();
 *
 * The audio engine is reference counted and shared by all consumers: the first
 * Startup() creates it, and the last matching Shutdown() destroys it.
 */

#define CLSID_SoundCommon "SoundCommon"
#define IID_ISoundCommon DACOM_MAKE_IID("ISoundCommon")

/*
 * Opaque handle to a single sound voice. The concrete type is private to the
 * SoundCommon implementation.
 */
typedef struct SoundVoice* HSOUNDVOICE;

// Loop count meaning "repeat forever" for SOUND_BUFFER::loopCount.
#define SOUND_LOOP_INFINITE 0xFF

/*
 * A 3D vector in the engine's coordinate space (left-handed, +z forward). Kept
 * deliberately independent of the math library and the audio backend.
 */
struct SOUND_VECTOR
{
	SINGLE x;
	SINGLE y;
	SINGLE z;
};

/*
 * Describes a block of PCM to play through a voice. Sample offsets are measured
 * in per-channel sample frames.
 */
struct SOUND_BUFFER
{
	const void* data;			// PCM sample bytes (must remain valid until consumed).
	U32 byteCount;				// Number of bytes at 'data'.
	U32 playBeginSample;		// First frame to play (0 = from the start).
	U32 loopBeginSample;		// First frame of the loop region.
	U32 loopLengthSamples;		// Frames in the loop region (0 with loopCount != 0 = whole buffer).
	U32 loopCount;				// 0 = play once; SOUND_LOOP_INFINITE = forever.
	BOOL32 endOfStream;			// TRUE to mark the final buffer of a stream.
};

/*
 * The listener (the "ear") used for 3D solutions.
 */
struct SOUND_LISTENER
{
	SOUND_VECTOR front;			// Forward orientation (unit length).
	SOUND_VECTOR top;			// Up orientation (unit length).
	SOUND_VECTOR position;
	SOUND_VECTOR velocity;
};

/*
 * A 3D sound source.
 */
struct SOUND_EMITTER
{
	SOUND_VECTOR front;			// Cone forward orientation (unit length).
	SOUND_VECTOR top;			// Cone up orientation (unit length).
	SOUND_VECTOR position;
	SOUND_VECTOR velocity;
	U32 channelCount;			// Channels in the source voice (1 or 2).
	SINGLE innerRadius;			// Distance of full volume.
	SINGLE curveDistanceScaler;	// Scales the distance roll-off curve.
	SINGLE dopplerScaler;		// Doppler intensity.

	BOOL32 hasCone;				// TRUE if the cone fields below are valid.
	SINGLE coneInnerAngle;		// Radians.
	SINGLE coneOuterAngle;		// Radians.
	SINGLE coneInnerVolume;		// Linear amplitude inside the inner cone.
	SINGLE coneOuterVolume;		// Linear amplitude outside the outer cone.
};

/*
 * ISoundCommon
 *
 * Backend-neutral audio engine and per-voice control. The method order and
 * signatures define the DACOM vtable and must not be reordered or changed.
 */
struct DACOM_NO_VTABLE ISoundCommon : public IDAComponent
{
	/*
	 * Starts (or, on later calls, references) the shared audio engine. Returns
	 * TRUE if the engine is running on return.
	 */
	DEFMETHOD_(BOOL32, Startup) () = 0;

	/*
	 * Releases this consumer's reference to the engine. The engine is destroyed
	 * once the last reference is released.
	 */
	DEFMETHOD_(void, Shutdown) () = 0;

	// Returns the number of output channels on the device (valid after Startup()).
	DEFMETHOD_(U32, GetOutputChannels) () = 0;

	// Creates a voice for the given PCM format. Returns null on failure.
	DEFMETHOD_(HSOUNDVOICE, CreateVoice) (const WAVEFORMATEX* format) = 0;

	// Stops and destroys a voice created by CreateVoice().
	DEFMETHOD_(void, DestroyVoice) (HSOUNDVOICE voice) = 0;

	// Queues a buffer of PCM on a voice. Returns TRUE on success.
	DEFMETHOD_(BOOL32, SubmitBuffer) (HSOUNDVOICE voice, const SOUND_BUFFER* buffer) = 0;

	// Playback control for a single voice.
	DEFMETHOD_(void, Start) (HSOUNDVOICE voice) = 0;
	DEFMETHOD_(void, Stop) (HSOUNDVOICE voice) = 0;
	DEFMETHOD_(void, Flush) (HSOUNDVOICE voice) = 0;

	// Returns the number of buffers still queued on a voice.
	DEFMETHOD_(U32, GetBuffersQueued) (HSOUNDVOICE voice) = 0;

	/*
	 * Sets the per-voice volume, in hundredths of a decibel of attenuation
	 * (DirectSound convention: 0 = full volume, -10000 = silence).
	 */
	DEFMETHOD_(void, SetVolume) (HSOUNDVOICE voice, S32 centibels) = 0;

	// Sets the per-voice pitch, as a multiplier of the source sample rate (1.0 = normal).
	DEFMETHOD_(void, SetFrequencyRatio) (HSOUNDVOICE voice, SINGLE ratio) = 0;

	/*
	 * Sets the per-voice stereo pan and reverb send. 'panCentibels' follows the
	 * DirectSound convention (-10000 = full left, 0 = centered, +10000 = full
	 * right), 'sourceChannels' is the voice's channel count, and 'reverbSend'
	 * is the wet level in the range 0..1.
	 */
	DEFMETHOD_(void, SetPan) (HSOUNDVOICE voice, S32 panCentibels, U32 sourceChannels, SINGLE reverbSend) = 0;

	/*
	 * Computes and applies a 3D solution (panning, attenuation and Doppler) to a
	 * voice from the given listener and emitter. 'frequencyRatio' is the base
	 * pitch the Doppler factor is multiplied into; 'reverbSend' scales the
	 * computed reverb level (0..1).
	 */
	DEFMETHOD_(void, Apply3D) (HSOUNDVOICE voice, const SOUND_LISTENER* listener, const SOUND_EMITTER* emitter, SINGLE frequencyRatio, SINGLE reverbSend) = 0;

	/*
	 * Configures the environmental reverb shared by all voices. 'environment' is
	 * an EAX-style environment index, 'volume' (0..1) the reverb return level,
	 * 'decayScale' scales the decay time, and 'damping' (0..2) the
	 * high-frequency absorption.
	 */
	DEFMETHOD_(void, SetReverb) (U32 environment, SINGLE volume, SINGLE decayScale, SINGLE damping) = 0;
};

#endif // ISOUNDCOMMON_H
