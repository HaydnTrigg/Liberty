//--------------------------------------------------------------------------//
//                                                                          //
//                               SoundMan.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Pbleisch $
*/

#include <windows.h>
#include <objbase.h>
#include <initguid.h>         
#include <DACOM.h>
#include <TComponent.h>
#include <FDump.h>
#include <TempStr.h>

#include "SoundMan.h"
#include "wavlib.h"
#include "EAX.H"


#define A3D_SUPPORTED 0

#if A3D_SUPPORTED
#include "Ia3d.h"
#endif

// default buffer parameters for buffers creation - the app doesn't control these directly
// *** NOTE: they all start off the same but are changed based on app requests and available resources
DWORD g_bufferFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRL3D | DSBCAPS_LOCDEFER;

// default value for audio device if none is specified in the ini file
const char* ID_NullGuid = "{00000000-0000-0000-0000-000000000000}";
// possible DSound speaker settings - the app and ini file can specify this by index
const U32 speakerSettings[SM_SPEAKER_NUM_SETTINGS] = { DSSPEAKER_MONO,
														DSSPEAKER_HEADPHONE,
														DSSPEAKER_STEREO,
														DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_MIN),
														DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
														DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE),
														DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_MAX),
														DSSPEAKER_QUAD,
														DSSPEAKER_SURROUND,
														DSSPEAKER_5POINT1 };

LPGUID ConvertStringToGUID(char* string, LPGUID guid);

#ifdef _DEBUG
bool debugStopProcessingSounds = false;
#endif

#define ALLOW_REMOVAL_FROM_HW 0

