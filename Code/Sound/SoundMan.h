/*
 * SoundMan.h
 *
 * COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.
 *
 * SoundManager - the engine's 3D positional sound-effects component.
 *
 * This is the reimplementation of the original DirectSound3D/EAX SoundManager on
 * top of the shared SoundCommon audio engine (acquired through DACOM as
 * ISoundCommon). The public DACOM interfaces (ISoundManager / ISoundManagerStatus
 * / ISoundArchetype) are preserved exactly so the rest of the engine binds to
 * them unchanged; only the backend has been replaced:
 *
 *   - DirectSound device / primary buffer / 3D listener -> the SoundCommon engine
 *     and a SOUND_LISTENER.
 *   - Per-sound DirectSound secondary buffers -> SoundCommon voices that read
 *     sample data straight from the owning archetype.
 *   - DirectSound3D positioning -> ISoundCommon::Apply3D.
 *   - EAX environmental reverb -> ISoundCommon reverb.
 *   - The DirectSound position-notification looping thread -> native engine
 *     buffer loop regions (no helper thread required).
 *
 * No XAudio2 headers are referenced here; all of that lives inside SoundCommon.
 */

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <DACOM.h>
#include "ISoundManager.h"
#include "ISoundListener.h"
#include "ISound.h"

#pragma warning (disable : 4786 4530)
#include <list>
#include <map>
#include <memory>

#include <3dMath.h>
#include <TSmartPointer.h>

#include "IProfileParser_Utility.h"

#include <ISoundCommon.h>

using namespace std;
struct SoundArchetype;
struct SoundInstance;

// Internal SoundInstance flag: 3D processing is disabled (2D sound).
const U32 SMI_DISABLE_3D = 0x20;

/*
 * Master reverb parameters, mirroring the values passed to set_master_reverb().
 */
typedef struct _REVERBINFO
{
	U32 baseEnv;
	SINGLE vol;
	SINGLE decay;
	SINGLE damping;
} REVERBINFO;

/*
 * Reference-counted holder for a SoundCommon voice. Voice handles are not COM
 * objects, so a SoundInstance (which is copied around inside std::list) shares
 * its voice through this holder; the voice is destroyed when the last referencing
 * instance goes away.
 */
struct VoiceHolder
{
	ISoundCommon* common;
	HSOUNDVOICE voice;

	VoiceHolder(ISoundCommon* c, HSOUNDVOICE v) : common(c), voice(v) {}
	~VoiceHolder() { if (common && voice) common->DestroyVoice(voice); }

	VoiceHolder(const VoiceHolder&) = delete;
	VoiceHolder& operator=(const VoiceHolder&) = delete;
};

/*
 * A single playing instance of an archetype: the sound source it came from, the
 * shared voice playing it, and cached 3D/playback state for the current frame.
 */
struct SoundInstance
{
	ISoundSource* soundSource;
	std::shared_ptr<VoiceHolder> m_voice;	// Shared source voice.

	U32		m_startTime;
	U32		m_submitted_index;

	U32		m_internal_flags;
	DWORD	m_DSOUND_buffer_flags;			// Retained for ISoundManagerStatus; informational only.

	SINGLE	m_cachedDistanceSquared;
	SINGLE	m_cachedMax;
	SINGLE	m_cachedMaxSquared;
	SINGLE	m_cachedMin;
	SINGLE	m_cachedMinSquared;
	Vector	m_cachedPositionV;
	Vector	m_cachedSoundPanV;

	SoundArchetype* m_archetype;

	inline void copy_instance_data(const SoundInstance& rhs);

	// Convenience accessor for the underlying voice (may be null).
	HSOUNDVOICE voice() const { return m_voice ? m_voice->voice : nullptr; }

	SoundInstance();
	SoundInstance(ISoundSource*);
	SoundInstance(const SoundInstance& rhs);
	void operator=(const SoundInstance& rhs);
	~SoundInstance();
	void free();
};

/*
 * A loaded sound: the decoded PCM samples plus the format and playback parameters
 * every instance of the sound shares.
 */
struct SoundArchetype : public ISoundArchetype
{
	SoundFile m_soundFile;
	U32 m_msDuration;
	U32 m_bufferFlags;

