//--------------------------------------------------------------------------//
//                                                                          //
//                              SoundMan.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Pbleisch $
*/

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef ISOUNDMANAGER_H
#include "ISoundManager.h"
#endif

#ifndef ISOUNDLISTENER
#include "ISoundListener.h"
#endif

#ifndef ISOUND_H
#include "ISound.h"
#endif

#include <dsound.h>
#pragma warning (disable : 4786 4530)
#include <list>
#include <map>

#include <3dMath.h>
#include <TSmartPointer.h>

//#include "da_heap_utility.h"

#include "Engine.h"
#include "IProfileParser_Utility.h"

#include "AllocLite.h"

using namespace std;
struct SoundArchetype;
struct SoundInstance;

// internal SM_BUFFER_FLAGS
const U32	SMI_BUFFER_IN_HARDWARE =		0x01;			// buffer was created in hardware
const U32	SMI_BUFFER_USE_HW_BUFFER =		0x02;			// try to use HW for this buffer
const U32	SMI_BUFFER_USE_HW_3D_BUFFER =	0x04;			// try to use 3D HW for this buffer
const U32	SMI_BUFFER_USING_NOTIFICATION = 0x08;
const U32	SMI_BUFFER_GIVE_UP_ON_HW =		0x10;			// creating a hw buffer for this instance failed when it shouldn't have
const U32	SMI_DISABLE_3D =				0x20;			// disable 3D control

typedef struct _REVERBINFO
{
	U32 baseEnv;
	SINGLE vol;
	SINGLE decay;
	SINGLE damping;
} REVERBINFO;

struct GenericDSoundBuffer
{
	LPDIRECTSOUNDBUFFER pBuffer;
	DSBUFFERDESC desc;

	GenericDSoundBuffer(LPWAVEFORMATEX fmt);
	~GenericDSoundBuffer();
};

// SoundInstance
struct SoundInstance
{
	ISoundSource * soundSource;
	LPDIRECTSOUNDBUFFER m_lpSoundBuffer;

	U32		m_startTime;
	U32		m_submitted_index;

	U32		m_internal_flags;
	DWORD	m_DSOUND_buffer_flags;

	SINGLE	m_cachedDistanceSquared;
	SINGLE	m_cachedMax;
	SINGLE	m_cachedMaxSquared;
	SINGLE	m_cachedMin;
	SINGLE	m_cachedMinSquared;
	Vector	m_cachedPositionV;
	Vector	m_cachedSoundPanV;

	HANDLE	m_loopEventHandle;
	U32		m_loopStartOffset;
	U32		m_loopEndOffset;

	SoundArchetype * m_archetype;
	
	inline void			copy_instance_data(const SoundInstance & rhs);

	SoundInstance();
	SoundInstance(ISoundSource *);
	SoundInstance(const SoundInstance &rhs);
	void operator=(const SoundInstance &rhs);
	~SoundInstance();
	void free();
};

// SoundArchetype implementation
struct SoundArchetype : public ISoundArchetype
{
	SoundFile m_soundFile;
	U32 m_msDuration;
	U32 m_bufferFlags;
	U32 m_numHWBuffers;

	SINGLE m_baseAttenuation;
	LPDIRECTSOUNDBUFFER m_lpSoundBuffer;
	LPDIRECTSOUNDBUFFER m_lpHardwareBuffer;
	WAVEFORMATEX m_dsWaveFormat;
	
	BEGIN_DACOM_MAP_INBOUND(SoundArchetype)
	DACOM_INTERFACE_ENTRY(ISoundArchetype)
	DACOM_INTERFACE_ENTRY2(IID_ISoundArchetype,ISoundArchetype)
	END_DACOM_MAP()
	
	SoundArchetype();
	~SoundArchetype();

	virtual void DACOM_API get_sound_format(SoundFormat *);
	virtual U32 DACOM_API get_samples(void * samples);
	virtual U32 DACOM_API get_sample_count();
	virtual SINGLE DACOM_API get_base_attenuation();
	virtual U32 DACOM_API get_num_channels();
	virtual U32 DACOM_API get_duration();
	virtual bool DACOM_API is_loopable();
	virtual void DACOM_API get_loop_params(U32* loop_start, U32* loop_end);

	virtual	void DACOM_API set_samples(void * samples, U32 length);
	virtual	GENRESULT DACOM_API set_base_attenuation(SINGLE attenuation);