// Generic buffer class - used when a dummy buffer is needed
// contains everything needed to create a temp buffer and releases the buffer when deleted
GenericDSoundBuffer::GenericDSoundBuffer(LPWAVEFORMATEX fmt)
{
	pBuffer = NULL;
	memset(&desc, 0, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRL3D;
	desc.dwBufferBytes = DSBSIZE_MIN;
	desc.lpwfxFormat = fmt;
}

GenericDSoundBuffer::~GenericDSoundBuffer()
{
	if (pBuffer)
	{
		pBuffer->Release();
		pBuffer = NULL;
	}
}

//
// SoundInstance methods
//

SoundInstance::SoundInstance()
{
	soundSource = NULL;
	m_lpSoundBuffer = NULL;
	m_internal_flags = 0;
	m_DSOUND_buffer_flags = 0;
	m_startTime = 0;

	m_cachedDistanceSquared = 0;
	m_cachedMax = 0;
	m_cachedMaxSquared = 0;
	m_cachedPositionV.set(0, 0, 0);
	m_cachedSoundPanV.set(0, 0, 0);
	m_archetype = NULL;
	m_loopEventHandle = NULL;
	m_loopStartOffset = 0;
}

SoundInstance::SoundInstance(ISoundSource* source)
{
	soundSource = NULL;
	m_lpSoundBuffer = NULL;
	m_internal_flags = 0;
	m_DSOUND_buffer_flags = 0;
	m_startTime = 0;

	m_cachedDistanceSquared = 0;
	m_cachedMax = 0;
	m_cachedMaxSquared = 0;
	m_cachedPositionV.set(0, 0, 0);
	m_cachedSoundPanV.set(0, 0, 0);
	m_archetype = NULL;
	m_loopEventHandle = NULL;
	m_loopStartOffset = 0;

	soundSource = source;
	if (soundSource)
	{
		soundSource->AddRef();
	}
}

SoundInstance::SoundInstance(const SoundInstance& rhs)
{
	soundSource = NULL;
	m_lpSoundBuffer = NULL;
	*this = rhs;
}

void SoundInstance::operator=(const SoundInstance& rhs)
{
	free();  // release any references and stop any buffer associated with this instance

	soundSource = rhs.soundSource;
	if (soundSource)
	{
		soundSource->AddRef();
	}

	m_lpSoundBuffer = rhs.m_lpSoundBuffer;
	if (m_lpSoundBuffer)
	{
		m_lpSoundBuffer->AddRef();
	}

	copy_instance_data(rhs);

	m_startTime = rhs.m_startTime;

	m_archetype = rhs.m_archetype;
	m_loopEventHandle = rhs.m_loopEventHandle;
	m_loopStartOffset = rhs.m_loopStartOffset;
}

inline void SoundInstance::copy_instance_data(const SoundInstance& rhs)
{
	// copies some of the data from another instance
	// *** NOTE: does not copy the soundsource, buffer, start_time, or archetype

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

inline void SoundInstance::free()
{
	if (m_lpSoundBuffer)
	{
		m_lpSoundBuffer->Release();
		m_lpSoundBuffer = NULL;
		if (m_internal_flags & SMI_BUFFER_IN_HARDWARE)
		{
			if (m_archetype)
			{
				m_archetype->release_instance_hw_buffer();
				m_archetype = NULL;
			}
		}
	}
	if (soundSource)
	{
		soundSource->Release();
		soundSource = NULL;
	}
}

//
// SoundArchetype methods
//
SoundArchetype::SoundArchetype()
{
	m_lpSoundBuffer = NULL;
	m_lpHardwareBuffer = NULL;
	m_numHWBuffers = 0;

	memset(&m_dsWaveFormat, 0, sizeof(WAVEFORMATEX));
	memset(&m_soundFile, 0, sizeof(SoundFile));
	m_baseAttenuation = 0;	// no attenuation - full volume
	m_msDuration = 0;
	m_bufferFlags = 0;
}

SoundArchetype::~SoundArchetype()
{
	if (m_lpSoundBuffer)
	{
		m_lpSoundBuffer->Release();
		m_lpSoundBuffer = NULL;
	}
	if (m_soundFile.samples)
	{
		delete[] m_soundFile.samples;
		m_soundFile.samples = NULL;
	}
	if (m_lpHardwareBuffer)
	{
		m_lpHardwareBuffer->Release();
		m_lpHardwareBuffer = NULL;
	}
}

// set the sound data of an archetype from a file
GENRESULT SoundArchetype::set_sound_data_from_file(IFileSystem* sourceFile, LPDIRECTSOUND lpds, U32 options)
{
	SoundFile soundFile;
	char buffer[128];
	sourceFile->GetFileName(buffer, 128);
	// load the wave file (using wavlib.lib)
	if (LoadWAV(sourceFile, soundFile))
		// set the the data for the arhchetype
		if SUCCEEDED(set_sound_data(&soundFile.format, soundFile.length, soundFile.loop_start, soundFile.loop_end, soundFile.samples + soundFile.start_offset, lpds, options))
		{
			delete[] soundFile.samples;
			return GR_OK;
		}
		else
		{	// wave loaded, but archetype was not created or properly initialized
			char buffer[128];
			sourceFile->GetFileName(buffer, 128);
			GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't set sound data for wav file <%s>.\n", buffer));
		}
	else
	{	// Load failed
		char buffer[128];
		sourceFile->GetFileName(buffer, 128);
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't load wav file <%s>.\n", buffer));
	}
	return GR_GENERIC;
}

// set the sound data of an archetype from an already loaded soundfile
GENRESULT SoundArchetype::set_sound_data(const SoundFormat* sourceData, U32 length, U32 loop_start, U32 loop_end, void* sample_buffer, LPDIRECTSOUND lpds, U32 options)
{
	m_bufferFlags = options;
	// set the soundfile data for the archetype
	memcpy(&m_soundFile.format, sourceData, sizeof(SoundFormat));
	m_soundFile.length = length;
	m_soundFile.loop_start = loop_start;
	m_soundFile.loop_end = loop_end;
	m_soundFile.samples = new char[m_soundFile.length];
	m_soundFile.num_samples = length / m_soundFile.format.bytes_per_sample;
	memcpy(m_soundFile.samples, sample_buffer, m_soundFile.length);
	// create a dsound buffer for the archetype
	m_dsWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_dsWaveFormat.nChannels = m_soundFile.format.num_channels;
	m_dsWaveFormat.nSamplesPerSec = m_soundFile.format.samples_per_sec;
	m_dsWaveFormat.wBitsPerSample = (m_soundFile.format.bytes_per_sample * 8) / m_dsWaveFormat.nChannels;
	m_dsWaveFormat.nBlockAlign = (m_dsWaveFormat.wBitsPerSample * m_dsWaveFormat.nChannels * 0.125);
	m_dsWaveFormat.nAvgBytesPerSec = (m_dsWaveFormat.nBlockAlign * m_dsWaveFormat.nSamplesPerSec);
	m_dsWaveFormat.cbSize = 0;
	m_msDuration = 1000 * m_soundFile.length / m_soundFile.format.bytes_per_sample / m_soundFile.format.samples_per_sec;

	DSBUFFERDESC dsbdesc;
	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwBufferBytes = m_soundFile.length;
	dsbdesc.lpwfxFormat = &m_dsWaveFormat;

	dsbdesc.dwFlags = g_bufferFlags;


	//if ( (m_bufferFlags & SM_ENABLE_3D_CONTROL) && (m_bufferFlags & SM_ENABLE_PAN_CONTROL) )
	//{
	//	GENERAL_WARNING("SoundMgr:Archetype requested 3D and Pan controls, disabling pan.\n");
	//	m_bufferFlags &= (~SM_ENABLE_PAN_CONTROL);	// remove the pan flag
	//}
	// add any archetype specific flags
	// if the app wants a frequency control, add the buffer creation flag
	//if (m_bufferFlags & SM_ENABLE_3D_CONTROL) 
	//{
	//	dsbdesc.dwFlags |= DSBCAPS_CTRL3D;
	//}
	//else if (m_bufferFlags & SM_ENABLE_PAN_CONTROL) 
	//{
	//	dsbdesc.dwFlags |= DSBCAPS_CTRLPAN;
	//}

	if (m_bufferFlags & SM_ENABLE_FREQUENCY_CONTROL)
	{
		dsbdesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
	}

	// add sticky focus if desired
	if (m_bufferFlags & SM_ENABLE_STICKY_FOCUS)
	{
		dsbdesc.dwFlags |= DSBCAPS_STICKYFOCUS;
	}

	// if the app wants the buffer muted at max distance, add the buffer creation flag
	if (m_bufferFlags & SM_MUTE_3D_AT_MAX_DISTANCE)
	{
		dsbdesc.dwFlags |= DSBCAPS_MUTE3DATMAXDISTANCE;
	}

	if (m_soundFile.loop_start || (m_soundFile.loop_end < m_soundFile.num_samples))
	{
		// this instance loops between markers, so set the offsets and 
		// add the flag for notification so soundmanager can loop the sound
		dsbdesc.dwFlags |= DSBCAPS_CTRLPOSITIONNOTIFY;
	}

	//if ( m_bufferFlags & SM_SOFTWARE_ONLY )
	//{
	//	m_bufferFlags &= (~DSBCAPS_LOCDEFER);
	//	m_bufferFlags |= DSBCAPS_LOCSOFTWARE;
	//}
	// Create buffer. 
	HRESULT r = lpds->CreateSoundBuffer(&dsbdesc, &m_lpSoundBuffer, NULL);
	if SUCCEEDED(r)
	{
		// write data now, duplicate buffers will be used to get new instances
		if SUCCEEDED(SoundManager::write_data_to_buffer(&m_soundFile, m_lpSoundBuffer, m_soundFile.length))
		{
			return GR_OK;
		}
	}
	else
	{
		// Failed. 
		m_lpSoundBuffer = NULL;
	}
	return GR_GENERIC;
}

void SoundArchetype::add_instance_hw_buffer()
{
	if (m_lpHardwareBuffer)
	{
		++m_numHWBuffers;
	}
}

void SoundArchetype::release_instance_hw_buffer()
{
	if (m_lpHardwareBuffer)
	{
		m_numHWBuffers--;
		if (!m_numHWBuffers)
		{
			m_lpHardwareBuffer->Release();
			m_lpHardwareBuffer = NULL;
		}
	}
}
// ISoundArchetype methods
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
// SoundManager methods
//

HANDLE SoundManager::LoopNotificationThread = NULL;
HANDLE SoundManager::ResetNotificationLoopEvent = NULL;
LONG SoundManager::ExitReq = 0;
InstanceList SoundManager::m_playList;

DWORD WINAPI SoundManager::loop_notification_thread_loop(LPVOID)
{
	while (true)
	{
		U32 numEvents = m_playList.size() + 1;	// add 1 for the ResetNotificationLoopEvent

		HANDLE* eventHandleArray = new HANDLE[numEvents];
		U32 i = 0;
		for (InstanceList::iterator itr = m_playList.begin(); itr != m_playList.end(); itr++)
		{
			if ((*itr).m_loopEventHandle != NULL)
			{
				eventHandleArray[i] = (*itr).m_loopEventHandle;
				i++;
			}
		}
		// add the reset event
		eventHandleArray[i] = ResetNotificationLoopEvent;

		DWORD eventIndex = WaitForMultipleObjects(numEvents, eventHandleArray, false, INFINITE);

		eventIndex -= WAIT_OBJECT_0;
		HANDLE eventHandle = eventHandleArray[eventIndex];

		if (eventHandle == ResetNotificationLoopEvent)
		{
			ResetEvent(ResetNotificationLoopEvent);
		}
		else
		{
			for (InstanceList::iterator itr = m_playList.begin(); itr != m_playList.end(); itr++)
			{
				if ((*itr).m_loopEventHandle == eventHandle)
				{
					if ((*itr).soundSource->is_looping())
					{
						(*itr).m_lpSoundBuffer->SetCurrentPosition((*itr).m_loopStartOffset);
						ResetEvent(eventHandle);
					}
				}
			}
		}

		if (eventHandleArray)
		{
			delete[] eventHandleArray;
		}

		if (ExitReq)
			break;
	}
	return 0;
}

bool SoundManager::start_looping_sound_with_markers(SoundInstance& instance)
{
	if (LoopNotificationThread == NULL)
	{
		DWORD thID;
		LoopNotificationThread = CreateThread(NULL, 0, loop_notification_thread_loop, NULL, 0, &thID);
		if (!LoopNotificationThread)
		{
			GENERAL_ERROR("SoundManager: couldn't create thread for looping sounds.\n");
			return false;
		}
	}

	if (instance.m_loopEventHandle != NULL)
	{
		CloseHandle(instance.m_loopEventHandle);
		instance.m_loopEventHandle = NULL;
	}

	instance.m_loopEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (instance.m_loopEventHandle)
	{
		LPDIRECTSOUNDNOTIFY lpDsNotify = NULL;
		HRESULT hr = instance.m_lpSoundBuffer->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&lpDsNotify);
		if SUCCEEDED(hr)
		{
			DSBPOSITIONNOTIFY PositionNotify[1];

			PositionNotify[0].dwOffset = instance.m_loopEndOffset;
			PositionNotify[0].hEventNotify = instance.m_loopEventHandle;
			HRESULT hr = lpDsNotify->SetNotificationPositions(1, PositionNotify);
			if SUCCEEDED(hr)
			{
				instance.m_internal_flags |= SMI_BUFFER_USING_NOTIFICATION;
				return true;
			}
			lpDsNotify->Release();
		}
		else
		{
			CloseHandle(instance.m_loopEventHandle);
			instance.m_loopEventHandle = NULL;
		}
	}

	return false;
}

SoundManager::SoundManager()
{
#ifdef DA_THREAD_SAFE
	InitializeCriticalSection(&m_archetypeLock);
#endif

	InitializeCriticalSection(&m_loopThreadLock);

	m_Initialized = false;
	m_lpds = NULL;
	m_lpdsPrimaryBuffer = NULL;
	m_lpdsListener = NULL;
	m_maxHardwareChannels = 0;
	m_maxChannels = 0;
	m_hardwareOptionsDesired = SM_DEFAULT_SETTINGS;
	m_numHardware3DBuffers = 0;
	m_hw_threshold_multiplier_squared = 4 * 4;
	//
	m_primarySampleRate = 22050;
	m_primaryNumChannels = 2;
	m_primaryBitsPerSample = 16;

	m_simpleCoordTransformation = 1;
	m_nextAvailableArchetype = 1;
	m_masterAttenuation = 0;		// no attenuation - full volume

	// used to restore user settings if EAX or A3D 1.2 is used
	m_originalReverb.baseEnv = 0;
	m_originalReverb.damping = 0.0f;
	m_originalReverb.decay = 0.0f;
	m_originalReverb.vol = 0.0f;

	// listener defaults
	m_listenerFront.set(0, 0, 1);
	m_listenerUp.set(0, 1, 0);
	m_listenerPosition.set(0, 0, 0);
	m_listenerVelocity.set(0, 0, 0);
	m_listenerDistanceFactor = 1; // units are 1 meter unless changed by the app
	m_listenerDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;
	m_listenerRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;

	ResetNotificationLoopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

SoundManager::~SoundManager()
{
	erase_archetypes();

	if (!m_playList.empty())
	{
		m_playList.clear();
	}
	if (!m_activeSoundList.empty())
	{
		m_activeSoundList.clear();
	}

	DeleteCriticalSection(&m_loopThreadLock);

#ifdef DA_THREAD_SAFE
	DeleteCriticalSection(&m_archetypeLock);
#endif
}

// ISoundManager methods
GENRESULT DACOM_API SoundManager::set_sound_format(SoundFormat* soundFormat)
{
	m_primarySampleRate = soundFormat->samples_per_sec;
	m_primaryNumChannels = soundFormat->num_channels;
	m_primaryBitsPerSample = soundFormat->bytes_per_sample * 8;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::get_sound_format(SoundFormat* soundFormat)
{
	soundFormat->samples_per_sec = m_primarySampleRate;
	soundFormat->num_channels = m_primaryNumChannels;
	soundFormat->bytes_per_sample = m_primaryBitsPerSample / 8;
	soundFormat->bytes_per_channel = soundFormat->bytes_per_sample / m_primaryNumChannels;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_master_attenuation(const SINGLE masterAttenuation)
{
	m_masterAttenuation = masterAttenuation;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_maximum_channels(const U32 maxChannels)
{
	if (maxChannels > m_maxHardwareChannels)
	{
		GENERAL_TRACE_1(TEMPSTR("SoundMgr:Max channels reset to %d. Card can only mix %d in HW.\n", maxChannels, m_maxHardwareChannels));
	}
	m_maxChannels = maxChannels;
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_speaker_configuration(const U32 speakerConfiguration)
{
	if ((SM_SPEAKER_HEADPHONE > speakerConfiguration) || (SM_SPEAKER_NUM_SETTINGS <= speakerConfiguration))
		return GR_GENERIC;

	m_speakerConfiguration = speakerConfiguration;
	// set DSounds speaker config to corresponding DSound setting
	m_lpds->SetSpeakerConfig(speakerSettings[m_speakerConfiguration]);
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
	// returns DSounds speaker config, could just return m_speakerConfiguration
	U32 currentSetting = SM_SPEAKER_STEREO;
	DWORD speakerConfiguration;
	m_lpds->GetSpeakerConfig(&speakerConfiguration);
	for (int i = SM_SPEAKER_HEADPHONE; i < SM_SPEAKER_NUM_SETTINGS; i++)
	{
		if (speakerConfiguration == speakerSettings[i])
		{
			currentSetting = i;
			break;
		}
	}
	return currentSetting;
}

GENRESULT DACOM_API SoundManager::set_master_reverb(const U32 baseEnv, const SINGLE vol, const SINGLE decay, const SINGLE damping)
{
	// clamp the values for EAX
	m_reverb.baseEnv = (baseEnv >= EAX_ENVIRONMENT_COUNT) ? EAX_ENVIRONMENT_GENERIC : baseEnv;
	m_reverb.vol = (vol < 0.0) ? 0.0 : ((vol > 1.0) ? 1.0 : vol);
	m_reverb.decay = (decay < 0.1) ? 0.1 : ((decay > 100.0) ? 100.0 : decay);
	m_reverb.damping = (damping < 0.0) ? 0.0 : ((damping > 2.0) ? 2.0 : damping);

	// Get a dummy buffer
	if (m_hardwareOptionsDesired & SM_USE_EAX) // if user says try to use EAX
	{
		WAVEFORMATEX fmt = { WAVE_FORMAT_PCM,2,22050,88200,4,16,0 };
		GenericDSoundBuffer dummyBuffer(&fmt);

		LPDIRECTSOUNDBUFFER pBuffer = dummyBuffer.pBuffer;
		LPKSPROPERTYSET pEAX;
		DWORD support = 0;
		HRESULT r = m_lpds->CreateSoundBuffer(&dummyBuffer.desc, &pBuffer, NULL);
		if (r == DS_OK)
		{
			if (pBuffer->QueryInterface(IID_IKsPropertySet, (void**)&pEAX) == DS_OK)
			{
				if ((pEAX->QuerySupport(DSPROPSETID_EAX_ReverbProperties, DSPROPERTY_EAX_ALL, &support) == DS_OK) &&
					(support & (KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET)))
				{
					if (!(m_hardwareOptionsUsed & SM_USE_EAX)) // first time setting the properties, so save the original values
					{
						EAX_REVERBPROPERTIES originalprops;
						pEAX->Get(DSPROPSETID_EAX_ReverbProperties, DSPROPERTY_EAX_ALL, NULL, 0, &originalprops, sizeof(EAX_REVERBPROPERTIES), NULL);
						m_originalReverb.baseEnv = originalprops.environment;
						m_originalReverb.vol = originalprops.fVolume;
						m_originalReverb.decay = originalprops.fDecayTime_sec;
						m_originalReverb.damping = originalprops.fDamping;
						m_hardwareOptionsUsed |= SM_USE_EAX;
					}
					// set the properties to the new one
					EAX_REVERBPROPERTIES props = { m_reverb.baseEnv, m_reverb.vol,m_reverb.decay,m_reverb.damping };
					pEAX->Set(DSPROPSETID_EAX_ReverbProperties, DSPROPERTY_EAX_ALL, NULL, 0, &props, sizeof(EAX_REVERBPROPERTIES));
				}
				else
				{
					// couldn't get the property set
					GENERAL_TRACE_1("SoundManager: EAX properties NOT available.\n");
				}
				pEAX->Release();
			}
		}
	}

#if A3D_SUPPORTED
	// if a3d was succesfully created, set the high frequency absorbtion factor
	if (m_hardwareOptionsUsed & SM_USE_A3D)
	{
		LPIA3D2 pIA3d2 = NULL;
		if (SUCCEEDED(m_lpds->QueryInterface(IID_IA3d2, (void**)&pIA3d2)))
		{
			SINGLE A3DDampingFactor = 2.0 - m_reverb.damping;
			pIA3d2->SetHFAbsorbFactor((A3DDampingFactor < 0.1) ? 0.1 : A3DDampingFactor); // A3D values are 0.1 to 2.0 are opposite EAX
			pIA3d2->Release();
		}
	}
#endif

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

	if (!m_lpdsListener)
		return GR_OK;
	m_lpdsListener->SetOrientation(m_listenerFront.x, m_listenerFront.y, m_listenerFront.z, m_listenerUp.x, m_listenerUp.y, m_listenerUp.z, DS3D_DEFERRED);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_position(const Vector* position)
{
	m_listenerPosition = *position;
	m_listenerPosition.z *= m_simpleCoordTransformation;
	if (!m_lpdsListener)
		return GR_OK;

	m_lpdsListener->SetPosition(m_listenerPosition.x, m_listenerPosition.y, m_listenerPosition.z, DS3D_DEFERRED);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_velocity(const Vector* velocity)
{
	m_listenerVelocity = *velocity;
	m_listenerVelocity.z *= m_simpleCoordTransformation;
	if (!m_lpdsListener)
		return GR_OK;
	m_lpdsListener->SetVelocity(m_listenerVelocity.x, m_listenerVelocity.y, m_listenerVelocity.z, DS3D_DEFERRED);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_distance_factor(const SINGLE distanceFactor)
{
	m_listenerDistanceFactor = distanceFactor;
	if (!m_lpdsListener)
		return GR_OK;
	m_lpdsListener->SetDistanceFactor(distanceFactor, DS3D_DEFERRED);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_doppler_factor(const SINGLE dopplerFactor)
{
	m_listenerDopplerFactor = dopplerFactor;
	if (!m_lpdsListener)
		return GR_OK;
	m_lpdsListener->SetDopplerFactor(dopplerFactor, DS3D_DEFERRED);
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::set_ear_rolloff_factor(const SINGLE rolloff)
{
	m_listenerRolloffFactor = rolloff;
	if (!m_lpdsListener)
		return GR_OK;
	m_lpdsListener->SetRolloffFactor(rolloff, DS3D_DEFERRED);
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
	return;
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

GENRESULT DACOM_API SoundManager::get_property(ISoundSource* sound, REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData, U32* sizeOfDataWritten)
{
	GENRESULT retVal = GR_GENERIC;

	// see if this sound is has a buffer
	InstanceList::iterator itr;
	if (exists_in_list(sound, m_playList, &itr))
	{
		LPKSPROPERTYSET pPropSet;
		if ((*itr).m_lpSoundBuffer->QueryInterface(IID_IKsPropertySet, (void**)&pPropSet) == DS_OK)
		{
			DWORD support = 0;
			pPropSet->QuerySupport(propGUID, propID, &support);
			if (support & (KSPROPERTY_SUPPORT_GET))
			{
				pPropSet->Get(propGUID, propID, NULL, 0, propData, sizeOfPropData, &reinterpret_cast<DWORD&>(sizeOfDataWritten));
				retVal = GR_OK;
			}
			pPropSet->Release();
		}
	}
	m_tempInstance.free();
	return retVal;
}

GENRESULT DACOM_API SoundManager::get_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData, U32* sizeOfDataWritten)
{
	GENRESULT retVal = GR_GENERIC;

	WAVEFORMATEX fmt = { WAVE_FORMAT_PCM,2,22050,88200,4,16,0 };
	GenericDSoundBuffer dummyBuffer(&fmt);
	LPDIRECTSOUNDBUFFER pBuffer = dummyBuffer.pBuffer;
	HRESULT hr = m_lpds->CreateSoundBuffer(&dummyBuffer.desc, &pBuffer, NULL);
	if (SUCCEEDED(hr))
	{
		LPKSPROPERTYSET pPropSet;
		if (m_lpdsPrimaryBuffer->QueryInterface(IID_IKsPropertySet, (void**)&pPropSet) == DS_OK)
		{
			DWORD support = 0;
			pPropSet->QuerySupport(propGUID, propID, &support);
			if (support & (KSPROPERTY_SUPPORT_GET))
			{
				pPropSet->Get(propGUID, propID, NULL, 0, propData, sizeOfPropData, &reinterpret_cast<DWORD&>(sizeOfDataWritten));
				retVal = GR_OK;
			}
			pPropSet->Release();
		}
	}
	return retVal;
}


GENRESULT DACOM_API SoundManager::set_property(ISoundSource* sound, REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData)
{
	GENRESULT retVal = GR_GENERIC;

	// see if this sound is has a buffer
	InstanceList::iterator itr;
	if (exists_in_list(sound, m_playList, &itr))
	{
		LPKSPROPERTYSET pPropSet;
		if ((*itr).m_lpSoundBuffer->QueryInterface(IID_IKsPropertySet, (void**)&pPropSet) == DS_OK)
		{
			DWORD support = 0;
			pPropSet->QuerySupport(propGUID, propID, &support);
			if (support & (KSPROPERTY_SUPPORT_SET))
			{
				pPropSet->Set(propGUID, propID, NULL, 0, propData, sizeOfPropData);
				retVal = GR_OK;
			}
			pPropSet->Release();
		}
	}
	m_tempInstance.free();
	return retVal;
}

GENRESULT DACOM_API SoundManager::set_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData)
{
	GENRESULT retVal = GR_GENERIC;

	WAVEFORMATEX fmt = { WAVE_FORMAT_PCM,2,22050,88200,4,16,0 };
	GenericDSoundBuffer dummyBuffer(&fmt);
	LPDIRECTSOUNDBUFFER pBuffer = dummyBuffer.pBuffer;
	HRESULT hr = m_lpds->CreateSoundBuffer(&dummyBuffer.desc, &pBuffer, NULL);
	if (SUCCEEDED(hr))
	{
		LPKSPROPERTYSET pPropSet;
		if (m_lpdsPrimaryBuffer->QueryInterface(IID_IKsPropertySet, (void**)&pPropSet) == DS_OK)
		{
			DWORD support = 0;
			pPropSet->QuerySupport(propGUID, propID, &support);
			if (support & (KSPROPERTY_SUPPORT_SET))
			{
				pPropSet->Set(propGUID, propID, NULL, 0, propData, sizeOfPropData);
				retVal = GR_OK;
			}
			pPropSet->Release();
		}
	}
	return retVal;
}

inline SOUND_ARCH_INDEX SoundManager::add_archetype(SOUND_ARCH* nu)
{
	CRIT(&m_archetypeLock);

	SOUND_ARCH_INDEX result = m_nextAvailableArchetype;
	m_archetypeMap.insert(ArchetypeMap::value_type(m_nextAvailableArchetype++, nu));
	return result;
}

SOUND_ARCH_INDEX DACOM_API SoundManager::create_archetype_from_raw_data(const SoundFormat& format, U32 length, U32 loop_start, U32 loop_end, void* sample_buffer, U32 options)
{
	SOUND_ARCH* nu = new DAComponent <SoundArchetype>;

	if (FAILED(nu->set_sound_data(&format, length, loop_start, loop_end, sample_buffer, m_lpds, options)))
	{
		nu->Release();
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create archetype %d.\n", m_nextAvailableArchetype));
		return SM_INVALID_ARCHETYPE;
	}
	else
	{

		return add_archetype(nu);
	}
}

SOUND_ARCH_INDEX DACOM_API SoundManager::create_archetype_from_soundfile(const SoundFile& soundFile, U32 options)
{
	SOUND_ARCH* nu = new DAComponent <SoundArchetype>;

	if (FAILED(nu->set_sound_data(&soundFile.format, soundFile.length, soundFile.loop_start, soundFile.loop_end, soundFile.samples, m_lpds, options)))
	{
		nu->Release();
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create archetype %d.\n", m_nextAvailableArchetype));
		return SM_INVALID_ARCHETYPE;
	}
	else
		return add_archetype(nu);
}

SOUND_ARCH_INDEX DACOM_API SoundManager::create_archetype(IFileSystem* sourceFile, U32 options)
{
	SOUND_ARCH* nu = new DAComponent <SoundArchetype>;

	if (FAILED(nu->set_sound_data_from_file(sourceFile, m_lpds, options)))
	{
		nu->Release();
		GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create archetype %d.\n", m_nextAvailableArchetype));
		return SM_INVALID_ARCHETYPE;
	}
	else
		return add_archetype(nu);
}

// deletes an archetype and its internal instances
GENRESULT DACOM_API SoundManager::destroy_archetype(SOUND_ARCH_INDEX archetype)
{
	SOUND_ARCH* dead = NULL;

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

		// release this archetype
		dead->Release();
		return GR_OK;
	}
	// archetype doesn't exist
	return GR_GENERIC;
}

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

float SoundManager::calculate_frequency_factor(float frequency)
{
	frequency = CLAMP(frequency, -100.0f, 100.0f);
	return pow(2.0f, frequency * 0.01f);
}

// returns a pointer to the archetypes ISoundArchetype interface 
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
		*archInterface = NULL;
	}
	return GR_GENERIC;
}

// add an active sound to the list that will be used during next update()
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
	return GR_GENERIC; // couldn't create a new node
}

// set the entire active sound list that will be used during next update()
GENRESULT DACOM_API SoundManager::set_active_sounds(ISoundSource* sounds[], U32 count)
{
	// m_activeSoundList is a list of pointers, so Release has to be called
	for (SoundSourceList::iterator itr = m_activeSoundList.begin(); itr != m_activeSoundList.end(); itr++)
	{
		(*itr)->Release();
	}

	m_activeSoundList.clear();

	if (count)
	{
		for (int i = 0; i < count; i++)
		{
			if (sounds[i])
			{
				m_activeSoundList.push_back(sounds[i]);
				sounds[i]->AddRef();
			}
			else
			{
				return GR_GENERIC; // couldn't create a new node
			}
		}
	}
	else
	{
		m_playList.clear();
	}
	return GR_OK;
}

// remove an active sound and stop it immediately
GENRESULT DACOM_API SoundManager::remove_active_sound(ISoundSource* sound)
{
	// remove from play list and stop sound
	InstanceList::iterator itr;
	if (exists_in_list(sound, m_playList, &itr))
	{
		m_playList.erase(itr);
	}

	// remove from active list
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
	// return m_numActiveSounds
	return m_activeSoundList.size();
}

U32 DACOM_API SoundManager::get_playing_sound_count()
{
	// return m_numPlayingSounds
	return m_playList.size();
}

GENRESULT DACOM_API SoundManager::get_playing_sound_status(int which, SoundStatus& playingStatus)
{
	int size = m_playList.size();
	if (which < 0 || which >= size)
	{
		return GR_GENERIC;
	}

	// Yes, this is slow and get slower as which increases, but the playing count should be small.
	InstanceList::iterator it = m_playList.begin();
	while (which--)
	{
		// This assert should never happen because of the check against the list size above.
		ASSERT(it != m_playList.end());
		++it;
	}

	const SoundInstance& si = (*it);
	playingStatus.dsoundBufferFlags = si.m_DSOUND_buffer_flags;
	playingStatus.internalFlags = si.m_internal_flags;
	playingStatus.soundSource = si.soundSource;

	return GR_OK;
}

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

	// if SM_USE_NO_HW was requested, disable SM_USE_2D_HARDWARE and SM_USE_3D_HARDWARE
	if (m_hardwareOptionsDesired & SM_USE_NO_HW)
	{
		m_hardwareOptionsDesired &= ~(SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE);
	}

	// get any info from the ini file
	U32 iniFileOption = 0;
	SINGLE iniFileFloat = 0;

	ICOManager* DACOM = DACOM_Acquire();

	// Audio extension properties (not in "SoundManager" section - in "AudioProperties" section)
	opt_get_u32(DACOM, profile_parser, "AudioProperties", "useEAX", (m_hardwareOptionsDesired & SM_USE_EAX), &iniFileOption);
	if (iniFileOption)
		m_hardwareOptionsDesired |= (SM_USE_EAX);
	else
		m_hardwareOptionsDesired &= (~SM_USE_EAX);

#if A3D_SUPPORTED
	opt_get_u32(DACOM, profile_parser, "AudioProperties", "useA3D", (m_hardwareOptionsDesired & SM_USE_A3D), &iniFileOption);
	if (iniFileOption)
		m_hardwareOptionsDesired |= (SM_USE_A3D);
	else
		m_hardwareOptionsDesired &= (~SM_USE_A3D);
#endif

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

	// soundManager specific properties
	// see if a specific device was desired
	opt_get_string(DACOM, profile_parser, "SoundManager", "deviceID", ID_NullGuid, m_deviceGuidString, 40);
	LPGUID lpguid = ConvertStringToGUID(m_deviceGuidString, &m_deviceGuid);
	unused(lpguid);

	char buffer[32];
	char SW_3D_type[32];
	U32 ALL_SW_FLAGS = SM_USE_3D_SOFTWARE_FULL | SM_USE_3D_SOFTWARE_LIGHT | SM_USE_3D_SOFTWARE_BASIC | SM_USE_3D_SOFTWARE_USER_DEFINED;

	switch (m_hardwareOptionsDesired & ALL_SW_FLAGS)
	{
	case SM_USE_3D_SOFTWARE_FULL:
		sprintf(buffer, "FULL");
		break;
	case SM_USE_3D_SOFTWARE_LIGHT:
		sprintf(buffer, "LIGHT");
		break;
	case SM_USE_3D_SOFTWARE_BASIC:
		sprintf(buffer, "BASIC");
		break;
	case SM_USE_3D_SOFTWARE_NONE:
		sprintf(buffer, "NONE");
		break;
	default:
		sprintf(buffer, "USER");
		break;
	}
	opt_get_string(DACOM, profile_parser, "SoundManager", "3D_SW_Algorithm", buffer, SW_3D_type, 32);
	m_hardwareOptionsDesired &= (~ALL_SW_FLAGS);
	if (!_stricmp("FULL", SW_3D_type))
	{
		m_hardwareOptionsDesired |= SM_USE_3D_SOFTWARE_FULL;
		m_hardwareOptionsUsed |= SM_USE_3D_SOFTWARE_FULL;
		m_DS3D_SW_ALG_GUID = DS3DALG_HRTF_FULL;
	}
	else if (!_stricmp("LIGHT", SW_3D_type))
	{
		m_hardwareOptionsDesired |= SM_USE_3D_SOFTWARE_LIGHT;
		m_hardwareOptionsUsed |= SM_USE_3D_SOFTWARE_LIGHT;
		m_DS3D_SW_ALG_GUID = DS3DALG_HRTF_LIGHT;
	}
	else if (!_stricmp("BASIC", SW_3D_type))
	{
		m_hardwareOptionsDesired |= SM_USE_3D_SOFTWARE_BASIC;
		m_hardwareOptionsUsed |= SM_USE_3D_SOFTWARE_BASIC;
		m_DS3D_SW_ALG_GUID = DS3DALG_NO_VIRTUALIZATION;
	}
	else if (!_stricmp("USER", SW_3D_type))
	{
		m_hardwareOptionsDesired |= SM_USE_3D_SOFTWARE_USER_DEFINED;
		m_hardwareOptionsUsed |= SM_USE_3D_SOFTWARE_USER_DEFINED;
		m_DS3D_SW_ALG_GUID = DS3DALG_DEFAULT;
	}
	else if (!_stricmp("NONE", SW_3D_type))
	{
		g_bufferFlags &= ~DSBCAPS_CTRL3D;	// disable 3D control for all 2D buffers
		g_bufferFlags |= DSBCAPS_CTRLPAN;	// add pan control for all 2D buffers
		m_DS3D_SW_ALG_GUID = GUID_NULL;
	}

	opt_get_u32(DACOM, profile_parser, "SoundManager", "useNoHW", (m_hardwareOptionsDesired & SM_USE_NO_HW), &iniFileOption);
	if (iniFileOption)
		m_hardwareOptionsDesired &= ~(SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE);

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

	opt_get_u32(DACOM, profile_parser, "SoundManager", "createAll2DInSoftware", (m_hardwareOptionsDesired & SM_CREATE_ALL_2D_IN_SOFTWARE), &iniFileOption);
	if (iniFileOption)
	{
		m_hardwareOptionsDesired |= SM_CREATE_ALL_2D_IN_SOFTWARE;
		m_hardwareOptionsUsed |= SM_CREATE_ALL_2D_IN_SOFTWARE;
	}
	else
	{
		m_hardwareOptionsDesired &= ~SM_CREATE_ALL_2D_IN_SOFTWARE;
		m_hardwareOptionsUsed &= ~SM_CREATE_ALL_2D_IN_SOFTWARE;
	}

	opt_get_u32(DACOM, profile_parser, "SoundManager", "speakerConfiguration", SM_SPEAKER_STEREO, &iniFileOption);
	m_speakerConfiguration = iniFileOption;

	opt_get_u32(DACOM, profile_parser, "SoundManager", "maxSoundChannels", m_maxChannels, &iniFileOption);
	m_maxChannels = iniFileOption;

	/// debug stuff that can be added to the ini file
	opt_get_u32(DACOM, profile_parser, "SoundManager", "use2DHW", (m_hardwareOptionsDesired & SM_USE_2D_HARDWARE), &iniFileOption);
	if (iniFileOption)
		m_hardwareOptionsDesired |= (SM_USE_2D_HARDWARE);
	else
		m_hardwareOptionsDesired &= (~SM_USE_2D_HARDWARE);

	opt_get_u32(DACOM, profile_parser, "SoundManager", "use3DHW", (m_hardwareOptionsDesired & SM_USE_3D_HARDWARE), &iniFileOption);
	if (iniFileOption)
		m_hardwareOptionsDesired |= (SM_USE_3D_HARDWARE);
	else
		m_hardwareOptionsDesired &= (~SM_USE_3D_HARDWARE);

	// initialize direct sound and adjust buffer creation flags if needed
	if SUCCEEDED(initialize_direct_sound())
	{
		// has to be set after DSound is initialized
		set_speaker_configuration(m_speakerConfiguration);
		// set the initial listener parameters
		set_listener_parameters();
		return GR_OK;
	}
	return GR_GENERIC;
}

void SoundManager::clean_archetype_references(U32 archetype)
{
	// stop and delete all instances of this archetype
	InstanceList::iterator itr = m_playList.begin();
	while (itr != m_playList.end())
	{
		if (archetype == (*itr).soundSource->get_archetype())
		{
			itr = m_playList.erase(itr);
		}
		else
		{
			itr++;
		}
	}
}

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

GENRESULT DACOM_API SoundManager::shutdown()
{
	erase_archetypes();

	if (LoopNotificationThread)
	{
		ExitReq = 1;
		CloseHandle(LoopNotificationThread);
		LoopNotificationThread = NULL;
	}

	if (!m_playList.empty())
	{
		m_playList.clear();
	}
	if (!m_activeSoundList.empty())
	{
		m_activeSoundList.clear();
	}

	// no more references to any ISoundSources at this point

	if (m_hardwareOptionsUsed & SM_USE_EAX) // reset EAX and/or A3D properties to their original values
		set_master_reverb(m_originalReverb.baseEnv, m_originalReverb.damping, m_originalReverb.decay, m_originalReverb.vol);

	if (m_lpdsPrimaryBuffer)
	{
		m_lpdsPrimaryBuffer->Release();
		m_lpdsPrimaryBuffer = NULL;
	}
	if (m_lpdsListener)
	{
		m_lpdsListener->Release();
		m_lpdsListener = NULL;
	}
	if (m_lpds)
	{
		m_lpds->Release();
		m_lpds = NULL;
		CoUninitialize();
	}
	return GR_OK;
}

GENRESULT SoundManager::get_directsound_interface(void** lpds)
{
	if (m_lpds)
	{
		*lpds = m_lpds;
		((LPDIRECTSOUND)*lpds)->AddRef();
		return GR_OK;
	}
	else
	{
		*lpds = NULL;
		return GR_GENERIC;
	}
}

U32 SoundManager::get_current_time_ms()
{
	return GetTickCount();
}

GENRESULT SoundManager::get_device_info(SM_DEVICEINFO* info)
{
	// these are the values that were set when this instance of soundManager was created
	info->deviceOptionsUsed = m_hardwareOptionsUsed;
	info->numHWChannels = m_maxChannels;
	info->numHW3DBuffers = m_numHardware3DBuffers;

	// get current buffers free
	DSCAPS caps;
	caps.dwSize = sizeof(DSCAPS);
	m_lpds->GetCaps(&caps);
	info->numHWChannelsFree = caps.dwFreeHw3DAllBuffers;
	info->numHW3DBuffersFree = caps.dwFreeHwMixingAllBuffers;
	info->deviceGUID = m_deviceGuid;

	return GR_OK;
}

void DACOM_API SoundManager::update(ISoundListener* IEar)
{
	bool needToUpdateThread = false;

#ifdef _DEBUG
	if (debugStopProcessingSounds)
		return;
#endif

	U32 playingSounds = 0;
	HRESULT r = 0;
	m_currentTime = GetTickCount();

	// update the listener paramaters id an IEar * was passed in
	if (IEar)
	{
		update_listener_parameters(IEar);
	}

	// process app's active sound list: m_activeSoundList
	SoundSourceList::iterator activeItr;
	SoundSourceList::iterator activeEndItr = m_activeSoundList.end();
	InstanceList::iterator playItr = m_playList.begin();

	// remove all sounds from the list of playing sounds that aren't in the current active sound list
	while (playItr != m_playList.end())
	{
		bool found = false;
		activeItr = m_activeSoundList.begin();
		while (activeItr != activeEndItr)
		{
			if ((ISoundSource*)(*activeItr) == (ISoundSource*)(*playItr).soundSource)
			{
				found = true;
				break;
			}
			activeItr++;
		}
		if (found)
		{
			// check the next sound
			playItr++;
		}
		else
		{
			// remove this sound from the play list
			playItr = m_playList.erase(playItr);
		}
	}
	playItr = m_playList.begin();
	activeItr = m_activeSoundList.begin();

	// process app's active sound list, creating/updating sounds that should be played
	SoundInstance tempInstance, newInstance;
	U32 list_index = 0;

#if ALLOW_REMOVAL_FROM_HW
	bool need_to_prioritize = m_activeSoundList.size() > m_maxHardwareChannels;
#endif

	while (activeItr != activeEndItr)
	{
		tempInstance.soundSource = *activeItr;
		tempInstance.m_submitted_index = list_index;
		if ((playingSounds < m_maxChannels) && tempInstance.soundSource->is_on() && need_to_play(tempInstance))
		{
			++playingSounds;
			// on and playing, so process this one
			if (exists_in_list(*activeItr, m_playList, &playItr))							// exists, so update its info
			{
				// get all of the precalced info from the temp instance and reset the buffer flags
				bool inHW = ((*playItr).m_internal_flags & SMI_BUFFER_IN_HARDWARE) != 0;
				(*playItr).copy_instance_data(tempInstance);
				(*playItr).m_submitted_index = list_index;
				U32 buffer_flags = (*playItr).m_internal_flags;

				if (inHW)
				{
					(*playItr).m_internal_flags |= SMI_BUFFER_IN_HARDWARE;					// put the IN_HW flag back

#if ALLOW_REMOVAL_FROM_HW
					if (
						(need_to_prioritize && (list_index > m_maxHardwareChannels)) ||	// sounds priority is lower than the number of HW channels available
						(!(buffer_flags & SMI_BUFFER_USE_HW_BUFFER))					// HW sound that should now be SW
						)
					{
						// stop the sound so that it will be restarted again with changes
						(*playItr).m_lpSoundBuffer->Stop();
						// remove the HW flags so that it will be played again in software
						(*playItr).m_internal_flags &= ~(SMI_BUFFER_USE_HW_BUFFER);
					}
#endif	// ALLOW_REMOVAL_FROM_HW
				}
				else
				{
					// if this instance is in SW and it should be in HW, stop the buffer so it will get started in HW
					if (
						(!(buffer_flags & SMI_BUFFER_GIVE_UP_ON_HW)) &&	// not a SW buffer that previously couldn't make it into HW
						(buffer_flags & SMI_BUFFER_USE_HW_BUFFER)			// SW sound that should now be HW
						)
					{
						(*playItr).m_lpSoundBuffer->Stop();
					}
				}

			}
			else																			// not in play list, so add temp instance
			{
				create_instance(tempInstance, newInstance);									// create a new instance
				newInstance.m_submitted_index = list_index;									// set its index
				m_spliceList.push_back(newInstance);										// add it to the splice list (which will be added to the playing list later)
			}
			// the push_back copies the instance, so get rid of it
			// release newInstance's references
			if (newInstance.soundSource)
			{
				newInstance.soundSource->Release();
				newInstance.soundSource = NULL;
			}
			if (newInstance.m_lpSoundBuffer)
			{
				newInstance.m_lpSoundBuffer->Release();
				newInstance.m_lpSoundBuffer = NULL;
			}
		}
		else																				// sound shouldn't be playing, so don't process this sound
		{
			if (exists_in_list(*activeItr, m_playList, &playItr))
			{
				CRIT_SEC(&m_loopThreadLock);
				needToUpdateThread = true;													// make sure the thread knows about the deletion
				m_playList.erase(playItr);													// remove it from the list of playing sounds
			}
		}
		++activeItr;
		++list_index;
	}
	tempInstance.soundSource = NULL;

	if (!m_spliceList.empty())
	{
		CRIT_SEC(&m_loopThreadLock);
		needToUpdateThread = true;															// make sure the thread knows about the new additions to the list
		m_playList.splice(m_playList.end(), m_spliceList);									// add the new sounds to the currently playing sounds
	}

	// play list now only contains playing or ready to play sounds
	playItr = m_playList.begin();
	InstanceList::iterator playEndItr = m_playList.end();
	while (playItr != playEndItr)
	{
		// *** HW stuff is now deferred to the play() method (by passing the DSBPLAY_LOCHARDWARE flag)
		//		if ( ((*playItr).m_internal_flags & SMI_BUFFER_USE_HW_BUFFER) && (!((*playItr).m_internal_flags & SMI_BUFFER_IN_HARDWARE)) )
		//		{
		//			CRIT_SEC(&m_loopThreadLock);	// create_hardware_buffer can change the dsound buffer ptr
		//			needToUpdateThread = true;
		//			create_hardware_buffer(*playItr);
		//		}
				// update all of the data for all instances and then commit them below
		update_instance_data(*playItr);
		++playItr;
	}

	// all sounds should now have buffers and be ready to play

	// tell DSound to process all the instance's sound data (set by update_instance_data() )
	if (m_lpdsListener)
	{
		m_lpdsListener->CommitDeferredSettings();
	}

	DWORD playFlags = 0;		// currently no default play flags
	// play them all
	playItr = m_playList.begin();
	playEndItr = m_playList.end();

	while (playItr != playEndItr)
	{
		playFlags = ((*playItr).soundSource->is_looping()) ? DSBPLAY_LOOPING : 0;
		// correct play position if necessary
		if (update_position(*playItr))
		{
			if ((playFlags & DSBPLAY_LOOPING) && ((*playItr).m_DSOUND_buffer_flags & DSBCAPS_CTRLPOSITIONNOTIFY))
			{
				start_looping_sound_with_markers(*playItr);
			}
			// play the sound specifying HW or SW
			DWORD loc_flag = (*playItr).m_internal_flags & SMI_BUFFER_USE_HW_BUFFER ? DSBPLAY_LOCHARDWARE : DSBPLAY_LOCSOFTWARE;
			r = (*playItr).m_lpSoundBuffer->Play(0, 0, playFlags | loc_flag);

			if (DSBPLAY_LOCHARDWARE == loc_flag) // so add the IN_HW flag
			{
				(*playItr).m_internal_flags |= SMI_BUFFER_IN_HARDWARE;
			}

			if FAILED(r)
			{
				if (DSERR_BUFFERLOST == r) // someone switched apps and our buffers were lost, so restore them
				{
					if (restore_lost_buffer(*playItr))
					{
						r = (*playItr).m_lpSoundBuffer->Play(0, 0, playFlags | loc_flag);  // play the sound again
						if FAILED(r)
						{
							GENERAL_TRACE_1(TEMPSTR("SoundMgr:Playing restored buffer failed, result %X.\n", r));
						}
					}
				}
				// DSERR_HWUNAVAIL doen't seem to exist in the header, although it does in the docs, so use something else
				if (FAILED(r) && (DSBPLAY_LOCHARDWARE == loc_flag))// no HW resources for this sound, so try SW
				{
					r = (*playItr).m_lpSoundBuffer->Play(0, 0, playFlags | DSBPLAY_LOCSOFTWARE);  // play the sound w/ SW specified
					if SUCCEEDED(r)
					{
						// had to force SW, so mark this instance so HW won't be tried again for this instance
						(*playItr).m_internal_flags |= SMI_BUFFER_GIVE_UP_ON_HW;
						(*playItr).m_internal_flags &= ~SMI_BUFFER_IN_HARDWARE;
					}
				}

				if FAILED(r)
				{
					GENERAL_TRACE_1(TEMPSTR("SoundMgr:Play failed, result %X.\n", r));
				}
			}
		}
		++playItr;
	}
	if (needToUpdateThread)
	{
		SetEvent(ResetNotificationLoopEvent);
	}
}

GENRESULT DACOM_API SoundManager::unknownA(void* value)
{
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::unknownB(void* value)
{
	return GR_OK;
}


// ISoundArchetype methods implemented by SoundManager
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
		U32 result = ar->m_dsWaveFormat.nChannels;
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
			ar->Release();
		}
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

// SoundManager methods
GENRESULT SoundManager::init(AGGDESC* desc)
{
	return GR_OK;
}

GENRESULT DACOM_API SoundManager::Initialize()
{
	DACOM_Acquire()->QueryInterface(IID_IProfileParser, profile_parser);
	m_Initialized = true;
	return GR_OK;
}

GENRESULT SoundManager::initialize_direct_sound()
{
	if (m_lpds != NULL)
		return GR_GENERIC;

	if FAILED(CoInitialize(NULL))
		return GR_GENERIC;

	HRESULT r;

#if A3D_SUPPORTED
	LPIA3D2 pIA3d2 = NULL;

	// try and get an A3D driver for the sound card if an A3D card was selected
	if (m_hardwareOptionsDesired & SM_USE_A3D)
	{
		r = CoCreateInstance(CLSID_A3d, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound, (void**)&m_lpds);
		if SUCCEEDED(r)
		{
			if (SUCCEEDED(m_lpds->QueryInterface(IID_IA3d2, (void**)&pIA3d2)))
			{
				r = pIA3d2->RegisterVersion(A3D_CURRENT_VERSION);
				if SUCCEEDED(r)
				{
					m_hardwareOptionsUsed |= SM_USE_A3D;
				}
				else
				{	// found the A3D DLL, couldn't register version 
					m_lpds->Release();
					m_lpds = NULL;
					pIA3d2->Release();
					pIA3d2 = NULL;
					GENERAL_NOTICE("SoundManager: A3D.dll present, but card not available\n");
				}
			}
			else
			{	// found the A3D DLL, but no card, so get rid of it and use plain DSound
				m_lpds->Release();
				m_lpds = NULL;
				pIA3d2 = NULL;
				GENERAL_NOTICE("SoundManager: A3D.dll present, but card not available\n");
			}
		}
	}
#endif

	if (m_lpds == NULL)
		CoCreateInstance(CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound, (void**)&m_lpds);

	if (m_lpds)
	{
		r = m_lpds->Initialize(&m_deviceGuid);
		if FAILED(r)
		{
			GENERAL_WARNING(TEMPSTR("SoundManager:Couldn't create DSound device %s.\n", m_deviceGuidString));
			if FAILED(m_lpds->Initialize(NULL)) // try the default device
			{
				GENERAL_WARNING("SoundManager:Couldn't initialize default DSound device.\n");
				m_lpds->Release();
				m_lpds = NULL;
				return GR_GENERIC;
			}
		}

		m_lpds->SetCooperativeLevel(m_hWnd, DSSCL_PRIORITY);

		DSCAPS caps;
		caps.dwSize = sizeof(DSCAPS);
		m_lpds->GetCaps(&caps);

		m_numHardware3DBuffers = caps.dwFreeHw3DAllBuffers;
		m_maxHardwareChannels = caps.dwFreeHwMixingAllBuffers;

		// if there are channels available but none set, set the max to the number available
		if ((m_maxHardwareChannels + m_numHardware3DBuffers) && !m_maxChannels)
		{
			m_maxChannels = (m_maxHardwareChannels > m_numHardware3DBuffers) ? m_maxHardwareChannels : m_numHardware3DBuffers;
			if (m_maxChannels < 8)
				m_maxChannels = 8;
		}

		DSBUFFERDESC desc;
		memset(&desc, 0, sizeof(DSBUFFERDESC));
		desc.dwSize = sizeof(DSBUFFERDESC);
		desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

		// set the CTRL3D flag on the primary buffer if 3D was requested
		if ((m_hardwareOptionsUsed & SM_USE_3D_HARDWARE) || (m_DS3D_SW_ALG_GUID != GUID_NULL))
		{
			desc.dwFlags |= DSBCAPS_CTRL3D;
		}

		m_lpds->CreateSoundBuffer(&desc, &m_lpdsPrimaryBuffer, NULL);
		if (m_lpdsPrimaryBuffer)
		{
			// intially, disable hardware then check for desired options
			m_hardwareOptionsUsed &= ~(SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE);
			// see if this is emulated OR there are no 3D capabilities OR there is a penalty for creating buffers
			// if any of these conditions are true, then disable HW
			if ((caps.dwFlags & DSCAPS_EMULDRIVER) || (caps.dwUnlockTransferRateHwBuffers))
			{
				m_hardwareOptionsDesired &= ~(SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE);
				GENERAL_NOTICE(TEMPSTR("ISA or slow sound card detected. Disabling HW buffers.\n"));
			}
			else
			{
				// Try to create a normal (2D) hardware buffer
				if (m_hardwareOptionsDesired & SM_USE_2D_HARDWARE)
				{
					// try to create a hw buffer
					WAVEFORMATEX fmt = { WAVE_FORMAT_PCM,2,22050,88200,4,16,0 };
					GenericDSoundBuffer dummyBuffer(&fmt);
					LPDIRECTSOUNDBUFFER pBuffer = dummyBuffer.pBuffer;
					dummyBuffer.desc.dwFlags = DSBCAPS_LOCHARDWARE;
					if SUCCEEDED(m_lpds->CreateSoundBuffer(&dummyBuffer.desc, &pBuffer, NULL))
					{
						m_hardwareOptionsUsed |= SM_USE_2D_HARDWARE;
					}
				}

				// Try to create a normal (2D) hardware buffer
				if (m_hardwareOptionsDesired & SM_USE_3D_HARDWARE)
				{
					// try to create a hw buffer
					WAVEFORMATEX fmt = { WAVE_FORMAT_PCM,2,22050,88200,4,16,0 };
					GenericDSoundBuffer dummyBuffer(&fmt);
					LPDIRECTSOUNDBUFFER pBuffer = dummyBuffer.pBuffer;
					dummyBuffer.desc.dwFlags = DSBCAPS_LOCHARDWARE | DSBCAPS_CTRL3D;
					if SUCCEEDED(m_lpds->CreateSoundBuffer(&dummyBuffer.desc, &pBuffer, NULL))
					{
						m_hardwareOptionsUsed |= SM_USE_3D_HARDWARE;
					}
				}
			}


			U32 blockAlign = (m_primaryNumChannels * m_primaryBitsPerSample) / 8;
			U32 bytesPerSec = (m_primarySampleRate * blockAlign);
			if (desc.dwFlags & DSBCAPS_CTRL3D)
			{
				m_lpdsPrimaryBuffer->QueryInterface(IID_IDirectSound3DListener, (void**)&m_lpdsListener);
			}
			WAVEFORMATEX fmt = { WAVE_FORMAT_PCM,(WORD)m_primaryNumChannels,m_primarySampleRate,bytesPerSec,(WORD)blockAlign,(WORD)m_primaryBitsPerSample,0 };
			m_lpdsPrimaryBuffer->SetFormat(&fmt);
			set_master_reverb(m_reverb.baseEnv, m_reverb.vol, m_reverb.decay, m_reverb.damping); // turn off any current EAX reverb settings
			//			m_lpdsPrimaryBuffer->Play(0,0,DSBPLAY_LOOPING); // uncomment to keep mixer always running

#if A3D_SUPPORTED
			if (pIA3d2)
			{
				// set the A3D speaker settings
				S32 FrontXtalkMode, RearXtalkMode;
				S32 OutputMode = OUTPUT_MODE_STEREO;
				switch (m_speakerConfiguration)
				{
				case SM_SPEAKER_HEADPHONE:
					FrontXtalkMode = OUTPUT_HEADPHONES;
					RearXtalkMode = OUTPUT_HEADPHONES;
					break;
				case SM_SPEAKER_MONO:
				case SM_SPEAKER_STEREO:
				case SM_SPEAKER_STEREO_MIN:
				case SM_SPEAKER_STEREO_NARROW:
					FrontXtalkMode = OUTPUT_SPEAKERS_NARROW;
					RearXtalkMode = OUTPUT_SPEAKERS_NARROW;
					break;
				case SM_SPEAKER_STEREO_WIDE:
				case SM_SPEAKER_STEREO_MAX:
					FrontXtalkMode = OUTPUT_SPEAKERS_WIDE;
					RearXtalkMode = OUTPUT_SPEAKERS_WIDE;
					break;
				case SM_SPEAKER_QUAD:
				case SM_SPEAKER_SURROUND:
				case SM_SPEAKER_5POINT1:
					FrontXtalkMode = OUTPUT_SPEAKERS_NARROW;
					RearXtalkMode = OUTPUT_SPEAKERS_WIDE;
					OutputMode = OUTPUT_MODE_QUAD;
					break;
				default:
					ASSERT("INVALID SPEAKER CONFIGURATION" && 0);
				}
				pIA3d2->SetOutputMode(FrontXtalkMode, RearXtalkMode, OutputMode);
				pIA3d2->Release();
			}
#endif //A3D_SUPPORTED

			report_audio_options();
			GENERAL_TRACE_1("SoundManager: SoundManager successfully initialized.\n");
			return GR_OK;
		}
		GENERAL_NOTICE("SoundManager: Couldn't create primary sound buffer.\n");
	}
	// something didn't work, so get rid of DSound
	if (m_lpds)
	{
		m_lpds->Release();
		m_lpds = NULL;
	}
	return GR_GENERIC;
}

void SoundManager::report_audio_options()
{
	GENERAL_NOTICE(TEMPSTR("SoundMgr:2D HW%s requested and %s with %d HW buffers available.\n",
		((m_hardwareOptionsDesired & SM_USE_2D_HARDWARE) ? "" : " NOT"),
		((m_hardwareOptionsUsed & SM_USE_2D_HARDWARE) ? "ENABLED" : "DISABLED"),
		m_maxHardwareChannels));

	GENERAL_NOTICE(TEMPSTR("SoundMgr:3D HW%s requested and %s with %d 3D HW buffers available.\n",
		((m_hardwareOptionsDesired & SM_USE_3D_HARDWARE) ? "" : " NOT"),
		((m_hardwareOptionsUsed & SM_USE_3D_HARDWARE) ? "ENABLED" : "DISABLED"),
		m_numHardware3DBuffers));

	if (m_hardwareOptionsUsed & SM_USE_3D_SOFTWARE_FULL)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Using Full Algorithm for DSound3D SW buffers\n"));
	}
	else if (m_hardwareOptionsUsed & SM_USE_3D_SOFTWARE_LIGHT)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Using Light Algorithm for DSound3D SW buffers\n"));
	}
	else if (m_hardwareOptionsUsed & SM_USE_3D_SOFTWARE_BASIC)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Using Basic Algorithm for DSound3D SW buffers\n"));
	}
	else if (m_hardwareOptionsUsed & SM_USE_3D_SOFTWARE_USER_DEFINED)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Using User Default for DSound3D SW buffers\n"));
	}
	else if (m_hardwareOptionsUsed & SM_USE_3D_SOFTWARE_NONE)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Disabling DSound3D for SW buffers\n"));
	}

	if (m_hardwareOptionsUsed & SM_USE_RIGHT_HANDED_SYSTEM)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Using Right Handed Coordinates\n"));
	}
	if (m_hardwareOptionsUsed & SM_CREATE_ALL_2D_IN_SOFTWARE)
	{
		GENERAL_NOTICE(TEMPSTR("SoundMgr:Forcing all 2D buffers to SW\n"));
	}

	GENERAL_NOTICE(TEMPSTR("SoundMgr:EAX %s\n",
		((m_hardwareOptionsUsed & SM_USE_EAX) ? "ENABLED" : "DISABLED")));

