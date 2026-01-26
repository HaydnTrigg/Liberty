//--------------------------------------------------------------------------//
//                                                                          //
//                              ISoundManager.h                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $
*/

/*
   This header file describes the SoundListener interface used by the Digital Anvil Sound Manager.  
   The Sound Manager provides a simple way to add sounds to an app.  The app implements the ISoundListener interface, 
   provides a list of sounds to play, implementing the ISoundSource interface for the sounds, and calls 
   update periodically causing Sound Manager to play the sounds.  Sound Manager handles all of the buffer
   creation and management using DirectSound, allows the user to set EAX and A3D properties if desired, 
   and supports DirectSound3D.

   //-----------------------------
   // ISoundManager Methods
   //-----------------------------

	GENRESULT set_sound_format (SoundFormat *soundFormat);
		INPUT:
			soundFormat: ptr to SoundFormat structure (defined in Wavlib.lib).

		RETURNS:
			GR_OK if successful, or some error code on failure.

		NOTES:
			Sets the format of the primary buffer.

	GENRESULT get_sound_format (SoundFormat *soundFormat);
		INPUT/OUTPUT:
			soundFormat: ptr to SoundFormat structure (defined in Wavlib.lib).

		RETURNS:
			GR_OK if successful, or some error code on failure.

		NOTES:
			Fills in soundFormat so that it contains the format of the primary buffer.

	GENRESULT set_master_attenuation (const SINGLE attenuation);
		INPUT:
			attenuation: Floating point decibal attenuation.  Valid values are between -100.0 and 0.0 .

		RETURNS:
			GR_OK if successful, or some error code on failure.

		NOTES:
			This value is applied to all instances.  Archetype and instance attenuation can also
			be set independently.

	GENRESULT set_maximum_channels (const U32 numChannels);
		INPUT:
			numChannels: Unsigned long maximum number of simultaneous sounds to play.

		RETURNS:
			GR_OK if successful, or some error code on failure.

		NOTES:
			This function controls the number of sounds that the sound manager will try to
			play simultaneously.  The number should be determined by the app based on the
			number of channels the card can handle in HW and other performance issues. The
			default value is the number of HW channels avaialable.

	GENRESULT set_speaker_configuration (const DWORD speakerConfiguration);
		INPUT:
			speakerConfiguration: DWORD representing the users (or app desired) speaker configuration.  
			Valid values are those (SM_SPEAKER_...) values defined in this file.

		RETURNS:
			GR_OK if successful, or some error code on failure.

		NOTES:
			This value can also be set in the ini file (speakerConfiguration = ) and processed by 
			Sound Manager at startup.

	SINGLE get_master_attenuation ();
		INPUT:
			None.

		RETURNS:
			Floating point decibal attenuation between -100.0 and 0.0 .

	U32 get_maximum_channels ();
		INPUT:
			None

  		RETURNS:
			Unsigned long maximum number of simultaneous sounds to play.

		NOTES:
			The app should make a decision based on the number of sounds the app needs to play vs the abilities
			of the sound card.  Too large a number on a bad card = degraded performance.  Too low could = bad
			gameplay.

  DWORD get_speaker_configuration ();
		INPUT:
			None.
		
		RETURNS:
			DWORD representing the current speaker configuration.  
			Valid values are those (SM_SPEAKER_...) values defined in this file.

	// 3D EAX/A3D/Property set extensions (for A3D only the damping factor is used)
	GENRESULT DACOM_API set_master_reverb(const U32 baseEnv, const SINGLE vol, const SINGLE decay, const SINGLE damping)
		INPUT:
			baseEnv: defined in EAX.h currently, 
			vol: reverb volume,
			decay: decay rate scale (1.0 is normal), 
			damping: damping rate scale (1.0 is normal)

		RETURNS:
			GR_OK if successful, GR_GENERIC otherwise.

		NOTES:
			This function sets the reverb effect for all sounds.  If EAX is supported, and enabled, all parameters are
			passed to EAX.  If A3D (1.2) is supported, and enabled, only the damping factor is passed to A3D.

	GENRESULT DACOM_API get_master_reverb(U32 *baseEnv, SINGLE *vol, SINGLE *decay, SINGLE *damping)
		INPUT:
			NONE

		OUTPUT:
			baseEnv: defined in EAX.h currently, 
			vol: reverb volume,
			decay: decay rate scale (1.0 is normal), 
			damping: damping rate scale (1.0 is normal)

		RETURNS:
			GR_OK if EAX and/or A3D are supported and enabled, GR_GENERIC otherwise.

	SOUND_ARCH_INDEX create_archetype (const SoundFormat *sformat, U32 length, U32 loop_start, U32 loop_end, const void *sample_buffer);
		INPUT:
			sformat: ptr to SoundFormat structure (WAVLIB.H).
			length: unsinged long length of the sample in bytes.
			loop_start: unsigned long starting point (byte) of looping sound.
			loop_end: unsigned long ending point (byte) of looping sound.
			sample_buffer: ptr to the sample data

		RETURNS:
			SOUND_ARCH_INDEX of newly created archetype, SM_INVALID_ARCHETYPE if archetype couldn't be created.

		NOTES:
			This function creates an archetype from a wav file that the app has already loaded. The current 
			implementation of SoundManager (and DirectSound) only supports looping entire the sound.

	SOUND_ARCH_INDEX create_archetype (SoundFile *sourceData);
		INPUT:
			sourceData: ptr to SoundFile structure (WAVLIB.H).

		RETURNS:
			SOUND_ARCH_INDEX of newly created archetype, SM_INVALID_ARCHETYPE if archetype couldn't be created.

		NOTES:
			This function creates an archetype from a wav file that the app has already loaded in the SoundFile format
			defined in WAVLIB.H . The current implementation of SoundManager (and DirectSound) only supports looping the
			entire sound.

	SOUND_ARCH_INDEX create_archetype (IFileSystem *sourceFile);
		INPUT:
			sourceFile: ptr to IFileSystem.
			length: unsinged long length of the sample in bytes.
			loop_start: unsigned long starting point (byte) of looping sound.
			loop_end: unsigned long ending point (byte) of looping sound.
			sample_buffer: ptr to the sample data

		RETURNS:
			SOUND_ARCH_INDEX of newly created archetype, SM_INVALID_ARCHETYPE if archetype couldn't be created.

		NOTES:
			This function loads and creates an archetype from a wav file. The current implementation of SoundManager
			(and DirectSound) only supports looping entire the sound.

	GENRESULT destroy_archetype (SOUND_ARCH_INDEX archetype);
		INPUT:
			archetype: SOUND_ARCH_INDEX, previously returned by create_archetype, of the archetype to destroy.

		RETURNS:
			GR_OK if successful, GR_GENERIC if the archetype was previously deleted or never existed.

		NOTES:
			This function destroys an archetype and stops and releases any instances of the archetype.

	GENRESULT get_archetype_interface (SOUND_ARCH_INDEX archetype, void ** archInterface);
		INPUT:
			archetype: SOUND_ARCH_INDEX, previously returned by create_archetype, whose ISoundArchetype 
			interface is desired.

		OUTPUT:
			archInterface: ISoundArchetype * for the archetype if successful, NULL if the archetype is deleted or never existed.
		
		RETURNS:
			GR_OK if successful, GR_GENERIC if the archetype is deleted or never existed.

	GENRESULT add_active_sound (ISoundSource *sound, U32 index);
		INPUT:
			sound: ptr to the ISoundSource interface of the sound to add.
			index: unsigned long position in the list of the sound to add. 0 inserts the sound at the head of the list and
			get_active_sound_count() adds the sound at the end of the list.

		RETURNS:
			GR_OK if successful, or some error code on failure.

	GENRESULT remove_active_sound (ISoundSource *sound);
		INPUT:
			sound: ptr to the ISoundSource interface of the sound to remove.

		RETURNS:
			GR_OK if successful, or some error code on failure.

		NOTES:
			This function removes the sound from SoundManager's play list and immediately
			stops the sound and releases any SSoundManager reference to the sound. (It is
			safe for an app to delete the sound after calling this function)

	GENRESULT set_active_sounds (ISoundSource *sounds[], U32 count);
		INPUT:
			sounds: ptr to the list of ISoundSource interfaces for the sounds.
			count: unsigned long number of sounds in the list.

		RETURNS:
			GR_OK if successful, or some error code on failure.

		NOTES:
			This function sets SoundManager's list of sounds to play. SoundManager's previous
			list is not destroyed until the app calls update(). (It is not safe for an app that 
			does not implement AddRef() and Release() for the sounds to delete the sounds in the 
			list after calling this function until calling this function with an empty list and 
			calling update() )

	U32 get_active_sound_count ();
		INPUT:
			None.

		RETURNS:
			Unsigned long number of sounds in SoundManager's play list.

	GENRESULT startup (HWND hWnd, U32 options);
		INPUT:
			hWnd: handle to the app's main window.
			options: unsigned long mask containing the SoundManager options (define in this file)
			the app desires.

		RETURNS:
			GR_OK if successful, GR_GENERIC on failure.

		NOTES:
			This function initializes DirectSound and determines the capabilities of the selected
			audio device.  After calling this function the app can get info about the device and
			start playing sounds.

	GENRESULT shutdown ();
		INPUT:
			None.

		RETURNS:
			GR_OK.

		NOTES:
			This function shuts down SoundManager and releases its references to DirectSound and
			all references to app sounds.

	GENRESULT get_directsound_interface (void ** lpds);
		INPUT/OUPUT:
			lpds: address of the pointer to the direct sound interface. Set to NULL on failure.

		RETURNS:
			GR_OK if successful, GR_GENERIC if an error occurs.

		NOTES:
			If successful, this function calls AddRef() for lpds.

	U32 get_current_time_ms ();
		INPUT:
			None.

		RETURNS:
			Unsigned long current time according to SoundManager in milliseconds.

	GENRESULT get_device_info (SM_DEVICEINFO *info);
		INPUT/OUPUT:
			info: address of a device info structure. Set to NULL on failure.

		RETURNS:
			GR_OK if successful, GR_GENERIC if an error occurs.

	void update (ISoundListener *IEar);
		INPUT:
			IEar: Ptr to the the app's ISoundListener implementation.

		RETURNS:
			Nothing.

		NOTES:
			This function synchronizes the playing sounds, starts new sounds, and releases
			all sound manager references to sounds that are no longer in the play list.  If
			IEar in not NULL, SoundManager updates the listener's parameters before playing
			any sounds. If IEar is NULL, only the sounds are updated and the listener information
			is not updated.  If the the app doesn't implement ISoundListener, the listener will 
			be at the origin with default DSound settings.  The app should call this on a regular 
			basis to make sure that sounds are started at the right time and that their data is 
			updated as needed.

*/