	GENRESULT set_sound_data(const SoundFormat *sourceData, U32 length, U32 loop_start, U32 loop_end, void *sample_buffer, LPDIRECTSOUND lpds, U32 options);
	GENRESULT set_sound_data_from_file(IFileSystem *sourceFile, LPDIRECTSOUND lpds, U32 options);

	void add_instance_hw_buffer();
	void release_instance_hw_buffer();
};

typedef DAComponent<SoundArchetype> SOUND_ARCH;
typedef map<U32, SOUND_ARCH*, less<U32>> ArchetypeMap;
typedef list<SoundInstance> InstanceList;
typedef list<ISoundSource*> SoundSourceList;

//typedef DAComponent<SoundArchetype> SOUND_ARCH;
//typedef AllocLite<SOUND_ARCH *> SOUND_ARCH_ALLOC;
//ALLOCLITE_GLOBAL (SOUND_ARCH_ALLOC);
//typedef map<U32, SOUND_ARCH *, less<U32>, SOUND_ARCH_ALLOC> ArchetypeMap;
//
//typedef AllocLite<SoundInstance> SOUND_INSTANCE_ALLOC;
//ALLOCLITE_GLOBAL (SOUND_INSTANCE_ALLOC)
//typedef list<SoundInstance, SOUND_INSTANCE_ALLOC> InstanceList;
//
//typedef AllocLite<ISoundSource *> SOUND_SOURCE_ALLOC;
//ALLOCLITE_GLOBAL (SOUND_SOURCE_ALLOC)
//typedef list<ISoundSource *, SOUND_SOURCE_ALLOC> SoundSourceList;

//---------------------------------------------------------------------------------
//-------------------------------SoundManager class declaration--------------------
//---------------------------------------------------------------------------------
//

struct SoundManager : public ISoundManager, ISoundManagerStatus, IAggregateComponent
{
protected:
	HWND m_hWnd;
	ArchetypeMap m_archetypeMap;

#ifdef DA_THREAD_SAFE
	CRITICAL_SECTION m_archetypeLock;
#endif

	CRITICAL_SECTION m_loopThreadLock;

	SoundSourceList m_activeSoundList;

	InstanceList m_spliceList;
	static InstanceList m_playList;

	SoundInstance m_tempInstance;

	SOUND_ARCH_INDEX m_nextAvailableArchetype;

	bool m_Initialized;
	char m_deviceGuidString[40];
	GUID m_deviceGuid;

	U32 m_hw_threshold_multiplier_squared;	// scale applied to min distance to determine whether a buffer gets hw
	SINGLE m_masterAttenuation;				// master attenuation applied to all sound instances
	U32 m_currentTime;						// current ms soundmanager time
	U32 m_maxHardwareChannels;				// max HW channels reported by the card
	U32 m_maxChannels;						// max total channels reported by the card
	U32 m_speakerConfiguration;				// current speaker configuration
	REVERBINFO m_reverb;					// current reverb info
	REVERBINFO m_originalReverb;			// reverb info to be reverted when soundmanager is shutdown
	U32 m_masterReverbMix;					// master amount of reverb to apply to all sound instances
	SINGLE m_originalA3DRollofFactor;		// original value to be reverted when soundmanager is shutdown
	GUID m_DS3D_SW_ALG_GUID;				// SW DS3D algorithm to use for 3D SW sounds


	U32 m_hardwareOptionsDesired;			// flags indicating the options the app requested
	U32 m_hardwareOptionsUsed;				// flags indicating the options that soundmanager was able to succesfully use
	U32 m_numHardware3DBuffers;				// number of HW buffers available
	U32 m_primarySampleRate;				
	U32 m_primaryNumChannels;
	U32 m_primaryBitsPerSample;
	S32 m_simpleCoordTransformation;		
	Transform m_earTransform;

	LPDIRECTSOUND3DLISTENER m_lpdsListener;// the listener ptr
	Vector m_listenerFront;
	Vector m_listenerUp;
	Vector m_listenerPan;
	Vector m_listenerPosition;
	Vector m_listenerVelocity;
	SINGLE m_listenerDistanceFactor;
	SINGLE m_listenerDopplerFactor;
	SINGLE m_listenerRolloffFactor;