#if A3D_SUPPORTED
	GENERAL_NOTICE(TEMPSTR("SoundMgr:A3D %s\n",
		((m_hardwareOptionsUsed & SM_USE_A3D) ? "ENABLED" : "DISABLED")));
#endif

	GENERAL_NOTICE(TEMPSTR("SoundMgr:Max channels set to %d.\n", m_maxChannels));
}

/*
GENRESULT SoundManager::create_hardware_buffer(SoundInstance &instance)
{
	// if this buffer is currently being looped w/ markers, leave it alone - just return
	if (instance.m_internal_flags & (SMI_BUFFER_USING_NOTIFICATION | SMI_BUFFER_GIVE_UP_ON_HW))
	{
		return GR_GENERIC;
	}

	DSCAPS caps;					// current device caps
	caps.dwSize = sizeof(DSCAPS);
	m_lpds->GetCaps(&caps);			// fill in caps to get available buffers

	if ( (caps.dwFreeHw3DAllBuffers) || (caps.dwFreeHwMixingAllBuffers) )
	{
		SOUND_ARCH* sarch;

		if (query_archetype (instance.soundSource->get_archetype(), sarch))
		{
			LPDIRECTSOUNDBUFFER ExistingBuffer = sarch->m_lpHardwareBuffer;
			LPDIRECTSOUNDBUFFER HWBuffer;
			HRESULT r = DSERR_GENERIC;
			if (ExistingBuffer)
			{
				if (instance.m_internal_flags & SMI_BUFFER_USE_HW_3D_BUFFER)
				{
					r = m_lpds->DuplicateSoundBuffer(ExistingBuffer, &HWBuffer);
					if (SUCCEEDED(r))
					{
						sarch->add_instance_hw_buffer();
					}
					else
					{
						GENERAL_TRACE_1( TEMPSTR("SoundMan: Duplicate 3D HW Buffer failed with %d 3D buffers free\n",caps.dwFreeHw3DAllBuffers));
					}
				}
			}
			if (FAILED(r))
			{
				DSBUFFERDESC dsbdesc;
				WAVEFORMATEX format;
				memcpy(&format, &sarch->m_dsWaveFormat, sizeof(WAVEFORMATEX));

				memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
				dsbdesc.dwSize = sizeof(DSBUFFERDESC);

				// clear the SW flag
				instance.m_DSOUND_buffer_flags &= ~DSBCAPS_LOCSOFTWARE;
				// add the default flags
				instance.m_DSOUND_buffer_flags |= g_bufferFlags;

				// add freq control if requested
				if (sarch->m_bufferFlags & SM_ENABLE_FREQUENCY_CONTROL)
				{
					instance.m_DSOUND_buffer_flags |= DSBCAPS_CTRLFREQUENCY;
				}

				// add mute at max distance if 3D and requested
				if ( (instance.m_DSOUND_buffer_flags & DSBCAPS_CTRL3D) && (sarch->m_bufferFlags & SM_MUTE_3D_AT_MAX_DISTANCE))
				{
					instance.m_DSOUND_buffer_flags |= DSBCAPS_MUTE3DATMAXDISTANCE;
				}

				dsbdesc.dwBufferBytes = sarch->m_soundFile.length;
				dsbdesc.lpwfxFormat = &format;

				dsbdesc.dwFlags = instance.m_DSOUND_buffer_flags;
				if SUCCEEDED(m_lpds->CreateSoundBuffer(&dsbdesc, &HWBuffer, NULL))
				{
					r = write_data_to_buffer(&sarch->m_soundFile, HWBuffer, sarch->m_soundFile.length);
					if (SUCCEEDED(r))
					{
						if (!sarch->m_lpHardwareBuffer)
						{
							sarch->m_lpHardwareBuffer = HWBuffer;
							sarch->m_lpHardwareBuffer->AddRef();
							sarch->add_instance_hw_buffer();
						}
					}
					else
					{
						GENERAL_WARNING( TEMPSTR("SoundMan: CreateHWBuffer failed to copy sample data\n"));
						HWBuffer->Release();
						HWBuffer = NULL;
					}
				}
///				else
//				{
//					if (instance.m_internal_flags & SMI_BUFFER_USE_HW_3D_BUFFER)
//						GENERAL_TRACE_1( TEMPSTR("SoundMan: Create 3D HW Buffer failed with %d 3D buffers free\n",caps.dwFreeHw3DAllBuffers));
//					else
//						GENERAL_TRACE_1( TEMPSTR("SoundMan: Create 2D HW Buffer failed with %d HW buffers free\n",caps.dwFreeHwMixingAllBuffers));
//				}
			}
			if SUCCEEDED(r)
			{
				if (instance.m_lpSoundBuffer)
				{
					instance.m_lpSoundBuffer->Release();
					instance.m_lpSoundBuffer = NULL;
				}
				instance.m_lpSoundBuffer = HWBuffer; // create calls addref, so no need to addref one and release the other
				instance.m_internal_flags |= SMI_BUFFER_IN_HARDWARE;
				instance.m_archetype = sarch;
				sarch->Release ();
				return GR_OK;
			}
			else
			{
				instance.m_internal_flags |= SMI_BUFFER_GIVE_UP_ON_HW;
			}

			sarch->Release ();
		}
	}
	return GR_GENERIC;
}
*/

