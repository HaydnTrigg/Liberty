#pragma once

#ifndef ITEXTURELIBRARY_H
#define ITEXTURELIBRARY_H

#include <DACOM.h>

#include "Pixel.h"
#include "dacom.h"

struct IFileSystem;
//

typedef U32 ITL_TEXTURE_ID;
typedef U32 ITL_TEXTURE_REF_ID;

const ITL_TEXTURE_ID		ITL_INVALID_ID = 0xFFFFFFFF;	// Invalid texture id.
const ITL_TEXTURE_REF_ID	ITL_INVALID_REF_ID = 0xFFFFFFFF;	// Invalid texture reference id.

//

#define ITL_FRAME_CURRENT	0xFFFFFFFF	// alias for the current frame.
#define ITL_FRAME_LAST		0xFFFFFFFE	// alias for the last frame.
#define ITL_FRAME_FIRST		0			// alias for the first frame.

//

typedef enum
{
	ITL_STATE_TEXTURE_LOD_LOAD,			// Load mipmap LODs from files
	ITL_STATE_TEXTURE_LOD_LOAD_BIAS,	// Detail level bias to load, expressed as a float LOD LEVEL NUMBER, default is 0.0

	ITL_STATE_TEXTURE_DIM_SQUARE,		// Autosquare textures. 0 to disable, 1 when necessary, 2 always.
	ITL_STATE_TEXTURE_DIM_POW2,			// Autoscale textures to POW2 when necessary.  0 to disable, 1 to magnify, 2 to go to nearest.

	ITL_STATE_TEXTURE_LOD_LOAD_MIN,		// Minimum dimensions of the highest texture detail level required before LOD scaling will take effect.  Default is 0
	ITL_STATE_TEXTURE_LOD_LOAD_SCALE,	// Detail level scale to load, expressed as a float percentage of total number of LOD levels in texture.  Default is 1.0, or all lod levels.

	ITL_STATE_TEXTURE_LOAD_MODE,		// Controls texture data override mode when loading.  The default is ITL_LM_USE_FIRST.

	ITL_STATE_MAX
} ITL_STATE;

//

typedef enum
{
	ITL_OPAQUE,
	ITL_ALPHA
} ITL_FORMAT;

//

typedef enum
{
	ITL_PLAY_ONE_TIME = 0,
	ITL_PLAY_LOOPED = 1,
	ITL_PLAY_PINGPONG = 2
} ITL_PLAYCOMMAND;


//

typedef enum
{
	ITL_LM_USE_FIRST,		// Use the texture data of the first located texture.  
	ITL_LM_USE_LAST,		// Use the texture data of the last located texture.
} ITL_LOADMODE;

//

// ------------------------------------------------------------------
// ITL_TEXTUREFRAME
//
// Textures are made up of at least one frame.
// 
struct ITL_TEXTUREFRAME_IRP
{
	U32		rp_texture_id;		// IRenderPipeline texture id.
	float	u0, v0;				// 
	float	u1, v1;				// 
};

// $todo replace this with correct type
struct CRCString
{
	const char* pString;
	U32 CRC;
};

/*
 * ITextureLibrary.h
 *
 * Public DACOM interfaces implemented by shading.dll's TextureLibrary
 * component. Two versioned interfaces: ITextureLibraryA (the base) and
 * ITextureLibrary2 (which extends it). Both are currently empty placeholders
 * modelled on the other shading interface headers.
 */

#define IID_ITextureLibraryA DACOM_MAKE_IID("ITextureLibraryA")
DACOM_INTERFACE(ITextureLibraryA, IID_ITextureLibraryA);
struct DACOM_NO_VTABLE ITextureLibraryA : public IDAComponent
{
	// Control loading of mipmaps, creating square textures, etc..
	//
	DEFMETHOD(set_library_state)(ITL_STATE state, U32 value) = 0;
	DEFMETHOD(get_library_state)(ITL_STATE state, U32* out_value) = 0;

	// Load texture library.
	//
	// load_library
	// Loads a texture library.
	// Opens the directory names 'library_name' within the current directory 
	// of the filesystem given and attempts to load all textures within
	// the library.  If library_name is NULL, "Texture library" is used.
	// Also opens the "Animation library" directory and loads all textures
	// within that library.
	//
	DEFMETHOD(load_library)(IFileSystem* IFS, const char* library_name) = 0;

	// Load texture.
	//
	// load_texture
	// First attempts to load texture_name from within the current
	// directory of the filesystem given.  If that fails, it
	// changes to the "Texture library" directory and retries.
	// If that fails, it changes to the "Animation library directory
	// and retries.  If that fails, returns failure.
	//
	DEFMETHOD(load_texture)(IFileSystem* IFS, const char* texture_name) = 0;

	// Releases (frees) textures.
	// If release_all is FALSE, release only textures with a refcount of zero (0).
	// If release_all is TRUE, release all.  Note this is very dangerous.
	//
	DEFMETHOD(free_library)(BOOL release_all) = 0;