	LPDIRECTSOUND m_lpds;						// the dsound ptr
	LPDIRECTSOUNDBUFFER m_lpdsPrimaryBuffer;	// the primary buffer ptr
	
	COMPTR<IProfileParser>	profile_parser;

	static HANDLE LoopNotificationThread;		// thread created when looping sounds w/ markers are started
	static HANDLE ResetNotificationLoopEvent;	// dummy event used to reset (and resync) the events the thread is waiting on
	static LONG ExitReq;
	

	BEGIN_DACOM_MAP_INBOUND(SoundManager)
	DACOM_INTERFACE_ENTRY(ISoundManager)
	DACOM_INTERFACE_ENTRY2(IID_ISoundManager, ISoundManager)
	DACOM_INTERFACE_ENTRY2(IID_ISoundManagerStatus, ISoundManagerStatus)
	END_DACOM_MAP()

public:
	SoundManager();
	~SoundManager();

	DA_HEAP_DEFINE_NEW_OPERATOR(SoundManager);

	// ISoundManager methods
	virtual GENRESULT DACOM_API set_sound_format(SoundFormat *soundFormat) override;
	virtual GENRESULT DACOM_API get_sound_format(SoundFormat *soundFormat) override;

	virtual GENRESULT DACOM_API set_master_attenuation(const SINGLE attenuation) override;
	virtual GENRESULT DACOM_API set_maximum_channels(const U32 numChannels) override;
	virtual GENRESULT DACOM_API set_speaker_configuration(const U32 speakerConfiguration) override;

	virtual SINGLE DACOM_API get_master_attenuation() override;
	virtual U32 DACOM_API get_maximum_channels() override;
	virtual U32 DACOM_API get_speaker_configuration() override;

	virtual GENRESULT DACOM_API set_master_reverb(const U32 baseEnv, const SINGLE vol, const SINGLE decay, const SINGLE damping) override;
	virtual void DACOM_API get_master_reverb(U32 *baseEnv, SINGLE *vol, SINGLE *decay, SINGLE *damping) override;
	
	// These need to be removed as soon as Freelancer implements an ISoundListener begin remove
	virtual GENRESULT DACOM_API set_ear_orientation(const Matrix * orientation) override;
	virtual GENRESULT DACOM_API set_ear_orientation(const Vector*, const Vector*) override;
	virtual GENRESULT DACOM_API set_ear_position(const Vector*) override;
	virtual GENRESULT DACOM_API set_ear_velocity(const Vector*) override;
	virtual GENRESULT DACOM_API set_ear_distance_factor(const SINGLE) override;
	virtual GENRESULT DACOM_API set_ear_doppler_factor(const SINGLE) override;
	virtual GENRESULT DACOM_API set_ear_rolloff_factor(const SINGLE) override;

	virtual void DACOM_API get_ear_orientation(Matrix * orientation) override;
	virtual void DACOM_API get_ear_orientation(Vector*, Vector*) override;
	virtual Vector DACOM_API get_ear_position() override;
	virtual Vector DACOM_API get_ear_velocity() override;
	virtual SINGLE DACOM_API get_ear_distance_factor() override;
	virtual SINGLE DACOM_API get_ear_doppler_factor() override;
	virtual SINGLE DACOM_API get_ear_rolloff_factor() override;
	// end of remove 