#ifndef ISOUNDMANGER_H
#define ISOUNDMANGER_H

#ifndef DACOM_H  
#include <DACOM.h>
#endif

#ifndef ISOUNDLISTENER_H
#include "ISoundListener.h"
#endif

#ifndef ISOUND_H
#include "ISound.h"
#endif

#ifndef WAVLIB_H
#include "wavlib.h"
#endif


enum SM_DEVICE_OPTIONS						// used when setting the device options and returned by getting available options
{
	SM_USE_2D_HARDWARE = 1,					// app wants/device has 2D hw mixing capabilites
	SM_USE_3D_HARDWARE = 2,					// app wants/device has 3D hw capabilites
	SM_USE_EAX = 4,							// app wants/device supports EAX
	SM_USE_A3D = 8,							// app wants/device supports A3D
	SM_USE_RIGHT_HANDED_SYSTEM = 16,
	SM_CREATE_ALL_2D_IN_SOFTWARE = 32,
	SM_USE_3D_SOFTWARE_USER_DEFINED = 64,	// *** CURRENT DSOUND IMPLEMENTATION = SM_USE_3D_SOFTWARE_BASIC *** app wants DS3D to use whatever the user's current selection
	SM_USE_3D_SOFTWARE_BASIC = 128,			// app wants DS3D to apply min calculations to 3D software buffers - only pan and volume adjustments to simulate 3D
	SM_USE_3D_SOFTWARE_LIGHT = 256,			// app wants DS3D to apply some calculations to 3D software buffers - decent results, less CPU
	SM_USE_3D_SOFTWARE_FULL = 512,			// app wants DS3D to apply maximum calculations to 3D software buffers - best results, more CPU
	SM_USE_3D_SOFTWARE_NONE = 1024,			// app wants no DS3D calcs applied to 3D SW buffers
	SM_USE_NO_HW = 2048,					// disables any hardware help	

