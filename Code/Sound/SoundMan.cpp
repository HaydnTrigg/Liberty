/*
 * SoundMan.cpp
 *
 * COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.
 *
 * XAudio2/X3DAudio implementation of the SoundManager. See SoundMan.h for the
 * mapping from the original DirectSound3D/EAX design onto the shared SoundCommon
 * engine.
 */

#include <windows.h>
#include <math.h>
#include <DACOM.h>
#include <TComponent.h>
#include <FDump.h>
#include <TempStr.h>

#include "SoundMan.h"
#include "WaveReader.h"
#include "EAX.H"			// EAX environment enumeration (used only to clamp/select reverb).

// 3D modes returned by ISoundSource::get_sound_mode(). These are the DirectSound
// DS3DMODE_* values the application still reports.
enum
{
	SOUND3D_NORMAL = 0,
	SOUND3D_HEADRELATIVE = 1,
	SOUND3D_DISABLE = 2
};

LPGUID ConvertStringToGUID(char* string, LPGUID guid);

// Default audio-device id when none is specified in the ini file.
const char* ID_NullGuid = "{00000000-0000-0000-0000-000000000000}";

#ifdef _DEBUG
bool debugStopProcessingSounds = false;
#endif

namespace
{
	/*
	 * Converts an engine Vector to the backend-neutral SOUND_VECTOR.
	 */
	SOUND_VECTOR ToSound(const Vector& v)
	{
		SOUND_VECTOR out;
		out.x = v.x;
		out.y = v.y;
		out.z = v.z;
		return out;
	}
}

//
// SoundInstance
//

SoundInstance::SoundInstance()
{
	soundSource = nullptr;
	m_internal_flags = 0;
	m_DSOUND_buffer_flags = 0;
	m_startTime = 0;
	m_submitted_index = 0;

	m_cachedDistanceSquared = 0;
	m_cachedMax = 0;
	m_cachedMaxSquared = 0;
	m_cachedMin = 0;
	m_cachedMinSquared = 0;
	m_cachedPositionV.set(0, 0, 0);
	m_cachedSoundPanV.set(0, 0, 0);
	m_archetype = nullptr;
}

SoundInstance::SoundInstance(ISoundSource* source) : SoundInstance()
{
	soundSource = source;
	if (soundSource)
	{
		soundSource->AddRef();
	}
}

SoundInstance::SoundInstance(const SoundInstance& rhs)
{
	soundSource = nullptr;
	*this = rhs;
}

void SoundInstance::operator=(const SoundInstance& rhs)
{
	free();		// Release any references held by this instance.

	soundSource = rhs.soundSource;
	if (soundSource)
	{
		soundSource->AddRef();
	}

	m_voice = rhs.m_voice;		// Shared ownership of the source voice.

	copy_instance_data(rhs);

	m_startTime = rhs.m_startTime;
	m_archetype = rhs.m_archetype;
}

/*
 * Copies the cached/flag data from another instance.
 *
 * NOTE: does not copy the sound source, voice, start time or archetype.
 */
inline void SoundInstance::copy_instance_data(const SoundInstance& rhs)
{
	m_internal_flags = rhs.m_internal_flags;
	m_submitted_index = rhs.m_submitted_index;
	m_DSOUND_buffer_flags = rhs.m_DSOUND_buffer_flags;
	m_cachedDistanceSquared = rhs.m_cachedDistanceSquared;
	m_cachedMax = rhs.m_cachedMax;
	m_cachedMaxSquared = rhs.m_cachedMaxSquared;
	m_cachedMin = rhs.m_cachedMin;
	m_cachedMinSquared = rhs.m_cachedMinSquared;
	m_cachedPositionV = rhs.m_cachedPositionV;
	m_cachedSoundPanV = rhs.m_cachedSoundPanV;
}

SoundInstance::~SoundInstance()
{
	free();
}

/*
 * Releases the instance's references. Releasing the shared voice destroys it once
 * the last instance lets go.
 */
inline void SoundInstance::free()
{
	m_voice.reset();
	m_archetype = nullptr;

	if (soundSource)
	{
		soundSource->Release();
		soundSource = nullptr;
	}
}

//
// SoundArchetype
//

SoundArchetype::SoundArchetype()
{
	m_msDuration = 0;
	m_bufferFlags = 0;
	m_baseAttenuation = 0;	// No attenuation - full volume.
	memset(&m_waveFormat, 0, sizeof(m_waveFormat));
	memset(&m_soundFile, 0, sizeof(SoundFile));
}

SoundArchetype::~SoundArchetype()
{
	if (m_soundFile.samples)
	{
		delete[] m_soundFile.samples;
		m_soundFile.samples = nullptr;
	}
}

/*
 * Returns true if the archetype loops between markers rather than over its whole
 * length.
 */
bool SoundArchetype::has_loop_region() const
{
	return m_soundFile.loop_start != 0 || m_soundFile.loop_end < m_soundFile.num_samples;
}

/*
 * Loads the archetype's sample data from a file and stores it. Returns GR_OK on
 * success, GR_GENERIC if the file could not be loaded or applied.
 */
GENRESULT SoundArchetype::set_sound_data_from_file(IFileSystem* sourceFile, U32 options)
{
	SoundFile soundFile;
	if (LoadWAV(sourceFile, soundFile))
	{
		if (SUCCEEDED(set_sound_data(&soundFile.format, soundFile.length, soundFile.loop_start, soundFile.loop_end, soundFile.samples + soundFile.start_offset, options)))
		{
			delete[] soundFile.samples;
			return GR_OK;
		}

		char buffer[128];
		sourceFile->GetFileName(buffer, 128);
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't set sound data for wav file <%s>.\n", buffer));
		delete[] soundFile.samples;
	}
	else
	{
		char buffer[128];
		sourceFile->GetFileName(buffer, 128);
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't load wav file <%s>.\n", buffer));
	}
	return GR_GENERIC;
}

/*
 * Stores the archetype's sample data and builds the matching PCM wave format.
 * A private copy of the samples is kept; XAudio2 source voices read from it
 * directly for the lifetime of the archetype. Returns GR_OK.
 */
GENRESULT SoundArchetype::set_sound_data(const SoundFormat* sourceData, U32 length, U32 loop_start, U32 loop_end, void* sample_buffer, U32 options)
{
	m_bufferFlags = options;

	memcpy(&m_soundFile.format, sourceData, sizeof(SoundFormat));
	m_soundFile.length = length;
	m_soundFile.loop_start = loop_start;
	m_soundFile.loop_end = loop_end;
	m_soundFile.num_samples = length / m_soundFile.format.bytes_per_sample;
	m_soundFile.samples = new char[m_soundFile.length];
	memcpy(m_soundFile.samples, sample_buffer, m_soundFile.length);

	// Build the PCM wave format.
	m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_waveFormat.nChannels = m_soundFile.format.num_channels;
	m_waveFormat.nSamplesPerSec = m_soundFile.format.samples_per_sec;
	m_waveFormat.wBitsPerSample = (WORD)((m_soundFile.format.bytes_per_sample * 8) / m_waveFormat.nChannels);
	m_waveFormat.nBlockAlign = (WORD)((m_waveFormat.wBitsPerSample * m_waveFormat.nChannels) / 8);
	m_waveFormat.nAvgBytesPerSec = m_waveFormat.nBlockAlign * m_waveFormat.nSamplesPerSec;
	m_waveFormat.cbSize = 0;

	m_msDuration = 1000 * m_soundFile.length / m_soundFile.format.bytes_per_sample / m_soundFile.format.samples_per_sec;

	return GR_OK;
}