	SINGLE m_baseAttenuation;
	WAVEFORMATEX m_waveFormat;

	BEGIN_DACOM_MAP_INBOUND(SoundArchetype)
		DACOM_INTERFACE_ENTRY(ISoundArchetype)
		DACOM_INTERFACE_ENTRY2(IID_ISoundArchetype, ISoundArchetype)
		END_DACOM_MAP()

	SoundArchetype();
	~SoundArchetype();

	virtual void DACOM_API get_sound_format(SoundFormat*);
	virtual U32 DACOM_API get_samples(void* samples);
	virtual U32 DACOM_API get_sample_count();
	virtual SINGLE DACOM_API get_base_attenuation();
	virtual U32 DACOM_API get_num_channels();
	virtual U32 DACOM_API get_duration();
	virtual bool DACOM_API is_loopable();
	virtual void DACOM_API get_loop_params(U32* loop_start, U32* loop_end);

	virtual void DACOM_API set_samples(void* samples, U32 length);
	virtual GENRESULT DACOM_API set_base_attenuation(SINGLE attenuation);

	GENRESULT set_sound_data(const SoundFormat* sourceData, U32 length, U32 loop_start, U32 loop_end, void* sample_buffer, U32 options);
	GENRESULT set_sound_data_from_file(IFileSystem* sourceFile, U32 options);

	// True if this archetype loops between markers (rather than over its whole length).
	bool has_loop_region() const;
};

typedef DAComponent<SoundArchetype> SOUND_ARCH;
typedef map<U32, SOUND_ARCH*, less<U32>> ArchetypeMap;
typedef list<SoundInstance> InstanceList;
typedef list<ISoundSource*> SoundSourceList;

/*
 * SoundManager
 *
 * The DACOM sound-effects component. The base-class order, virtual method order
 * and signatures define the DACOM vtables and must not be reordered or changed.
 */
struct SoundManager : public ISoundManager, ISoundManagerStatus, IAggregateComponent
{
protected:
	HWND m_hWnd;
	ArchetypeMap m_archetypeMap;

#ifdef DA_THREAD_SAFE
	CRITICAL_SECTION m_archetypeLock;
#endif

	CRITICAL_SECTION m_listLock;

	SoundSourceList m_activeSoundList;

	InstanceList m_spliceList;
	static InstanceList m_playList;

	SoundInstance m_tempInstance;

	SOUND_ARCH_INDEX m_nextAvailableArchetype;

	bool m_Initialized;
	char m_deviceGuidString[40];
	GUID m_deviceGuid;

	SINGLE m_masterAttenuation;				// Master attenuation applied to all instances.
	U32 m_currentTime;						// Current SoundManager time, in milliseconds.
	U32 m_maxChannels;						// Maximum simultaneous sounds to mix.
	U32 m_speakerConfiguration;				// Current speaker configuration.
	REVERBINFO m_reverb;					// Current reverb settings.
	REVERBINFO m_originalReverb;			// Reverb settings to revert to on shutdown.
	U32 m_masterReverbMix;					// Master reverb amount applied to all instances.

	U32 m_hardwareOptionsDesired;			// Options the app requested.
	U32 m_hardwareOptionsUsed;				// Options actually used.
	S32 m_simpleCoordTransformation;		// +1 left-handed, -1 right-handed (negates the z axis).
	Transform m_earTransform;

	ISoundCommon* m_common;					// Shared audio engine (acquired via DACOM).

	// Listener state.
	Vector m_listenerFront;
	Vector m_listenerUp;
	Vector m_listenerPan;
	Vector m_listenerPosition;
	Vector m_listenerVelocity;
	SINGLE m_listenerDistanceFactor;
	SINGLE m_listenerDopplerFactor;
	SINGLE m_listenerRolloffFactor;

	SOUND_LISTENER m_listener;				// Rebuilt from the listener state each update().

	COMPTR<IProfileParser> profile_parser;

