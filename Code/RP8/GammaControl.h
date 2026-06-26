#pragma once

#ifndef __GAMMACONTROL_H__
#define __GAMMACONTROL_H__

#include <DACOM.h>

#include <d3d8types.h>

//--------------------------------------------------------------------------//
//--------------------------IGammaControl Interface-------------------------//
//--------------------------------------------------------------------------//

typedef struct IGammaControl* LPGAMMACONTROL;

enum IGC_COMPONENT
{
	IGC_RED = 1,
	IGC_GREEN = 2,
	IGC_BLUE = 4,
	IGC_ALL = 7
};

#define IID_IGammaControl DACOM_MAKE_IID("IGammaControl")
struct DACOM_NO_VTABLE IGammaControl : public IDAComponent
{
	// set_gamma_function
	//
	// These parameters define the function:
	//
	// for entry in [0,256)
	// ramp[which][entry] = ((U16) ((float)0xFFFF) * (bias + pow( slope*(entry/256) + black_offset, 1/display_gamma )) ) );
	//
	//  Parameter		Useful Interval		Meaning
	//  ............................................................................................................
	//	slope			[ 0, inf)			Linear Slope.  This provides the basic direction of the function.
	//	black_offset	[-1, 1]				Black Offset.  This can be used to control the actual value of "blackness."
	//										This is very rarely used and can almost always be zero.
	//	bias			[-1, 1]				Bias.  This can be used to wholesale translate the function vertically.		
	//	display_gamma   ( 0, 3]				Estimated gamma of the display.  This is (most likely) attained through an empirical
	//										process.  There is no way to determine the gamma of the monitor in software and
	//										as such, we use this estimated display_gamma.  1/display_gamma is used as the
	//										gamma correction value.  Gamma for PC displays varies (realistically) between
	//										1.7 to about 2.8, with about 2.2 being average.  Use a display_gamma of 1.0 to 
	//										disable gamma correction.
	//
	//  Examples:
	//  set_gamma_function( IGC_ALL, 1, 0, 0, 1 );		// No gamma correction at all, all input values map to output values.
	//  set_gamma_function( IGC_RED, 1, 0, 1, 1 );		// No gamma correction at all, all red components map to 100% red.
	//  set_gamma_function( IGC_ALL, 1, 0, 0, 2.2 );	// Gamma correction of about .45.
	//  set_gamma_function( IGC_ALL, 2, 0, 0, 1 );		// Linear gamma correction, output values are double their input values.
	//
	// **NOTE** set_gamma_function is a convienence function that internally calls
	// set_gamma_ramp, hence if you need more control than set_gamma_function allows, 
	// use set-gamma_ramp.
	//
	DEFMETHOD(set_gamma_function)(IGC_COMPONENT which, float display_gamma, float bias = 0.0, float slope = 1.0, float black_offset = 0.0) = 0;

	// set_gamma_ramp
	//
	// Sets the gamma ramp(s) to the values specified by ramp.
	//
	// igc_component can be any of the values of IGC_COMPONENT
	//
	// ramp CAN NOT be NULL.  
	//
	// When setting new ramp levels, keep in mind that that the levels you set 
	// in the arrays are only used when your application is in full-screen mode. 
	// Whenever your application changes to windowed mode, the ramp levels are 
	// set aside, taking effect again when the application reinstates full-screen 
	// mode. 	
	//
	// Note that the values in the ramp are unsigned 16bit values, hence if
	// you are only using 8bit values, they should go in the HIGH byte of the
	// 16bit value.
	//
	DEFMETHOD(set_gamma_ramp)(IGC_COMPONENT igc_component, U16* ramp) = 0;

	// get_gamma_ramp
	//
	// Gets the gamma ramp of the specified component.
	// 
	// igc_component CAN NOT be IGC_ALL.
	// out_ramp CAN NOT be NULL;
	//
	DEFMETHOD(get_gamma_ramp)(IGC_COMPONENT igc_component, U16* out_ramp) = 0;

	// set_calibration_enable
	//
	// Returns GR_OK if device supports calibrated gamma ramps.
	//
	DEFMETHOD(set_calibration_enable)(bool enabled) = 0;

	// get_calibration_enable
	//
	// Returns GR_OK if device supports calibrated gamma ramps.
	//
	DEFMETHOD(get_calibration_enable)(void) = 0;
};

#endif
