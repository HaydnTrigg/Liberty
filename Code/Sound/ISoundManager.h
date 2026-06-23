/*
 * ISoundManager.h
 *
 * COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.
 *
 * Public interfaces for the Digital Anvil Sound Manager, the engine's 3D
 * positional sound-effects component.
 *
 * Usage model: the application implements ISoundListener (the "ear") and
 * ISoundSource (each sound), registers its active sounds, and calls
 * ISoundManager::update() each frame. The SoundManager loads sounds as
 * "archetypes" (decoded PCM plus its format/loop parameters), creates a playing
 * instance for each active source, and keeps every instance positioned, pitched,
 * attenuated and looped.
 *
 * This header defines four interfaces - ISoundArchetype, ISoundManager,
 * ISoundManagerStatus - plus the option/speaker enumerations and the device-info
 * struct. The original DirectSound3D/EAX backend has been reimplemented on top of
 * the shared SoundCommon (XAudio2) engine, but the interfaces are unchanged so
 * the rest of the engine binds to them exactly as before.
 */

#ifndef ISOUNDMANAGER_H
#define ISOUNDMANAGER_H

#include <DACOM.h>
#include "ISoundListener.h"
#include "ISound.h"
#include "WaveReader.h"

/*
 * Device/startup options. Passed to ISoundManager::startup() to request
 * capabilities and returned to report which were actually used.
 */
enum SM_DEVICE_OPTIONS
{
	SM_USE_2D_HARDWARE = 1,					// App wants / device has 2D hardware mixing.
	SM_USE_3D_HARDWARE = 2,					// App wants / device has 3D hardware mixing.
	SM_USE_EAX = 4,							// App wants / device supports EAX reverb.
	SM_USE_A3D = 8,							// App wants / device supports A3D.
	SM_USE_RIGHT_HANDED_SYSTEM = 16,		// Use a right-handed coordinate system.
	SM_CREATE_ALL_2D_IN_SOFTWARE = 32,
	SM_USE_3D_SOFTWARE_USER_DEFINED = 64,	// Use the user's current software-3D selection.
	SM_USE_3D_SOFTWARE_BASIC = 128,			// Minimal software 3D (pan/volume only).
	SM_USE_3D_SOFTWARE_LIGHT = 256,			// Light software 3D (decent results, less CPU).
	SM_USE_3D_SOFTWARE_FULL = 512,			// Full software 3D (best results, more CPU).
	SM_USE_3D_SOFTWARE_NONE = 1024,			// No software-3D calculations.
	SM_USE_NO_HW = 2048,					// Disable any hardware acceleration.

	SM_DEFAULT_SETTINGS = SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE | SM_USE_EAX | SM_USE_A3D | SM_USE_RIGHT_HANDED_SYSTEM | SM_USE_3D_SOFTWARE_LIGHT
};

/*
 * Per-archetype options, supplied when an archetype is created.
 */
enum SM_ARCHETYPE_OPTIONS
{
	SM_MUTE_3D_AT_MAX_DISTANCE = 1,			// Silence the sound beyond its max distance.
	SM_ENABLE_FREQUENCY_CONTROL = 2,		// Allow per-instance frequency (pitch) control.
	SM_ENABLE_STICKY_FOCUS = 4,
	SM_DISABLE_STICKY_FOCUS = 8,
	SM_ARCHETYPE_DEFAULT = SM_MUTE_3D_AT_MAX_DISTANCE | SM_DISABLE_STICKY_FOCUS
};

/*
 * 3D processing modes for a sound (see ISoundSource::get_sound_mode()).
 */
enum SM_3D_PROCESSING_MODES
{
	SM_INGNORE_3D_PROECESSING = 0,			// No 3D calculations.
	SM_NORMAL_3D_PROCESSING,				// Sound parameters are absolute.
	SM_LISTENER_RELATIVE_3D_PROCESSING,		// Sound parameters are relative to the listener.
	SM_NUM_3D_PROCESSING_MODES
};

/*
 * Speaker configurations.
 */
enum SM_SPEAKER_CONFIG
{
	SM_SPEAKER_MONO = 0,
	SM_SPEAKER_HEADPHONE,
	SM_SPEAKER_STEREO,
	SM_SPEAKER_STEREO_MIN,		//   5 deg arc
	SM_SPEAKER_STEREO_NARROW,	//  10 deg arc
	SM_SPEAKER_STEREO_WIDE,		//  20 deg arc
	SM_SPEAKER_STEREO_MAX,		// 180 deg arc
	SM_SPEAKER_QUAD,			// 4
	SM_SPEAKER_SURROUND,		// 2 w/surround
	SM_SPEAKER_5POINT1,			// 5.1
	SM_SPEAKER_NUM_SETTINGS
};