	// update
	//
	//
	DEFMETHOD(update)(SINGLE dt) = 0;

	DEFMETHOD(set_unknown_string)(DWORD unknown, const char* unknown_string) = 0;
	DEFMETHOD(get_unknown_string)(DWORD unknown, char* out_unknown_string, U32 max_buf_size) = 0;

	// Create a texture.  If the texture already exists, the previous
	// id is returned.  If the texture does not exist, internal texture
	// structures are allocated and the handle returned.  
	// If out_texture_id is NULL, the refcount is untouched.  If 
	// out_texture_id is not NULL, the refcount of the texture is 
	// incremented and the id returned.
	//
	DEFMETHOD(get_texture_id)(const char* texture_name, ITL_TEXTURE_ID* out_texture_id) = 0;
	DEFMETHOD(get_texture_id2)(CRCString* crc_texture_name /* $todo correct type here */, ITL_TEXTURE_ID* out_texture_id) = 0;

	// Query whether there is a texture id associated with a given name.
	// If the texture does not exist, returns failure.  Use this to determine if
	// a texture exists in the library instead of get_texture_id() which
	// will allocate a texture if it does not exist.  The refcount of the
	// texture is untouched.
	//
	DEFMETHOD(has_texture_id)(const char* texture_name) = 0;

	// Increment/decrement the texture ref count.
	//
	// add_ref_texture_id:
	// If out_texture_ref_id is non-NULL, the ref count is increased, and a new texture 
	// reference id is returned.
	// If out_texture_ref_id is NULL, the ref count is increased, but the new id is 
	// not returned.
	// ITL_TEXTURE_REF_IDs are used with the automagic frame updating methods below.
	//
	// release_texture_id:
	// If there are no more outstanding references (ref count <= 0) to a texture,
	// the texture is destroyed and all resources released.  
	//
	// add_ref_texture_ref:
	// increment the ref count on a texture ref.  This is so clients can have multiple
	// pointers to the same reference so frame timings are in sync.
	//
	// release_texture_ref:
	// If there are no more outstanding references (ref count <= 0) to a texture,
	// the texture is destroyed and all resources released.  
	//
	DEFMETHOD(add_ref_texture_id)(ITL_TEXTURE_ID texture_id, ITL_TEXTURE_REF_ID* out_texture_ref_id) = 0;
	DEFMETHOD(release_texture_id)(ITL_TEXTURE_ID texture_id) = 0;
	DEFMETHOD(add_ref_texture_ref)(ITL_TEXTURE_REF_ID texture_id) = 0;
	DEFMETHOD(release_texture_ref)(ITL_TEXTURE_REF_ID texture_ref_id) = 0;


	// Set/get the name associated with a given texture id
	// If the id does not exist, returns failure.
	//
	DEFMETHOD(get_texture_name)(ITL_TEXTURE_ID texture_id, char* out_texture_name, U32 max_buf_size) = 0;

	// EMAURER Additionally, if the texture name already exists in the database, returns failure.
	DEFMETHOD(set_texture_name)(ITL_TEXTURE_ID texture_id, const char* texture_name) = 0;

	// Set/get texture data
	//
	// set_texture_frame:
	// frame_num is a zero based index into the frames.
	// frame_num CAN be the aliases ITL_FRAME_FIRST or ITL_FRAME_LAST.
	// frame_num CAN NOT be the alias ITL_FRAME_CURRENT.
	// if frame_num > frame_count, the frame list is expanded to include the
	// necessary frames.  
	//
	// txm_id_idx is an index in the interval [0, frame_count) that represents
	// which texture (of the texture set loaded for this animation) to use.
	//
	// These methods are used to dynamically build texture data.
	// 
	// get_texture_frame:
	// frame_num is a zero based index into the frames.
	// frame_num CAN be the aliases ITL_FRAME_FIRST or ITL_FRAME_LAST.
	// frame_num CAN NOT be the alias ITL_FRAME_CURRENT.
	// frame_num CAN NOT be greater or equal to frame_count.
	// On return, out_frame->rp_texture_id is the *actual* renderpipe texture id
	// and not the index.
	//
	DEFMETHOD(set_texture_frame_texture)(ITL_TEXTURE_ID texture_id, U32 txm_id_idx, U32 rp_texture_id) = 0;
	DEFMETHOD(set_texture_frame_id)(ITL_TEXTURE_ID texture_id, U32 frame_num, U32 txm_id_idx) = 0;
	DEFMETHOD(set_texture_frame_rect)(ITL_TEXTURE_ID texture_id, U32 frame_num, float u0, float v0, float u1, float v1) = 0;
	DEFMETHOD(get_texture_frame)(ITL_TEXTURE_ID texture_id, U32 frame_num, ITL_TEXTUREFRAME_IRP* out_frame) = 0;

	// get_texture_format
	//
	// This retrieves the pixel format of the specified frame of the given texture.
	//
	DEFMETHOD(get_texture_format)(ITL_TEXTURE_ID texture_id, U32 frame_num, PixelFormat* out_texture_format) = 0;