	SM_DEFAULT_SETTINGS = SM_USE_2D_HARDWARE | SM_USE_3D_HARDWARE | SM_USE_EAX | SM_USE_A3D | SM_USE_RIGHT_HANDED_SYSTEM | SM_USE_3D_SOFTWARE_LIGHT
};

enum SM_ARCHETYPE_OPTIONS
{
	SM_MUTE_3D_AT_MAX_DISTANCE = 1,
	SM_ENABLE_FREQUENCY_CONTROL = 2,
	SM_ENABLE_STICKY_FOCUS = 4,
	SM_DISABLE_STICKY_FOCUS = 8,
	SM_ARCHETYPE_DEFAULT = SM_MUTE_3D_AT_MAX_DISTANCE | SM_DISABLE_STICKY_FOCUS
};

enum SM_3D_PROCESSING_MODES
{
	SM_INGNORE_3D_PROECESSING = 0,			// does no 3D calculations
	SM_NORMAL_3D_PROCESSING,				// paramaters of the sound are absolute
	SM_LISTENER_RELATIVE_3D_PROCESSING,		// parameters of the sound are relative to the listener paramaters
	SM_NUM_3D_PROCESSING_MODES
};

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

struct ISoundArchetype : public IDAComponent
{
	virtual void DACOM_API get_sound_format(SoundFormat *) = 0;
	virtual U32 DACOM_API get_samples(void * samples) = 0;
	virtual U32 DACOM_API get_sample_count() = 0;
	virtual SINGLE DACOM_API get_base_attenuation() = 0;
	virtual U32 DACOM_API get_num_channels() = 0;
	virtual U32 DACOM_API get_duration() = 0;