/*
 * Audio-device description returned by SoundManager::get_device_info().
 */
struct SM_DEVICEINFO
{
	U32 size;
	U32 deviceOptionsUsed;
	U32 numHWChannels;
	U32 numHW3DBuffers;
	U32 numHWChannelsFree;
	U32 numHW3DBuffersFree;
	GUID deviceGUID;
};

class Vector;
struct IDirectSound;
#define ISOUNDARCHETYPE_VERSION 1
#define IID_ISoundArchetype DACOM_MAKE_IID("ISoundArchetype")

/*
 * ISoundArchetype
 *
 * A loaded sound: its decoded PCM samples plus the format and playback
 * parameters every instance shares. The method order and signatures define the
 * DACOM vtable and must not be reordered or changed.
 */
struct ISoundArchetype : public IDAComponent
{
	// Fills 'soundFormat' with the archetype's sample format.
	virtual void DACOM_API get_sound_format(SoundFormat*) = 0;

	// Returns the sample data and its length in bytes.
	virtual U32 DACOM_API get_samples(void* samples) = 0;

	// Returns the sample data length, in bytes.
	virtual U32 DACOM_API get_sample_count() = 0;

	// Returns the base attenuation applied to every instance, in decibels.
	virtual SINGLE DACOM_API get_base_attenuation() = 0;

	// Returns the channel count of the sample data.
	virtual U32 DACOM_API get_num_channels() = 0;

	// Returns the duration of the sound, in milliseconds.
	virtual U32 DACOM_API get_duration() = 0;

	// Overwrites 'length' bytes of the sample data.
	virtual void DACOM_API set_samples(void* samples, U32 length) = 0;

	// Sets the base attenuation (clamped to -100.0..0.0 decibels).
	virtual GENRESULT DACOM_API set_base_attenuation(SINGLE attenuation) = 0;

	// Returns true if the archetype can loop.
	virtual bool DACOM_API is_loopable() = 0;

	// Returns the loop region, in samples.
	virtual void DACOM_API get_loop_params(U32* loop_start, U32* loop_end) = 0;
};

#define ISOUNDMANAGER_VERSION 1
#define IID_ISoundManager DACOM_MAKE_IID("ISoundManager")

/*
 * ISoundManager
 *
 * The main sound-effects component. The method order and signatures define the
 * DACOM vtable and must not be reordered or changed.
 */
struct ISoundManager : public IDAComponent
{
	// --- Global / primary output format ---

	// Sets/gets the preferred output sample format.
	virtual GENRESULT DACOM_API set_sound_format(SoundFormat* soundFormat) = 0;
	virtual GENRESULT DACOM_API get_sound_format(SoundFormat* soundFormat) = 0;

	// Master attenuation applied to all instances, in decibels (-100.0..0.0).
	virtual GENRESULT DACOM_API set_master_attenuation(const SINGLE attenuation) = 0;

	// Maximum number of sounds mixed simultaneously.
	virtual GENRESULT DACOM_API set_maximum_channels(const U32 numChannels) = 0;

	// Speaker configuration (an SM_SPEAKER_* value).
	virtual GENRESULT DACOM_API set_speaker_configuration(const U32 speakerConfiguration) = 0;

	virtual SINGLE DACOM_API get_master_attenuation() = 0;
	virtual U32 DACOM_API get_maximum_channels() = 0;
	virtual U32 DACOM_API get_speaker_configuration() = 0;

	/*
	 * Master environmental reverb. 'baseEnv' is an EAX environment, 'vol' the
	 * reverb volume, 'decay' the decay-time scale, and 'damping' the damping
	 * scale (1.0 is normal). Applied to all sounds when reverb is enabled.
	 */
	virtual GENRESULT DACOM_API set_master_reverb(const U32 baseEnv, const SINGLE vol, const SINGLE decay, const SINGLE damping) = 0;
	virtual void DACOM_API get_master_reverb(U32* baseEnv, SINGLE* vol, SINGLE* decay, SINGLE* damping) = 0;