//
// ISoundArchetype methods
//

void DACOM_API SoundArchetype::get_sound_format(SoundFormat* soundFormat)
{
	memcpy(soundFormat, &m_soundFile.format, sizeof(SoundFormat));
}

U32 DACOM_API SoundArchetype::get_samples(void* samples)
{
	samples = m_soundFile.samples;
	return m_soundFile.length;
}

U32 DACOM_API SoundArchetype::get_sample_count()
{
	return m_soundFile.length;
}

SINGLE DACOM_API SoundArchetype::get_base_attenuation()
{
	return m_baseAttenuation;
}

U32 DACOM_API SoundArchetype::get_num_channels()
{
	return m_soundFile.format.num_channels;
}

U32 DACOM_API SoundArchetype::get_duration()
{
	return m_msDuration;
}

bool DACOM_API SoundArchetype::is_loopable()
{
	return true;
}

void DACOM_API SoundArchetype::get_loop_params(U32* loop_start, U32* loop_end)
{
	*loop_start = m_soundFile.loop_start;
	*loop_end = m_soundFile.loop_end;
}

void DACOM_API SoundArchetype::set_samples(void* samples, U32 length)
{
	memcpy(m_soundFile.samples, samples, length);
}

GENRESULT DACOM_API SoundArchetype::set_base_attenuation(SINGLE attenuation)
{
	if (attenuation > 0.0)
	{
		attenuation = 0.0;
	}
	else if (attenuation < -100.0)
	{
		attenuation = -100.0;
	}
	m_baseAttenuation = attenuation;
	return GR_OK;
}

/*
 * RAII guard that enters a critical section on construction and leaves it on
 * destruction.
 */
struct Crit
{
	Crit(CRITICAL_SECTION* csec) : sec(csec)
	{
		ASSERT(sec);
		EnterCriticalSection(sec);
	}

	~Crit(void)
	{
		LeaveCriticalSection(sec);
	}

private:
	CRITICAL_SECTION* sec;
};

#define CRIT_SEC(x) Crit _critical_section_ (x)

#ifdef DA_THREAD_SAFE
#define CRIT(x) Crit _critical_section_ (x)
#else
#define CRIT(x) (void)0
#endif

//
// SoundManager
//

InstanceList SoundManager::m_playList;

SoundManager::SoundManager()
{
#ifdef DA_THREAD_SAFE
	InitializeCriticalSection(&m_archetypeLock);
#endif

	InitializeCriticalSection(&m_listLock);

	m_hWnd = nullptr;
	m_common = nullptr;
	m_Initialized = false;
	m_hardwareOptionsDesired = SM_DEFAULT_SETTINGS;
	m_hardwareOptionsUsed = 0;

	m_simpleCoordTransformation = 1;
	m_nextAvailableArchetype = 1;
	m_masterAttenuation = 0;		// No attenuation - full volume.
	m_currentTime = 0;
	m_maxChannels = 0;
	m_speakerConfiguration = SM_SPEAKER_STEREO;
	m_masterReverbMix = 0;

	m_deviceGuidString[0] = 0;
	memset(&m_deviceGuid, 0, sizeof(m_deviceGuid));
	memset(&m_listener, 0, sizeof(m_listener));

	m_reverb.baseEnv = 0;
	m_reverb.vol = 0.0f;
	m_reverb.decay = 0.1f;
	m_reverb.damping = 1.0f;
	m_originalReverb = m_reverb;

	// Listener defaults.
	m_listenerFront.set(0, 0, 1);
	m_listenerUp.set(0, 1, 0);
	m_listenerPan.set(0, 0, 0);
	m_listenerPosition.set(0, 0, 0);
	m_listenerVelocity.set(0, 0, 0);
	m_listenerDistanceFactor = 1;	// Units are 1 meter unless changed by the app.
	m_listenerDopplerFactor = 1.0f;
	m_listenerRolloffFactor = 1.0f;
}

SoundManager::~SoundManager()
{
	erase_archetypes();

	m_playList.clear();
	m_activeSoundList.clear();

	if (m_common != nullptr)
	{
		m_common->Shutdown();
		m_common->Release();
		m_common = nullptr;
	}

	DeleteCriticalSection(&m_listLock);

#ifdef DA_THREAD_SAFE
	DeleteCriticalSection(&m_archetypeLock);
#endif
}

//
// ISoundManager methods
//

/*
 * Records the preferred output format. The actual output format is owned by the
 * XAudio2 mastering voice; this preference is retained for get_sound_format() but
 * does not reconfigure the device.
 */
GENRESULT DACOM_API SoundManager::set_sound_format(SoundFormat* soundFormat)
{
	unused(soundFormat);
	return GR_OK;
}

/*
 * Reports the output format. XAudio2 mixes in 32-bit float; a 16-bit PCM
 * equivalent is reported for compatibility.
 */