	virtual void DACOM_API set_samples(void * samples, U32 length) = 0;
	virtual GENRESULT DACOM_API set_base_attenuation(SINGLE attenuation) = 0;
	virtual bool DACOM_API is_loopable() = 0;
	virtual void DACOM_API get_loop_params(U32* loop_start, U32* loop_end) = 0;
};

#define ISOUNDMANAGER_VERSION 1
#define IID_ISoundManager DACOM_MAKE_IID("ISoundManager")
struct ISoundManager : public IDAComponent
{
// global/primary buffer modifiers
	virtual GENRESULT DACOM_API set_sound_format(SoundFormat *soundFormat) = 0;
	virtual GENRESULT DACOM_API get_sound_format(SoundFormat *soundFormat) = 0;

	virtual GENRESULT DACOM_API set_master_attenuation(const SINGLE attenuation) = 0;
	virtual GENRESULT DACOM_API set_maximum_channels(const U32 numChannels) = 0;
	virtual GENRESULT DACOM_API set_speaker_configuration(const U32 speakerConfiguration) = 0;

	virtual SINGLE DACOM_API get_master_attenuation() = 0;
	virtual U32 DACOM_API get_maximum_channels() = 0;
	virtual U32 DACOM_API get_speaker_configuration() = 0;

	// 3D EAX/A3D/Property set extensions (for A3D only the damping factor is used)
	virtual GENRESULT DACOM_API set_master_reverb(const U32 baseEnv, const SINGLE vol, const SINGLE decay, const SINGLE damping) = 0;
	virtual void DACOM_API get_master_reverb(U32 *baseEnv, SINGLE *vol, SINGLE *decay, SINGLE *damping) = 0;

	// don't use these methods.  implement ISoundListener and use update(ISoundListener *IEar)
// listener property modifiers	
	virtual GENRESULT DACOM_API set_ear_orientation(const Matrix * orientation) = 0;
	virtual GENRESULT DACOM_API set_ear_orientation(const Vector *front, const Vector *top) = 0;
	virtual GENRESULT DACOM_API set_ear_position(const Vector *) = 0;
	virtual GENRESULT DACOM_API set_ear_velocity(const Vector *) = 0;
	virtual GENRESULT DACOM_API set_ear_distance_factor(const SINGLE) = 0;
	virtual GENRESULT DACOM_API set_ear_doppler_factor(const SINGLE) = 0;
	virtual GENRESULT DACOM_API set_ear_rolloff_factor(const SINGLE) = 0;
	virtual void DACOM_API get_ear_orientation(Matrix * orientation) = 0;
	virtual void DACOM_API get_ear_orientation(Vector *front, Vector *top) = 0;
	virtual Vector DACOM_API get_ear_position() = 0;
	virtual Vector DACOM_API get_ear_velocity() = 0;
	virtual SINGLE DACOM_API get_ear_distance_factor() = 0;
	virtual SINGLE DACOM_API get_ear_doppler_factor() = 0;
	virtual SINGLE DACOM_API get_ear_rolloff_factor() = 0;

