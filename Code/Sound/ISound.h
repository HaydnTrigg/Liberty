/*
 * ISound.h
 *
 * COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.
 *
 * The sound-source interface for the Digital Anvil Sound Manager. The application
 * implements ISoundSource for every sound it wants played and hands the list to
 * the SoundManager, which queries these methods each update() to position, pitch,
 * attenuate and loop the sound.
 *
 * Every getter returns GR_OK when it supplies a value, or GR_NOT_IMPLEMENTED to
 * accept the SoundManager default noted on each method. All distances are in
 * meters, attenuations in decibels (0.0 = full volume, -100.0 = silence), and
 * angles where noted in degrees.
 */

#ifndef ISOUND_H
#define ISOUND_H

#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "typedefs.h"
class Vector;

typedef S32 SOUND_ARCH_INDEX;
const SOUND_ARCH_INDEX SM_INVALID_ARCHETYPE = -1;

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in
// place of the version number. Keep the second parameter in sync with the value
// of the explicit version macro, and increment both when the interface changes.
#define ISOUNDSOURCE_VERSION 1
#define IID_ISoundSource MAKE_IID("ISoundSource", 1)

/*
 * ISoundSource
 *
 * The method order and signatures define the DACOM vtable and must not be
 * reordered or changed.
 */
struct ISoundSource : public IDAComponent
{
	// Reserved legacy slot; preserved to keep the vtable layout intact.
	virtual void DACOM_API unknown() = 0;

	/*
	 * Returns this sound's start time, in SoundManager milliseconds. Seed it from
	 * ISoundManager::get_current_time_ms() so the clocks stay in sync.
	 */
	virtual U32 DACOM_API get_start_time() = 0;

	// Returns true while the sound should be playing.
	virtual bool DACOM_API is_on() = 0;

	/*
	 * Returns true if the sound is spatialized. A 3D sound may return false to
	 * temporarily suppress 3D processing (pan/volume only).
	 */
	virtual bool DACOM_API is_3D() = 0;

	// Returns true if the sound should loop.
	virtual bool DACOM_API is_looping() = 0;

	// Returns the archetype this sound is an instance of.
	virtual SOUND_ARCH_INDEX get_archetype() = 0;

	// Decibel attenuation, -100.0..0.0. Default: 0.0.
	virtual GENRESULT DACOM_API get_attenuation(SINGLE*) = 0;

	/*
	 * Frequency scale of the archetype's sample rate (0.5 = half speed, 2.0 =
	 * double). Requires SM_ENABLE_FREQUENCY_CONTROL on the archetype. Default: 1.0.
	 */
	virtual GENRESULT DACOM_API get_frequency(SINGLE*) = 0;

	// Stereo pan, -100 (full left) .. 100 (full right). Default: 0 (centered).
	virtual GENRESULT DACOM_API get_pan(S32*) = 0;

	// World position of the sound. Default: (0,0,0).
	virtual GENRESULT DACOM_API get_position(Vector*) = 0;

	/*
	 * Minimum distance: the sound is at full volume at or within this distance.
	 * Default: 1.
	 */
	virtual GENRESULT DACOM_API get_min_distance(SINGLE*) = 0;

	/*
	 * Maximum distance: attenuation stops increasing beyond it (and the sound is
	 * muted past it if the archetype has SM_MUTE_3D_AT_MAX_DISTANCE). Default: the
	 * DirectSound near-infinite default.
	 */
	virtual GENRESULT DACOM_API get_max_distance(SINGLE*) = 0;

	/*
	 * Inner/outer cone angles, in degrees. Between the cones the sound fades to
	 * the outside-cone attenuation. Default: 360 for both.
	 */
	virtual GENRESULT DACOM_API get_cone_angles(DWORD* insideAngle, DWORD* outsideAngle) = 0;

	// Direction the sound cone points. Default: (0,0,0).
	virtual GENRESULT DACOM_API get_cone_orientation(Vector*) = 0;

	// Attenuation applied outside the outer cone, in decibels. Default: -100.0.
	virtual GENRESULT DACOM_API get_cone_outside_attenuation(SINGLE*) = 0;

	// Velocity of the sound (drives Doppler). Default: (0,0,0).
	virtual GENRESULT DACOM_API get_velocity(Vector*) = 0;

	/*
	 * Per-instance reverb mix, scaling the master reverb set via
	 * ISoundManager::set_master_reverb(). Default: 0.0 (no reverb).
	 */
	virtual GENRESULT DACOM_API get_reverb_mix(SINGLE*) = 0;

	/*
	 * 3D processing mode: disabled, normal (absolute), or head-relative
	 * (coordinates track the listener). Default: normal.
	 */
	virtual GENRESULT DACOM_API get_sound_mode(S32*) = 0;

	/*
	 * Apply mode (immediate vs deferred). The SoundManager always applies changes
	 * in deferred mode. Default: deferred.
	 */
	virtual GENRESULT DACOM_API get_apply_mode(S32*) = 0;
};

#endif // ISOUND_H
