#pragma once

#ifndef __RENDERPIPELINE_H__
#define __RENDERPIPELINE_H__

#include <DACOM.h>

#include <d3d8.h>

//--------------------------------------------------------------------------//
//--------------------------IRenderPipeline8B Interface---------------------//
//--------------------------------------------------------------------------//

struct RGB
{
	U8 r, g, b;
};

class Matrix4;
class Transform;

#include "RPVertexBuffer.h"
#include "RPIndexBuffer.h"
#include "RPTexture.h"
#include "Pixel.h"

typedef struct IRenderPipeline8B* LPRENDERPIPELINE;

struct PixelFormat_OLD
{
	PFenum pf;
	D3DFORMAT d3d;
	DWORD unknown8;
	DWORD unknownC;
	DWORD unknown10;
	DWORD unknown14;
	DWORD unknown18;
	DWORD unknown1C;
	DWORD unknown20;
	DWORD unknown24;
	DWORD unknown28;
	DWORD unknown2C;
	DWORD unknown30;
	DWORD unknown34;
	DWORD unknown38;
	DWORD unknown3C;
	DWORD unknown40;
	DWORD unknown44;
	DWORD unknown48;
};

enum RPPIPELINESTATE
{
	RP_TEXTURE,
	RP_LIGHTING,
	RP_TEXTURE_LOD,
	RP_TEXTURE_ALLOW_8BIT,
	RP_TEXTURE_ALLOW_32BIT,
	RP_TEXTURE_ALLOW_DXT,
	RP_BROKEN_MULTITEXTURE,
	RP_VIEWSPACE_LIGHTS,
	RP_CLEAR_COLOR,
	RP_CLEAR_ZDEPTH,
	RP_CLEAR_STENCIL,
	RP_STATE_CACHE,
	RP_BROKEN_MOD2X,
	RP_BROKEN_FLIP,
	RP_MAX_PIPELINE_STATE
};

enum RPDEVICEABILITY
{
	RP_A_ABILITY0,
	RP_A_DEVICE_GAMMA,
	RP_A_ABILITY2,
	RP_A_ABILITY3,
	RP_A_DEVICE_GEOMETRY, // DEVICE_GEOMETRY
	RP_A_TEXTURE_SQUARE_ONLY, // TEXTURE_SQUARE_ONLY
	RP_A_TEXTURE_MAX_WIDTH, // TEXTURE_MAX_WIDTH
	RP_A_TEXTURE_MAX_HEIGHT, // TEXTURE_MAX_HEIGHT
	RP_A_TEXTURE_CUBEMAPS, // TEXTURE_CUBEMAPS
	RP_A_DEPTH_BIAS, // DEPTH_BIAS
	RP_A_FOG_VERTEX, // FOG_VERTEX
	RP_A_FOG_PIXEL, // FOG_PIXEL
	RP_A_FOG_RANGE, // FOG_RANGE
	RP_A_FOG_W, // FOG_W
	RP_A_RASTER_ANTIALIASEDGES, // RASTER_ANTIALIASEDGES
	RP_A_DEVICE_SUPPORT_LEVEL, // DEVICE_SUPPORT_LEVEL
	RP_A_TEXTURE_TRILINEAR, // TEXTURE_TRILINEAR
	RP_A_DEVICE_BAD_MODE, // DEVICE_BAD_MODE
	RP_A_DEVICE_BAD_4444, // DEVICE_BAD_4444
	RP_A_DEVICE_NO_PARALLELISM,
	RP_A_MAX_ABILITY
};
static_assert(RP_A_MAX_ABILITY == 20);

struct RPDEVICEINFO
{
	D3DADAPTER_IDENTIFIER8 direct3d_adapter_identifier;
	D3DDISPLAYMODE direct3d_display_mode;
	D3DCAPS8 direct3d_caps;
	DWORD unknown510;
	DWORD unknown514;
	DWORD unknown518;
	DWORD unknown51C;
};
static_assert(sizeof(RPDEVICEINFO) == 0x520);

struct RPDISPLAYMODEINFO
{
	U32 mode_num;
	U32 width;
	U32 height;
	U32 refresh_rate;
	PFenum render_pf;
};

