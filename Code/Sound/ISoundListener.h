/*
 * ISoundListener.h
 *
 * COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.
 *
 * The listener ("ear") interface for the Digital Anvil Sound Manager. The
 * application implements ISoundListener and passes it to ISoundManager::update()
 * each frame; the SoundManager reads the listener's orientation, position,
 * velocity and 3D tuning factors and uses them to position every active sound.
 *
 * Every getter returns GR_OK when it supplies a value, or GR_NOT_IMPLEMENTED to
 * accept the SoundManager default noted on each method.
 */

#ifndef ISOUNDLISTENER_H
#define ISOUNDLISTENER_H

#ifndef DACOM_H
#include <DACOM.h>
#endif

#include "typedefs.h"

class Vector;
class Matrix;

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in
// place of the version number. Keep the second parameter in sync with the value
// of the explicit version macro, and increment both when the interface changes.
#define ISOUNDLISTENER_VERSION 1
#define IID_ISoundListener MAKE_IID("ISoundListener", 1)

/*
 * ISoundListener
 *
 * The method order and signatures define the DACOM vtable and must not be
 * reordered or changed.
 */
struct ISoundListener : public IDAComponent
{
	/*
	 * Returns the listener orientation: 'back' is the -forward direction and 'up'
	 * the up direction. Default: back (0,0,1), up (0,1,0).
	 */
	virtual GENRESULT DACOM_API get_ear_orientation(Vector* back, Vector* up) = 0;

	// Returns the listener position. Default: (0,0,0).
	virtual GENRESULT DACOM_API get_ear_position(Vector* position) = 0;

	// Returns the listener velocity (drives Doppler). Default: (0,0,0).
	virtual GENRESULT DACOM_API get_ear_velocity(Vector* velocity) = 0;

	/*
	 * Returns the distance factor: the number of application units in one meter
	 * (all internal calculations are in meters). Default: 1.0.
	 */
	virtual GENRESULT DACOM_API get_ear_distance_factor(SINGLE* distance) = 0;

	/*
	 * Returns the Doppler factor scaling how much Doppler is applied (valid range
	 * 0.0..10.0; 1.0 is normal). Default: 1.0.
	 */
	virtual GENRESULT DACOM_API get_ear_doppler_factor(SINGLE* doppler) = 0;

	/*
	 * Returns the rolloff factor scaling distance attenuation (valid range
	 * 0.0..10.0; 1.0 is normal). Default: 1.0.
	 */
	virtual GENRESULT DACOM_API get_ear_rolloff_factor(SINGLE* rolloff) = 0;
};

#endif // ISOUNDLISTENER_H