	BEGIN_DACOM_MAP_INBOUND(SoundManager)
		DACOM_INTERFACE_ENTRY(ISoundManager)
		DACOM_INTERFACE_ENTRY2(IID_ISoundManager, ISoundManager)
		DACOM_INTERFACE_ENTRY2(IID_ISoundManagerStatus, ISoundManagerStatus)
		END_DACOM_MAP()

public:
	SoundManager();
	~SoundManager();

	DA_HEAP_DEFINE_NEW_OPERATOR(SoundManager);

	// *** ISoundManager methods ***

	virtual GENRESULT DACOM_API set_sound_format(SoundFormat* soundFormat) override;
	virtual GENRESULT DACOM_API get_sound_format(SoundFormat* soundFormat) override;

	virtual GENRESULT DACOM_API set_master_attenuation(const SINGLE attenuation) override;
	virtual GENRESULT DACOM_API set_maximum_channels(const U32 numChannels) override;
	virtual GENRESULT DACOM_API set_speaker_configuration(const U32 speakerConfiguration) override;

	virtual SINGLE DACOM_API get_master_attenuation() override;
	virtual U32 DACOM_API get_maximum_channels() override;
	virtual U32 DACOM_API get_speaker_configuration() override;

	virtual GENRESULT DACOM_API set_master_reverb(const U32 baseEnv, const SINGLE vol, const SINGLE decay, const SINGLE damping) override;
	virtual void DACOM_API get_master_reverb(U32* baseEnv, SINGLE* vol, SINGLE* decay, SINGLE* damping) override;

	// Direct listener accessors. Prefer implementing ISoundListener and using
	// update(ISoundListener*); these are retained for compatibility.
	virtual GENRESULT DACOM_API set_ear_orientation(const Matrix* orientation) override;
	virtual GENRESULT DACOM_API set_ear_orientation(const Vector*, const Vector*) override;
	virtual GENRESULT DACOM_API set_ear_position(const Vector*) override;
	virtual GENRESULT DACOM_API set_ear_velocity(const Vector*) override;
	virtual GENRESULT DACOM_API set_ear_distance_factor(const SINGLE) override;
	virtual GENRESULT DACOM_API set_ear_doppler_factor(const SINGLE) override;
	virtual GENRESULT DACOM_API set_ear_rolloff_factor(const SINGLE) override;

	virtual void DACOM_API get_ear_orientation(Matrix* orientation) override;
	virtual void DACOM_API get_ear_orientation(Vector*, Vector*) override;
	virtual Vector DACOM_API get_ear_position() override;
	virtual Vector DACOM_API get_ear_velocity() override;
	virtual SINGLE DACOM_API get_ear_distance_factor() override;
	virtual SINGLE DACOM_API get_ear_doppler_factor() override;
	virtual SINGLE DACOM_API get_ear_rolloff_factor() override;