	// Expansion mechanism.
	virtual GENRESULT DACOM_API get_property(ISoundSource *sound, REFGUID propGUID, const U32 propID, void *propData, const U32 sizeOfPropData, U32 * sizeOfDataWritten) = 0;
	virtual GENRESULT DACOM_API get_global_property(REFGUID propGUID, const U32 propID, void *propData, const U32 sizeOfPropData, U32 * sizeOfDataWritten) = 0;
	virtual GENRESULT DACOM_API set_property(ISoundSource *sound, REFGUID propGUID, const U32 propID, void *propData, const U32 sizeOfPropData) = 0;
	virtual GENRESULT DACOM_API set_global_property(REFGUID propGUID, const U32 propID, void *propData, const U32 sizeOfPropData) = 0;

	// Archetype management
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_raw_data (const SoundFormat &, U32 length, U32 loop_start, U32 loop_end, void *sample_buffer, U32 options) = 0;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype_from_soundfile (const SoundFile &sourceData, U32 options) = 0;
	virtual SOUND_ARCH_INDEX DACOM_API create_archetype (IFileSystem *sourceFile, U32 options) = 0;
	virtual GENRESULT DACOM_API destroy_archetype (SOUND_ARCH_INDEX archetype) = 0;
	virtual GENRESULT DACOM_API get_archetype_interface(SOUND_ARCH_INDEX archetype, void ** archInterface) = 0;

	// Playbuffer management
	virtual GENRESULT DACOM_API add_active_sound (ISoundSource *sound, U32 index) = 0;
	virtual GENRESULT DACOM_API remove_active_sound (ISoundSource *sound) = 0;
	virtual GENRESULT DACOM_API set_active_sounds (ISoundSource *sounds[], U32 count) = 0;
	virtual U32 DACOM_API get_active_sound_count () = 0;
	virtual U32 DACOM_API get_playing_sound_count () = 0;

	// Misc.
	virtual GENRESULT DACOM_API startup(HWND hWnd, U32 options) = 0;
	virtual GENRESULT DACOM_API shutdown() = 0;
	virtual GENRESULT DACOM_API get_directsound_interface(void **) = 0;
	virtual U32 get_current_time_ms() = 0;
	virtual GENRESULT get_device_info(SM_DEVICEINFO *) = 0;
	virtual void DACOM_API update (ISoundListener *IEar = NULL) = 0;
	virtual GENRESULT DACOM_API unknownA(void* value) = 0; // #TODO unsure what this is (seems to be called when pressing F1 to open menu)
	virtual GENRESULT DACOM_API unknownB(void* value) = 0; // #TODO unsure what this is (seems to be called when pressing F1 to close menu)
};

struct SoundStatus
{
	const ISoundSource * soundSource;         // the sound source interface for the sound
	U32		             internalFlags;       // the internal sound manager flags for the sound
	DWORD	             dsoundBufferFlags;   // the DirectSound buffer flags for this sound
};

#define ISOUNDMANAGERSTATUS_VERSION 1
#define IID_ISoundManagerStatus DACOM_MAKE_IID("ISoundManagerStatus")
struct ISoundManagerStatus : public IDAComponent
{
	// Returns the ISoundSource pointer for the indicated playing sound.
	// Use get_playing_sound_count() from ISoundManager to get the number of playing sounds.
	// NOTE: The given ISoundSource pointer returned does not have is ref count increased before returning. If you
	// intend to hold on to it, you must ref count it yourself.
	// Returns GR_OK if 0 <= which < get_playing_sound_count().
	// Returns GR_GENERIC otherwise.
	virtual GENRESULT DACOM_API get_playing_sound_status (int which, SoundStatus &playingStatus) = 0;
};

#endif