struct RPBUFFERSINFO
{
	DWORD adapter;
	U32 refresh_rate;
	U32 width;
	U32 height;
	U32 format;
	DWORD unknown14_auto_depth_stencil_format1;
	DWORD unknown18_auto_depth_stencil_format2;
	DWORD unknown1C;
	DWORD unknown20_buffer_count_plus_one;
	BYTE unknown24;
	BYTE unknown25_fullscreen;
	BYTE unknown26;
	BYTE unknown27;
};

struct RPDEVICESTATS
	/*{
	 // note for this structure that "last frame" means the
	 // time between the last begin_scene() and end_scene().

	 U32 is_thrashing; // non-zero if last frame caused thrashing.
	 U32 num_texture_managed; // number of textures currently being managed by the system.
	 U32 num_texture_activated; // number of textures that were activated (used) last frame.
	 U32 num_texture_vidmem_activated; // number of textures that were activated (used) last frame that were in video memory.
	 U32 num_texture_vidmem_created; // number of textures that were created in video memory last frame.
	 U32 num_texture_vidmem_evicted; // number of textures that were removed from video memory last frame.
	 U32 num_texture_vidmem; // number of textures currently in video memory.
	 U32 sizeof_texture_vidmem; // number of video memory bytes used by current textures.
	 U32 sizeof_texture_sysmem; // number of system memory bytes used by current textures.

	 U32 num_dp_calls; // number of draw_primitive calls
	 U32 num_dp_primitives; // number of primitives submitted (tris+points+lines)
	 U32 num_dip_calls; // number of draw_indexed_primitive calls
	 U32 num_dip_primitives; // number of primitives submitted (tris+points+lines)
	}*/;

struct RPLOCKDATA
{
	U32 width;
	U32 height;
	U32 pitch;
	void* pixels;
	PFenum pf;
};

typedef D3DLIGHT8* IRP_LIGHTHANDLE;

// Flags used with irp_ibf_flags parameter of create_index_buffer()
#define IRP_IBF_READ (1<<0)						// client would like read access to the vertex buffer
#define IRP_IBF_SOFTWAREPROCESSING (1<<1)		// buffer used with software vertex processing

// Flags used with irp_vbf_flags parameter of create_vertex_buffer()
#define IRP_VBF_READ (1<<0)						// client would like read access to the vertex buffer
#define IRP_VBF_SOFTWAREPROCESSING (1<<1)		// buffer used with software vertex processing
// #define IRP_VBF_NO_CLIP	(1<<1)				// no need to clip data in the vertex buffer
// #define IRP_VBF_SYSTEM	(1<<2)				// force buffer into system memory

#define IID_IRenderPipeline8B DACOM_MAKE_IID("IRenderPipeline8B")
struct DACOM_NO_VTABLE IRenderPipeline8B : public IDAComponent
{
	// IDAComponent methods

	DACOM_DEFMETHOD(QueryInterface)(const C8* interface_name, void** instance) = 0;
	DACOM_DEFMETHOD_(U32, AddRef)(void) = 0;
	DACOM_DEFMETHOD_(U32, Release)(void) = 0;

	// IRenderPipeline8B methods

	// startup
	//
	// "Start up" a rendering device. Note that this does not 
	// actually create any buffers (or the actual device), but it
	// does collect metadata about the device (abilities, names, etc.).
	//
	// profile_name defines an optional section in the profile (.ini)
	// that contains the device id and type to use. If profile_name
	// is NULL, the section that is pre-defined in the profile will
	// be used. i.e. Suppose the profile contains the following:
	//
	// [System]
	// IRenderPipeline = PimpedOut
	//
	// [PimpedOut]
	// DeviceId = {01010101-0101-0101-0101-010101010101}
	// DeviceType = Hardware
	//
	// [WimpedOut]
	// DeviceId = {00000000-0000-0000-0000-000000000000}
	// DeviceType = Software
	//
	// A call to startup( NULL ) would use the "PimpedOut" profile, while
	// a call to startup( "WimpedOut" ) would use the "WimpedOut" profile.
	//
	// **NO** other methods on this interface can be called before a
	// successful startup(), doing so will cause undefined behavior.
	//
	// Calling startup() will shutdown any previously "started" device.
	// 
	DACOM_DEFMETHOD(startup)(const char* profile_name);