	// Set/get texture frame rate
	//
	DEFMETHOD(set_texture_frame_rate)(ITL_TEXTURE_ID texture_id, float fps_rate) = 0;
	DEFMETHOD(get_texture_frame_rate)(ITL_TEXTURE_ID texture_id, float* out_fps_rate) = 0;

	// Get the total number of frames for the texture.
	// Empty (no image data) and invalid textures have *zero* frames.
	// Non-animated textures have *one* frame.
	// Animated textures have >1 frame.
	//
	DEFMETHOD(get_texture_frame_count)(ITL_TEXTURE_ID texture_id, U32* out_frame_count) = 0;

	// Get the texture id from the texture ref id
	//
	// Useful if you only want to hang on to references but still want access
	// to the texture data (like name, etc..)
	// Does not touch the reference count.
	//
	DEFMETHOD(get_texture_ref_texture_id)(ITL_TEXTURE_REF_ID texture_ref_id, ITL_TEXTURE_ID* out_texture_id) = 0;

	// Get texture frame data
	//
	// frame_num is a zero based index into the frames and must be < frame_count.
	// frame_num can be the aliases ITL_FRAME_FIRST, ITL_FRAME_LAST, or ITL_FRAME_CURRENT.  
	// If ITL_FRAME_CURRENT is used, the texture library will automagically update the 
	// current frame.
	// On return, out_frame->rp_texture_id is the *actual* renderpipe texture id
	// and not the index.
	//
	DEFMETHOD(get_texture_ref_frame)(ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num, ITL_TEXTUREFRAME_IRP* out_frame) = 0;

	// Set/Get the current frame time of the texture.
	//
	// set_texture_ref_frame_time:
	// sets the current frame number (ITL_FRAME_CURRENT) based on the given
	// frame time.  If the frame number is greater than the frame count,
	// the frame is set to the last frame.  
	//
	// get_texture_ref_frame_time:
	// returns the current frame time without automagically updating.
	// 
	DEFMETHOD(set_texture_ref_frame_time)(ITL_TEXTURE_REF_ID texture_ref_id, float frame_time) = 0;
	DEFMETHOD(get_texture_ref_frame_time)(ITL_TEXTURE_REF_ID texture_ref_id, float* out_frame_time) = 0;

	// Set/Get the current frame number of the texture ref.
	//
	// set_texture_ref_frame_num:
	// sets the current frame number (ITL_FRAME_CURRENT) to the given frame number.  
	// If the frame number is greater than the frame count, the frame is set to the 
	// last frame.  
	//
	// get_texture_ref_frame_num:
	// returns the current frame number without automagically updating.
	// 
	DEFMETHOD(set_texture_ref_frame_num)(ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num) = 0;
	DEFMETHOD(get_texture_ref_frame_num)(ITL_TEXTURE_REF_ID texture_ref_id, U32* out_frame_num) = 0;

	// Set/get texture frame rate
	//
	DEFMETHOD(set_texture_ref_frame_rate)(ITL_TEXTURE_REF_ID texture_ref_id, float fps_rate) = 0;
	DEFMETHOD(get_texture_ref_frame_rate)(ITL_TEXTURE_REF_ID texture_ref_id, float* out_fps_rate) = 0;

	// Set/get texture frame rate
	//
	DEFMETHOD(set_texture_ref_play_mode)(ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND play_command) = 0;
	DEFMETHOD(get_texture_ref_play_mode)(ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND* out_play_command) = 0;

	// update_texture_ref
	//
	DEFMETHOD(update_texture_ref)(ITL_TEXTURE_REF_ID texture_ref_id, SINGLE dt) = 0;

	// Enumerate all of the available textures 
	//
	DEFMETHOD(get_texture_count)(U32* out_num_textures) = 0;
	DEFMETHOD(get_texture)(U32 texture_num, ITL_TEXTURE_ID* out_texture_id) = 0;
};

#define IID_ITextureLibrary2 DACOM_MAKE_IID("ITextureLibrary2")
DACOM_INTERFACE(ITextureLibrary2, IID_ITextureLibrary2);
struct DACOM_NO_VTABLE ITextureLibrary2 : public ITextureLibraryA
{
	DEFMETHOD(sub_6EC2300)(ITL_TEXTURE_REF_ID texture_ref_id, void*) = 0;
	DEFMETHOD(sub_6EC23A0)(ITL_TEXTURE_REF_ID texture_ref_id, DWORD, void*) = 0;
	DEFMETHOD(sub_6EC2490)(ITL_TEXTURE_REF_ID texture_ref_id, DWORD) = 0;
	DEFMETHOD(sub_6EC2680)(ITL_TEXTURE_REF_ID texture_ref_id, void*) = 0;
	DEFMETHOD(sub_6EC37F0)(void*, void*) = 0;
	DEFMETHOD(sub_6EC1950)(void*) = 0;
};

#endif // ITEXTURELIBRARY_H