// Determine whether this sound should be put in HW (or remain in HW if already created in HW)
void SoundManager::check_hardware_buffer_status(SoundInstance& instance)
{
	// default to no HW for the instance
	instance.m_internal_flags &= ~SMI_BUFFER_USE_HW_BUFFER;

	// if m_loopEventHandle != NULL then this instance is looping between markers, so don't mess with it
	if ((instance.m_internal_flags & SMI_BUFFER_GIVE_UP_ON_HW) || (instance.m_loopEventHandle != NULL))
	{
		// don't event try
		return;
	}

	if (instance.soundSource->is_3D())										// 3D sound
	{
		instance.m_internal_flags &= ~SMI_DISABLE_3D;						// get rid of the disable flag - used when updating dsound
		// initialize the values
		instance.m_cachedPositionV = m_listenerPosition;
		instance.m_cachedMin = 10;
		instance.m_cachedMax = 500;
		// get the current values from the soundsource
		instance.soundSource->get_position(&instance.m_cachedPositionV);
		instance.soundSource->get_max_distance(&instance.m_cachedMax);
		instance.soundSource->get_min_distance(&instance.m_cachedMin);
		// calc the other related values
		instance.m_cachedPositionV.z *= m_simpleCoordTransformation;
		instance.m_cachedMaxSquared = instance.m_cachedMax * instance.m_cachedMax;
		instance.m_cachedMinSquared = instance.m_cachedMin * instance.m_cachedMin;
		instance.m_cachedSoundPanV.set(instance.m_cachedPositionV.x - m_listenerPosition.x, instance.m_cachedPositionV.y - m_listenerPosition.y, instance.m_cachedPositionV.z - m_listenerPosition.z);
		instance.m_cachedDistanceSquared = instance.m_cachedSoundPanV.magnitude_squared();

		// if HW is available, see if this instance gets a 3D HW buffer
		if (m_hardwareOptionsUsed & SM_USE_3D_HARDWARE)
		{
			// if all HW buffers are 3D or the sound is closer than half max distance try to use 3D HW
			if ((m_maxHardwareChannels == m_numHardware3DBuffers) ||
				((instance.m_cachedDistanceSquared)) < (instance.m_cachedMinSquared * m_hw_threshold_multiplier_squared))
			{
				instance.m_internal_flags |= SMI_BUFFER_USE_HW_BUFFER;
			}
		}
	}
	else																		// 2D sound
	{
		// if the app didn't specify all 2D sounds are created in SW and
		// the card is capable of HW, put this instance in HW
		instance.m_internal_flags |= SMI_DISABLE_3D;							// add the disable flag
		if (
			(!(m_hardwareOptionsUsed & SM_CREATE_ALL_2D_IN_SOFTWARE)) &&	// app didn't specify all 2D are SW
			(m_hardwareOptionsUsed & SM_USE_2D_HARDWARE))					// AND 2D HW is available
		{
			instance.m_internal_flags |= SMI_BUFFER_USE_HW_BUFFER;
		}
	}
}