	// shutdown
	//
	// Cleanup the device created with the most recent startup().
	//
	// **NO** other methods on this interface can be called after a
	// successful shutdown(), doing so will cause undefined behavior.
	//
	// shutdown() will automatically be called before the component
	// that implements this interface is destroyed. Hence, it is not
	// strictly necessary to call shutdown() unless you want to explicitly
	// shutdown the current device (i.e. to clear all of the device data out).
	//
	DACOM_DEFMETHOD(shutdown)(void);

	// set_pipeline_state
	//
	// Set the current value of a piece of pipeline state. 
	//
	DACOM_DEFMETHOD(set_pipeline_state)(RPPIPELINESTATE state, U32 value) = 0;

	// get_pipeline_state
	//
	// Retrieve the current value of a piece of pipeline state.
	//
	DACOM_DEFMETHOD(get_pipeline_state)(RPPIPELINESTATE state, U32* value) = 0;

	// get_device_info
	// 
	// Retrieves information about the device currently in use, if no
	// device has been "started", this method returns an error.
	//
	DACOM_DEFMETHOD(get_device_info)(RPDEVICEINFO* info) = 0;

	// query_device_ability
	//
	// Determine whether the current device supports a given feature (ability).
	// 
	// ability is one of the defined device abilities.
	// out_answer points to the buffer to place the response into. The length
	// of this buffer depends on the ability being queried.
	//
	DACOM_DEFMETHOD(query_device_ability)(RPDEVICEABILITY ability, U32* out_answer) = 0;

	// get_num_display_modes
	//
	// Retrieve the number of display modes that the current device 
	// supports.
	//
	DACOM_DEFMETHOD(get_num_display_modes)(U32* count) = 0;

	// get_display_mode
	//
	// Retrieve a description of a particular device display mode.
	//
	// mode_num is the value RP_CURRENT or a value in the 
	// range [0, num_modes) where num_modes is the value returned 
	// by get_num_display_modes(...).
	//
	// mode cannot be NULL
	//
	DACOM_DEFMETHOD(get_display_mode)(RPDISPLAYMODEINFO* mode, U32 mode_num) = 0;

	DACOM_DEFMETHOD(select_mode)(RPBUFFERSINFO* mode, U32* adapter) = 0;

	// create_buffers
	//
	// Creates render buffers according to the input parameters and the
	// currently configured pipeline state, associates the buffers with the
	// given window, and sets all of the transform and render state to their 
	// default values.
	//
	// Note that any previously created buffers are destroyed. 
	//
	// **NOTE** Some methods on this interface are **ONLY** available (i.e.
	// return success) after a successful create_buffers call. Such methods
	// are noted in their description.
	//
	// If this method returns failure, then there are no render buffers 
	// available (even if a previous call to create_buffers() succeeded).
	//
	DACOM_DEFMETHOD(create_buffers)(HWND hwnd, RPBUFFERSINFO* requested, RPBUFFERSINFO* out_buffersinfo) = 0;

	DACOM_DEFMETHOD(get_buffers)(U32* adapter, RPBUFFERSINFO* out_buffersinfo) = 0;

	// destroy_buffers
	// 
	// Destroy the render buffers created with a previous call to create_buffers.
	//
	// **NOTE** Some methods on this interface are **NO LONGER** available (i.e.
	// always return failure) after a successful destroy_buffers call. Such 
	// methods are noted in their description.
	//
	DACOM_DEFMETHOD(destroy_buffers)(void) = 0;

	// clear_buffers
	//
	// Clear the render buffers with the currently configured clear values.
	//
	// rp_clear_flags is a combination (bitwise-or) of the RP_CLEAR_*_BIT 
	// values defined above.
	//
	// If viewport_sub_rect is NULL, the entire viewport is cleared.
	// If viewport_sub_rect is not NULL, only the specified portion of the
	// viewport is cleared.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(clear_buffers)(U32 rp_clear_flags, RECT* viewport_sub_rect) = 0;