	// --- Listener properties ---
	// Prefer implementing ISoundListener and using update(ISoundListener*); these
	// direct setters/getters are retained for compatibility.
	virtual GENRESULT DACOM_API set_ear_orientation(const Matrix* orientation) = 0;
	virtual GENRESULT DACOM_API set_ear_orientation(const Vector* front, const Vector* top) = 0;
	virtual GENRESULT DACOM_API set_ear_position(const Vector*) = 0;
	virtual GENRESULT DACOM_API set_ear_velocity(const Vector*) = 0;
	virtual GENRESULT DACOM_API set_ear_distance_factor(const SINGLE) = 0;
	virtual GENRESULT DACOM_API set_ear_doppler_factor(const SINGLE) = 0;
	virtual GENRESULT DACOM_API set_ear_rolloff_factor(const SINGLE) = 0;
	virtual void DACOM_API get_ear_orientation(Matrix* orientation) = 0;
	virtual void DACOM_API get_ear_orientation(Vector* front, Vector* top) = 0;
	virtual Vector DACOM_API get_ear_position() = 0;
	virtual Vector DACOM_API get_ear_velocity() = 0;
	virtual SINGLE DACOM_API get_ear_distance_factor() = 0;
	virtual SINGLE DACOM_API get_ear_doppler_factor() = 0;
	virtual SINGLE DACOM_API get_ear_rolloff_factor() = 0;

	// --- Property-set expansion mechanism ---
	virtual GENRESULT DACOM_API get_property(ISoundSource* sound, REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData, U32* sizeOfDataWritten) = 0;
	virtual GENRESULT DACOM_API get_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData, U32* sizeOfDataWritten) = 0;
	virtual GENRESULT DACOM_API set_property(ISoundSource* sound, REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData) = 0;
	virtual GENRESULT DACOM_API set_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData) = 0;

	// --- Archetype management ---
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_raw_data(const SoundFormat&, U32 length, U32 loop_start, U32 loop_end, void* sample_buffer, U32 options) = 0;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_soundfile(const SoundFile& sourceData, U32 options) = 0;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype(IFileSystem* sourceFile, U32 options) = 0;
	virtual GENRESULT DACOM_API destroy_archetype(SOUND_ARCH_INDEX archetype) = 0;
	virtual GENRESULT DACOM_API get_archetype_interface(SOUND_ARCH_INDEX archetype, void** archInterface) = 0;

	// --- Active (play) list management ---
	virtual GENRESULT DACOM_API add_active_sound(ISoundSource* sound, U32 index) = 0;
	virtual GENRESULT DACOM_API remove_active_sound(ISoundSource* sound) = 0;
	virtual GENRESULT DACOM_API set_active_sounds(ISoundSource* sounds[], U32 count) = 0;
	virtual U32 DACOM_API get_active_sound_count() = 0;
	virtual U32 DACOM_API get_playing_sound_count() = 0;

	// --- Lifecycle / per-frame update ---
	virtual GENRESULT DACOM_API startup(HWND hWnd, U32 options) = 0;
	virtual GENRESULT DACOM_API shutdown() = 0;
	virtual GENRESULT DACOM_API get_directsound_interface(void**) = 0;
	virtual U32 get_current_time_ms() = 0;
	virtual GENRESULT get_device_info(SM_DEVICEINFO*) = 0;
	virtual void DACOM_API update(ISoundListener* IEar = nullptr) = 0;
	virtual GENRESULT DACOM_API unknownA(void* value) = 0; // #TODO unsure what this is (seems to be called when pressing F1 to open menu)
	virtual GENRESULT DACOM_API unknownB(void* value) = 0; // #TODO unsure what this is (seems to be called when pressing F1 to close menu)
};

/*
 * Snapshot of a single playing sound, returned by
 * ISoundManagerStatus::get_playing_sound_status().
 */
struct SoundStatus
{
	const ISoundSource* soundSource;	// The sound source interface for the sound.
	U32                 internalFlags;	// The SoundManager's internal flags for the sound.
	DWORD               dsoundBufferFlags;	// The DirectSound buffer flags for the sound (informational).
};

#define ISOUNDMANAGERSTATUS_VERSION 1
#define IID_ISoundManagerStatus DACOM_MAKE_IID("ISoundManagerStatus")

/*
 * ISoundManagerStatus
 *
 * Diagnostic access to the playing-sound list. The method order and signatures
 * define the DACOM vtable and must not be reordered or changed.
 */
struct ISoundManagerStatus : public IDAComponent
{
	/*
	 * Fills 'playingStatus' for the playing sound at index 'which' (0-based, see
	 * ISoundManager::get_playing_sound_count()). The returned ISoundSource is not
	 * AddRef'd; ref-count it yourself if you retain it. Returns GR_OK when
	 * 0 <= which < get_playing_sound_count(), GR_GENERIC otherwise.
	 */
	virtual GENRESULT DACOM_API get_playing_sound_status(int which, SoundStatus& playingStatus) = 0;
};

#endif // ISOUNDMANAGER_H