// returns true if the sound should be playing
inline bool SoundManager::need_to_play(SoundInstance& instance)
{
	// IMPORTANT: in_range() updates instance data (and the cached data) and checks the HW status of the instance
	return ((!finished(instance)) &&										// sound hasn't finished
		(m_currentTime >= instance.soundSource->get_start_time()) &&	// sound should have started
		(in_range(instance)));											// sound is audible
}

// returns true if the sound is still playing
// (takes freq changed into consideration)
inline bool SoundManager::finished(const SoundInstance& instance)
{
	bool result = true;

	if (instance.soundSource->is_looping())
	{
		result = false;		// looping sounds are never finished
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

			// scale the sounds duration based on the freq change
			result = m_currentTime > (instance.soundSource->get_start_time() + (ar->m_msDuration / freqFactor));
			ar->Release();
		}
	}

	return result;
}

inline bool SoundManager::in_range(SoundInstance& instance)
{
	// this call determines whether this sound should be put in a HW buffer
	// *** IMPORTANT this call also updates cached values for the sound
	check_hardware_buffer_status(instance);

	if ((!(instance.m_DSOUND_buffer_flags & DSBCAPS_MUTE3DATMAXDISTANCE)) ||		// if app doesn't want sounds muted at max dist, return true
		(!(instance.soundSource->is_3D())))					// 2D sounds are always in range
	{
		return true;
	}

	// check that this sound is closer than max distance
	return (instance.m_cachedDistanceSquared < instance.m_cachedMaxSquared);
}