	// Generic property-set extensions (no-ops under the XAudio2 backend).
	virtual GENRESULT DACOM_API get_property(ISoundSource* sound, REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData, U32* sizeOfDataWritten)  override;
	virtual GENRESULT DACOM_API get_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData, U32* sizeOfDataWritten)  override;
	virtual GENRESULT DACOM_API set_property(ISoundSource* sound, REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData)  override;
	virtual GENRESULT DACOM_API set_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData)  override;

	// Archetype creation/destruction.
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_raw_data(const SoundFormat&, U32 length, U32 loop_start, U32 loop_end, void* sample_buffer, U32 options)  override;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_soundfile(const SoundFile& sourceData, U32 options)  override;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype(IFileSystem* sourceFile, U32 options)  override;
	virtual GENRESULT DACOM_API destroy_archetype(SOUND_ARCH_INDEX archetype)  override;

	// Active-sound list management.
	virtual GENRESULT DACOM_API add_active_sound(ISoundSource* sound, U32 index) override;
	virtual GENRESULT DACOM_API remove_active_sound(ISoundSource* sound) override;
	virtual GENRESULT DACOM_API set_active_sounds(ISoundSource* sounds[], U32 count) override;
	virtual U32 DACOM_API get_active_sound_count() override;
	virtual U32 DACOM_API get_playing_sound_count() override;

	// Lifecycle / per-frame update.
	GENRESULT init(AGGDESC* desc);
	GENRESULT DACOM_API Initialize() override;
	GENRESULT initialize_engine();
	virtual GENRESULT DACOM_API startup(HWND hWnd, U32 options) override;
	virtual GENRESULT DACOM_API shutdown() override;
	virtual GENRESULT DACOM_API get_directsound_interface(void**) override;
	virtual U32 get_current_time_ms() override;
	virtual GENRESULT get_device_info(SM_DEVICEINFO*) override;
	virtual void DACOM_API update(ISoundListener* IEar) override;
	virtual GENRESULT DACOM_API unknownA(void* value) override;
	virtual GENRESULT DACOM_API unknownB(void* value) override;

	// Archetype accessors indexed by SOUND_ARCH_INDEX.
	virtual GENRESULT DACOM_API get_archetype_interface(SOUND_ARCH_INDEX archetype, void** archInterface) override;
	virtual void DACOM_API get_sound_format(SOUND_ARCH_INDEX archetype, SoundFormat*);
	virtual U32 DACOM_API get_samples(SOUND_ARCH_INDEX archetype, void* samples);
	virtual U32 DACOM_API get_sample_count(SOUND_ARCH_INDEX archetype);
	virtual SINGLE DACOM_API get_base_attenuation(SOUND_ARCH_INDEX archetype);
	virtual U32 DACOM_API get_num_channels(SOUND_ARCH_INDEX archetype);
	virtual void DACOM_API set_samples(SOUND_ARCH_INDEX archetype, void* samples, U32 length);
	virtual bool DACOM_API is_loopable(SOUND_ARCH_INDEX archetype);

	// *** ISoundManagerStatus methods ***
	virtual GENRESULT DACOM_API get_playing_sound_status(int which, SoundStatus& playingStatus) override;

protected:

	// --- Internal helpers ---

	// Logs the audio options that ended up in effect.
	void report_audio_options();

	// Builds a playing instance (a voice over the archetype data) for a source.
	GENRESULT create_instance(SoundInstance& instance, SoundInstance& newInstance);

	// Returns the 1-based position of a source in 'checkList' (0 if absent).
	U32 exists_in_list(const ISoundSource* source, InstanceList& checkList, InstanceList::iterator* returnItr = NULL);

	// True if the sound should currently be playing.
	bool need_to_play(SoundInstance& instance);

	// True if the (non-looping) sound has finished, accounting for pitch.
	bool finished(const SoundInstance& instance);

	// True if the sound is within audible range (refreshes cached 3D data).
	bool in_range(SoundInstance& instance);

	// Pushes the instance's current data (volume/pan/3D) to its voice.
	void update_instance_data(SoundInstance& instance);

	// Computes the sample offset a (re)started sound should begin at.
	bool update_position(SoundInstance& instance, U32& beginSample);

	// (Re)submits the archetype PCM to a voice and starts playback.
	void play_instance(SoundInstance& instance, U32 beginSample);

	// Reads the listener parameters from the application's ISoundListener.
	void update_listener_parameters(ISoundListener* IEar);

	// Caches an instance's 3D data (position, min/max distance, listener distance).
	void cache_instance_3d_data(SoundInstance& instance);

	// Inserts an archetype into the map and returns its new index.
	SOUND_ARCH_INDEX add_archetype(SOUND_ARCH* nu);

	// Stops and removes every playing instance of 'archetype'.
	void clean_archetype_references(U32 archetype);

	// Destroys every archetype in the database.
	void erase_archetypes(void);

	// QueryInterface-style lookup: on success 'output' is AddRef'd (Release when done).
	bool query_archetype(SOUND_ARCH_INDEX archetype, SOUND_ARCH*& output);

	// Converts a frequency value (-100..100) to a sample-rate multiplier.
	static float calculate_frequency_factor(float frequency);
};

#define CLSID_SoundManager "SoundManager"

/*
 * Component registration hooks, invoked from the merged binary's single entry
 * point (DllMain.cpp). SOUND_DEC is the host module's export/import decoration.
 */
extern "C"
{
	SOUND_DEC void Register_SoundManager();
	SOUND_DEC void Shutdown_SoundManager();
}

#endif // SOUND_MANAGER_H