	// swap_buffers
	//
	// This is called when a client wants to make the current frame's 
	// rendering visible. One should always call this at the end of a frame
	// after everything has been rendered and end_scene() is called.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(swap_buffers)(void) = 0;

	// lock_buffer
	//
	// Acquire a pointer to the pixel data in the render buffer.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(lock_buffer)(RPLOCKDATA* lockData) = 0;

	// unlock_buffer
	//
	// Release lock acquired in lock_buffer.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(unlock_buffer)(void) = 0;

	// get_buffer_interface
	//
	// Acquire an interface to some extended functionality attached
	// to the render buffers. i.e. The video stream control, etc...
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_buffer_interface)(const char* iid, void** out_iif) = 0;

	// get_device_stats
	//
	// Retrieves statistics about the current device such as texture
	// usage, primitive counts, etc. Note that this method can stall
	// the pipeline and may have other performance side effects.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_device_stats)(RPDEVICESTATS* stat) = 0;

	// set_viewport
	//
	// Set the current viewport transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_viewport)(int x, int y, int w, int h) = 0;

	// get_viewport
	//
	// Get the current viewport transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_viewport)(int* out_x, int* out_y, int* out_w, int* out_h) = 0;

	// set_depth_range
	//
	// Set the range of (normalized) z values that will be used when
	// rendering primitives. 
	//
	// The range is typically set to lower = 0.0, upper = 1.0 which 
	// means that the entire range of the depth buffer should be used.
	// However, one could use other values to "dice" up the depth buffer
	// into sectors to control depth buffer aliasing.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_depth_range)(float lower_z_bound, float upper_z_bound) = 0;

	// get_depth_range
	//
	// Get the range of (normalized) z values that will be used when
	// rendering primitives. 
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_depth_range)(float* lower_z_bound, float* upper_z_bound) = 0;

	// set_window
	//
	// Set the current window transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_window)(HWND wnd, int x, int y, int w, int h) = 0;

	// get_window
	//
	// Get the current window transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_window)(HWND* out_wnd, int* out_x, int* out_y, int* out_w, int* out_h) = 0;

	DACOM_DEFMETHOD(set_world)(const Transform& world) = 0;
	DACOM_DEFMETHOD(get_world)(Transform& world) = 0;
	DACOM_DEFMETHOD(set_view)(const Transform& view) = 0;
	DACOM_DEFMETHOD(get_view)(Transform& view) = 0;

	// set_modelview
	//
	// Set the current modelview transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_modelview)(const Transform& modelview) = 0;

	// get_modelview
	//
	// Retrieve the current modelview transform
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_modelview)(Transform& modelview) = 0;

	// set_projection
	//
	// Set the current projection transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_projection)(const Matrix4& projection) = 0;

	// get_projection
	//
	// Get the current projection transform.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_projection)(Matrix4& projection) = 0;

	// set_lookat
	//
	// Set the current modelview transform such that it simulates an eye
	// at E=(eyex, eyey, eyez) looking toward C=(centerx,centery,centerz) with
	// a *world space* up vector approximating U=(upx, upy, upz).
	//
	// Note that if (V = E-C) == zero or V is parallel to U, then the resulting
	// modelview is undefined.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_lookat)(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz) = 0;

	// set_ortho
	//
	// Set the current projection transform to be an orthographic projection
	// with the view bounds set to the rectangle upperleft = (left,top) and
	// lowerright = (right,bottom) and a near clip plane set to nearval and
	// the far clip plane set to 1.0.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_ortho)(float left, float right, float bottom, float top, float nearval, float farval) = 0;

	// set_perspective
	//
	// Set the current projection transform to be a perspective projection
	// with a vertical field-of-view of 'fovy' and a horizontal-to-vertical
	// aspect ratio of 'aspect'.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_perspective)(float fovy, float aspect, float znear, float zfar) = 0;

	// set_light
	//
	// Set the light properties associated with the given light_index. The specified
	// index can be any valid unsigned 32-bit value.
	//
	// NOTE: this method does not affect whether the given light is enabled or 
	// NOTE: not... i.e. lights can be set and enabled orthogonally.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_light)(IRP_LIGHTHANDLE handle, const D3DLIGHT8* light_values) = 0;

	DACOM_DEFMETHOD(destroy_light)(IRP_LIGHTHANDLE handle) = 0;

	// get_light
	//
	// Get the light properties associated with the given light index.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_light)(IRP_LIGHTHANDLE handle, D3DLIGHT8* out_light_values) = 0;

	// set_light_enable
	//
	// Enable the given light.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_light_enable)(IRP_LIGHTHANDLE handle, U32 enable) = 0;

	// get_light_enable 
	//
	// Determine whether the given light is enabled.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_light_enable)(IRP_LIGHTHANDLE handle, U32* out_enable) = 0;

	// set_material
	//
	// Set the current material properties.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_material)(D3DMATERIAL8* material_values) = 0;

	// get_material
	//
	// Get the current material properties.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_material)(D3DMATERIAL8* out_material_values) = 0;

	// create_texture
	//
	// Create a texture surface. 
	//
	// width, height - The dimensions of the texture. If these values are larger
	// than the maximum dimensions supported by the device, this 
	// method will fail.
	//
	// desiredformat - The desired format of the new texture. This can either be
	// a fully-specified PixelFormat_OLD or a PixelFormat_OLD that specifies
	// a texture format class. If no suitable format is available
	// (i.e. the fully-specified format is not available nor are any
	// of the formats in the texture format class), this method will 
	// fail.
	//
	// num_lod - The number of levels of detail in the texture. If this value
	// is non-zero, a mipmap chain of num_lod length will be created.
	// If this value is zero, a non-mipmapped texture will be created.
	// 
	// irp_ctf_flags - Flags that control the type of texture to create. This can 
	// either be zero (0) to create a normal texture, or a combination
	// of the IRP_CTF_* flags defined above.
	//
	// out_handle - Reference to the place to store the handle to the created texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(create_texture)(int width, int height, const PFenum* desiredformat, int num_lod, U32 irp_ctf_flags, IRP_TEXTUREHANDLE* out_htexture) = 0;

	// destroy_texture
	//
	// Destroy a texture created with create_texture(). 
	//
	// htexture cannot be zero (0) nor RP_CURRENT.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(destroy_texture)(IRP_TEXTUREHANDLE htexture) = 0;

	// is_texture
	//
	// Determine whether a given texture handle references a valid texture.
	//
	// htexture cannot be zero (0) nor RP_CURRENT.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(is_texture)(IRP_TEXTUREHANDLE htexture) = 0;

	// lock_texture
	//
	// Retrieve a pointer to the texel data of the given level of detail of a texture.
	// One can lock (and unlock) different levels of detail independently.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(lock_texture)(IRP_TEXTUREHANDLE htexture, U32 subsurface, RPLOCKDATA* lockData) = 0;

	// unlock_texture
	//
	// Release lock acquired in lock_texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(unlock_texture)(IRP_TEXTUREHANDLE htexture, U32 subsurface) = 0;

	// get_texture_format
	//
	// Get the texture format of a given texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_texture_format)(IRP_TEXTUREHANDLE htexture, PFenum* out_pf) = 0;

	// get_texture_dim
	//
	// Get the width, height, and the number of levels of detail of a given texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_texture_dim)(IRP_TEXTUREHANDLE htexture, U32* out_width, U32* out_height, U32* out_num_lod) = 0;

	// get_texture_interface
	//
	// Retrieve extension interfaces from a particular texture.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_texture_interface)(IRP_TEXTUREHANDLE htexture, const char* iid, void** out_iif) = 0;

	// set_texture_level_data
	//
	// Set the texel data for the given level of detail of the given texture. 
	//
	// src_width, src_height - The width and height (in texels) of the source texel and alpha data.
	// src_stride - The width (in bytes) of the source texel data.
	// src_format - The format of the source texel data.
	// src_pixel - The source texel data.
	// src_alpha - The source alpha data. If not NULL, the 8-bit values in this buffer
	// are used as the alpha channel of the texel data (even if src_pixels 
	// contains alpha data.) If this value is NULL and src_format specifies
	// a format with an alpha channel, alpha values of 255 will be generated.
	// src_palette - The palette for the source data. If src_format specifies palettized
	// data, this palette is used. It is the responsibility of the client
	// to make sure that the data in src_pixel does not reference any entry
	// that does not exist in src_palette.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_texture_level_data)(IRP_TEXTUREHANDLE htexture, U32 subsurface, int src_width, int src_height, int src_stride, const PFenum* src_format, const void* src_pixel, const void* src_alpha, const RGB* src_palette) = 0;

	// blit_texture
	//
	// 
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(blit_texture)(IRP_TEXTUREHANDLE hDest, U32 destLevel, RECT destRect, IRP_TEXTUREHANDLE hSrc, U32 srcLevel, RECT srcRect) = 0;

	DACOM_DEFMETHOD(set_render_target)(UNKNOWN a2, UNKNOWN a3, UNKNOWN a4) = 0;
	DACOM_DEFMETHOD(get_render_target)(void* a2) = 0;

	// begin_scene
	//
	// Delimits the beginning of a scene frame. Call this only once
	// per frame per render target.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(begin_scene)(void) = 0;

	// end_scene
	//
	// Delimits the end of a scene frame. Call this only once per 
	// frame per render target.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(end_scene)(void) = 0;

	DACOM_DEFMETHOD(reset_render_states_to_defaults)(void) = 0;

	// set_render_state
	//
	// Set the current value of a render state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_render_state)(D3DRENDERSTATETYPE state, U32 value) = 0;

	// get_render_state
	//
	// Get the current value of a render state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_render_state)(D3DRENDERSTATETYPE state, U32* value) = 0;

	// set_texture_stage_state 
	//
	// Set the value of a piece of texture stage state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_texture_stage_state)(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value) = 0;

	// get_texture_stage_state
	//
	// Get the value of a piece of texture stage state.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_texture_stage_state)(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32* value) = 0;

	// set_texture_stage_transform
	//
	// Set the value of the texture coordinate transform for the given texture stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_texture_stage_transform)(U32 stage, Matrix4 const& mat4) = 0;

	// get_texture_stage_transform
	//
	// Get the value of the texture coordinate transform for the given texture stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_texture_stage_transform)(U32 stage, Matrix4& out_mat4) = 0;

	// set_texture_stage_texture
	//
	// Set the current texture of a texture stage. 
	// If htexture == 0, no texture is available in the stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(set_texture_stage_texture)(U32 stage, IRP_TEXTUREHANDLE htexture) = 0;

	// get_texture_stage_texture
	//
	// Get the current texture of a texture stage.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(get_texture_stage_texture)(U32 stage, IRP_TEXTUREHANDLE* out_htexture) = 0;

	// verify_state
	//
	// Determine whether a primitive could be rendered with the currently 
	// set render and texture stage state values.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(verify_state)(void) = 0;

	// draw_primitive
	//
	// Render a primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(draw_primitive)(D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, U32 flags) = 0;

	// draw_indexed_primitive
	//
	// Render an indexed primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(draw_indexed_primitive)(D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, const U16* indices, U32 num_indices, U32 flags) = 0;

	// draw_primitive
	//
	// Render a primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(draw_primitive_vb)(D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, U32 flags) = 0;

	// draw_indexed_primitive
	//
	// Render an indexed primitive of the given type with the given vertices.
	//
	// This method will always return failure 'outside' of successful create_buffers
	// and destroy_buffers calls.
	//
	DACOM_DEFMETHOD(draw_indexed_primitive_vb)(D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, const U16* indices, U32 num_indices, U32 flags) = 0;

	DACOM_DEFMETHOD(add_light)(IRP_LIGHTHANDLE handle) = 0;
	DACOM_DEFMETHOD(remove_light)(IRP_LIGHTHANDLE handle) = 0;
	DACOM_DEFMETHOD(update_light)(IRP_LIGHTHANDLE handle) = 0;
	DACOM_DEFMETHOD(set_world_n)(UNKNOWN a2, Transform* transform) = 0;
};

extern "C"
{
	RP8_DEC IComponentFactory* CreateDirectX8Factory();
	RP8_DEC IComponentFactory* CreateNewRenderPipelineFactory();
	RP8_DEC void Register_DirectX8();
	RP8_DEC void Register_NewRenderPipeline();
}

#endif