// returns the position of the instance in the specified list (or 0 if not found) and optionally returns the existing instance
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

// creates a new instance of a sound and adds the instance to the archetypes internal list
GENRESULT SoundManager::create_instance(SoundInstance& instance, SoundInstance& newInstance)
{
	SOUND_ARCH* ar;

	SOUND_ARCH_INDEX archetype = instance.soundSource->get_archetype();

	if (query_archetype(archetype, ar))
	{
		HRESULT r = m_lpds->DuplicateSoundBuffer(ar->m_lpSoundBuffer, &newInstance.m_lpSoundBuffer);
		if SUCCEEDED(r)
		{
			newInstance.copy_instance_data(instance);
			newInstance.soundSource = instance.soundSource;
			newInstance.soundSource->AddRef();

			// get the archetype's buffer flags
			DSBCAPS bufferCaps;
			memset(&bufferCaps, 0, sizeof(DSBCAPS));
			bufferCaps.dwSize = sizeof(DSBCAPS);
			ar->m_lpSoundBuffer->GetCaps(&bufferCaps);

			// copy the buffer flags from the archetype's buffer
			newInstance.m_DSOUND_buffer_flags = bufferCaps.dwFlags;

			if (!(newInstance.m_DSOUND_buffer_flags & DSBCAPS_CTRL3D))
			{
				newInstance.m_internal_flags |= SMI_DISABLE_3D;
			}

			// store the loop region if reqested
			if (newInstance.m_DSOUND_buffer_flags & DSBCAPS_CTRLPOSITIONNOTIFY)
			{
				newInstance.m_loopStartOffset = ar->m_soundFile.loop_start * ar->m_soundFile.format.bytes_per_sample;
				newInstance.m_loopEndOffset = ar->m_soundFile.loop_end * ar->m_soundFile.format.bytes_per_sample;
			}

			newInstance.m_archetype = ar;
			ar->Release();
			return GR_OK;
		}

		ar->Release();
	}
	GENERAL_WARNING(TEMPSTR("SoundMgr:Couldn't create an instance of archetype %d.\n", archetype));
	return GR_GENERIC;
}