	// generic property set extensions to allow getting/setting of new properties not currently supported
	virtual GENRESULT DACOM_API get_property(ISoundSource *sound, REFGUID propGUID, const U32 propID, void *propData, const U32 sizeOfPropData, U32 * sizeOfDataWritten)  override;
	virtual GENRESULT DACOM_API get_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData, U32* sizeOfDataWritten)  override;
	virtual GENRESULT DACOM_API set_property(ISoundSource* sound, REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData)  override;
	virtual GENRESULT DACOM_API set_global_property(REFGUID propGUID, const U32 propID, void* propData, const U32 sizeOfPropData)  override;

	// archetype creation/destruction methods
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_raw_data (const SoundFormat &, U32 length, U32 loop_start, U32 loop_end, void *sample_buffer, U32 options)  override;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_soundfile(const SoundFile& sourceData, U32 options)  override;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype(IFileSystem* sourceFile, U32 options)  override;
	virtual GENRESULT DACOM_API destroy_archetype(SOUND_ARCH_INDEX archetype)  override;

	// sound instance management methods
	virtual GENRESULT DACOM_API add_active_sound (ISoundSource *sound, U32 index) override; // add a single
	virtual GENRESULT DACOM_API remove_active_sound (ISoundSource *sound) override; // remove a sound from the active list
	virtual GENRESULT DACOM_API set_active_sounds (ISoundSource *sounds[], U32 count) override; // this is the list.
	virtual U32 DACOM_API get_active_sound_count () override;
	virtual U32 DACOM_API get_playing_sound_count () override;

	// SoundManager methods
	GENRESULT init (AGGDESC * desc);
	GENRESULT DACOM_API Initialize() override;
	GENRESULT initialize_direct_sound();
	virtual GENRESULT DACOM_API startup(HWND hWnd, U32 options) override;
	virtual GENRESULT DACOM_API shutdown() override;
	virtual GENRESULT DACOM_API get_directsound_interface(void **) override;
	virtual U32 get_current_time_ms() override;
	virtual GENRESULT get_device_info(SM_DEVICEINFO *) override;
	virtual void DACOM_API update (ISoundListener *IEar) override; // syncs the need_to_play properties to the active sound properties.
	virtual GENRESULT DACOM_API unknownA(void* value) override; // #TODO unsure what this is (seems to be called when pressing F1 to open menu)
	virtual GENRESULT DACOM_API unknownB(void* value) override; // #TODO unsure what this is (seems to be called when pressing F1 to close menu)
	static GENRESULT write_data_to_buffer(SoundFile * sourceData, LPDIRECTSOUNDBUFFER lpdsBuffer, U32 length = 0);

	// SoundArchetype indexed methods
	virtual GENRESULT DACOM_API get_archetype_interface(SOUND_ARCH_INDEX archetype, void ** archInterface) override;
	virtual void DACOM_API get_sound_format(SOUND_ARCH_INDEX archetype, SoundFormat *);
	virtual U32 DACOM_API get_samples(SOUND_ARCH_INDEX archetype, void * samples);
	virtual U32 DACOM_API get_sample_count(SOUND_ARCH_INDEX archetype);
	virtual SINGLE DACOM_API get_base_attenuation(SOUND_ARCH_INDEX archetype);
	virtual U32 DACOM_API get_num_channels(SOUND_ARCH_INDEX archetype);
	virtual void DACOM_API set_samples(SOUND_ARCH_INDEX archetype, void * samples, U32 length);
	virtual bool DACOM_API is_loopable(SOUND_ARCH_INDEX archetype);

	// --- ISoundManagerStatus methods
	virtual GENRESULT DACOM_API get_playing_sound_status (int which, SoundStatus &playingStatus) override;

protected:

	// internal methods
	void report_audio_options();
	SOUND_ARCH_INDEX get_next_archetype_index();
	GENRESULT create_instance(SoundInstance &instance, SoundInstance &newInstance);
	GENRESULT delete_instance(ISoundSource *sound, U32 index);
	U32 exists_in_list(const ISoundSource * source, InstanceList &checkList, InstanceList::iterator *returnItr = NULL);
	bool need_to_play(SoundInstance &instance);
	bool finished(const SoundInstance &instance);
	bool in_range(SoundInstance &instance);
	void update_instance_data(SoundInstance &instance);
	bool update_position(SoundInstance &instance);
	void set_listener_parameters();
	void update_listener_parameters(ISoundListener *IEar);
	GENRESULT create_hardware_buffer(SoundInstance &instance);
	void check_hardware_buffer_status(SoundInstance &instance);
	bool restore_lost_buffer(SoundInstance &instance);
	SOUND_ARCH_INDEX add_archetype (SOUND_ARCH* nu);

	static DWORD WINAPI loop_notification_thread_loop(LPVOID);
	bool start_looping_sound_with_markers(SoundInstance &instance);

	//EMAURER trashes instances that were referring to 'archetype'
	void clean_archetype_references (U32 archetype);

	//EMAURER zap all of the archetypes in the database.
	void erase_archetypes (void);

	//EMAURER like QueryInterface, if result is true the reference count of 'output'
	//has been increased by 1. call Release () when done.
	bool query_archetype(SOUND_ARCH_INDEX archetype, SOUND_ARCH*& output);

	static float calculate_frequency_factor(float frequency);
};

#endif