GENRESULT DACOM_API SoundManager::get_sound_format(SoundFormat* soundFormat)
{
	soundFormat->num_channels = (unsigned short)(m_common ? m_common->GetOutputChannels() : 2);
	soundFormat->samples_per_sec = 0;
	soundFormat->bytes_per_channel = 2;
	soundFormat->bytes_per_sample = (unsigned short)(soundFormat->bytes_per_channel * soundFormat->num_channels);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_master_attenuation(const SINGLE masterAttenuation)
{
	m_masterAttenuation = masterAttenuation;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_maximum_channels(const U32 maxChannels)
{
	m_maxChannels = maxChannels;
	return GR_OK;
}

/*
 * Records the speaker configuration. XAudio2 inherits the speaker layout from the
 * OS, so the value is retained only for get_speaker_configuration().
 */
GENRESULT DACOM_API SoundManager::set_speaker_configuration(const U32 speakerConfiguration)
{
	if ((SM_SPEAKER_HEADPHONE > speakerConfiguration) || (SM_SPEAKER_NUM_SETTINGS <= speakerConfiguration))
		return GR_GENERIC;

	m_speakerConfiguration = speakerConfiguration;
	return GR_OK;
}

SINGLE DACOM_API SoundManager::get_master_attenuation()
{
	return m_masterAttenuation;
}

U32 DACOM_API SoundManager::get_maximum_channels()
{
	return m_maxChannels;
}

U32 DACOM_API SoundManager::get_speaker_configuration()
{
	return m_speakerConfiguration;
}

/*
 * Clamps and stores the master reverb settings, and pushes them to the engine if
 * EAX reverb is enabled.
 */
GENRESULT DACOM_API SoundManager::set_master_reverb(const U32 baseEnv, const SINGLE vol, const SINGLE decay, const SINGLE damping)
{
	m_reverb.baseEnv = (baseEnv >= EAX_ENVIRONMENT_COUNT) ? EAX_ENVIRONMENT_GENERIC : baseEnv;
	m_reverb.vol = (vol < 0.0) ? 0.0 : ((vol > 1.0) ? 1.0 : vol);
	m_reverb.decay = (decay < 0.1) ? 0.1 : ((decay > 100.0) ? 100.0 : decay);
	m_reverb.damping = (damping < 0.0) ? 0.0 : ((damping > 2.0) ? 2.0 : damping);

	if ((m_hardwareOptionsDesired & SM_USE_EAX) && m_common != nullptr)
	{
		m_common->SetReverb(m_reverb.baseEnv, m_reverb.vol, m_reverb.decay, m_reverb.damping);
	}

	return GR_OK;
}

void DACOM_API SoundManager::get_master_reverb(U32* baseEnv, SINGLE* vol, SINGLE* decay, SINGLE* damping)
{
	*baseEnv = m_reverb.baseEnv;
	*vol = m_reverb.vol;
	*decay = m_reverb.decay;
	*damping = m_reverb.damping;
}

GENRESULT DACOM_API SoundManager::set_ear_orientation(const Matrix* orientation)
{
	Vector k = orientation->get_k();
	Vector j = orientation->get_j();
	return set_ear_orientation(&k, &j);
}

GENRESULT DACOM_API SoundManager::set_ear_orientation(const Vector* back, const Vector* up)
{
	m_listenerFront = -*back;
	m_listenerFront.z *= m_simpleCoordTransformation;
	m_listenerUp = *up;
	m_listenerUp.z *= m_simpleCoordTransformation;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_position(const Vector* position)
{
	m_listenerPosition = *position;
	m_listenerPosition.z *= m_simpleCoordTransformation;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_velocity(const Vector* velocity)
{
	m_listenerVelocity = *velocity;
	m_listenerVelocity.z *= m_simpleCoordTransformation;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_distance_factor(const SINGLE distanceFactor)
{
	m_listenerDistanceFactor = distanceFactor;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_doppler_factor(const SINGLE dopplerFactor)
{
	m_listenerDopplerFactor = dopplerFactor;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_rolloff_factor(const SINGLE rolloff)
{
	m_listenerRolloffFactor = rolloff;
	return GR_OK;
}

void DACOM_API SoundManager::get_ear_orientation(Matrix* orientation)
{
	Vector back, up, side;

	get_ear_orientation(&back, &up);

	if (m_hardwareOptionsDesired & SM_USE_RIGHT_HANDED_SYSTEM)
		side = cross_product(m_listenerFront, m_listenerUp);
	else
		side = cross_product(m_listenerUp, m_listenerFront);
	side.z *= m_simpleCoordTransformation;

	orientation->set_i(side);
	orientation->set_j(up);
	orientation->set_k(back);
}

void DACOM_API SoundManager::get_ear_orientation(Vector* back, Vector* up)
{
	*back = m_listenerFront;
	back->z *= m_simpleCoordTransformation;
	*back = -*back;
	*up = m_listenerUp;
	up->z *= m_simpleCoordTransformation;
}

Vector DACOM_API SoundManager::get_ear_position()
{
	Vector v = m_listenerPosition;
	v.z *= m_simpleCoordTransformation;
	return v;
}

Vector DACOM_API SoundManager::get_ear_velocity()
{
	Vector v = m_listenerVelocity;
	v.z *= m_simpleCoordTransformation;
	return v;
}

SINGLE DACOM_API SoundManager::get_ear_distance_factor()
{
	return m_listenerDistanceFactor;
}

SINGLE DACOM_API SoundManager::get_ear_doppler_factor()
{
	return m_listenerDopplerFactor;
}

SINGLE DACOM_API SoundManager::get_ear_rolloff_factor()
{
	return m_listenerRolloffFactor;
}

/*
 * The per-buffer property-set extensions were a DirectSound/EAX feature with no
 * XAudio2 equivalent. Environmental reverb is now driven by set_master_reverb()
 * and the per-sound reverb mix; these accessors are retained for ABI but no-op.
 */
GENRESULT DACOM_API SoundManager::get_property(ISoundSource*, REFGUID, const U32, void*, const U32, U32*)
{
	return GR_GENERIC;
}

GENRESULT DACOM_API SoundManager::get_global_property(REFGUID, const U32, void*, const U32, U32*)
{
	return GR_GENERIC;
}

GENRESULT DACOM_API SoundManager::set_property(ISoundSource*, REFGUID, const U32, void*, const U32)
{
	return GR_GENERIC;
}

GENRESULT DACOM_API SoundManager::set_global_property(REFGUID, const U32, void*, const U32)
{
	return GR_GENERIC;
}

/*
 * Inserts an archetype into the map and returns its new index.
 */
inline SOUND_ARCH_INDEX SoundManager::add_archetype(SOUND_ARCH* nu)
{
	CRIT(&m_archetypeLock);

	SOUND_ARCH_INDEX result = m_nextAvailableArchetype;
	m_archetypeMap.insert(ArchetypeMap::value_type(m_nextAvailableArchetype++, nu));
	return result;
}

SOUND_ARCH_INDEX DACOM_API SoundManager::create_archetype_from_raw_data(const SoundFormat& format, U32 length, U32 loop_start, U32 loop_end, void* sample_buffer, U32 options)
{
	SOUND_ARCH* nu = new DAComponent<SoundArchetype>;

	if (FAILED(nu->set_sound_data(&format, length, loop_start, loop_end, sample_buffer, options)))
	{
		nu->Release();
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create archetype %d.\n", m_nextAvailableArchetype));
		return SM_INVALID_ARCHETYPE;
	}
	return add_archetype(nu);
}

SOUND_ARCH_INDEX DACOM_API SoundManager::create_archetype_from_soundfile(const SoundFile& soundFile, U32 options)
{
	SOUND_ARCH* nu = new DAComponent<SoundArchetype>;

	if (FAILED(nu->set_sound_data(&soundFile.format, soundFile.length, soundFile.loop_start, soundFile.loop_end, soundFile.samples, options)))
	{
		nu->Release();
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create archetype %d.\n", m_nextAvailableArchetype));
		return SM_INVALID_ARCHETYPE;
	}
	return add_archetype(nu);
}

SOUND_ARCH_INDEX DACOM_API SoundManager::create_archetype(IFileSystem* sourceFile, U32 options)
{
	SOUND_ARCH* nu = new DAComponent<SoundArchetype>;

	if (FAILED(nu->set_sound_data_from_file(sourceFile, options)))
	{
		nu->Release();
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create archetype %d.\n", m_nextAvailableArchetype));
		return SM_INVALID_ARCHETYPE;
	}
	return add_archetype(nu);
}

/*
 * Destroys an archetype and stops/removes every instance of it.
 */
GENRESULT DACOM_API SoundManager::destroy_archetype(SOUND_ARCH_INDEX archetype)
{
	SOUND_ARCH* dead = nullptr;

	{
		CRIT(&m_archetypeLock);
		ArchetypeMap::iterator it = m_archetypeMap.find(archetype);

		if (it != m_archetypeMap.end())
		{
			dead = it->second;
			m_archetypeMap.erase(it);
		}
	}

	if (dead)
	{
		clean_archetype_references(archetype);
		dead->Release();
		return GR_OK;
	}
	return GR_GENERIC;
}

/*
 * Looks up an archetype by index. On success 'arch_ptr' is AddRef'd; the caller
 * must Release it. Returns true if found.
 */
bool SoundManager::query_archetype(SOUND_ARCH_INDEX archetype, SOUND_ARCH*& arch_ptr)
{
	bool gr = false;

	if (SM_INVALID_ARCHETYPE != archetype)
	{
		CRIT(&m_archetypeLock);

		ArchetypeMap::iterator it = m_archetypeMap.find(archetype);

		if (it != m_archetypeMap.end())
		{
			arch_ptr = it->second;
			arch_ptr->AddRef();
			gr = true;
		}
	}

	if (!gr)
	{
		GENERAL_TRACE_2(TEMPSTR("SoundMgr:Invalid archetype index %d. Last valid archetype index is %d.\n", archetype, m_nextAvailableArchetype - 1));
	}

	return gr;
}

/*
 * Converts a frequency value (clamped to -100..100) into a sample-rate
 * multiplier: 2^(frequency / 100).
 */
float SoundManager::calculate_frequency_factor(float frequency)
{
	frequency = CLAMP(frequency, -100.0f, 100.0f);
	return pow(2.0f, frequency * 0.01f);
}

/*
 * Returns the archetype's ISoundArchetype interface. Returns GR_OK on success,
 * GR_GENERIC if the archetype is unknown.
 */
GENRESULT DACOM_API SoundManager::get_archetype_interface(SOUND_ARCH_INDEX archetype, void** archInterface)
{
	if (archInterface)
	{
		SOUND_ARCH* ar;

		if (query_archetype(archetype, ar))
		{
			if (ar->QueryInterface(IID_ISoundArchetype, archInterface) != GR_OK)
			{
				GENERAL_WARNING("Failed to query the IID_ISoundArchetype.\n");
				ar->Release();
				return GR_GENERIC;
			}
			ar->Release();
			return GR_OK;
		}
		*archInterface = nullptr;
	}
	return GR_GENERIC;
}

/*
 * Adds a sound to the active list at position 'index' (or the end if 'index' is
 * past the end). The list is consumed during the next update().
 */
GENRESULT DACOM_API SoundManager::add_active_sound(ISoundSource* sound, U32 index)
{
	if (sound)
	{
		U32 currentIndex = 0;
		SoundSourceList::iterator itr = m_activeSoundList.begin();
		sound->AddRef();
		if (index < m_activeSoundList.size())
		{
			while ((itr != m_activeSoundList.end()) && (currentIndex < index))
			{
				++itr;
				++currentIndex;
			}
			m_activeSoundList.insert(itr, sound);
		}
		else
		{
			m_activeSoundList.push_back(sound);
		}
		return GR_OK;
	}
	return GR_GENERIC;
}

/*
 * Replaces the entire active sound list. The list is consumed during the next
 * update(). An empty list also clears the play list.
 */
GENRESULT DACOM_API SoundManager::set_active_sounds(ISoundSource* sounds[], U32 count)
{
	for (SoundSourceList::iterator itr = m_activeSoundList.begin(); itr != m_activeSoundList.end(); itr++)
	{
		(*itr)->Release();
	}

	m_activeSoundList.clear();

	if (count)
	{
		for (U32 i = 0; i < count; i++)
		{
			if (sounds[i])
			{
				m_activeSoundList.push_back(sounds[i]);
				sounds[i]->AddRef();
			}
			else
			{
				return GR_GENERIC;
			}
		}
	}
	else
	{
		m_playList.clear();
	}
	return GR_OK;
}

/*
 * Removes a sound from the active and play lists, stopping it immediately.
 */
GENRESULT DACOM_API SoundManager::remove_active_sound(ISoundSource* sound)
{
	InstanceList::iterator itr;
	if (exists_in_list(sound, m_playList, &itr))
	{
		m_playList.erase(itr);
	}

	SoundSourceList::iterator activeItr = m_activeSoundList.begin();
	while (activeItr != m_activeSoundList.end())
	{
		if (*activeItr == sound)
		{
			(*activeItr)->Release();
			activeItr = m_activeSoundList.erase(activeItr);
		}
		else
		{
			++activeItr;
		}
	}

	return GR_OK;
}

U32 DACOM_API SoundManager::get_active_sound_count()
{
	return m_activeSoundList.size();
}

U32 DACOM_API SoundManager::get_playing_sound_count()
{
	return m_playList.size();
}

GENRESULT DACOM_API SoundManager::get_playing_sound_status(int which, SoundStatus& playingStatus)
{
	int size = m_playList.size();
	if (which < 0 || which >= size)
	{
		return GR_GENERIC;
	}

	InstanceList::iterator it = m_playList.begin();
	while (which--)
	{
		ASSERT(it != m_playList.end());
		++it;
	}

	const SoundInstance& si = (*it);
	playingStatus.dsoundBufferFlags = si.m_DSOUND_buffer_flags;
	playingStatus.internalFlags = si.m_internal_flags;
	playingStatus.soundSource = si.soundSource;

	return GR_OK;
}

/*
 * Initializes the component (if needed), applies the requested options and ini
 * overrides, and starts the audio engine. Returns GR_OK on success.
 */
GENRESULT DACOM_API SoundManager::startup(HWND hWnd, U32 options)
{
	if (!m_Initialized)
		Initialize();

	m_hWnd = hWnd;
	if (!options)
	{
		GENERAL_ERROR("SoundManager: startup options must be non-zero.\n");
		return GR_GENERIC;
	}

	m_hardwareOptionsDesired = options;

	if (m_hardwareOptionsDesired & SM_USE_NO_HW)
	{
		m_hardwareOptionsDesired &= ~(SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE);
	}

	// Read any overrides from the ini file.
	U32 iniFileOption = 0;
	SINGLE iniFileFloat = 0;

	ICOManager* DACOM = DACOM_Acquire();

	opt_get_u32(DACOM, profile_parser, "AudioProperties", "useEAX", (m_hardwareOptionsDesired & SM_USE_EAX), &iniFileOption);
	if (iniFileOption)
		m_hardwareOptionsDesired |= (SM_USE_EAX);
	else
		m_hardwareOptionsDesired &= (~SM_USE_EAX);

	opt_get_u32(DACOM, profile_parser, "AudioProperties", "reverbBaseEnvironment", 0, &iniFileOption);
	m_reverb.baseEnv = iniFileOption;
	opt_get_float(DACOM, profile_parser, "AudioProperties", "reverbMasterVolume", 0.0f, &iniFileFloat);
	m_reverb.vol = iniFileFloat;
	opt_get_float(DACOM, profile_parser, "AudioProperties", "reverbDecayTime", 0.1f, &iniFileFloat);
	m_reverb.decay = iniFileFloat;
	opt_get_float(DACOM, profile_parser, "AudioProperties", "reverbDamping", 1.0f, &iniFileFloat);
	m_reverb.damping = iniFileFloat;
	opt_get_float(DACOM, profile_parser, "AudioProperties", "reverbMix", 0.0f, &iniFileFloat);
	m_masterReverbMix = iniFileFloat;

	opt_get_string(DACOM, profile_parser, "SoundManager", "deviceID", ID_NullGuid, m_deviceGuidString, 40);
	ConvertStringToGUID(m_deviceGuidString, &m_deviceGuid);

	opt_get_u32(DACOM, profile_parser, "SoundManager", "rightHandedCoordinates", (m_hardwareOptionsDesired & SM_USE_RIGHT_HANDED_SYSTEM), &iniFileOption);
	if (iniFileOption)
	{
		m_hardwareOptionsDesired |= SM_USE_RIGHT_HANDED_SYSTEM;
		m_hardwareOptionsUsed |= SM_USE_RIGHT_HANDED_SYSTEM;
		m_simpleCoordTransformation = -1;
	}
	else
	{
		m_hardwareOptionsDesired &= ~SM_USE_RIGHT_HANDED_SYSTEM;
	}

	opt_get_u32(DACOM, profile_parser, "SoundManager", "speakerConfiguration", SM_SPEAKER_STEREO, &iniFileOption);
	m_speakerConfiguration = iniFileOption;

	opt_get_u32(DACOM, profile_parser, "SoundManager", "maxSoundChannels", m_maxChannels, &iniFileOption);
	m_maxChannels = iniFileOption;

	if (SUCCEEDED(initialize_engine()))
	{
		set_speaker_configuration(m_speakerConfiguration);
		return GR_OK;
	}
	return GR_GENERIC;
}

/*
 * Stops and removes every play-list instance of 'archetype'.
 */
void SoundManager::clean_archetype_references(U32 archetype)
{
	InstanceList::iterator itr = m_playList.begin();
	while (itr != m_playList.end())
	{
		if (archetype == (U32)(*itr).soundSource->get_archetype())
		{
			itr = m_playList.erase(itr);
		}
		else
		{
			itr++;
		}
	}
}

/*
 * Releases every archetype in the database.
 */
void SoundManager::erase_archetypes(void)
{
	CRIT(&m_archetypeLock);

	for (ArchetypeMap::iterator itr = m_archetypeMap.begin(); itr != m_archetypeMap.end(); itr++)
	{
		clean_archetype_references((*itr).first);
		(*itr).second->Release();
	}

	m_archetypeMap.clear();
}

/*
 * Releases all archetypes and sounds and shuts the audio engine down.
 */
GENRESULT DACOM_API SoundManager::shutdown()
{
	erase_archetypes();

	m_playList.clear();
	m_activeSoundList.clear();

	// No more references to any ISoundSources at this point.

	if (m_common != nullptr)
	{
		m_common->Shutdown();
		m_common->Release();
		m_common = nullptr;
	}
	return GR_OK;
}

/*
 * The XAudio2 backend exposes no DirectSound device.
 */
GENRESULT SoundManager::get_directsound_interface(void** lpds)
{
	*lpds = nullptr;
	return GR_GENERIC;
}

U32 SoundManager::get_current_time_ms()
{
	return GetTickCount();
}

GENRESULT SoundManager::get_device_info(SM_DEVICEINFO* info)
{
	info->deviceOptionsUsed = m_hardwareOptionsUsed;
	info->numHWChannels = m_maxChannels;
	info->numHW3DBuffers = m_maxChannels;
	const U32 playing = (U32)m_playList.size();
	info->numHWChannelsFree = (m_maxChannels > playing) ? (m_maxChannels - playing) : 0;
	info->numHW3DBuffersFree = info->numHWChannelsFree;
	info->deviceGUID = m_deviceGuid;
	return GR_OK;
}

/*
 * Per-frame update. Refreshes the listener (if an IEar is supplied), reconciles
 * the play list against the active list, then updates and (re)starts every
 * playing instance.
 */
void DACOM_API SoundManager::update(ISoundListener* IEar)
{
#ifdef _DEBUG
	if (debugStopProcessingSounds)
		return;
#endif

	U32 playingSounds = 0;
	m_currentTime = GetTickCount();

	// Update the listener parameters if an IEar was supplied.
	if (IEar)
	{
		update_listener_parameters(IEar);
	}

	// Rebuild the listener from the current listener state.
	m_listener.front = ToSound(m_listenerFront);
	m_listener.top = ToSound(m_listenerUp);
	m_listener.position = ToSound(m_listenerPosition);
	m_listener.velocity = ToSound(m_listenerVelocity);

	SoundSourceList::iterator activeItr;
	SoundSourceList::iterator activeEndItr = m_activeSoundList.end();
	InstanceList::iterator playItr = m_playList.begin();

	// Remove playing sounds that are no longer in the active sound list.
	while (playItr != m_playList.end())
	{
		bool found = false;
		for (activeItr = m_activeSoundList.begin(); activeItr != activeEndItr; ++activeItr)
		{
			if ((ISoundSource*)(*activeItr) == (ISoundSource*)(*playItr).soundSource)
			{
				found = true;
				break;
			}
		}
		if (found)
			++playItr;
		else
			playItr = m_playList.erase(playItr);
	}

	// Process the active sound list, creating/updating sounds that should play.
	SoundInstance tempInstance, newInstance;
	U32 list_index = 0;

	for (activeItr = m_activeSoundList.begin(); activeItr != activeEndItr; ++activeItr, ++list_index)
	{
		tempInstance.soundSource = *activeItr;		// Borrow (no AddRef).
		tempInstance.m_submitted_index = list_index;

		if ((playingSounds < m_maxChannels) && tempInstance.soundSource->is_on() && need_to_play(tempInstance))
		{
			++playingSounds;

			if (exists_in_list(*activeItr, m_playList, &playItr))
			{
				// Already playing - refresh its cached data.
				(*playItr).copy_instance_data(tempInstance);
				(*playItr).m_submitted_index = list_index;
			}
			else
			{
				// Not playing yet - create and queue a new instance.
				create_instance(tempInstance, newInstance);
				newInstance.m_submitted_index = list_index;
				m_spliceList.push_back(newInstance);
			}

			// Release the transient references held by newInstance (the splice list
			// keeps its own copy).
			newInstance.free();
		}
		else
		{
			if (exists_in_list(*activeItr, m_playList, &playItr))
			{
				CRIT_SEC(&m_listLock);
				m_playList.erase(playItr);
			}
		}
	}
	tempInstance.soundSource = nullptr;		// It was a borrow; don't release in the destructor.

	if (!m_spliceList.empty())
	{
		CRIT_SEC(&m_listLock);
		m_playList.splice(m_playList.end(), m_spliceList);
	}

	// Update every instance's data, then (re)start any that need it.
	for (playItr = m_playList.begin(); playItr != m_playList.end(); ++playItr)
	{
		update_instance_data(*playItr);
	}

	for (playItr = m_playList.begin(); playItr != m_playList.end(); ++playItr)
	{
		U32 beginSample = 0;
		if (update_position(*playItr, beginSample))
		{
			play_instance(*playItr, beginSample);
		}
	}
}

GENRESULT DACOM_API SoundManager::unknownA(void* value)
{
	unused(value);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::unknownB(void* value)
{
	unused(value);
	return GR_OK;
}

//
// ISoundArchetype accessors indexed by SOUND_ARCH_INDEX
//

void DACOM_API SoundManager::get_sound_format(SOUND_ARCH_INDEX archetype, SoundFormat* soundFormat)
{
	SOUND_ARCH* ar;
	if (query_archetype(archetype, ar))
	{
		ar->get_sound_format(soundFormat);
		ar->Release();
	}
}

U32 DACOM_API SoundManager::get_samples(SOUND_ARCH_INDEX archetype, void* samples)
{
	SOUND_ARCH* ar;
	if (query_archetype(archetype, ar))
	{
		U32 result = ar->get_samples(samples);
		ar->Release();
		return result;
	}
	return 0;
}

U32 DACOM_API SoundManager::get_sample_count(SOUND_ARCH_INDEX archetype)
{
	SOUND_ARCH* ar;
	if (query_archetype(archetype, ar))
	{
		U32 result = ar->get_sample_count();
		ar->Release();
		return result;
	}
	return 0;
}

SINGLE DACOM_API SoundManager::get_base_attenuation(SOUND_ARCH_INDEX archetype)
{
	SOUND_ARCH* ar;
	if (query_archetype(archetype, ar))
	{
		SINGLE result = ar->m_baseAttenuation;
		ar->Release();
		return result;
	}
	return 0;
}

U32 DACOM_API SoundManager::get_num_channels(SOUND_ARCH_INDEX archetype)
{
	SOUND_ARCH* ar;
	if (query_archetype(archetype, ar))
	{
		U32 result = ar->m_waveFormat.nChannels;
		ar->Release();
		return result;
	}
	return 0;
}

void DACOM_API SoundManager::set_samples(SOUND_ARCH_INDEX archetype, void* samples, U32 length)
{
	SOUND_ARCH* ar;
	if (query_archetype(archetype, ar))
	{
		if (ar->m_soundFile.length == length)
		{
			ar->set_samples(samples, length);
		}
		ar->Release();
	}
}

bool DACOM_API SoundManager::is_loopable(SOUND_ARCH_INDEX archetype)
{
	SOUND_ARCH* ar;
	if (query_archetype(archetype, ar))
	{
		bool result = ar->is_loopable();
		ar->Release();
		return result;
	}
	return false;
}

//
// SoundManager lifecycle helpers
//

GENRESULT SoundManager::init(AGGDESC* desc)
{
	unused(desc);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::Initialize()
{
	DACOM_Acquire()->QueryInterface(IID_IProfileParser, profile_parser);
	m_Initialized = true;
	return GR_OK;
}

/*
 * Acquires the shared SoundCommon engine through DACOM, starts it, and finalizes
 * the effective audio options. Returns GR_OK on success.
 */
GENRESULT SoundManager::initialize_engine()
{
	if (m_common != nullptr)
		return GR_GENERIC;

	AGGDESC commonDesc(CLSID_SoundCommon);
	ICOManager* manager = DACOM_Acquire();
	if (manager == nullptr || manager->CreateInstance(&commonDesc, (void**)&m_common) != GR_OK || m_common == nullptr)
	{
		m_common = nullptr;
		GENERAL_WARNING("SoundManager: failed to acquire the SoundCommon engine.\n");
		return GR_GENERIC;
	}
	if (!m_common->Startup())
	{
		m_common->Release();
		m_common = nullptr;
		GENERAL_WARNING("SoundManager: failed to start the audio engine.\n");
		return GR_GENERIC;
	}

	// XAudio2 mixes in software with no fixed hardware channel limit; expose the
	// requested options minus the (meaningless) hardware-buffer flags.
	m_hardwareOptionsUsed &= ~(SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE);
	if (m_hardwareOptionsDesired & SM_USE_EAX)
		m_hardwareOptionsUsed |= SM_USE_EAX;

	// Pick a generous default voice budget if the app/ini didn't specify one.
	if (m_maxChannels == 0)
		m_maxChannels = 64;

	// Push the configured reverb to the engine.
	set_master_reverb(m_reverb.baseEnv, m_reverb.vol, m_reverb.decay, m_reverb.damping);

	report_audio_options();
	GENERAL_TRACE_1("SoundManager: SoundManager successfully initialized (XAudio2).\n");
	return GR_OK;
}

void SoundManager::report_audio_options()
{
	GENERAL_NOTICE(TEMPSTR("SoundMgr:Reverb (EAX) %s\n",
		((m_hardwareOptionsUsed & SM_USE_EAX) ? "ENABLED" : "DISABLED")));

	if (m_hardwareOptionsUsed & SM_USE_RIGHT_HANDED_SYSTEM)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Using Right Handed Coordinates\n"));
	}

	GENERAL_NOTICE(TEMPSTR("SoundMgr:Max channels set to %d.\n", m_maxChannels));
}

/*
 * Caches an instance's 3D data (position, min/max distance, distance to the
 * listener) and flags whether the instance is 2D.
 */
void SoundManager::cache_instance_3d_data(SoundInstance& instance)
{
	if (instance.soundSource->is_3D())
	{
		instance.m_internal_flags &= ~SMI_DISABLE_3D;

		instance.m_cachedPositionV = m_listenerPosition;
		instance.m_cachedMin = 10;
		instance.m_cachedMax = 500;
		instance.soundSource->get_position(&instance.m_cachedPositionV);
		instance.soundSource->get_max_distance(&instance.m_cachedMax);
		instance.soundSource->get_min_distance(&instance.m_cachedMin);

		instance.m_cachedPositionV.z *= m_simpleCoordTransformation;
		instance.m_cachedMaxSquared = instance.m_cachedMax * instance.m_cachedMax;
		instance.m_cachedMinSquared = instance.m_cachedMin * instance.m_cachedMin;
		instance.m_cachedSoundPanV.set(instance.m_cachedPositionV.x - m_listenerPosition.x,
			instance.m_cachedPositionV.y - m_listenerPosition.y,
			instance.m_cachedPositionV.z - m_listenerPosition.z);
		instance.m_cachedDistanceSquared = instance.m_cachedSoundPanV.magnitude_squared();
	}
	else
	{
		instance.m_internal_flags |= SMI_DISABLE_3D;
	}
}

/*
 * Returns true if the sound should currently be playing.
 *
 * NOTE: in_range() refreshes the cached 3D data for the instance.
 */
inline bool SoundManager::need_to_play(SoundInstance& instance)
{
	return ((!finished(instance)) &&
		(m_currentTime >= instance.soundSource->get_start_time()) &&
		(in_range(instance)));
}

/*
 * Returns true if the (non-looping) sound has finished, taking the frequency
 * (pitch) change into account. Looping sounds never finish.
 */
inline bool SoundManager::finished(const SoundInstance& instance)
{
	bool result = true;

	if (instance.soundSource->is_looping())
	{
		result = false;
	}
	else
	{
		SOUND_ARCH* ar;
		SOUND_ARCH_INDEX archetype = instance.soundSource->get_archetype();
		if (query_archetype(archetype, ar))
		{
			SINGLE freqFactor = 1.0f;
			float frequency;
			if (SUCCEEDED(instance.soundSource->get_frequency(&frequency)))
			{
				freqFactor = SoundManager::calculate_frequency_factor(frequency);
			}

			result = m_currentTime > (instance.soundSource->get_start_time() + (U32)(ar->m_msDuration / freqFactor));
			ar->Release();
		}
	}

	return result;
}

/*
 * Returns true if the sound is within audible range. Refreshes the cached 3D
 * values for the instance; 2D sounds (and sounds that aren't muted at max
 * distance) are always in range.
 */
inline bool SoundManager::in_range(SoundInstance& instance)
{
	cache_instance_3d_data(instance);

	if (!instance.soundSource->is_3D())
	{
		return true;
	}

	bool muteAtMax = false;
	SOUND_ARCH* ar;
	if (query_archetype(instance.soundSource->get_archetype(), ar))
	{
		muteAtMax = (ar->m_bufferFlags & SM_MUTE_3D_AT_MAX_DISTANCE) != 0;
		ar->Release();
	}

	if (!muteAtMax)
	{
		return true;
	}

	return instance.m_cachedDistanceSquared < instance.m_cachedMaxSquared;
}

/*
 * Returns the 1-based position of 'source' in 'checkList' (or 0 if absent),
 * optionally returning the matching iterator.
 */
inline U32 SoundManager::exists_in_list(const ISoundSource* source, InstanceList& checkList, InstanceList::iterator* returnItr)
{
	U32 position = 1;
	InstanceList::iterator itr;
	for (itr = checkList.begin(); itr != checkList.end(); itr++)
	{
		if (source == (const ISoundSource*)(*itr).soundSource)
		{
			if (returnItr)
			{
				*returnItr = itr;
			}
			return position;
		}
		++position;
	}

	if (returnItr)
		*returnItr = itr;
	return 0;
}

/*
 * Creates a new playing instance of a sound: a source voice over the archetype's
 * data. Returns GR_OK on success, GR_GENERIC if the archetype is unknown or the
 * voice could not be created.
 */
GENRESULT SoundManager::create_instance(SoundInstance& instance, SoundInstance& newInstance)
{
	SOUND_ARCH* ar;
	SOUND_ARCH_INDEX archetype = instance.soundSource->get_archetype();

	if (query_archetype(archetype, ar))
	{
		HSOUNDVOICE voice = m_common->CreateVoice(&ar->m_waveFormat);
		if (voice)
		{
			newInstance.copy_instance_data(instance);
			newInstance.soundSource = instance.soundSource;
			newInstance.soundSource->AddRef();
			newInstance.m_voice = std::make_shared<VoiceHolder>(m_common, voice);
			newInstance.m_DSOUND_buffer_flags = 0;

			// m_archetype is a borrowed pointer: archetypes outlive their instances
			// (destroy_archetype removes the instances first).
			newInstance.m_archetype = ar;
			ar->Release();
			return GR_OK;
		}
		ar->Release();
	}

	GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create an instance of archetype %d.\n", archetype));
	return GR_GENERIC;
}

/*
 * Reads the instance's current parameters from its sound source and pushes them
 * to the voice: volume, then either a 2D pan or a full 3D solution.
 */
void SoundManager::update_instance_data(SoundInstance& instance)
{
	HSOUNDVOICE voice = instance.voice();
	if (!voice || !instance.m_archetype)
		return;

	SoundArchetype* ar = instance.m_archetype;

	// User-controlled pitch (only if the archetype enabled frequency control).
	SINGLE freqFactor = 1.0f;
	float frequency;
	if ((ar->m_bufferFlags & SM_ENABLE_FREQUENCY_CONTROL) && SUCCEEDED(instance.soundSource->get_frequency(&frequency)))
	{
		freqFactor = calculate_frequency_factor(frequency);
	}

	// Per-instance reverb send (only when reverb is enabled).
	SINGLE reverbMix = 0.0f;
	if (m_hardwareOptionsUsed & SM_USE_EAX)
	{
		instance.soundSource->get_reverb_mix(&reverbMix);
	}

	// Attenuation: decibels, converted to the engine's hundredths-of-a-decibel.
	SINGLE attenuation = 0.0f;
	instance.soundSource->get_attenuation(&attenuation);
	SINGLE totalDb = attenuation + ar->m_baseAttenuation + m_masterAttenuation;
	m_common->SetVolume(voice, (S32)(totalDb * 100.0f));

	// 2D sound: stereo pan only.
	if (instance.m_internal_flags & SMI_DISABLE_3D)
	{
		S32 pan = 0;
		instance.soundSource->get_pan(&pan);					// -100..100
		m_common->SetFrequencyRatio(voice, freqFactor);
		m_common->SetPan(voice, pan * 100, ar->m_waveFormat.nChannels, reverbMix);
		return;
	}

	// 3D sound.
	S32 mode = SOUND3D_NORMAL;
	instance.soundSource->get_sound_mode(&mode);
	if (mode == SOUND3D_DISABLE)
	{
		// Spatialization disabled this frame: play centered.
		m_common->SetFrequencyRatio(voice, freqFactor);
		m_common->SetPan(voice, 0, ar->m_waveFormat.nChannels, reverbMix);
		return;
	}

	Vector position = instance.m_cachedPositionV;
	if (mode == SOUND3D_HEADRELATIVE)
		position = position + m_listenerPosition;	// Expressed relative to the listener.

	SOUND_EMITTER emitter;
	memset(&emitter, 0, sizeof(emitter));
	emitter.channelCount = ar->m_waveFormat.nChannels;
	emitter.front = ToSound(Vector(0, 0, 1));
	emitter.top = ToSound(Vector(0, 1, 0));
	emitter.position = ToSound(position);

	Vector velocity(0, 0, 0);
	if (SUCCEEDED(instance.soundSource->get_velocity(&velocity)))
	{
		velocity.z *= m_simpleCoordTransformation;
	}
	emitter.velocity = ToSound(velocity);

	// Optional sound cone.
	DWORD insideAngle = 360, outsideAngle = 360;
	if (SUCCEEDED(instance.soundSource->get_cone_angles(&insideAngle, &outsideAngle)))
	{
		Vector coneDir(0, 0, 1);
		instance.soundSource->get_cone_orientation(&coneDir);
		coneDir.z *= m_simpleCoordTransformation;
		float mag = sqrtf(coneDir.x * coneDir.x + coneDir.y * coneDir.y + coneDir.z * coneDir.z);
		if (mag > 0.0001f)
		{
			coneDir.x /= mag; coneDir.y /= mag; coneDir.z /= mag;
			emitter.front = ToSound(coneDir);
		}

		SINGLE outsideVolume = 0.0f;
		instance.soundSource->get_cone_outside_attenuation(&outsideVolume);

		emitter.hasCone = TRUE;
		emitter.coneInnerAngle = insideAngle * (3.14159265f / 180.0f);
		emitter.coneOuterAngle = outsideAngle * (3.14159265f / 180.0f);
		emitter.coneInnerVolume = 1.0f;
		emitter.coneOuterVolume = (outsideVolume <= -100.0f) ? 0.0f : powf(10.0f, outsideVolume / 20.0f);
	}

	emitter.innerRadius = instance.m_cachedMin;
	// The distance scaler sets the range over which the sound rolls off; tie it to
	// the min distance and the listener rolloff factor.
	float scaler = instance.m_cachedMin * ((m_listenerRolloffFactor > 0.0f) ? (1.0f / m_listenerRolloffFactor) : 1.0f);
	emitter.curveDistanceScaler = (scaler > 0.0f) ? scaler : 1.0f;
	emitter.dopplerScaler = m_listenerDopplerFactor;

	m_common->Apply3D(voice, &m_listener, &emitter, freqFactor, reverbMix);
}

/*
 * Computes the sample offset a (re)started sound should begin at, based on how
 * long ago it was supposed to start. Returns false when the sound is already
 * playing (and should not be restarted) or has expired.
 */
bool SoundManager::update_position(SoundInstance& instance, U32& beginSample)
{
	beginSample = 0;

	HSOUNDVOICE voice = instance.voice();
	if (!voice || !instance.m_archetype)
		return false;

	U32 startTime = instance.soundSource->get_start_time();

	// If the start time is unchanged and the voice is still playing, leave it be.
	if (startTime == instance.m_startTime)
	{
		if (m_common->GetBuffersQueued(voice) > 0)
		{
			return false;
		}
	}

	SoundArchetype* ar = instance.m_archetype;
	SoundFile* soundFile = &ar->m_soundFile;

	SINGLE freqFactor = 1.0f;
	if (ar->m_bufferFlags & SM_ENABLE_FREQUENCY_CONTROL)
	{
		float frequency;
		if (SUCCEEDED(instance.soundSource->get_frequency(&frequency)))
		{
			freqFactor = calculate_frequency_factor(frequency);
		}
	}

	U32 msecs = m_currentTime - startTime;
	msecs = (U32)(msecs * freqFactor);

	// Elapsed time, in sample frames.
	U32 offset = (U32)(((U64)msecs * soundFile->format.samples_per_sec) / 1000);

	const bool looping = instance.soundSource->is_looping();
	if (soundFile->num_samples <= offset)
	{
		if (!looping)
		{
			return false;	// Finished; should have been caught by finished().
		}

		U32 loopLength = (soundFile->loop_end > soundFile->loop_start)
			? (soundFile->loop_end - soundFile->loop_start)
			: soundFile->num_samples;
		offset -= soundFile->num_samples;
		offset = soundFile->loop_start + (offset % loopLength);
	}

	beginSample = offset;
	instance.m_startTime = startTime;
	return true;
}

/*
 * (Re)submits the archetype's PCM to the voice and starts playback. Looping
 * sounds always start from the top; loop regions are applied via the engine's
 * native buffer looping.
 */
void SoundManager::play_instance(SoundInstance& instance, U32 beginSample)
{
	HSOUNDVOICE voice = instance.voice();
	if (!voice || !instance.m_archetype)
		return;

	SoundArchetype* ar = instance.m_archetype;
	const bool looping = instance.soundSource->is_looping();

	m_common->Stop(voice);
	m_common->Flush(voice);

	SOUND_BUFFER buffer = {};
	buffer.data = ar->m_soundFile.samples;
	buffer.byteCount = ar->m_soundFile.length;

	if (looping)
	{
		// Native engine looping. Loop regions require the play cursor to begin at
		// or before the loop, so looping sounds always start from the top (the
		// original mid-loop start was only a cosmetic sync nicety).
		if (ar->has_loop_region())
		{
			buffer.loopBeginSample = ar->m_soundFile.loop_start;
			buffer.loopLengthSamples = ar->m_soundFile.loop_end - ar->m_soundFile.loop_start;
		}
		buffer.loopCount = SOUND_LOOP_INFINITE;
	}
	else
	{
		buffer.playBeginSample = beginSample;
		buffer.endOfStream = TRUE;
	}

	if (m_common->SubmitBuffer(voice, &buffer))
	{
		m_common->Start(voice);
	}
}

/*
 * Reads the listener parameters from the application's ISoundListener and stores
 * those it provides.
 */
void SoundManager::update_listener_parameters(ISoundListener* IEar)
{
	Vector back(0, 0, 1);
	Vector up(0, 1, 0);
	Vector v(0, 0, 0);
	SINGLE s = 1.0f;

	if (SUCCEEDED(IEar->get_ear_orientation(&back, &up)))
	{
		set_ear_orientation(&back, &up);
	}
	if (SUCCEEDED(IEar->get_ear_position(&v)))
	{
		set_ear_position(&v);
	}
	if (SUCCEEDED(IEar->get_ear_velocity(&v)))
	{
		set_ear_velocity(&v);
	}
	if (SUCCEEDED(IEar->get_ear_distance_factor(&s)))
	{
		set_ear_distance_factor(s);
	}
	if (SUCCEEDED(IEar->get_ear_doppler_factor(&s)))
	{
		set_ear_doppler_factor(s);
	}
	if (SUCCEEDED(IEar->get_ear_rolloff_factor(&s)))
	{
		set_ear_rolloff_factor(s);
	}
}

/*
 * Converts a string like "{8BA700E0-2A25-11d2-AE71-0000F4A24D28}" into a GUID.
 * A GUID of all zeros is equivalent to the NULL guid. (Originally from PBleisch.)
 */
LPGUID ConvertStringToGUID(char* string, LPGUID guid)
{
	if (!strcmp(string, ID_NullGuid))
	{
		memset(guid, 0, sizeof(GUID));
		return nullptr;
	}
	else
	{
		DWORD d1, d2, d3;
		char d4[8], sz[200], s[3], * p, * start;

		p = string;
		while (*p && *p != '{') p++;

		strcpy(sz, ++p);

		start = p;
		while (*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) p++;
		p++;

		d1 = strtoul(start, nullptr, 16);

		start = p;
		while (*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) p++;
		p++;

		d2 = strtoul(start, nullptr, 16);

		start = p;
		while (*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) p++;
		p++;

		d3 = strtoul(start, nullptr, 16);

		s[0] = *p++;
		s[1] = *p++;
		s[2] = 0;

		d4[0] = (char)strtoul(s, nullptr, 16);

		s[0] = *p++;
		s[1] = *p++;
		s[2] = 0;

		d4[1] = (char)strtoul(s, nullptr, 16);

		p++;

		for (int i = 0; i < 6; i++)
		{
			s[0] = *p++;
			s[1] = *p++;
			s[2] = 0;

			d4[2 + i] = (char)strtoul(s, nullptr, 16);
		}

		guid->Data1 = d1;
		guid->Data2 = (unsigned short)d2;
		guid->Data3 = (unsigned short)d3;
		memcpy(guid->Data4, d4, 8);
	}
	return guid;
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//
//
// DllMain lives in the merged binary's single entry point (Sound/DllMain.cpp),
// which calls these registration hooks.

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_soundManagerFactory = nullptr;

extern "C"
{
	/*
	 * Creates the DACOM factory for the SoundManager component.
	 */
	IComponentFactory* CreateSoundManagerFactory(void)
	{
		return new DAComponentFactory2<DAComponentAggregate<SoundManager>, AGGDESC>(CLSID_SoundManager);
	}

	/*
	 * Registers the SoundManager component factory with the component manager.
	 */
	void Register_SoundManager()
	{
		g_soundManagerFactory = RegisterComponentFactory<DAComponentAggregate<SoundManager>>(DACOM_LIBRARY_NAME, CLSID_SoundManager, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the SoundManager component factory.
	 */
	void Shutdown_SoundManager()
	{
		if (g_soundManagerFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_soundManagerFactory, CLSID_SoundManager);
			g_soundManagerFactory = nullptr;
		}
	}
}