// copies the sample to data to the specified secondary buffer
GENRESULT SoundManager::write_data_to_buffer(SoundFile* sourceData, LPDIRECTSOUNDBUFFER lpdsBuffer, U32 length)
{
	DWORD dwWriteCursor = 0;
	DWORD dwWriteBytes = length;
	LPVOID lplpvAudioPtr1 = NULL;
	DWORD lpdwAudioBytes1 = 0;
	LPVOID lplpvAudioPtr2 = NULL;
	DWORD lpdwAudioBytes2 = 0;
	DWORD dwFlags = 0;

	if (!length) // copy entire buffer if no length specified
		dwFlags |= DSBLOCK_ENTIREBUFFER;

	if SUCCEEDED(lpdsBuffer->Lock(dwWriteCursor, dwWriteBytes, &lplpvAudioPtr1, &lpdwAudioBytes1,
		&lplpvAudioPtr2, &lpdwAudioBytes2,
		dwFlags))
	{
		memcpy(lplpvAudioPtr1, sourceData->samples, lpdwAudioBytes1);
		if (lpdwAudioBytes2) // this should always be null, because these are allways static buffers
			memcpy(lplpvAudioPtr2, (char*)sourceData->samples + lpdwAudioBytes1, lpdwAudioBytes2);
		lpdsBuffer->Unlock(lplpvAudioPtr1, lpdwAudioBytes1, lplpvAudioPtr2, lpdwAudioBytes2);
		return GR_OK;
	}
	GENERAL_ERROR("Couldn't lock sound buffer.\n");
	return GR_GENERIC;
}

// restores buffers lost by another app, with primary buffer write access, taking over direct sound
inline bool SoundManager::restore_lost_buffer(SoundInstance& instance)
{
	if (DS_OK == instance.m_lpSoundBuffer->Restore())
	{
		SOUND_ARCH* ar;

		if (query_archetype(instance.soundSource->get_archetype(), ar))
		{
			bool result = SUCCEEDED(write_data_to_buffer(&ar->m_soundFile, instance.m_lpSoundBuffer, ar->m_soundFile.length));
			ar->Release();
			return result;
		}
	}
	return false;
}

// get the current data for this instance and update the appropriate ds3d stuff
void SoundManager::update_instance_data(SoundInstance& instance)
{
	SINGLE attenuation = 0.0f;
	SINGLE freqFactor = 1.0f;
	DWORD freq;
	LPDIRECTSOUND3DBUFFER lpDs3dBuffer = NULL;
	S32 soundApplyMode = DS3D_DEFERRED;

	// get the 3D buffer interface
	instance.m_lpSoundBuffer->QueryInterface(IID_IDirectSound3DBuffer, (LPVOID*)&lpDs3dBuffer);

	// not a 3d sound, so set the pan
	if (instance.m_internal_flags & SMI_DISABLE_3D)
	{
		// disable 3D if it has been turned off
		if (lpDs3dBuffer)
			lpDs3dBuffer->SetMode(DS3DMODE_DISABLE, soundApplyMode);
		// get the current pan amount and, if successful, set it
		S32 pan;
		if SUCCEEDED(instance.soundSource->get_pan(&pan))
		{
			pan *= 100;
			pan = (pan < DSBPAN_LEFT) ? DSBPAN_LEFT : ((pan > DSBPAN_RIGHT) ? DSBPAN_RIGHT : pan);
			HRESULT hr = instance.m_lpSoundBuffer->SetPan(pan);
			unused(hr);
		}
	}
	else
	{
		if (lpDs3dBuffer)	// have a 3D interface, so set all of the properties
		{
			// this is a hardware buffer or the app wants to use ds3d for software also, so set every parameter
			DWORD dw1, dw2;
			SINGLE s;
			S32 u;
			Vector v;

			instance.soundSource->get_apply_mode(&soundApplyMode);  // update now or defer until all are updated

			if SUCCEEDED(instance.soundSource->get_sound_mode(&u)) // normal, no 3d, or head releative
				lpDs3dBuffer->SetMode(u, soundApplyMode);

			// only get the rest of the values if DS3D is not disabled
			if (u != DS3DMODE_DISABLE)
			{
				if SUCCEEDED(instance.soundSource->get_cone_angles(&dw1, &dw2))
				{
					lpDs3dBuffer->SetConeAngles(dw1, dw2, soundApplyMode);
					v.set(0, 0, 0);
					instance.soundSource->get_cone_orientation(&v);
					v.z *= m_simpleCoordTransformation;
					lpDs3dBuffer->SetConeOrientation(v.x, v.y, v.z, soundApplyMode);
					s = DS3D_DEFAULTCONEOUTSIDEVOLUME;
					instance.soundSource->get_cone_outside_attenuation(&s);
					lpDs3dBuffer->SetConeOutsideVolume(s * 100, soundApplyMode);
				}

				if SUCCEEDED(instance.soundSource->get_velocity(&v))
				{
					v.z *= m_simpleCoordTransformation;
					lpDs3dBuffer->SetVelocity(v.x, v.y, v.z, soundApplyMode);
				}

				lpDs3dBuffer->SetMinDistance(instance.m_cachedMin, soundApplyMode);
				lpDs3dBuffer->SetMaxDistance(instance.m_cachedMax, soundApplyMode);
				lpDs3dBuffer->SetPosition(instance.m_cachedPositionV.x, instance.m_cachedPositionV.y, instance.m_cachedPositionV.z, soundApplyMode);
			}
		}
	}

	// set frequency if enabled
	float frequency;
	if ((instance.m_archetype->m_bufferFlags & SM_ENABLE_FREQUENCY_CONTROL) && SUCCEEDED(instance.soundSource->get_frequency(&frequency)))
	{
		freqFactor = SoundManager::calculate_frequency_factor(frequency);
		if (SUCCEEDED(instance.m_lpSoundBuffer->GetFrequency(&freq)))
		{
			SOUND_ARCH* ar;
			query_archetype(instance.soundSource->get_archetype(), ar);
			U32 sampleRate = ar->m_dsWaveFormat.nSamplesPerSec;
			ar->Release();

			U32 frequency = freqFactor * sampleRate;
			frequency = (frequency > DSBFREQUENCY_MAX) ? DSBFREQUENCY_MAX : ((frequency < DSBFREQUENCY_MIN) ? DSBFREQUENCY_MIN : frequency);
			instance.m_lpSoundBuffer->SetFrequency(frequency);
		}
	}

	// calculate and set the attenuation for this instance
	instance.soundSource->get_attenuation(&attenuation);
	SINGLE totalAttenuation = 100 * (attenuation + instance.m_archetype->m_baseAttenuation + m_masterAttenuation);
	totalAttenuation = (totalAttenuation > DSBVOLUME_MAX) ? DSBVOLUME_MAX : ((totalAttenuation < DSBVOLUME_MIN) ? DSBVOLUME_MIN : totalAttenuation);

	instance.m_lpSoundBuffer->SetVolume(totalAttenuation);

	// set the EAX parameters for this sound if enabled
	if (m_hardwareOptionsUsed & SM_USE_EAX) // EAX was succesfully initialized, so get reverbMix for this instance
	{
		SINGLE reverbMix = 0.0f;	// no reverb
		if (GR_OK == (instance.soundSource->get_reverb_mix(&reverbMix)))
		{
			LPKSPROPERTYSET pEAX;
			if (instance.m_lpSoundBuffer->QueryInterface(IID_IKsPropertySet, (void**)&pEAX) == DS_OK)
			{
				DWORD support = 0;
				pEAX->QuerySupport(DSPROPSETID_EAXBUFFER_ReverbProperties, DSPROPERTY_EAXBUFFER_REVERBMIX, &support);
				if (support & (KSPROPERTY_SUPPORT_SET))
				{
					pEAX->Set(DSPROPSETID_EAXBUFFER_ReverbProperties, DSPROPERTY_EAXBUFFER_REVERBMIX, NULL, 0, &reverbMix, sizeof(SINGLE));
				}
				pEAX->Release();
			}
		}
	}

	if (lpDs3dBuffer)
	{
		lpDs3dBuffer->Release();
	}
}

// set the play cursor to the appropriate place in the buffer
// this currently only modifies sounds that aren't playing or whose start time has been changed since the sound started playing
// returns false if the sound shouldn't be play()ed (already started or has expired)
bool SoundManager::update_position(SoundInstance& instance)
{
	U32 startTime = instance.soundSource->get_start_time();

	// see if the sound is playing
	if (startTime == instance.m_startTime)
	{
		DWORD status;
		HRESULT hr = instance.m_lpSoundBuffer->GetStatus(&status);
		if (SUCCEEDED(hr) && (status & (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING)))
		{
			// don't recalculate position if currently playing and start time hasn't changed
			return false;
		}
	}

	SOUND_ARCH* ar;
	if (query_archetype(instance.soundSource->get_archetype(), ar))
	{
		U32			offset;
		SINGLE		freqFactor = 1.0f;
		SoundFile* soundFile = &ar->m_soundFile;					// get the sound format from the archetype
		U32			msecs = m_currentTime - instance.soundSource->get_start_time();

		DWORD freq;	// just used to test the GetFrequency function (even though the buffer might have been created w/ the freq flag, it might not be available)
		if ((instance.m_DSOUND_buffer_flags & DSBCAPS_CTRLFREQUENCY) && SUCCEEDED(instance.m_lpSoundBuffer->GetFrequency(&freq)))
		{
			float frequency;
			if (SUCCEEDED(instance.soundSource->get_frequency(&frequency)))
			{
				freqFactor = SoundManager::calculate_frequency_factor(frequency);
			}
		}

		// adjust the time for the frequency
		msecs *= freqFactor;

		// calc how many samples to skip
		offset = msecs * soundFile->format.samples_per_sec / 1000; // samples to offset

		// convert the offset from samples to bytes
		offset *= soundFile->format.bytes_per_sample;


		if (soundFile->length <= offset)
		{
			if (instance.soundSource->is_looping())
			{
				// looping sound, so
				// subtract the length to calc how far into the loop the sound should be starting the sound
				offset -= soundFile->length;
				offset = soundFile->loop_start + (offset % (soundFile->loop_end - soundFile->loop_start)); // bytes to offset
			}
			else
			{
				// not looping and the offset is greater than the length of the file, so just return false (and don't play the sound)
				// should have been caught by is_finished()
				return false;
			}
		}

		if (offset) // if we still have an offset, then set the play position
		{
			instance.m_lpSoundBuffer->SetCurrentPosition(offset);
		}
		instance.m_startTime = startTime;  // next time we'll know we already playing this sound and whether the sound has a new start time

		ar->Release();

		return true;
	}
	return false;
}

// 
// This function sets the initial listener parameters
void SoundManager::set_listener_parameters()
{
	if (m_lpdsListener)
	{
		m_lpdsListener->SetOrientation(m_listenerFront.x, m_listenerFront.y, m_listenerFront.z,
			m_listenerUp.x, m_listenerUp.y, m_listenerUp.z, DS3D_DEFERRED);
		m_lpdsListener->SetPosition(m_listenerPosition.x, m_listenerPosition.y, m_listenerPosition.z, DS3D_DEFERRED);
		m_lpdsListener->SetVelocity(m_listenerVelocity.x, m_listenerVelocity.y, m_listenerVelocity.z, DS3D_DEFERRED);
		m_lpdsListener->SetDistanceFactor(m_listenerDistanceFactor, DS3D_DEFERRED);
		m_lpdsListener->SetDopplerFactor(m_listenerDopplerFactor, DS3D_DEFERRED);
		m_lpdsListener->SetRolloffFactor(m_listenerRolloffFactor, DS3D_DEFERRED);
	}
}

// 
// This function gets the listener parameters from the app and updates the DS3DListener.
void SoundManager::update_listener_parameters(ISoundListener* IEar)
{
	Vector back(0, 0, 1);
	Vector up(0, 1, 0);
	Vector v(0, 0, 0);
	SINGLE s = 1.0f;

	// get and, if successful, store the current values;
	if SUCCEEDED(IEar->get_ear_orientation(&back, &up))
	{
		set_ear_orientation(&back, &up);
	}
	if SUCCEEDED(IEar->get_ear_position(&v))
	{
		set_ear_position(&v);
	}
	if SUCCEEDED(IEar->get_ear_velocity(&v))
	{
		set_ear_velocity(&v);
	}
	if SUCCEEDED(IEar->get_ear_distance_factor(&s))
	{
		set_ear_distance_factor(s);
	}
	if SUCCEEDED(IEar->get_ear_doppler_factor(&s))
	{
		set_ear_doppler_factor(s);
	}
	if SUCCEEDED(IEar->get_ear_rolloff_factor(&s))
	{
		set_ear_rolloff_factor(s);
	}

	// send them to DSOUND
	set_listener_parameters();
}

// stolen from PBleisch - thanks!
// ------------------------------------------------------------------
//
// Converts string like this: "{8BA700E0-2A25-11d2-AE71-0000F4A24D28}"
// into a GUID.
//
// Note that a GUID of all zeros is equivelent to the NULL guid.
//
LPGUID ConvertStringToGUID(char* string, LPGUID guid)
{

	if (!strcmp(string, ID_NullGuid)) {
		memset(guid, 0, sizeof(GUID));
		return NULL;
	}
	else {
		DWORD d1, d2, d3;
		char d4[8], sz[200], s[3], * p, * start;

		p = string;
		while (*p && *p != '{') p++;

		strcpy(sz, ++p);

		start = p;
		while (*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) p++;
		p++;

		d1 = strtoul(start, NULL, 16);

		start = p;
		while (*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) p++;
		p++;

		d2 = strtoul(start, NULL, 16);

		start = p;
		while (*p && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) p++;
		p++;

		d3 = strtoul(start, NULL, 16);

		s[0] = *p++;
		s[1] = *p++;
		s[2] = 0;

		d4[0] = (char)strtoul(s, NULL, 16);

		s[0] = *p++;
		s[1] = *p++;
		s[2] = 0;

		d4[1] = (char)strtoul(s, NULL, 16);

		p++;

		for (int i = 0; i < 6; i++) {
			s[0] = *p++;
			s[1] = *p++;
			s[2] = 0;

			d4[2 + i] = (char)strtoul(s, NULL, 16);
		}

		guid->Data1 = d1;
		guid->Data2 = (unsigned short)d2;
		guid->Data3 = (unsigned short)d3;
		memcpy(guid->Data4, d4, 8);
	}
	return guid;
}

//****************************************************************************
//*                                                                          *
//*  DLLMain() called on startup/shutdown                                    *
//*                                                                          *
//****************************************************************************
//
BOOL DACOM_API DllMain(HINSTANCE hinstDLL,  //)
	DWORD     fdwReason,
	LPVOID    lpvReserved)
{
	ICOManager* DACOM = NULL;				// Handle to component manager
	IComponentFactory* server;

	switch (fdwReason)
	{
		//
		// DLL_PROCESS_ATTACH: Create object server component and register it 
		// with DACOM manager
		//

	case DLL_PROCESS_ATTACH:

		//DA_HEAP_ACQUIRE_HEAP(HEAP);
		//DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);


#define CLSID_SoundManager "SoundManager"

		DACOM = DACOM_Acquire();
		if (DACOM == NULL) {
			GENERAL_WARNING("SoundManager: DllMain: unable to get DACOM!\n");
			break;
		}
		if ((server = new DAComponentFactory2<DAComponentAggregate<SoundManager>, AGGDESC>(CLSID_SoundManager)) != NULL) {
			if (DACOM_Acquire()->RegisterComponent(server, CLSID_SoundManager, DACOM_LOW_PRIORITY) != GR_OK)
			{
				GENERAL_WARNING("SoundManager: DllMain: unable to register component!\n");
			}
			server->Release();
		}
		else
		{
			GENERAL_WARNING("SoundManager: DllMain: unable to create component factory!\n");
		}

		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

