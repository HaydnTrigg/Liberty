#include "RenderPipeline.h"
#include "VertexBufferManager.h"
#include "RPDraw.h"
#include "RPIndexBuffer.h"
#include "RPVertexBuffer.h"
#include "GammaControl.h"
#include "RPTexture.h"
#include "CachedMatrix.h"
#include "CachedViewport.h"
#include "CachedTexture.h"
#include "CachedGeometry.h"
#include "StateInfo.h"
#include "DX8/DX8IndexBuffer.h"

#include <d3d8.h>
#include <Dump.h>
#include <FLHook_st6.h>
#include <TSmartPointer.h>

#define uint U32

TRAMPOLINE(IRenderPipeline8B*, __thiscall, DirectX8_Ctor, _sub_6D01143, IRenderPipeline8B* _this);
TRAMPOLINE(IRenderPipeline8B*, __thiscall, DirectX8_Dtor, _sub_6D01689, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __thiscall, DirectX8_init, _sub_6D01A06, IRenderPipeline8B* _this, AGGDESC* pDesc);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_startup, _sub_6D05DA0, IRenderPipeline8B* _this, const char* profile_name);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_shutdown, _sub_6D07AA8, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_pipeline_state, _sub_6D081C2, IRenderPipeline8B* _this, RPPIPELINESTATE state, U32 value);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_pipeline_state, _sub_6D0823E, IRenderPipeline8B* _this, RPPIPELINESTATE state, U32* value);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_device_info, _sub_6D082D1, IRenderPipeline8B* _this, RPDEVICEINFO* info);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_query_device_ability, _sub_6D083D2, IRenderPipeline8B* _this, RPDEVICEABILITY ability, U32* out_answer);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_num_display_modes, _sub_6D08461, IRenderPipeline8B* _this, U32* count);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_display_mode, _sub_6D084F2, IRenderPipeline8B* _this, RPDISPLAYMODEINFO* mode, U32 mode_num);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_select_mode, _sub_6D0869D, IRenderPipeline8B* _this, RPBUFFERSINFO* mode, U32* adapter);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_create_buffers, _sub_6D08811, IRenderPipeline8B* _this, HWND hwnd, RPBUFFERSINFO* requested, RPBUFFERSINFO* out_buffersinfo);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_buffers, _sub_6D098F3, IRenderPipeline8B* _this, U32* adapter, RPBUFFERSINFO* out_buffersinfo);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_destroy_buffers, _sub_6D09982, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_clear_buffers, _sub_6D09B4C, IRenderPipeline8B* _this, U32 rp_clear_flags, RECT* viewport_sub_rect);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_swap_buffers, _sub_6D09D28, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_lock_buffer, _sub_6D09F96, IRenderPipeline8B* _this, RPLOCKDATA* lockData);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_unlock_buffer, _sub_6D0A6C1, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_buffer_interface, _sub_6D0A7EC, IRenderPipeline8B* _this, const char* iid, void** out_iif);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_device_stats, _sub_6D0A863, IRenderPipeline8B* _this, RPDEVICESTATS* stat);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_viewport, _sub_6D0A8D0, IRenderPipeline8B* _this, int x, int y, int w, int h);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_viewport, _sub_6D0AABC, IRenderPipeline8B* _this, int* out_x, int* out_y, int* out_w, int* out_h);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_depth_range, _sub_6D0ABB2, IRenderPipeline8B* _this, float lower_z_bound, float upper_z_bound);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_depth_range, _sub_6D0ACA6, IRenderPipeline8B* _this, float* lower_z_bound, float* upper_z_bound);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_window, _sub_6D0AD7D, IRenderPipeline8B* _this, HWND hwnd, int x, int y, int w, int h);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_window, _sub_6D0AE23, IRenderPipeline8B* _this, HWND* out_hwnd, int* out_x, int* out_y, int* out_w, int* out_h);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_world, _sub_6D0B09D, IRenderPipeline8B* _this, const Transform& world);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_world, _sub_6D0B3DD, IRenderPipeline8B* _this, Transform& world);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_view, _sub_6D0B607, IRenderPipeline8B* _this, const Transform& view);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_view, _sub_6D0B939, IRenderPipeline8B* _this, Transform& view);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_modelview, _sub_6D0BB58, IRenderPipeline8B* _this, const Transform& modelview);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_modelview, _sub_6D0BF39, IRenderPipeline8B* _this, Transform& modelview);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_projection, _sub_6D0C29E, IRenderPipeline8B* _this, const Matrix4& projection);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_projection, _sub_6D0C438, IRenderPipeline8B* _this, Matrix4& projection);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_lookat, _sub_6D0C57C, IRenderPipeline8B* _this, float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_ortho, _sub_6D0C9C2, IRenderPipeline8B* _this, float left, float right, float bottom, float top, float nearval, float farval);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_perspective, _sub_6D0CB25, IRenderPipeline8B* _this, float fovy, float aspect, float znear, float zfar);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_light, _sub_6D0CE57, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle, const D3DLIGHT8* light_values);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_destroy_light, _sub_6D0CF9C, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_light, _sub_6D0D044, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle, D3DLIGHT8* out_light_values);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_light_enable, _sub_6D0D0D7, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle, U32 enable);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_light_enable, _sub_6D0D157, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle, U32* out_enable);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_material, _sub_6D0D1DC, IRenderPipeline8B* _this, D3DMATERIAL8* material_values);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_material, _sub_6D0D310, IRenderPipeline8B* _this, D3DMATERIAL8* out_material_values);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_create_texture, _sub_6D0D628, IRenderPipeline8B* _this, int width, int height, const PFenum* desiredformat, int num_lod, U32 irp_ctf_flags, IRP_TEXTUREHANDLE* out_htexture);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_destroy_texture, _sub_6D0D997, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_is_texture, _sub_6D0DC51, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_lock_texture, _sub_6D0DCE3, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture, U32 subsurface, RPLOCKDATA* lockData);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_unlock_texture, _sub_6D0E1AF, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture, U32 subsurface);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_texture_format, _sub_6D0E3D9, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture, PFenum* out_pf);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_texture_dim, _sub_6D0E6D4, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture, U32* out_width, U32* out_height, U32* out_num_lod);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_texture_interface, _sub_6D0EA0B, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture, const char* iid, void** out_iif);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_texture_level_data, _sub_6D0EA78, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE htexture, U32 subsurface, int src_width, int src_height, int src_stride, const PFenum* src_format, const void* src_pixel, const void* src_alpha, const RGB* src_palette);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_blit_texture, _sub_6D0F1BC, IRenderPipeline8B* _this, IRP_TEXTUREHANDLE hDest, U32 destLevel, RECT destRect, IRP_TEXTUREHANDLE hSrc, U32 srcLevel, RECT srcRect);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_render_target, _sub_6D0F698, IRenderPipeline8B* _this, UNKNOWN a2, UNKNOWN a3, UNKNOWN a4);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_render_target, _sub_6D0F70F, IRenderPipeline8B* _this, void* a2);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_begin_scene, _sub_6D0F786, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_end_scene, _sub_6D0F8B5, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_reset_render_states_to_defaults, _sub_6D0F9E4, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_render_state, _sub_6D0FA68, IRenderPipeline8B* _this, D3DRENDERSTATETYPE state, U32 value);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_render_state, _sub_6D0FB44, IRenderPipeline8B* _this, D3DRENDERSTATETYPE state, U32* value);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_texture_stage_state, _sub_6D0FBCF, IRenderPipeline8B* _this, U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_texture_stage_state, _sub_6D0FCD5, IRenderPipeline8B* _this, U32 stage, D3DTEXTURESTAGESTATETYPE state, U32* value);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_texture_stage_transform, _sub_6D0FDD2, IRenderPipeline8B* _this, U32 stage, Matrix4 const& mat4);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_texture_stage_transform, _sub_6D0FF7D, IRenderPipeline8B* _this, U32 stage, Matrix4& out_mat4);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_texture_stage_texture, _sub_6D100D3, IRenderPipeline8B* _this, U32 stage, IRP_TEXTUREHANDLE htexture);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_texture_stage_texture, _sub_6D10247, IRenderPipeline8B* _this, U32 stage, IRP_TEXTUREHANDLE* out_htexture);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_verify_state, _sub_6D10322, IRenderPipeline8B* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_draw_primitive, _sub_6D1067F, IRenderPipeline8B* _this, D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, U32 flags);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_draw_indexed_primitive, _sub_6D1097D, IRenderPipeline8B* _this, D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, const U16* indices, U32 num_indices, U32 flags);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_draw_primitive_vb, _sub_6D10C89, IRenderPipeline8B* _this, D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, U32 flags);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_draw_indexed_primitive_vb, _sub_6D10E4B, IRenderPipeline8B* _this, D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, const U16* indices, U32 num_indices, U32 flags);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_add_light, _sub_6D0CCB2, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_remove_light, _sub_6D0CD32, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_update_light, _sub_6D0CDDB, IRenderPipeline8B* _this, IRP_LIGHTHANDLE handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_world_n, _sub_6D0AF55, IRenderPipeline8B* _this, UNKNOWN a2, Transform* transform);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_initialize, _sub_6D11354, IVertexBufferManager* _this, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_cleanup, _sub_6D1135D, IVertexBufferManager* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_acquire_vertex_buffer, _sub_6D114EA, IVertexBufferManager* _this, D3DFORMAT vertex_format, U32 num_verts, VertexBufferAcquire* out_vbmem);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_release_vertex_buffer, _sub_6D11877, IVertexBufferManager* _this, VertexBufferAcquire* vbmem);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_IVertexBufferManager_Unknown1C, _sub_6D118BC, IVertexBufferManager* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_copy_vertex_data, _sub_6D114C5, IVertexBufferManager* _this, void* dst_buffer, U32 dst_vertex_format, VertexBufferDesc* src_vb_desc, U32 start_vertex, U32 num_vertices);
TRAMPOLINE(HRESULT, __stdcall, DirectX8_draw_indexed_primitive2, _sub_6D111E1, IRPDraw* _this, D3DPRIMITIVETYPE type, U32 min_index, U32 num_verts, U32 start_index, U32 count);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_create_index_buffer, _sub_6D12D4E, IRPIndexBuffer* _this, U32 num_indices, IRP_INDEXBUFFERHANDLE* out_ib_handle, U8 irp_ibf_flags);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_destroy_index_buffer, _sub_6D13002, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE* ib_handle);
TRAMPOLINE(HRESULT, __stdcall, DirectX8_create_ib, _sub_6D131B2, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE ib_handle, U32 num_indices);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_copy_indices, _sub_6D13847, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, U16 const* indices, U32 num_indices);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_lock_ib, _sub_6D134EC, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, void*& out_data, U32 num_indices);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_unlock_ib, _sub_6D13794, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE ib_handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_select_ib, _sub_6D13BCE, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE ib_handle, U32 base_index, UNKNOWN a4, UNKNOWN a5);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_ib_count, _sub_6D13D23, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE ib_handle, U32* out_count);
TRAMPOLINE(BOOL32, __stdcall, DirectX8_is_ib_valid, _sub_6D13B69, IRPIndexBuffer* _this, IRP_INDEXBUFFERHANDLE ib_handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_create_vb, _sub_6D118C8, IRPVertexBuffer* _this, U32 vertex_format, U32 num_verts, IRP_VERTEXBUFFERHANDLE* out_vb_handle, U8 irp_vbf_flags);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_destroy_vb, _sub_6D11DB3, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE& vb_handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_ressize_vb, _sub_6D11F78, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE vb_handle, U32 format, U32 num_verts);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_copy_vertices, _sub_6D1228C, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE vb_handle, U32* offset, VertexBufferDesc* src_vb_desc, U32 start_vertex, U32 num_vertices);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_lock_vb, _sub_6D126BB, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE vb_handle, U32* start_index, void*& out_data, U32 num_verts);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_unlock_vb, _sub_6D12A8B, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE vb_handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_RPVertexBuffer_Unknown24, _sub_6D12B30, IRPVertexBuffer* _this, UNKNOWN);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_select_vb, _sub_6D12B9D, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE vb_handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_vb_count, _sub_6D12CDC, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE vb_handle, U32* out_vertex_format, U32* out_num_verts);
TRAMPOLINE(BOOL32, __stdcall, DirectX8_is_vb_valid, _sub_6D12B3C, IRPVertexBuffer* _this, IRP_VERTEXBUFFERHANDLE vb_handle);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_gamma_function, _sub_6D153E6, IGammaControl* _this, IGC_COMPONENT which, float display_gamma, float bias, float slope, float black_offset);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_gamma_ramp, _sub_6D1560C, IGammaControl* _this, IGC_COMPONENT igc_component, U16* ramp);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_gamma_ramp, _sub_6D15757, IGammaControl* _this, IGC_COMPONENT igc_component, U16* out_ramp);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_set_calibration_enable, _sub_6D1535D, IGammaControl* _this, bool enabled);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_get_calibration_enable, _sub_6D153A4, IGammaControl* _this);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_print_screen, _sub_6D13D70, IRPTexture* _this, IFileSystem* IFS, const char* child);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_load_texture, _sub_6D148D4, IRPTexture* _this, IFileSystem* IFS, const char* child, IRP_TEXTUREHANDLE* out_htexture);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_load_surface_from_file, _sub_6D1455E, IRPTexture* _this, UNKNOWN* a2_interface, UNKNOWN a3, UNKNOWN a4, UNKNOWN a5);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_load_dds_info, _sub_6D152E4, IRPTexture* _this, DDSInfo* out_info, DWORD membytesize, void* mapped_file_mem);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_load_cubemap, _sub_6D14DD7, IRPTexture* _this, IFileSystem* IFS, const char* child, IRP_TEXTUREHANDLE* out_htexture);
TRAMPOLINE(GENRESULT, __stdcall, DirectX8_Initialize, _sub_6D01C68, IAggregateComponent* _this);

#define CLSID_DirectX8 "DirectX8"

struct CACHED_PIPELINE_STATE
{
	DWORD value;
	DWORD unknown4;
	DWORD valid;
};

class DirectX8 : IRenderPipeline8B, IVertexBufferManager, IRPDraw, IRPIndexBuffer, IRPVertexBuffer, IGammaControl, IRPTexture, IAggregateComponent
{
public:
	COMPTR<IProfileParser> profile_parser;
	char ini_device_profile[128];
	char current_device_profile[128];
	DWORD direct3d_behavior_flags;
	DWORD unknown128;
	LPDIRECT3D8 direct3d;
	IDirect3DDevice8* direct3d_device;
	IDirect3DSurface8* direct3d_surface;
	CACHED_GEOMETRY curr_hw_geometry;
	DX8IndexBuffer scratchIB;
	DWORD unknown168;
	DWORD unknown16C;
	DWORD unknown170;
	DWORD unknown174;
	DWORD unknown178;
	DWORD unknown17C;
	DWORD unknown180;
	BYTE unknown184_is_locked;
	BYTE unknown185;
	BYTE unknown186;
	BYTE unknown187;
	DWORD unknown188;
	DWORD unknown18C;
	DWORD unknown190;
	DWORD unknown194;
	st6::vector<RPDEVICEINFO> unknown198;
	DWORD direct3d_adapter;
	D3DDEVTYPE direct3d_device_type;
	char configuration_database_file[128];
	DWORD unknown230;
	DWORD unknown234;
	RPBUFFERSINFO buffers_info;
	HWND hwnd;
	DWORD window_x;
	DWORD window_y;
	DWORD window_width;
	DWORD window_height;
	HWND window;
	PipelineStateArray pipeline_state_info;
	AbilitiesArray abilities_info;
	RenderStateArray render_state_info;
	TextureStageStateArray texture_stage_state_info[D3DTSS_NUM_STAGES];
	CACHED_PIPELINE_STATE pipeline_states[RP_MAX_PIPELINE_STATE];
	U32 device_abilities[20];
	DWORD unknown44C;
	DWORD unknown450;
	DWORD unknown454;
	DWORD unknown458;
	DWORD unknown45C;
	DWORD unknown460;
	DWORD unknown464;
	DWORD unknown468;
	DWORD unknown46C;
	DWORD unknown470;
	DWORD unknown474;
	DWORD unknown478;
	DWORD unknown47C;
	DWORD unknown480;
	DWORD unknown484;
	DWORD unknown488;
	DWORD unknown48C;
	DWORD unknown490;
	DWORD unknown494;
	DWORD unknown498;
	DWORD unknown49C;
	DWORD unknown4A0;
	DWORD unknown4A4;
	DWORD unknown4A8;
	DWORD unknown4AC;
	DWORD unknown4B0;
	DWORD unknown4B4;
	DWORD unknown4B8;
	DWORD unknown4BC;
	DWORD unknown4C0;
	DWORD unknown4C4;
	DWORD unknown4C8;
	DWORD unknown4CC;
	DWORD unknown4D0;
	DWORD unknown4D4;
	DWORD unknown4D8;
	DWORD unknown4DC;
	DWORD unknown4E0;
	DWORD unknown4E4;
	DWORD unknown4E8;
	DWORD unknown4EC;
	DWORD unknown4F0;
	DWORD unknown4F4;
	DWORD unknown4F8;
	DWORD unknown4FC;
	DWORD unknown500;
	DWORD unknown504;
	DWORD unknown508;
	DWORD unknown50C;
	DWORD unknown510;
	DWORD unknown514;
	DWORD unknown518;
	DWORD unknown51C;
	DWORD unknown520;
	DWORD unknown524;
	DWORD unknown528;
	DWORD unknown52C;
	DWORD unknown530;
	DWORD unknown534;
	DWORD unknown538;
	DWORD unknown53C;
	DWORD unknown540;
	DWORD unknown544;
	DWORD unknown548;
	DWORD unknown54C;
	DWORD unknown550;
	DWORD unknown554;
	DWORD unknown558;
	DWORD unknown55C;
	DWORD unknown560;
	DWORD unknown564;
	DWORD unknown568;
	DWORD unknown56C;
	DWORD unknown570;
	DWORD unknown574;
	DWORD unknown578;
	DWORD unknown57C;
	DWORD unknown580;
	DWORD unknown584;
	DWORD unknown588;
	DWORD unknown58C;
	DWORD unknown590;
	DWORD unknown594;
	DWORD unknown598;
	DWORD unknown59C;
	DWORD unknown5A0;
	DWORD unknown5A4;
	DWORD unknown5A8;
	DWORD unknown5AC;
	DWORD unknown5B0;
	DWORD unknown5B4;
	DWORD unknown5B8;
	DWORD unknown5BC;
	DWORD unknown5C0;
	DWORD unknown5C4;
	DWORD unknown5C8;
	DWORD unknown5CC;
	DWORD unknown5D0;
	DWORD unknown5D4;
	DWORD unknown5D8;
	DWORD unknown5DC;
	DWORD unknown5E0;
	DWORD unknown5E4;
	DWORD unknown5E8;
	DWORD unknown5EC;
	DWORD unknown5F0;
	DWORD unknown5F4;
	DWORD unknown5F8;
	DWORD unknown5FC;
	DWORD unknown600;
	DWORD unknown604;
	DWORD unknown608;
	DWORD unknown60C;
	DWORD unknown610;
	DWORD unknown614;
	DWORD unknown618;
	DWORD unknown61C;
	DWORD unknown620;
	DWORD unknown624;
	DWORD unknown628;
	DWORD unknown62C;
	DWORD unknown630;
	DWORD unknown634;
	DWORD unknown638;
	DWORD unknown63C;
	DWORD unknown640;
	DWORD unknown644;
	DWORD unknown648;
	DWORD unknown64C;
	DWORD unknown650;
	DWORD unknown654;
	DWORD unknown658;
	DWORD unknown65C;
	DWORD unknown660;
	DWORD unknown664;
	DWORD unknown668;
	DWORD unknown66C;
	DWORD unknown670;
	DWORD unknown674;
	DWORD unknown678;
	DWORD unknown67C;
	DWORD unknown680;
	DWORD unknown684;
	DWORD unknown688;
	DWORD unknown68C;
	DWORD unknown690;
	DWORD unknown694;
	DWORD unknown698;
	DWORD unknown69C;
	DWORD unknown6A0;
	DWORD unknown6A4;
	DWORD unknown6A8;
	DWORD unknown6AC;
	DWORD unknown6B0;
	DWORD unknown6B4;
	DWORD unknown6B8;
	DWORD unknown6BC;
	DWORD unknown6C0;
	DWORD unknown6C4;
	DWORD unknown6C8;
	DWORD unknown6CC;
	DWORD unknown6D0;
	DWORD unknown6D4;
	DWORD unknown6D8;
	DWORD unknown6DC;
	DWORD unknown6E0;
	DWORD unknown6E4;
	DWORD unknown6E8;
	DWORD unknown6EC;
	DWORD unknown6F0;
	DWORD unknown6F4;
	DWORD unknown6F8;
	DWORD unknown6FC;
	DWORD unknown700;
	DWORD unknown704;
	DWORD unknown708;
	DWORD unknown70C;
	DWORD unknown710;
	DWORD unknown714;
	DWORD unknown718;
	DWORD unknown71C;
	DWORD unknown720;
	DWORD unknown724;
	DWORD unknown728;
	DWORD unknown72C;
	DWORD unknown730;
	DWORD unknown734;
	DWORD unknown738;
	DWORD unknown73C;
	DWORD unknown740;
	DWORD unknown744;
	DWORD unknown748;
	DWORD unknown74C;
	DWORD unknown750;
	DWORD unknown754;
	DWORD unknown758;
	DWORD unknown75C;
	DWORD unknown760;
	DWORD unknown764;
	DWORD unknown768;
	DWORD unknown76C;
	DWORD unknown770;
	DWORD unknown774;
	DWORD unknown778;
	DWORD unknown77C;
	DWORD unknown780;
	DWORD unknown784;
	DWORD unknown788;
	DWORD unknown78C;
	DWORD unknown790;
	DWORD unknown794;
	DWORD unknown798;
	DWORD unknown79C;
	DWORD unknown7A0;
	DWORD unknown7A4;
	DWORD unknown7A8;
	DWORD unknown7AC;
	DWORD unknown7B0;
	DWORD unknown7B4;
	DWORD unknown7B8;
	DWORD unknown7BC;
	DWORD unknown7C0;
	DWORD unknown7C4;
	DWORD unknown7C8;
	DWORD unknown7CC;
	DWORD unknown7D0;
	DWORD unknown7D4;
	DWORD unknown7D8;
	DWORD unknown7DC;
	DWORD unknown7E0;
	DWORD unknown7E4;
	DWORD unknown7E8;
	DWORD unknown7EC;
	DWORD unknown7F0;
	DWORD unknown7F4;
	DWORD unknown7F8;
	DWORD unknown7FC;
	DWORD unknown800;
	DWORD unknown804;
	DWORD unknown808;
	DWORD unknown80C;
	DWORD unknown810;
	DWORD unknown814;
	DWORD unknown818;
	DWORD unknown81C;
	DWORD unknown820;
	DWORD unknown824;
	DWORD unknown828;
	DWORD unknown82C;
	DWORD unknown830;
	DWORD unknown834;
	DWORD unknown838;
	DWORD unknown83C;
	DWORD unknown840;
	DWORD unknown844;
	DWORD unknown848;
	DWORD unknown84C;
	DWORD unknown850;
	DWORD unknown854;
	DWORD unknown858;
	DWORD unknown85C;
	DWORD unknown860;
	DWORD unknown864;
	DWORD unknown868;
	DWORD unknown86C;
	DWORD unknown870;
	DWORD unknown874;
	DWORD unknown878;
	DWORD unknown87C;
	DWORD unknown880;
	DWORD unknown884;
	DWORD unknown888;
	DWORD unknown88C;
	DWORD unknown890;
	DWORD unknown894;
	DWORD unknown898;
	DWORD unknown89C;
	DWORD unknown8A0;
	DWORD unknown8A4;
	DWORD unknown8A8;
	DWORD unknown8AC;
	DWORD unknown8B0;
	DWORD unknown8B4;
	DWORD unknown8B8;
	DWORD unknown8BC;
	DWORD unknown8C0;
	DWORD unknown8C4;
	DWORD unknown8C8;
	DWORD unknown8CC;
	DWORD unknown8D0;
	DWORD unknown8D4;
	DWORD unknown8D8;
	DWORD unknown8DC;
	DWORD unknown8E0;
	DWORD unknown8E4;
	DWORD unknown8E8;
	DWORD unknown8EC;
	DWORD unknown8F0;
	DWORD unknown8F4;
	DWORD unknown8F8;
	DWORD unknown8FC;
	DWORD unknown900;
	DWORD unknown904;
	DWORD unknown908;
	DWORD unknown90C;
	DWORD unknown910;
	DWORD unknown914;
	DWORD unknown918;
	DWORD unknown91C;
	DWORD unknown920;
	DWORD unknown924;
	DWORD unknown928;
	DWORD unknown92C;
	DWORD unknown930;
	DWORD unknown934;
	DWORD unknown938;
	DWORD unknown93C;
	DWORD unknown940;
	DWORD unknown944;
	DWORD unknown948;
	DWORD unknown94C;
	DWORD unknown950;
	DWORD unknown954;
	DWORD unknown958;
	DWORD unknown95C;
	DWORD unknown960;
	DWORD unknown964;
	DWORD unknown968;
	DWORD unknown96C;
	DWORD unknown970;
	DWORD unknown974;
	DWORD unknown978;
	DWORD unknown97C;
	DWORD unknown980;
	DWORD unknown984;
	DWORD unknown988;
	DWORD unknown98C;
	DWORD unknown990;
	DWORD unknown994;
	DWORD unknown998;
	DWORD unknown99C;
	DWORD unknown9A0;
	DWORD unknown9A4;
	DWORD unknown9A8;
	DWORD unknown9AC;
	DWORD unknown9B0;
	DWORD unknown9B4;
	DWORD unknown9B8;
	DWORD unknown9BC;
	DWORD unknown9C0;
	DWORD unknown9C4;
	DWORD unknown9C8;
	DWORD unknown9CC;
	DWORD unknown9D0;
	DWORD unknown9D4;
	DWORD unknown9D8;
	DWORD unknown9DC;
	DWORD unknown9E0;
	DWORD unknown9E4;
	DWORD unknown9E8;
	DWORD unknown9EC;
	DWORD unknown9F0;
	DWORD unknown9F4;
	DWORD unknown9F8;
	DWORD unknown9FC;
	DWORD unknownA00;
	DWORD unknownA04;
	DWORD unknownA08;
	DWORD unknownA0C;
	DWORD unknownA10;
	DWORD unknownA14;
	DWORD unknownA18;
	DWORD unknownA1C;
	DWORD unknownA20;
	DWORD unknownA24;
	DWORD unknownA28;
	DWORD unknownA2C;
	DWORD unknownA30;
	DWORD unknownA34;
	DWORD unknownA38;
	DWORD unknownA3C;
	DWORD unknownA40;
	DWORD unknownA44;
	DWORD unknownA48;
	DWORD unknownA4C;
	DWORD unknownA50;
	DWORD unknownA54;
	DWORD unknownA58;
	DWORD unknownA5C;
	DWORD unknownA60;
	DWORD unknownA64;
	DWORD unknownA68;
	DWORD unknownA6C;
	DWORD unknownA70;
	DWORD unknownA74;
	DWORD unknownA78;
	DWORD unknownA7C;
	DWORD unknownA80;
	DWORD unknownA84;
	DWORD unknownA88;
	DWORD unknownA8C;
	DWORD unknownA90;
	DWORD unknownA94;
	DWORD unknownA98;
	DWORD unknownA9C;
	DWORD unknownAA0;
	DWORD unknownAA4;
	DWORD unknownAA8;
	DWORD unknownAAC;
	DWORD unknownAB0;
	DWORD unknownAB4;
	DWORD unknownAB8;
	DWORD unknownABC;
	DWORD unknownAC0;
	DWORD unknownAC4;
	DWORD unknownAC8;
	DWORD unknownACC;
	DWORD unknownAD0;
	DWORD unknownAD4;
	DWORD unknownAD8;
	DWORD unknownADC;
	DWORD unknownAE0;
	DWORD unknownAE4;
	DWORD unknownAE8;
	DWORD unknownAEC;
	DWORD unknownAF0;
	DWORD unknownAF4;
	DWORD unknownAF8;
	DWORD unknownAFC;
	DWORD unknownB00;
	DWORD unknownB04;
	DWORD unknownB08;
	DWORD unknownB0C;
	DWORD unknownB10;
	DWORD unknownB14;
	DWORD unknownB18;
	DWORD unknownB1C;
	DWORD unknownB20;
	DWORD unknownB24;
	DWORD unknownB28;
	DWORD unknownB2C;
	DWORD unknownB30;
	DWORD unknownB34;
	DWORD unknownB38;
	DWORD unknownB3C;
	DWORD unknownB40;
	DWORD unknownB44;
	DWORD unknownB48;
	DWORD unknownB4C;
	DWORD unknownB50;
	DWORD unknownB54;
	DWORD unknownB58;
	DWORD unknownB5C;
	DWORD unknownB60;
	DWORD unknownB64;
	DWORD unknownB68;
	DWORD unknownB6C;
	DWORD unknownB70;
	DWORD unknownB74;
	DWORD unknownB78;
	DWORD unknownB7C;
	DWORD unknownB80;
	DWORD unknownB84;
	DWORD unknownB88;
	DWORD unknownB8C;
	DWORD unknownB90;
	DWORD unknownB94;
	DWORD unknownB98;
	DWORD unknownB9C;
	DWORD unknownBA0;
	DWORD unknownBA4;
	DWORD unknownBA8;
	DWORD unknownBAC;
	DWORD unknownBB0;
	DWORD unknownBB4;
	DWORD unknownBB8;
	DWORD unknownBBC;
	DWORD unknownBC0;
	DWORD unknownBC4;
	DWORD unknownBC8;
	DWORD unknownBCC;
	DWORD unknownBD0;
	DWORD unknownBD4;
	DWORD unknownBD8;
	DWORD unknownBDC;
	DWORD unknownBE0;
	DWORD unknownBE4;
	DWORD unknownBE8;
	DWORD unknownBEC;
	DWORD unknownBF0;
	DWORD unknownBF4;
	DWORD unknownBF8;
	DWORD unknownBFC;
	DWORD unknownC00;
	DWORD unknownC04;
	DWORD unknownC08;
	DWORD unknownC0C;
	DWORD unknownC10;
	DWORD unknownC14;
	DWORD unknownC18;
	DWORD unknownC1C;
	DWORD unknownC20;
	DWORD unknownC24;
	DWORD unknownC28;
	DWORD unknownC2C;
	DWORD unknownC30;
	DWORD unknownC34;
	DWORD unknownC38;
	DWORD unknownC3C;
	DWORD unknownC40;
	DWORD unknownC44;
	DWORD unknownC48;
	DWORD unknownC4C;
	DWORD unknownC50;
	DWORD unknownC54;
	DWORD unknownC58;
	DWORD unknownC5C;
	DWORD unknownC60;
	DWORD unknownC64;
	DWORD unknownC68;
	DWORD unknownC6C;
	DWORD unknownC70;
	DWORD unknownC74;
	DWORD unknownC78;
	DWORD unknownC7C;
	DWORD unknownC80;
	DWORD unknownC84;
	DWORD unknownC88;
	DWORD unknownC8C;
	DWORD unknownC90;
	DWORD unknownC94;
	DWORD unknownC98;
	DWORD unknownC9C;
	DWORD unknownCA0;
	DWORD unknownCA4;
	DWORD unknownCA8;
	DWORD unknownCAC;
	DWORD unknownCB0;
	DWORD unknownCB4;
	DWORD unknownCB8;
	DWORD unknownCBC;
	DWORD unknownCC0;
	DWORD unknownCC4;
	DWORD unknownCC8;
	DWORD unknownCCC;
	DWORD unknownCD0;
	DWORD unknownCD4;
	DWORD unknownCD8;
	DWORD unknownCDC;
	DWORD unknownCE0;
	DWORD unknownCE4;
	DWORD unknownCE8;
	DWORD unknownCEC;
	DWORD unknownCF0;
	DWORD unknownCF4;
	DWORD unknownCF8;
	DWORD unknownCFC;
	DWORD unknownD00;
	DWORD unknownD04;
	DWORD unknownD08;
	DWORD unknownD0C;
	DWORD unknownD10;
	DWORD unknownD14;
	DWORD unknownD18;
	DWORD unknownD1C;
	DWORD unknownD20;
	DWORD unknownD24;
	DWORD unknownD28;
	DWORD unknownD2C;
	DWORD unknownD30;
	DWORD unknownD34;
	DWORD unknownD38;
	DWORD unknownD3C;
	DWORD unknownD40;
	DWORD unknownD44;
	DWORD unknownD48;
	DWORD unknownD4C;
	DWORD unknownD50;
	DWORD unknownD54;
	DWORD unknownD58;
	DWORD unknownD5C;
	DWORD unknownD60;
	DWORD unknownD64;
	DWORD unknownD68;
	DWORD unknownD6C;
	DWORD unknownD70;
	DWORD unknownD74;
	DWORD unknownD78;
	DWORD unknownD7C;
	DWORD unknownD80;
	DWORD unknownD84;
	DWORD unknownD88;
	DWORD unknownD8C;
	DWORD unknownD90;
	DWORD unknownD94;
	DWORD unknownD98;
	DWORD unknownD9C;
	DWORD unknownDA0;
	DWORD unknownDA4;
	DWORD unknownDA8;
	DWORD unknownDAC;
	DWORD unknownDB0;
	DWORD unknownDB4;
	DWORD unknownDB8;
	DWORD unknownDBC;
	DWORD unknownDC0;
	DWORD unknownDC4;
	DWORD unknownDC8;
	DWORD unknownDCC;
	DWORD unknownDD0;
	DWORD unknownDD4;
	DWORD unknownDD8;
	DWORD unknownDDC;
	DWORD unknownDE0;
	DWORD unknownDE4;
	DWORD unknownDE8;
	DWORD unknownDEC;
	DWORD unknownDF0;
	DWORD unknownDF4;
	DWORD unknownDF8;
	DWORD unknownDFC;
	DWORD unknownE00;
	DWORD unknownE04;
	DWORD unknownE08;
	DWORD unknownE0C;
	DWORD unknownE10;
	DWORD unknownE14;
	DWORD unknownE18;
	DWORD unknownE1C;
	DWORD unknownE20;
	DWORD unknownE24;
	DWORD unknownE28;
	DWORD unknownE2C;
	DWORD unknownE30;
	DWORD unknownE34;
	DWORD unknownE38;
	DWORD unknownE3C;
	DWORD unknownE40;
	DWORD unknownE44;
	DWORD unknownE48;
	DWORD unknownE4C;
	DWORD unknownE50;
	DWORD unknownE54;
	DWORD unknownE58;
	DWORD unknownE5C;
	DWORD unknownE60;
	DWORD unknownE64;
	DWORD unknownE68;
	DWORD unknownE6C;
	DWORD unknownE70;
	DWORD unknownE74;
	DWORD unknownE78;
	DWORD unknownE7C;
	DWORD unknownE80;
	DWORD unknownE84;
	DWORD unknownE88;
	DWORD unknownE8C;
	DWORD unknownE90;
	DWORD unknownE94;
	DWORD unknownE98;
	DWORD unknownE9C;
	DWORD unknownEA0;
	DWORD unknownEA4;
	DWORD unknownEA8;
	DWORD unknownEAC;
	DWORD unknownEB0;
	DWORD unknownEB4;
	DWORD unknownEB8;
	DWORD unknownEBC;
	DWORD unknownEC0;
	DWORD unknownEC4;
	DWORD unknownEC8;
	DWORD unknownECC;
	DWORD unknownED0;
	DWORD unknownED4;
	DWORD unknownED8;
	DWORD unknownEDC;
	DWORD unknownEE0;
	DWORD unknownEE4;
	DWORD unknownEE8;
	DWORD unknownEEC;
	DWORD unknownEF0;
	DWORD unknownEF4;
	DWORD unknownEF8;
	DWORD unknownEFC;
	DWORD unknownF00;
	DWORD unknownF04;
	DWORD unknownF08;
	DWORD unknownF0C;
	DWORD unknownF10;
	DWORD unknownF14;
	DWORD unknownF18;
	DWORD unknownF1C;
	DWORD unknownF20;
	DWORD unknownF24;
	DWORD unknownF28;
	DWORD unknownF2C;
	DWORD unknownF30;
	DWORD unknownF34;
	DWORD unknownF38;
	DWORD unknownF3C;
	DWORD unknownF40;
	DWORD unknownF44;
	DWORD unknownF48;
	DWORD unknownF4C;
	DWORD unknownF50;
	DWORD unknownF54;
	DWORD unknownF58;
	DWORD unknownF5C;
	DWORD unknownF60;
	DWORD unknownF64;
	DWORD unknownF68;
	DWORD unknownF6C;
	DWORD unknownF70;
	DWORD unknownF74;
	DWORD unknownF78;
	DWORD unknownF7C;
	DWORD unknownF80;
	DWORD unknownF84;
	DWORD unknownF88;
	DWORD unknownF8C;
	DWORD unknownF90;
	DWORD unknownF94;
	DWORD unknownF98;
	DWORD unknownF9C;
	DWORD unknownFA0;
	DWORD unknownFA4;
	DWORD unknownFA8;
	DWORD unknownFAC;
	DWORD unknownFB0;
	DWORD unknownFB4;
	DWORD unknownFB8;
	DWORD unknownFBC;
	DWORD unknownFC0;
	DWORD unknownFC4;
	DWORD unknownFC8;
	DWORD unknownFCC;
	DWORD unknownFD0;
	DWORD unknownFD4;
	DWORD unknownFD8;
	DWORD unknownFDC;
	DWORD unknownFE0;
	DWORD unknownFE4;
	DWORD unknownFE8;
	DWORD unknownFEC;
	DWORD unknownFF0;
	DWORD unknownFF4;
	DWORD unknownFF8;
	DWORD unknownFFC;
	DWORD unknown1000;
	DWORD unknown1004;
	DWORD unknown1008;
	DWORD unknown100C;
	DWORD unknown1010;
	DWORD unknown1014;
	DWORD unknown1018;
	DWORD unknown101C;
	DWORD unknown1020;
	DWORD unknown1024;
	DWORD unknown1028;
	DWORD unknown102C;
	DWORD unknown1030;
	DWORD unknown1034;
	DWORD unknown1038;
	DWORD unknown103C;
	DWORD unknown1040;
	DWORD unknown1044;
	DWORD unknown1048;
	DWORD unknown104C;
	DWORD unknown1050;
	DWORD unknown1054;
	DWORD unknown1058;
	DWORD unknown105C;
	DWORD unknown1060;
	DWORD unknown1064;
	DWORD unknown1068;
	DWORD unknown106C;
	DWORD unknown1070;
	DWORD unknown1074;
	DWORD unknown1078;
	DWORD unknown107C;
	DWORD unknown1080;
	DWORD unknown1084;
	DWORD unknown1088;
	DWORD unknown108C;
	DWORD unknown1090;
	DWORD unknown1094;
	DWORD unknown1098;
	DWORD unknown109C;
	DWORD unknown10A0;
	DWORD unknown10A4;
	DWORD unknown10A8;
	DWORD unknown10AC;
	DWORD unknown10B0;
	DWORD unknown10B4;
	DWORD unknown10B8;
	DWORD unknown10BC;
	DWORD unknown10C0;
	DWORD unknown10C4;
	DWORD unknown10C8;
	DWORD unknown10CC;
	DWORD unknown10D0;
	DWORD unknown10D4;
	DWORD unknown10D8;
	DWORD unknown10DC;
	DWORD unknown10E0;
	DWORD unknown10E4;
	DWORD unknown10E8;
	DWORD unknown10EC;
	DWORD unknown10F0;
	DWORD unknown10F4;
	DWORD unknown10F8;
	DWORD unknown10FC;
	DWORD unknown1100;
	DWORD unknown1104;
	DWORD unknown1108;
	DWORD unknown110C;
	DWORD unknown1110;
	DWORD unknown1114;
	DWORD unknown1118;
	DWORD unknown111C;
	DWORD unknown1120;
	DWORD unknown1124;
	DWORD unknown1128;
	DWORD unknown112C;
	DWORD unknown1130;
	DWORD unknown1134;
	DWORD unknown1138;
	DWORD unknown113C;
	DWORD unknown1140;
	DWORD unknown1144;
	DWORD unknown1148;
	DWORD unknown114C;
	DWORD unknown1150;
	DWORD unknown1154;
	DWORD unknown1158;
	DWORD unknown115C;
	DWORD unknown1160;
	DWORD unknown1164;
	DWORD unknown1168;
	DWORD unknown116C;
	DWORD unknown1170;
	DWORD unknown1174;
	DWORD unknown1178;
	DWORD unknown117C;
	DWORD unknown1180;
	DWORD unknown1184;
	DWORD unknown1188;
	DWORD unknown118C;
	DWORD unknown1190;
	DWORD unknown1194;
	DWORD unknown1198;
	DWORD unknown119C;
	DWORD unknown11A0;
	DWORD unknown11A4;
	DWORD unknown11A8;
	DWORD unknown11AC;
	DWORD unknown11B0;
	DWORD unknown11B4;
	DWORD unknown11B8;
	DWORD unknown11BC;
	DWORD unknown11C0;
	DWORD unknown11C4;
	DWORD unknown11C8;
	DWORD unknown11CC;
	DWORD unknown11D0;
	DWORD unknown11D4;
	DWORD unknown11D8;
	DWORD unknown11DC;
	DWORD unknown11E0;
	DWORD unknown11E4;
	DWORD unknown11E8;
	DWORD unknown11EC;
	DWORD unknown11F0;
	DWORD unknown11F4;
	DWORD unknown11F8;
	DWORD unknown11FC;
	DWORD unknown1200;
	DWORD unknown1204;
	DWORD unknown1208;
	DWORD unknown120C;
	DWORD unknown1210;
	DWORD unknown1214;
	DWORD unknown1218;
	DWORD unknown121C;
	DWORD unknown1220;
	DWORD unknown1224;
	DWORD unknown1228;
	DWORD unknown122C;
	DWORD unknown1230;
	DWORD unknown1234;
	DWORD unknown1238;
	DWORD unknown123C;
	DWORD unknown1240;
	DWORD unknown1244;
	DWORD unknown1248;
	DWORD unknown124C;
	DWORD unknown1250;
	DWORD unknown1254;
	DWORD unknown1258;
	DWORD unknown125C;
	DWORD unknown1260;
	DWORD unknown1264;
	DWORD unknown1268;
	DWORD unknown126C;
	DWORD unknown1270;
	DWORD unknown1274;
	DWORD unknown1278;
	DWORD unknown127C;
	DWORD unknown1280;
	DWORD unknown1284;
	DWORD unknown1288;
	DWORD unknown128C;
	DWORD unknown1290;
	DWORD unknown1294;
	DWORD unknown1298;
	DWORD unknown129C;
	DWORD unknown12A0;
	DWORD unknown12A4;
	DWORD unknown12A8;
	DWORD unknown12AC;
	DWORD unknown12B0;
	DWORD unknown12B4;
	DWORD unknown12B8;
	DWORD unknown12BC;
	DWORD unknown12C0;
	DWORD unknown12C4;
	DWORD unknown12C8;
	DWORD unknown12CC;
	DWORD unknown12D0;
	DWORD unknown12D4;
	DWORD unknown12D8;
	DWORD unknown12DC;
	DWORD unknown12E0;
	DWORD unknown12E4;
	DWORD unknown12E8;
	DWORD unknown12EC;
	DWORD unknown12F0;
	DWORD unknown12F4;
	DWORD unknown12F8;
	DWORD unknown12FC;
	DWORD unknown1300;
	DWORD unknown1304;
	DWORD unknown1308;
	DWORD unknown130C;
	DWORD unknown1310;
	DWORD unknown1314;
	DWORD unknown1318;
	DWORD unknown131C;
	DWORD unknown1320;
	DWORD unknown1324;
	DWORD unknown1328;
	DWORD unknown132C;
	DWORD unknown1330;
	DWORD unknown1334;
	DWORD unknown1338;
	DWORD unknown133C;
	DWORD unknown1340;
	DWORD unknown1344;
	DWORD unknown1348;
	DWORD unknown134C;
	DWORD unknown1350;
	DWORD unknown1354;
	DWORD unknown1358;
	DWORD unknown135C;
	DWORD unknown1360;
	DWORD unknown1364;
	DWORD unknown1368;
	DWORD unknown136C;
	DWORD unknown1370;
	DWORD unknown1374;
	DWORD unknown1378;
	DWORD unknown137C;
	DWORD unknown1380;
	DWORD unknown1384;
	DWORD unknown1388;
	DWORD unknown138C;
	DWORD unknown1390;
	DWORD unknown1394;
	DWORD unknown1398;
	DWORD unknown139C;
	DWORD unknown13A0;
	DWORD unknown13A4;
	DWORD unknown13A8;
	DWORD unknown13AC;
	DWORD unknown13B0;
	DWORD unknown13B4;
	DWORD unknown13B8;
	DWORD unknown13BC;
	DWORD unknown13C0;
	DWORD unknown13C4;
	DWORD unknown13C8;
	DWORD unknown13CC;
	DWORD unknown13D0;
	DWORD unknown13D4;
	DWORD unknown13D8;
	DWORD unknown13DC;
	DWORD unknown13E0;
	DWORD unknown13E4;
	DWORD unknown13E8;
	DWORD unknown13EC;
	DWORD unknown13F0;
	DWORD unknown13F4;
	DWORD unknown13F8;
	DWORD unknown13FC;
	DWORD unknown1400;
	DWORD unknown1404;
	DWORD unknown1408;
	DWORD unknown140C;
	DWORD unknown1410;
	DWORD unknown1414;
	DWORD unknown1418;
	DWORD unknown141C;
	DWORD unknown1420;
	DWORD unknown1424;
	DWORD unknown1428;
	DWORD unknown142C;
	DWORD unknown1430;
	DWORD unknown1434;
	DWORD unknown1438;
	DWORD unknown143C;
	DWORD unknown1440;
	DWORD unknown1444;
	DWORD unknown1448;
	DWORD unknown144C;
	DWORD unknown1450;
	DWORD unknown1454;
	DWORD unknown1458;
	DWORD unknown145C;
	DWORD unknown1460;
	DWORD unknown1464;
	DWORD unknown1468;
	DWORD unknown146C;
	DWORD unknown1470;
	DWORD unknown1474;
	DWORD unknown1478;
	DWORD unknown147C;
	DWORD unknown1480;
	DWORD unknown1484;
	DWORD unknown1488;
	DWORD unknown148C;
	DWORD unknown1490;
	DWORD unknown1494;
	DWORD unknown1498;
	DWORD unknown149C;
	DWORD unknown14A0;
	DWORD unknown14A4;
	DWORD unknown14A8;
	DWORD unknown14AC;
	DWORD unknown14B0;
	DWORD unknown14B4;
	DWORD unknown14B8;
	DWORD unknown14BC;
	DWORD unknown14C0;
	DWORD unknown14C4;
	DWORD unknown14C8;
	DWORD unknown14CC;
	DWORD unknown14D0;
	DWORD unknown14D4;
	DWORD unknown14D8;
	DWORD unknown14DC;
	DWORD unknown14E0;
	DWORD unknown14E4;
	DWORD unknown14E8;
	DWORD unknown14EC;
	DWORD unknown14F0;
	DWORD unknown14F4;
	DWORD unknown14F8;
	DWORD unknown14FC;
	DWORD unknown1500;
	DWORD unknown1504;
	DWORD unknown1508;
	DWORD unknown150C;
	DWORD unknown1510;
	DWORD unknown1514;
	DWORD unknown1518;
	DWORD unknown151C;
	DWORD unknown1520;
	DWORD unknown1524;
	DWORD unknown1528;
	DWORD unknown152C;
	DWORD unknown1530;
	DWORD unknown1534;
	DWORD unknown1538;
	DWORD unknown153C;
	DWORD unknown1540;
	DWORD unknown1544;
	DWORD unknown1548;
	DWORD unknown154C;
	DWORD unknown1550;
	DWORD unknown1554;
	DWORD unknown1558;
	DWORD unknown155C;
	DWORD unknown1560;
	DWORD unknown1564;
	DWORD unknown1568;
	DWORD unknown156C;
	DWORD unknown1570;
	DWORD unknown1574;
	DWORD unknown1578;
	DWORD unknown157C;
	DWORD unknown1580;
	DWORD unknown1584;
	DWORD unknown1588;
	DWORD unknown158C;
	DWORD unknown1590;
	DWORD unknown1594;
	DWORD unknown1598;
	DWORD unknown159C;
	DWORD unknown15A0;
	DWORD unknown15A4;
	DWORD unknown15A8;
	DWORD unknown15AC;
	DWORD unknown15B0;
	DWORD unknown15B4;
	DWORD unknown15B8;
	DWORD unknown15BC;
	DWORD unknown15C0;
	DWORD unknown15C4;
	DWORD unknown15C8;
	DWORD unknown15CC;
	DWORD unknown15D0;
	DWORD unknown15D4;
	DWORD unknown15D8;
	DWORD unknown15DC;
	DWORD unknown15E0;
	DWORD unknown15E4;
	DWORD unknown15E8;
	DWORD unknown15EC;
	DWORD unknown15F0;
	DWORD unknown15F4;
	DWORD unknown15F8;
	DWORD unknown15FC;
	DWORD unknown1600;
	DWORD unknown1604;
	DWORD unknown1608;
	DWORD unknown160C;
	DWORD unknown1610;
	DWORD unknown1614;
	DWORD unknown1618;
	DWORD unknown161C;
	DWORD unknown1620;
	DWORD unknown1624;
	DWORD unknown1628;
	DWORD unknown162C;
	DWORD unknown1630;
	DWORD unknown1634;
	DWORD unknown1638;
	DWORD unknown163C;
	DWORD unknown1640;
	DWORD unknown1644;
	DWORD unknown1648;
	DWORD unknown164C;
	DWORD unknown1650;
	DWORD unknown1654;
	DWORD unknown1658;
	DWORD unknown165C;
	DWORD unknown1660;
	DWORD unknown1664;
	DWORD unknown1668;
	DWORD unknown166C;
	DWORD unknown1670;
	DWORD unknown1674;
	DWORD unknown1678;
	DWORD unknown167C;
	DWORD unknown1680;
	DWORD unknown1684;
	DWORD unknown1688;
	DWORD unknown168C;
	DWORD unknown1690;
	DWORD unknown1694;
	DWORD unknown1698;
	DWORD unknown169C;
	DWORD unknown16A0;
	DWORD unknown16A4;
	DWORD unknown16A8;
	DWORD unknown16AC;
	DWORD unknown16B0;
	DWORD unknown16B4;
	DWORD unknown16B8;
	DWORD unknown16BC;
	DWORD unknown16C0;
	DWORD unknown16C4;
	DWORD unknown16C8;
	DWORD unknown16CC;
	DWORD unknown16D0;
	DWORD unknown16D4;
	DWORD unknown16D8;
	DWORD unknown16DC;
	DWORD unknown16E0;
	DWORD unknown16E4;
	DWORD unknown16E8;
	DWORD unknown16EC;
	DWORD unknown16F0;
	DWORD unknown16F4;
	DWORD unknown16F8;
	DWORD unknown16FC;
	DWORD unknown1700;
	DWORD unknown1704;
	DWORD unknown1708;
	DWORD unknown170C;
	DWORD unknown1710;
	DWORD unknown1714;
	DWORD unknown1718;
	DWORD unknown171C;
	DWORD unknown1720;
	DWORD unknown1724;
	DWORD unknown1728;
	DWORD unknown172C;
	DWORD unknown1730;
	DWORD unknown1734;
	DWORD unknown1738;
	DWORD unknown173C;
	DWORD unknown1740;
	DWORD unknown1744;
	DWORD unknown1748;
	DWORD unknown174C;
	DWORD unknown1750;
	DWORD unknown1754;
	DWORD unknown1758;
	DWORD unknown175C;
	DWORD unknown1760;
	DWORD unknown1764;
	DWORD unknown1768;
	DWORD unknown176C;
	DWORD unknown1770;
	DWORD unknown1774;
	DWORD unknown1778;
	DWORD unknown177C;
	DWORD unknown1780;
	DWORD unknown1784;
	DWORD unknown1788;
	DWORD unknown178C;
	DWORD unknown1790;
	DWORD unknown1794;
	DWORD unknown1798;
	DWORD unknown179C;
	DWORD unknown17A0;
	DWORD unknown17A4;
	DWORD unknown17A8;
	DWORD unknown17AC;
	DWORD unknown17B0;
	DWORD unknown17B4;
	DWORD unknown17B8;
	DWORD unknown17BC;
	DWORD unknown17C0;
	DWORD unknown17C4;
	DWORD unknown17C8;
	DWORD unknown17CC;
	DWORD unknown17D0;
	DWORD unknown17D4;
	DWORD unknown17D8;
	DWORD unknown17DC;
	DWORD unknown17E0;
	DWORD unknown17E4;
	DWORD unknown17E8;
	DWORD unknown17EC;
	DWORD unknown17F0;
	DWORD unknown17F4;
	DWORD unknown17F8;
	DWORD unknown17FC;
	DWORD unknown1800;
	DWORD unknown1804;
	DWORD unknown1808;
	DWORD unknown180C;
	DWORD unknown1810;
	DWORD unknown1814;
	DWORD unknown1818;
	DWORD unknown181C;
	DWORD unknown1820;
	DWORD unknown1824;
	DWORD unknown1828;
	DWORD unknown182C;
	DWORD unknown1830;
	DWORD unknown1834;
	DWORD unknown1838;
	DWORD unknown183C;
	DWORD unknown1840;
	DWORD unknown1844;
	DWORD unknown1848;
	DWORD unknown184C;
	DWORD unknown1850;
	DWORD unknown1854;
	DWORD unknown1858;
	DWORD unknown185C;
	DWORD unknown1860;
	DWORD unknown1864;
	DWORD unknown1868;
	DWORD unknown186C;
	DWORD unknown1870;
	DWORD unknown1874;
	DWORD unknown1878;
	DWORD unknown187C;
	DWORD unknown1880;
	DWORD unknown1884;
	DWORD unknown1888;
	DWORD unknown188C;
	DWORD unknown1890;
	DWORD unknown1894;
	DWORD unknown1898;
	DWORD unknown189C;
	DWORD unknown18A0;
	DWORD unknown18A4;
	DWORD unknown18A8;
	DWORD unknown18AC;
	DWORD unknown18B0;
	DWORD unknown18B4;
	DWORD unknown18B8;
	DWORD unknown18BC;
	DWORD unknown18C0;
	DWORD unknown18C4;
	DWORD unknown18C8;
	DWORD unknown18CC;
	DWORD unknown18D0;
	DWORD unknown18D4;
	DWORD unknown18D8;
	DWORD unknown18DC;
	DWORD unknown18E0;
	DWORD unknown18E4;
	DWORD unknown18E8;
	DWORD unknown18EC;
	DWORD unknown18F0;
	DWORD unknown18F4;
	DWORD unknown18F8;
	DWORD unknown18FC;
	DWORD unknown1900;
	DWORD unknown1904;
	DWORD unknown1908;
	DWORD unknown190C;
	DWORD unknown1910;
	DWORD unknown1914;
	DWORD unknown1918;
	DWORD unknown191C;
	DWORD unknown1920;
	DWORD unknown1924;
	DWORD unknown1928;
	DWORD unknown192C;
	DWORD unknown1930;
	DWORD unknown1934;
	DWORD unknown1938;
	DWORD unknown193C;
	DWORD unknown1940;
	DWORD unknown1944;
	DWORD unknown1948;
	DWORD unknown194C;
	DWORD unknown1950;
	DWORD unknown1954;
	DWORD unknown1958;
	DWORD unknown195C;
	DWORD unknown1960;
	DWORD unknown1964;
	DWORD unknown1968;
	DWORD unknown196C;
	DWORD unknown1970;
	DWORD unknown1974;
	DWORD unknown1978;
	DWORD unknown197C;
	DWORD unknown1980;
	DWORD unknown1984;
	DWORD unknown1988;
	DWORD unknown198C;
	DWORD unknown1990;
	DWORD unknown1994;
	DWORD unknown1998;
	DWORD unknown199C;
	DWORD unknown19A0;
	DWORD unknown19A4;
	DWORD unknown19A8;
	DWORD unknown19AC;
	DWORD unknown19B0;
	DWORD unknown19B4;
	DWORD unknown19B8;
	DWORD unknown19BC;
	DWORD unknown19C0;
	DWORD unknown19C4;
	DWORD unknown19C8;
	DWORD unknown19CC;
	DWORD unknown19D0;
	DWORD unknown19D4;
	DWORD unknown19D8;
	DWORD unknown19DC;
	DWORD unknown19E0;
	DWORD unknown19E4;
	DWORD unknown19E8;
	DWORD unknown19EC;
	DWORD unknown19F0;
	DWORD unknown19F4;
	DWORD unknown19F8;
	DWORD unknown19FC;
	DWORD unknown1A00;
	DWORD unknown1A04;
	DWORD unknown1A08;
	DWORD unknown1A0C;
	DWORD unknown1A10;
	DWORD unknown1A14;
	DWORD unknown1A18;
	DWORD unknown1A1C;
	DWORD unknown1A20;
	DWORD unknown1A24;
	DWORD unknown1A28;
	DWORD unknown1A2C;
	DWORD unknown1A30;
	DWORD unknown1A34;
	DWORD unknown1A38;
	DWORD unknown1A3C;
	DWORD unknown1A40;
	DWORD unknown1A44;
	DWORD unknown1A48;
	DWORD unknown1A4C;
	DWORD unknown1A50;
	DWORD unknown1A54;
	DWORD unknown1A58;
	DWORD unknown1A5C;
	DWORD unknown1A60;
	DWORD unknown1A64;
	DWORD unknown1A68;
	DWORD unknown1A6C;
	DWORD unknown1A70;
	DWORD unknown1A74;
	DWORD unknown1A78;
	DWORD unknown1A7C;
	DWORD unknown1A80;
	DWORD unknown1A84;
	DWORD unknown1A88;
	DWORD unknown1A8C;
	DWORD unknown1A90;
	DWORD unknown1A94;
	DWORD unknown1A98;
	DWORD unknown1A9C;
	DWORD unknown1AA0;
	DWORD unknown1AA4;
	DWORD unknown1AA8;
	DWORD unknown1AAC;
	DWORD unknown1AB0;
	DWORD unknown1AB4;
	DWORD unknown1AB8;
	DWORD unknown1ABC;
	DWORD unknown1AC0;
	DWORD unknown1AC4;
	DWORD unknown1AC8;
	DWORD unknown1ACC;
	DWORD unknown1AD0;
	DWORD unknown1AD4;
	DWORD unknown1AD8;
	DWORD unknown1ADC;
	DWORD unknown1AE0;
	DWORD unknown1AE4;
	DWORD unknown1AE8;
	DWORD unknown1AEC;
	DWORD unknown1AF0;
	DWORD unknown1AF4;
	DWORD unknown1AF8;
	DWORD unknown1AFC;
	DWORD unknown1B00;
	DWORD unknown1B04;
	DWORD unknown1B08;
	DWORD unknown1B0C;
	DWORD unknown1B10;
	DWORD unknown1B14;
	DWORD unknown1B18;
	DWORD unknown1B1C;
	DWORD unknown1B20;
	DWORD unknown1B24;
	DWORD unknown1B28;
	DWORD unknown1B2C;
	DWORD unknown1B30;
	DWORD unknown1B34;
	DWORD unknown1B38;
	DWORD unknown1B3C;
	DWORD unknown1B40;
	DWORD unknown1B44;
	DWORD unknown1B48;
	DWORD unknown1B4C;
	DWORD unknown1B50;
	DWORD unknown1B54;
	DWORD unknown1B58;
	DWORD unknown1B5C;
	DWORD unknown1B60;
	DWORD unknown1B64;
	DWORD unknown1B68;
	DWORD unknown1B6C;
	DWORD unknown1B70;
	DWORD unknown1B74;
	DWORD unknown1B78;
	DWORD unknown1B7C;
	DWORD unknown1B80;
	DWORD unknown1B84;
	DWORD unknown1B88;
	DWORD unknown1B8C;
	DWORD unknown1B90;
	DWORD unknown1B94;
	DWORD unknown1B98;
	DWORD unknown1B9C;
	DWORD unknown1BA0;
	DWORD unknown1BA4;
	DWORD unknown1BA8;
	DWORD unknown1BAC;
	DWORD unknown1BB0;
	DWORD unknown1BB4;
	DWORD unknown1BB8;
	DWORD unknown1BBC;
	DWORD unknown1BC0;
	DWORD unknown1BC4;
	DWORD unknown1BC8;
	DWORD unknown1BCC;
	DWORD unknown1BD0;
	DWORD unknown1BD4;
	DWORD unknown1BD8;
	DWORD unknown1BDC;
	DWORD unknown1BE0;
	DWORD unknown1BE4;
	DWORD unknown1BE8;
	DWORD unknown1BEC;
	DWORD unknown1BF0;
	DWORD unknown1BF4;
	DWORD unknown1BF8;
	DWORD unknown1BFC;
	DWORD unknown1C00;
	DWORD unknown1C04;
	DWORD unknown1C08;
	DWORD unknown1C0C;
	DWORD unknown1C10;
	DWORD unknown1C14;
	DWORD unknown1C18;
	DWORD unknown1C1C;
	DWORD unknown1C20;
	DWORD unknown1C24;
	DWORD unknown1C28;
	DWORD unknown1C2C;
	DWORD unknown1C30;
	DWORD unknown1C34;
	DWORD unknown1C38;
	DWORD unknown1C3C;
	DWORD unknown1C40;
	DWORD unknown1C44;
	DWORD unknown1C48;
	DWORD unknown1C4C;
	DWORD unknown1C50;
	DWORD unknown1C54;
	DWORD unknown1C58;
	DWORD unknown1C5C;
	DWORD unknown1C60;
	DWORD unknown1C64;
	DWORD unknown1C68;
	DWORD unknown1C6C;
	DWORD unknown1C70;
	DWORD unknown1C74;
	DWORD unknown1C78;
	DWORD unknown1C7C;
	DWORD unknown1C80;
	DWORD unknown1C84;
	DWORD unknown1C88;
	DWORD unknown1C8C;
	DWORD unknown1C90;
	DWORD unknown1C94;
	DWORD unknown1C98;
	DWORD unknown1C9C;
	DWORD unknown1CA0;
	DWORD unknown1CA4;
	DWORD unknown1CA8;
	DWORD unknown1CAC;
	DWORD unknown1CB0;
	DWORD unknown1CB4;
	DWORD unknown1CB8;
	DWORD unknown1CBC;
	DWORD unknown1CC0;
	DWORD unknown1CC4;
	DWORD unknown1CC8;
	DWORD unknown1CCC;
	DWORD unknown1CD0;
	DWORD unknown1CD4;
	DWORD unknown1CD8;
	DWORD unknown1CDC;
	DWORD unknown1CE0;
	DWORD unknown1CE4;
	DWORD unknown1CE8;
	DWORD unknown1CEC;
	DWORD unknown1CF0;
	DWORD unknown1CF4;
	DWORD unknown1CF8;
	DWORD unknown1CFC;
	DWORD unknown1D00;
	DWORD unknown1D04;
	DWORD unknown1D08;
	DWORD unknown1D0C;
	DWORD unknown1D10;
	DWORD unknown1D14;
	DWORD unknown1D18;
	DWORD unknown1D1C;
	DWORD unknown1D20;
	DWORD unknown1D24;
	DWORD unknown1D28;
	DWORD unknown1D2C;
	DWORD unknown1D30;
	DWORD unknown1D34;
	DWORD unknown1D38;
	DWORD unknown1D3C;
	DWORD unknown1D40;
	DWORD unknown1D44;
	DWORD unknown1D48;
	DWORD unknown1D4C;
	DWORD unknown1D50;
	DWORD unknown1D54;
	DWORD unknown1D58;
	DWORD unknown1D5C;
	DWORD unknown1D60;
	DWORD unknown1D64;
	DWORD unknown1D68;
	DWORD unknown1D6C;
	DWORD unknown1D70;
	DWORD unknown1D74;
	DWORD unknown1D78;
	DWORD unknown1D7C;
	DWORD unknown1D80;
	DWORD unknown1D84;
	DWORD unknown1D88;
	DWORD unknown1D8C;
	DWORD unknown1D90;
	DWORD unknown1D94;
	DWORD unknown1D98;
	DWORD unknown1D9C;
	DWORD unknown1DA0;
	DWORD unknown1DA4;
	DWORD unknown1DA8;
	DWORD unknown1DAC;
	DWORD unknown1DB0;
	DWORD unknown1DB4;
	DWORD unknown1DB8;
	DWORD unknown1DBC;
	DWORD unknown1DC0;
	DWORD unknown1DC4;
	DWORD unknown1DC8;
	DWORD unknown1DCC;
	DWORD unknown1DD0;
	DWORD unknown1DD4;
	DWORD unknown1DD8;
	DWORD unknown1DDC;
	DWORD unknown1DE0;
	DWORD unknown1DE4;
	DWORD unknown1DE8;
	DWORD unknown1DEC;
	DWORD unknown1DF0;
	DWORD unknown1DF4;
	DWORD unknown1DF8;
	DWORD unknown1DFC;
	DWORD unknown1E00;
	DWORD unknown1E04;
	DWORD unknown1E08;
	DWORD unknown1E0C;
	DWORD unknown1E10;
	DWORD unknown1E14;
	DWORD unknown1E18;
	DWORD unknown1E1C;
	DWORD unknown1E20;
	DWORD unknown1E24;
	DWORD unknown1E28;
	DWORD unknown1E2C;
	DWORD unknown1E30;
	DWORD unknown1E34;
	DWORD unknown1E38;
	DWORD unknown1E3C;
	DWORD unknown1E40;
	DWORD unknown1E44;
	DWORD unknown1E48;
	DWORD unknown1E4C;
	DWORD unknown1E50;
	DWORD unknown1E54;
	DWORD unknown1E58;
	DWORD unknown1E5C;
	DWORD unknown1E60;
	DWORD unknown1E64;
	DWORD unknown1E68;
	DWORD unknown1E6C;
	DWORD unknown1E70;
	DWORD unknown1E74;
	DWORD unknown1E78;
	DWORD unknown1E7C;
	DWORD unknown1E80;
	DWORD unknown1E84;
	DWORD unknown1E88;
	DWORD unknown1E8C;
	DWORD unknown1E90;
	DWORD unknown1E94;
	DWORD unknown1E98;
	DWORD unknown1E9C;
	DWORD unknown1EA0;
	DWORD unknown1EA4;
	DWORD unknown1EA8;
	DWORD unknown1EAC;
	DWORD unknown1EB0;
	DWORD unknown1EB4;
	DWORD unknown1EB8;
	DWORD unknown1EBC;
	DWORD unknown1EC0;
	DWORD unknown1EC4;
	DWORD unknown1EC8;
	DWORD unknown1ECC;
	DWORD unknown1ED0;
	DWORD unknown1ED4;
	DWORD unknown1ED8;
	DWORD unknown1EDC;
	DWORD unknown1EE0;
	DWORD unknown1EE4;
	DWORD unknown1EE8;
	DWORD unknown1EEC;
	DWORD unknown1EF0;
	DWORD unknown1EF4;
	DWORD unknown1EF8;
	DWORD unknown1EFC;
	DWORD unknown1F00;
	DWORD unknown1F04;
	DWORD unknown1F08;
	DWORD unknown1F0C;
	DWORD unknown1F10;
	DWORD unknown1F14;
	DWORD unknown1F18;
	DWORD unknown1F1C;
	DWORD unknown1F20;
	DWORD unknown1F24;
	DWORD unknown1F28;
	DWORD unknown1F2C;
	DWORD unknown1F30;
	DWORD unknown1F34;
	DWORD unknown1F38;
	DWORD unknown1F3C;
	DWORD unknown1F40;
	DWORD unknown1F44;
	DWORD unknown1F48;
	DWORD unknown1F4C;
	DWORD unknown1F50;
	DWORD unknown1F54;
	DWORD unknown1F58;
	DWORD unknown1F5C;
	DWORD unknown1F60;
	DWORD unknown1F64;
	DWORD unknown1F68;
	DWORD unknown1F6C;
	DWORD unknown1F70;
	DWORD unknown1F74;
	DWORD unknown1F78;
	DWORD unknown1F7C;
	DWORD unknown1F80;
	DWORD unknown1F84;
	DWORD unknown1F88;
	DWORD unknown1F8C;
	DWORD unknown1F90;
	CACHED_TEXTURE curr_hw_texture[8];
	DWORD unknown1FD4;
	DWORD unknown1FD8;
	DWORD unknown1FDC;
	DWORD unknown1FE0;
	DWORD unknown1FE4;
	DWORD unknown1FE8;
	DWORD unknown1FEC;
	DWORD unknown1FF0;
	DWORD unknown1FF4;
	DWORD unknown1FF8;
	DWORD unknown1FFC;
	DWORD unknown2000;
	DWORD unknown2004;
	DWORD unknown2008;
	DWORD unknown200C;
	DWORD unknown2010;
	DWORD unknown2014;
	DWORD unknown2018;
	DWORD unknown201C;
	DWORD unknown2020;
	DWORD unknown2024;
	DWORD unknown2028;
	DWORD unknown202C;
	DWORD unknown2030;
	DWORD unknown2034;
	DWORD unknown2038;
	DWORD unknown203C;
	DWORD unknown2040;
	DWORD unknown2044;
	DWORD unknown2048;
	DWORD unknown204C;
	DWORD unknown2050;
	DWORD unknown2054;
	DWORD unknown2058;
	DWORD unknown205C;
	DWORD unknown2060;
	DWORD unknown2064;
	DWORD unknown2068;
	DWORD unknown206C;
	DWORD unknown2070;
	DWORD unknown2074;
	DWORD unknown2078;
	DWORD unknown207C;
	DWORD unknown2080;
	DWORD unknown2084;
	DWORD unknown2088;
	DWORD unknown208C;
	DWORD unknown2090;
	DWORD unknown2094;
	DWORD unknown2098;
	DWORD unknown209C;
	DWORD unknown20A0;
	DWORD unknown20A4;
	DWORD unknown20A8;
	DWORD unknown20AC;
	DWORD unknown20B0;
	DWORD unknown20B4;
	DWORD unknown20B8;
	DWORD unknown20BC;
	DWORD unknown20C0;
	DWORD unknown20C4;
	DWORD unknown20C8;
	DWORD unknown20CC;
	DWORD unknown20D0;
	DWORD unknown20D4;
	DWORD unknown20D8;
	DWORD unknown20DC;
	DWORD unknown20E0;
	DWORD unknown20E4;
	DWORD unknown20E8;
	DWORD unknown20EC;
	DWORD unknown20F0;
	DWORD unknown20F4;
	DWORD unknown20F8;
	DWORD unknown20FC;
	DWORD unknown2100;
	DWORD unknown2104;
	DWORD unknown2108;
	DWORD unknown210C;
	DWORD unknown2110;
	DWORD unknown2114;
	DWORD unknown2118;
	DWORD unknown211C;
	DWORD unknown2120;
	DWORD unknown2124;
	DWORD unknown2128;
	DWORD unknown212C;
	DWORD unknown2130;
	DWORD unknown2134;
	DWORD unknown2138;
	DWORD unknown213C;
	DWORD unknown2140;
	DWORD unknown2144;
	DWORD unknown2148;
	DWORD unknown214C;
	DWORD unknown2150;
	DWORD unknown2154;
	DWORD unknown2158;
	DWORD unknown215C;
	DWORD unknown2160;
	DWORD unknown2164;
	DWORD unknown2168;
	DWORD unknown216C;
	DWORD unknown2170;
	DWORD unknown2174;
	DWORD unknown2178;
	DWORD unknown217C;
	DWORD unknown2180;
	DWORD unknown2184;
	DWORD unknown2188;
	DWORD unknown218C;
	DWORD unknown2190;
	DWORD unknown2194;
	DWORD unknown2198;
	DWORD unknown219C;
	DWORD unknown21A0;
	DWORD unknown21A4;
	DWORD unknown21A8;
	DWORD unknown21AC;
	DWORD unknown21B0;
	DWORD unknown21B4;
	DWORD unknown21B8;
	DWORD unknown21BC;
	DWORD unknown21C0;
	DWORD unknown21C4;
	DWORD unknown21C8;
	DWORD unknown21CC;
	DWORD unknown21D0;
	DWORD unknown21D4;
	DWORD unknown21D8;
	DWORD unknown21DC;
	DWORD unknown21E0;
	DWORD unknown21E4;
	DWORD unknown21E8;
	DWORD unknown21EC;
	DWORD unknown21F0;
	DWORD unknown21F4;
	DWORD unknown21F8;
	CACHED_MATRIX curr_hw_view;
	CACHED_MATRIX curr_hw_world;
	CACHED_MATRIX curr_hw_projection;
	CACHED_VIEWPORT curr_hw_viewport;
	DWORD unknown22E4;
	DWORD unknown22E8;
	DWORD unknown22EC;
	DWORD unknown22F0;
	DWORD unknown22F4;
	DWORD unknown22F8;

	BEGIN_DACOM_MAP_INBOUND(DirectX8)
		DACOM_INTERFACE_ENTRY(IRenderPipeline8B)
		DACOM_INTERFACE_ENTRY2(IID_IRenderPipeline8B, IRenderPipeline8B)
		DACOM_INTERFACE_ENTRY(IVertexBufferManager)
		DACOM_INTERFACE_ENTRY2(IID_IVertexBufferManager, IVertexBufferManager)
		DACOM_INTERFACE_ENTRY(IRPDraw)
		DACOM_INTERFACE_ENTRY2(IID_IRPDraw, IRPDraw)
		DACOM_INTERFACE_ENTRY(IRPIndexBuffer)
		DACOM_INTERFACE_ENTRY2(IID_IRPIndexBuffer, IRPIndexBuffer)
		DACOM_INTERFACE_ENTRY(IRPVertexBuffer)
		DACOM_INTERFACE_ENTRY2(IID_IRPVertexBuffer, IRPVertexBuffer)
		DACOM_INTERFACE_ENTRY(IGammaControl)
		DACOM_INTERFACE_ENTRY2(IID_IGammaControl, IGammaControl)
		DACOM_INTERFACE_ENTRY(IRPTexture)
		DACOM_INTERFACE_ENTRY2(IID_IRPTexture, IRPTexture)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent, IAggregateComponent)
		END_DACOM_MAP()

	DA_HEAP_DEFINE_NEW_OPERATOR(DirectX8);

	DirectX8();
	~DirectX8();
	GENRESULT init(AGGDESC* pDesc);

	// IDAComponent methods

	DACOM_DEFMETHOD(QueryInterface)(const C8* interface_name, void** instance) override = 0;
	DACOM_DEFMETHOD_(U32, AddRef)(void) override = 0;
	DACOM_DEFMETHOD_(U32, Release)(void) override = 0;

	// IRenderPipeline8B methods

	DACOM_DEFMETHOD(startup)(const char* profile_name) override;
	DACOM_DEFMETHOD(shutdown)(void) override;
	DACOM_DEFMETHOD(set_pipeline_state)(RPPIPELINESTATE state, U32 value) override;
	DACOM_DEFMETHOD(get_pipeline_state)(RPPIPELINESTATE state, U32* value) override;
	DACOM_DEFMETHOD(get_device_info)(RPDEVICEINFO* info) override;
	DACOM_DEFMETHOD(query_device_ability)(RPDEVICEABILITY ability, U32* out_answer) override;
	DACOM_DEFMETHOD(get_num_display_modes)(U32* count) override;
	DACOM_DEFMETHOD(get_display_mode)(RPDISPLAYMODEINFO* mode, U32 mode_num) override;
	DACOM_DEFMETHOD(select_mode)(RPBUFFERSINFO* mode, U32* adapter) override;
	DACOM_DEFMETHOD(create_buffers)(HWND hwnd, RPBUFFERSINFO* requested, RPBUFFERSINFO* out_buffersinfo) override;
	DACOM_DEFMETHOD(get_buffers)(U32* adapter, RPBUFFERSINFO* out_buffersinfo) override;
	DACOM_DEFMETHOD(destroy_buffers)(void) override;
	DACOM_DEFMETHOD(clear_buffers)(U32 rp_clear_flags, RECT* viewport_sub_rect) override;
	DACOM_DEFMETHOD(swap_buffers)(void) override;
	DACOM_DEFMETHOD(lock_buffer)(RPLOCKDATA* lockData) override;
	DACOM_DEFMETHOD(unlock_buffer)(void) override;
	DACOM_DEFMETHOD(get_buffer_interface)(const char* iid, void** out_iif) override;
	DACOM_DEFMETHOD(get_device_stats)(RPDEVICESTATS* stat) override;
	DACOM_DEFMETHOD(set_viewport)(int x, int y, int w, int h) override;
	DACOM_DEFMETHOD(get_viewport)(int* out_x, int* out_y, int* out_w, int* out_h) override;
	DACOM_DEFMETHOD(set_depth_range)(float lower_z_bound, float upper_z_bound) override;
	DACOM_DEFMETHOD(get_depth_range)(float* lower_z_bound, float* upper_z_bound) override;
	DACOM_DEFMETHOD(set_window)(HWND hwnd, int x, int y, int w, int h) override;
	DACOM_DEFMETHOD(get_window)(HWND* out_hwnd, int* out_x, int* out_y, int* out_w, int* out_h) override;
	DACOM_DEFMETHOD(set_world)(const Transform& world) override;
	DACOM_DEFMETHOD(get_world)(Transform& world) override;
	DACOM_DEFMETHOD(set_view)(const Transform& view) override;
	DACOM_DEFMETHOD(get_view)(Transform& view) override;
	DACOM_DEFMETHOD(set_modelview)(const Transform& modelview) override;
	DACOM_DEFMETHOD(get_modelview)(Transform& modelview) override;
	DACOM_DEFMETHOD(set_projection)(const Matrix4& projection) override;
	DACOM_DEFMETHOD(get_projection)(Matrix4& projection) override;
	DACOM_DEFMETHOD(set_lookat)(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz) override;
	DACOM_DEFMETHOD(set_ortho)(float left, float right, float bottom, float top, float nearval, float farval) override;
	DACOM_DEFMETHOD(set_perspective)(float fovy, float aspect, float znear, float zfar) override;
	DACOM_DEFMETHOD(set_light)(IRP_LIGHTHANDLE handle, const D3DLIGHT8* light_values) override;
	DACOM_DEFMETHOD(destroy_light)(IRP_LIGHTHANDLE handle) override;
	DACOM_DEFMETHOD(get_light)(IRP_LIGHTHANDLE handle, D3DLIGHT8* out_light_values) override;
	DACOM_DEFMETHOD(set_light_enable)(IRP_LIGHTHANDLE handle, U32 enable) override;
	DACOM_DEFMETHOD(get_light_enable)(IRP_LIGHTHANDLE handle, U32* out_enable) override;
	DACOM_DEFMETHOD(set_material)(D3DMATERIAL8* material_values) override;
	DACOM_DEFMETHOD(get_material)(D3DMATERIAL8* out_material_values) override;
	DACOM_DEFMETHOD(create_texture)(int width, int height, const PFenum* desiredformat, int num_lod, U32 irp_ctf_flags, IRP_TEXTUREHANDLE* out_htexture) override;
	DACOM_DEFMETHOD(destroy_texture)(IRP_TEXTUREHANDLE htexture) override;
	DACOM_DEFMETHOD(is_texture)(IRP_TEXTUREHANDLE htexture) override;
	DACOM_DEFMETHOD(lock_texture)(IRP_TEXTUREHANDLE htexture, U32 subsurface, RPLOCKDATA* lockData) override;
	DACOM_DEFMETHOD(unlock_texture)(IRP_TEXTUREHANDLE htexture, U32 subsurface) override;
	DACOM_DEFMETHOD(get_texture_format)(IRP_TEXTUREHANDLE htexture, PFenum* out_pf) override;
	DACOM_DEFMETHOD(get_texture_dim)(IRP_TEXTUREHANDLE htexture, U32* out_width, U32* out_height, U32* out_num_lod) override;
	DACOM_DEFMETHOD(get_texture_interface)(IRP_TEXTUREHANDLE htexture, const char* iid, void** out_iif) override;
	DACOM_DEFMETHOD(set_texture_level_data)(IRP_TEXTUREHANDLE htexture, U32 subsurface, int src_width, int src_height, int src_stride, const PFenum* src_format, const void* src_pixel, const void* src_alpha, const RGB* src_palette) override;
	DACOM_DEFMETHOD(blit_texture)(IRP_TEXTUREHANDLE hDest, U32 destLevel, RECT destRect, IRP_TEXTUREHANDLE hSrc, U32 srcLevel, RECT srcRect) override;
	DACOM_DEFMETHOD(set_render_target)(UNKNOWN a2, UNKNOWN a3, UNKNOWN a4) override;
	DACOM_DEFMETHOD(get_render_target)(void* a2) override;
	DACOM_DEFMETHOD(begin_scene)(void) override;
	DACOM_DEFMETHOD(end_scene)(void) override;
	DACOM_DEFMETHOD(reset_render_states_to_defaults)(void) override;
	DACOM_DEFMETHOD(set_render_state)(D3DRENDERSTATETYPE state, U32 value) override;
	DACOM_DEFMETHOD(get_render_state)(D3DRENDERSTATETYPE state, U32* value) override;
	DACOM_DEFMETHOD(set_texture_stage_state)(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value) override;
	DACOM_DEFMETHOD(get_texture_stage_state)(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32* value) override;
	DACOM_DEFMETHOD(set_texture_stage_transform)(U32 stage, Matrix4 const& mat4) override;
	DACOM_DEFMETHOD(get_texture_stage_transform)(U32 stage, Matrix4& out_mat4) override;
	DACOM_DEFMETHOD(set_texture_stage_texture)(U32 stage, IRP_TEXTUREHANDLE htexture) override;
	DACOM_DEFMETHOD(get_texture_stage_texture)(U32 stage, IRP_TEXTUREHANDLE* out_htexture) override;
	DACOM_DEFMETHOD(verify_state)(void) override;
	DACOM_DEFMETHOD(draw_primitive)(D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, U32 flags) override;
	DACOM_DEFMETHOD(draw_indexed_primitive)(D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, const U16* indices, U32 num_indices, U32 flags) override;
	DACOM_DEFMETHOD(draw_primitive_vb)(D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, U32 flags) override;
	DACOM_DEFMETHOD(draw_indexed_primitive_vb)(D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, const U16* indices, U32 num_indices, U32 flags) override;
	DACOM_DEFMETHOD(add_light)(IRP_LIGHTHANDLE handle) override;
	DACOM_DEFMETHOD(remove_light)(IRP_LIGHTHANDLE handle) override;
	DACOM_DEFMETHOD(update_light)(IRP_LIGHTHANDLE handle) override;
	DACOM_DEFMETHOD(set_world_n)(UNKNOWN a2, Transform* transform) override;

	// IVertexBufferManager methods

	DACOM_DEFMETHOD(initialize)(UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN) override;
	DACOM_DEFMETHOD(cleanup)() override;
	DACOM_DEFMETHOD(acquire_vertex_buffer)(D3DFORMAT vertex_format, U32 num_verts, VertexBufferAcquire* out_vbmem) override;
	DACOM_DEFMETHOD(release_vertex_buffer)(VertexBufferAcquire* vbmem) override;
	DACOM_DEFMETHOD(IVertexBufferManager_Unknown1C)() override;
	DACOM_DEFMETHOD(copy_vertex_data)(void* dst_buffer, U32 dst_vertex_format, VertexBufferDesc* src_vb_desc, U32 start_vertex, U32 num_vertices) override;

	// IRPDraw methods

	DACOM_DEFMETHOD_(HRESULT, draw_indexed_primitive)(D3DPRIMITIVETYPE type, U32 min_index, U32 num_verts, U32 start_index, U32 count) override;

	// IRPIndexBuffer methods

	DACOM_DEFMETHOD(create_index_buffer)(U32 num_indices, IRP_INDEXBUFFERHANDLE* out_ib_handle, U8 irp_ibf_flags) override;
	DACOM_DEFMETHOD(destroy_index_buffer)(IRP_INDEXBUFFERHANDLE* ib_handle) override;
	DACOM_DEFMETHOD_(HRESULT, create_ib)(IRP_INDEXBUFFERHANDLE ib_handle, U32 num_indices) override;
	DACOM_DEFMETHOD(copy_indices)(IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, U16 const* indices, U32 num_indices) override;
	DACOM_DEFMETHOD(lock_ib)(IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, void*& out_data, U32 num_indices) override;
	DACOM_DEFMETHOD(unlock_ib)(IRP_INDEXBUFFERHANDLE ib_handle) override;
	DACOM_DEFMETHOD(select_ib)(IRP_INDEXBUFFERHANDLE ib_handle, U32 base_index, UNKNOWN a4, UNKNOWN a5) override;
	DACOM_DEFMETHOD(get_ib_count)(IRP_INDEXBUFFERHANDLE ib_handle, U32* out_count) override;
	DACOM_DEFMETHOD_(BOOL32, is_ib_valid)(IRP_INDEXBUFFERHANDLE ib_handle) override;

	// IRPVertexBuffer methods

	DACOM_DEFMETHOD(create_vb)(U32 vertex_format, U32 num_verts, IRP_VERTEXBUFFERHANDLE* out_vb_handle, U8 irp_vbf_flags) override;
	DACOM_DEFMETHOD(destroy_vb)(IRP_VERTEXBUFFERHANDLE& vb_handle) override;
	DACOM_DEFMETHOD(ressize_vb)(IRP_VERTEXBUFFERHANDLE vb_handle, U32 format, U32 num_verts) override;
	DACOM_DEFMETHOD(copy_vertices)(IRP_VERTEXBUFFERHANDLE vb_handle, U32* offset, VertexBufferDesc* src_vb_desc, U32 start_vertex, U32 num_vertices) override;
	DACOM_DEFMETHOD(lock_vb)(IRP_VERTEXBUFFERHANDLE vb_handle, U32* start_index, void*& out_data, U32 num_verts) override;
	DACOM_DEFMETHOD(unlock_vb)(IRP_VERTEXBUFFERHANDLE vb_handle) override;
	DACOM_DEFMETHOD(RPVertexBuffer_Unknown24)(UNKNOWN) override;
	DACOM_DEFMETHOD(select_vb)(IRP_VERTEXBUFFERHANDLE vb_handle) override;
	DACOM_DEFMETHOD(get_vb_count)(IRP_VERTEXBUFFERHANDLE vb_handle, U32* out_vertex_format, U32* out_num_verts) override;
	DACOM_DEFMETHOD_(BOOL32, is_vb_valid)(IRP_VERTEXBUFFERHANDLE vb_handle) override;

	// IGammaControl methods

	DACOM_DEFMETHOD(set_gamma_function)(IGC_COMPONENT which, float display_gamma, float bias = 0.0, float slope = 1.0, float black_offset = 0.0) override;
	DACOM_DEFMETHOD(set_gamma_ramp)(IGC_COMPONENT igc_component, U16* ramp) override;
	DACOM_DEFMETHOD(get_gamma_ramp)(IGC_COMPONENT igc_component, U16* out_ramp) override;
	DACOM_DEFMETHOD(set_calibration_enable)(bool enabled) override;
	DACOM_DEFMETHOD(get_calibration_enable)(void) override;

	// IRPTexture methods

	DACOM_DEFMETHOD(print_screen)(IFileSystem* IFS, const char* child) override;
	DACOM_DEFMETHOD(load_texture)(IFileSystem* IFS, const char* child, IRP_TEXTUREHANDLE* out_htexture) override;
	DACOM_DEFMETHOD(load_surface_from_file)(UNKNOWN* a2_interface, UNKNOWN a3, UNKNOWN a4, UNKNOWN a5) override;
	DACOM_DEFMETHOD(load_dds_info)(DDSInfo* out_info, DWORD membytesize, void* mapped_file_mem) override;
	DACOM_DEFMETHOD(load_cubemap)(IFileSystem* IFS, const char* child, IRP_TEXTUREHANDLE* out_htexture) override;

	// IAggregateComponent methods

	DACOM_DEFMETHOD(Initialize) (void) override;
};
static_assert(sizeof(DAComponentAggregate<DirectX8>) == 0x230C); // 8972

// 6D01143
DirectX8::DirectX8()
{
	debug_point;
	DirectX8_Ctor(this);
	debug_point;
}

// 6D01689
DirectX8::~DirectX8()
{
	debug_point;
	DirectX8_Dtor(this);
	debug_point;
}

GENRESULT DirectX8::init(AGGDESC* pDesc)
{
	GENRESULT gr = DirectX8_init(this, pDesc);
	return gr;
}

GENRESULT DirectX8::startup(const char* profile_name)
{
	GENRESULT gr = DirectX8_startup(this, profile_name);
	return gr;
}

GENRESULT DirectX8::shutdown(void)
{
	GENRESULT gr = DirectX8_shutdown(this);
	return gr;
}

GENRESULT DirectX8::set_pipeline_state(RPPIPELINESTATE state, U32 value)
{
	GENRESULT gr = DirectX8_set_pipeline_state(this, state, value);
	return gr;
}

GENRESULT DirectX8::get_pipeline_state(RPPIPELINESTATE state, U32* value)
{
	GENRESULT gr = DirectX8_get_pipeline_state(this, state, value);
	return gr;
}

GENRESULT DirectX8::get_device_info(RPDEVICEINFO* info)
{
	GENRESULT gr = DirectX8_get_device_info(this, info);
	return gr;
}

GENRESULT DirectX8::query_device_ability(RPDEVICEABILITY ability, U32* out_answer)
{
	GENRESULT gr = DirectX8_query_device_ability(this, ability, out_answer);
	return gr;
}

GENRESULT DirectX8::get_num_display_modes(U32* count)
{
	GENRESULT gr = DirectX8_get_num_display_modes(this, count);
	return gr;
}

GENRESULT DirectX8::get_display_mode(RPDISPLAYMODEINFO* mode, U32 mode_num)
{
	GENRESULT gr = DirectX8_get_display_mode(this, mode, mode_num);
	return gr;
}

GENRESULT DirectX8::select_mode(RPBUFFERSINFO* mode, U32* adapter)
{
	GENRESULT gr = DirectX8_select_mode(this, mode, adapter);
	return gr;
}

GENRESULT DirectX8::create_buffers(HWND hwnd, RPBUFFERSINFO* requested, RPBUFFERSINFO* out_buffersinfo)
{
	GENRESULT gr = DirectX8_create_buffers(this, hwnd, requested, out_buffersinfo);
	return gr;
}

GENRESULT DirectX8::get_buffers(U32* adapter, RPBUFFERSINFO* out_buffersinfo)
{
	GENRESULT gr = DirectX8_get_buffers(this, adapter, out_buffersinfo);
	return gr;
}

GENRESULT DirectX8::destroy_buffers(void)
{
	GENRESULT gr = DirectX8_destroy_buffers(this);
	return gr;
}

GENRESULT DirectX8::clear_buffers(U32 rp_clear_flags, RECT* viewport_sub_rect)
{
	GENRESULT gr = DirectX8_clear_buffers(this, rp_clear_flags, viewport_sub_rect);
	return gr;
}

GENRESULT DirectX8::swap_buffers(void)
{
	GENRESULT gr = DirectX8_swap_buffers(this);
	return gr;
}

GENRESULT DirectX8::lock_buffer(RPLOCKDATA* lockData)
{
	GENRESULT gr = DirectX8_lock_buffer(this, lockData);
	return gr;
}

GENRESULT DirectX8::unlock_buffer(void)
{
	GENRESULT gr = DirectX8_unlock_buffer(this);
	return gr;
}

GENRESULT DirectX8::get_buffer_interface(const char* iid, void** out_iif)
{
	GENRESULT gr = DirectX8_get_buffer_interface(this, iid, out_iif);
	return gr;
}

GENRESULT DirectX8::get_device_stats(RPDEVICESTATS* stat)
{
	GENRESULT gr = DirectX8_get_device_stats(this, stat);
	return gr;
}

GENRESULT DirectX8::set_viewport(int x, int y, int w, int h)
{
	GENRESULT gr = DirectX8_set_viewport(this, x, y, w, h);
	return gr;
}

GENRESULT DirectX8::get_viewport(int* out_x, int* out_y, int* out_w, int* out_h)
{
	GENRESULT gr = DirectX8_get_viewport(this, out_x, out_y, out_w, out_h);
	return gr;
}

GENRESULT DirectX8::set_depth_range(float lower_z_bound, float upper_z_bound)
{
	GENRESULT gr = DirectX8_set_depth_range(this, lower_z_bound, upper_z_bound);
	return gr;
}

GENRESULT DirectX8::get_depth_range(float* lower_z_bound, float* upper_z_bound)
{
	GENRESULT gr = DirectX8_get_depth_range(this, lower_z_bound, upper_z_bound);
	return gr;
}

GENRESULT DirectX8::set_window(HWND hwnd, int x, int y, int w, int h)
{
	GENRESULT gr = DirectX8_set_window(this, hwnd, x, y, w, h);
	return gr;
}

GENRESULT DirectX8::get_window(HWND* out_hwnd, int* out_x, int* out_y, int* out_w, int* out_h)
{
	GENRESULT gr = DirectX8_get_window(this, out_hwnd, out_x, out_y, out_w, out_h);
	return gr;
}

GENRESULT DirectX8::set_world(const Transform& world)
{
	GENRESULT gr = DirectX8_set_world(this, world);
	return gr;
}

GENRESULT DirectX8::get_world(Transform& world)
{
	GENRESULT gr = DirectX8_get_world(this, world);
	return gr;
}

GENRESULT DirectX8::set_view(const Transform& view)
{
	GENRESULT gr = DirectX8_set_view(this, view);
	return gr;
}

GENRESULT DirectX8::get_view(Transform& view)
{
	GENRESULT gr = DirectX8_get_view(this, view);
	return gr;
}

GENRESULT DirectX8::set_modelview(const Transform& modelview)
{
	GENRESULT gr = DirectX8_set_modelview(this, modelview);
	return gr;
}

GENRESULT DirectX8::get_modelview(Transform& modelview)
{
	GENRESULT gr = DirectX8_get_modelview(this, modelview);
	return gr;
}

GENRESULT DirectX8::set_projection(const Matrix4& projection)
{
	GENRESULT gr = DirectX8_set_projection(this, projection);
	return gr;
}

GENRESULT DirectX8::get_projection(Matrix4& projection)
{
	GENRESULT gr = DirectX8_get_projection(this, projection);
	return gr;
}

GENRESULT DirectX8::set_lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz)
{
	GENRESULT gr = DirectX8_set_lookat(this, eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
	return gr;
}

GENRESULT DirectX8::set_ortho(float left, float right, float bottom, float top, float nearval, float farval)
{
	GENRESULT gr = DirectX8_set_ortho(this, left, right, bottom, top, nearval, farval);
	return gr;
}

GENRESULT DirectX8::set_perspective(float fovy, float aspect, float znear, float zfar)
{
	GENRESULT gr = DirectX8_set_perspective(this, fovy, aspect, znear, zfar);
	return gr;
}

GENRESULT DirectX8::set_light(IRP_LIGHTHANDLE handle, const D3DLIGHT8* light_values)
{
	GENRESULT gr = DirectX8_set_light(this, handle, light_values);
	return gr;
}

GENRESULT DirectX8::destroy_light(IRP_LIGHTHANDLE handle)
{
	GENRESULT gr = DirectX8_destroy_light(this, handle);
	return gr;
}

GENRESULT DirectX8::get_light(IRP_LIGHTHANDLE handle, D3DLIGHT8* out_light_values)
{
	GENRESULT gr = DirectX8_get_light(this, handle, out_light_values);
	return gr;
}

GENRESULT DirectX8::set_light_enable(IRP_LIGHTHANDLE handle, U32 enable)
{
	GENRESULT gr = DirectX8_set_light_enable(this, handle, enable);
	return gr;
}

GENRESULT DirectX8::get_light_enable(IRP_LIGHTHANDLE handle, U32* out_enable)
{
	GENRESULT gr = DirectX8_get_light_enable(this, handle, out_enable);
	return gr;
}

GENRESULT DirectX8::set_material(D3DMATERIAL8* material_values)
{
	GENRESULT gr = DirectX8_set_material(this, material_values);
	return gr;
}

GENRESULT DirectX8::get_material(D3DMATERIAL8* out_material_values)
{
	GENRESULT gr = DirectX8_get_material(this, out_material_values);
	return gr;
}

GENRESULT DirectX8::create_texture(int width, int height, const PFenum* desiredformat, int num_lod, U32 irp_ctf_flags, IRP_TEXTUREHANDLE* out_htexture)
{
	GENRESULT gr = DirectX8_create_texture(this, width, height, desiredformat, num_lod, irp_ctf_flags, out_htexture);
	return gr;
}

GENRESULT DirectX8::destroy_texture(IRP_TEXTUREHANDLE htexture)
{
	GENRESULT gr = DirectX8_destroy_texture(this, htexture);
	return gr;
}

GENRESULT DirectX8::is_texture(IRP_TEXTUREHANDLE htexture)
{
	GENRESULT gr = DirectX8_is_texture(this, htexture);
	return gr;
}

GENRESULT DirectX8::lock_texture(IRP_TEXTUREHANDLE htexture, U32 subsurface, RPLOCKDATA* lockData)
{
	GENRESULT gr = DirectX8_lock_texture(this, htexture, subsurface, lockData);
	return gr;
}

GENRESULT DirectX8::unlock_texture(IRP_TEXTUREHANDLE htexture, U32 subsurface)
{
	GENRESULT gr = DirectX8_unlock_texture(this, htexture, subsurface);
	return gr;
}

GENRESULT DirectX8::get_texture_format(IRP_TEXTUREHANDLE htexture, PFenum* out_pf)
{
	GENRESULT gr = DirectX8_get_texture_format(this, htexture, out_pf);
	return gr;
}

GENRESULT DirectX8::get_texture_dim(IRP_TEXTUREHANDLE htexture, U32* out_width, U32* out_height, U32* out_num_lod)
{
	GENRESULT gr = DirectX8_get_texture_dim(this, htexture, out_width, out_height, out_num_lod);
	return gr;
}

GENRESULT DirectX8::get_texture_interface(IRP_TEXTUREHANDLE htexture, const char* iid, void** out_iif)
{
	GENRESULT gr = DirectX8_get_texture_interface(this, htexture, iid, out_iif);
	return gr;
}

GENRESULT DirectX8::set_texture_level_data(IRP_TEXTUREHANDLE htexture, U32 subsurface, int src_width, int src_height, int src_stride, const PFenum* src_format, const void* src_pixel, const void* src_alpha, const RGB* src_palette)
{
	GENRESULT gr = DirectX8_set_texture_level_data(this, htexture, subsurface, src_width, src_height, src_stride, src_format, src_pixel, src_alpha, src_palette);
	return gr;
}

GENRESULT DirectX8::blit_texture(IRP_TEXTUREHANDLE hDest, U32 destLevel, RECT destRect, IRP_TEXTUREHANDLE hSrc, U32 srcLevel, RECT srcRect)
{
	GENRESULT gr = DirectX8_blit_texture(this, hDest, destLevel, destRect, hSrc, srcLevel, srcRect);
	return gr;
}

GENRESULT DirectX8::set_render_target(UNKNOWN a2, UNKNOWN a3, UNKNOWN a4)
{
	GENRESULT gr = DirectX8_set_render_target(this, a2, a3, a4);
	return gr;
}

GENRESULT DirectX8::get_render_target(void* a2)
{
	GENRESULT gr = DirectX8_get_render_target(this, a2);
	return gr;
}

GENRESULT DirectX8::begin_scene(void)
{
	GENRESULT gr = DirectX8_begin_scene(this);
	return gr;
}

GENRESULT DirectX8::end_scene(void)
{
	GENRESULT gr = DirectX8_end_scene(this);
	return gr;
}

GENRESULT DirectX8::reset_render_states_to_defaults(void)
{
	GENRESULT gr = DirectX8_reset_render_states_to_defaults(this);
	return gr;
}

GENRESULT DirectX8::set_render_state(D3DRENDERSTATETYPE state, U32 value)
{
	GENRESULT gr = DirectX8_set_render_state(this, state, value);
	return gr;
}

GENRESULT DirectX8::get_render_state(D3DRENDERSTATETYPE state, U32* value)
{
	GENRESULT gr = DirectX8_get_render_state(this, state, value);
	return gr;
}

GENRESULT DirectX8::set_texture_stage_state(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value)
{
	GENRESULT gr = DirectX8_set_texture_stage_state(this, stage, state, value);
	return gr;
}

GENRESULT DirectX8::get_texture_stage_state(U32 stage, D3DTEXTURESTAGESTATETYPE state, U32* value)
{
	GENRESULT gr = DirectX8_get_texture_stage_state(this, stage, state, value);
	return gr;
}

GENRESULT DirectX8::set_texture_stage_transform(U32 stage, Matrix4 const& mat4)
{
	GENRESULT gr = DirectX8_set_texture_stage_transform(this, stage, mat4);
	return gr;
}

GENRESULT DirectX8::get_texture_stage_transform(U32 stage, Matrix4& out_mat4)
{
	GENRESULT gr = DirectX8_get_texture_stage_transform(this, stage, out_mat4);
	return gr;
}

GENRESULT DirectX8::set_texture_stage_texture(U32 stage, IRP_TEXTUREHANDLE htexture)
{
	GENRESULT gr = DirectX8_set_texture_stage_texture(this, stage, htexture);
	return gr;
}

GENRESULT DirectX8::get_texture_stage_texture(U32 stage, IRP_TEXTUREHANDLE* out_htexture)
{
	GENRESULT gr = DirectX8_get_texture_stage_texture(this, stage, out_htexture);
	return gr;
}

GENRESULT DirectX8::verify_state(void)
{
	GENRESULT gr = DirectX8_verify_state(this);
	return gr;
}

GENRESULT DirectX8::draw_primitive(D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, U32 flags)
{
	GENRESULT gr = DirectX8_draw_primitive(this, type, vertex_format, verts, num_verts, flags);
	return gr;
}

GENRESULT DirectX8::draw_indexed_primitive(D3DPRIMITIVETYPE type, U32 vertex_format, const void* verts, U32 num_verts, const U16* indices, U32 num_indices, U32 flags)
{
	GENRESULT gr = DirectX8_draw_indexed_primitive(this, type, vertex_format, verts, num_verts, indices, num_indices, flags);
	return gr;
}

GENRESULT DirectX8::draw_primitive_vb(D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, U32 flags)
{
	GENRESULT gr = DirectX8_draw_primitive_vb(this, type, vb_handle, start_vert, num_verts, flags);
	return gr;
}

GENRESULT DirectX8::draw_indexed_primitive_vb(D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, const U16* indices, U32 num_indices, U32 flags)
{
	GENRESULT gr = DirectX8_draw_indexed_primitive_vb(this, type, vb_handle, start_vert, num_verts, indices, num_indices, flags);
	return gr;
}

GENRESULT DirectX8::add_light(IRP_LIGHTHANDLE handle)
{
	GENRESULT gr = DirectX8_add_light(this, handle);
	return gr;
}

GENRESULT DirectX8::remove_light(IRP_LIGHTHANDLE handle)
{
	GENRESULT gr = DirectX8_remove_light(this, handle);
	return gr;
}

GENRESULT DirectX8::update_light(IRP_LIGHTHANDLE handle)
{
	GENRESULT gr = DirectX8_update_light(this, handle);
	return gr;
}

GENRESULT DirectX8::set_world_n(UNKNOWN a2, Transform* transform)
{
	GENRESULT gr = DirectX8_set_world_n(this, a2, transform);
	return gr;
}

GENRESULT DirectX8::initialize(U32 a2, U32 a3, U32 a4, U32 a5)
{
	GENRESULT gr = DirectX8_initialize(this, a2, a3, a4, a5);
	return gr;
}

GENRESULT DirectX8::cleanup()
{
	GENRESULT gr = DirectX8_cleanup(this);
	return gr;
}

GENRESULT DirectX8::acquire_vertex_buffer(D3DFORMAT vertex_format, U32 num_verts, VertexBufferAcquire* out_vbmem)
{
	GENRESULT gr = DirectX8_acquire_vertex_buffer(this, vertex_format, num_verts, out_vbmem);
	return gr;
}

GENRESULT DirectX8::release_vertex_buffer(VertexBufferAcquire* vbmem)
{
	GENRESULT gr = DirectX8_release_vertex_buffer(this, vbmem);
	return gr;
}

GENRESULT DirectX8::IVertexBufferManager_Unknown1C()
{
	GENRESULT gr = DirectX8_IVertexBufferManager_Unknown1C(this);
	return gr;
}

GENRESULT DirectX8::copy_vertex_data(void* dst_buffer, U32 dst_vertex_format, VertexBufferDesc* src_vb_desc, U32 start_vertex, U32 num_vertices)
{
	GENRESULT gr = DirectX8_copy_vertex_data(this, dst_buffer, dst_vertex_format, src_vb_desc, num_vertices, start_vertex);
	return gr;
}

HRESULT DirectX8::draw_indexed_primitive(D3DPRIMITIVETYPE type, U32 min_index, U32 num_verts, U32 start_index, U32 count)
{
	HRESULT hr = DirectX8_draw_indexed_primitive2(this, type, min_index, num_verts, start_index, count);
	return hr;
}

GENRESULT DirectX8::create_index_buffer(U32 num_indices, IRP_INDEXBUFFERHANDLE* out_ib_handle, U8 irp_ibf_flags)
{
	GENRESULT gr = DirectX8_create_index_buffer(this, num_indices, out_ib_handle, irp_ibf_flags);
	return gr;
}

GENRESULT DirectX8::destroy_index_buffer(IRP_INDEXBUFFERHANDLE* ib_handle)
{
	GENRESULT gr = DirectX8_destroy_index_buffer(this, ib_handle);
	return gr;
}

HRESULT DirectX8::create_ib(IRP_INDEXBUFFERHANDLE ib_handle, U32 num_indices)
{
	HRESULT hr = DirectX8_create_ib(this, ib_handle, num_indices);
	return hr;
}

GENRESULT DirectX8::copy_indices(IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, U16 const* indices, U32 num_indices)
{
	GENRESULT gr = DirectX8_copy_indices(this, ib_handle, start_index, indices, num_indices);
	return gr;
}

GENRESULT DirectX8::lock_ib(IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, void*& out_data, U32 num_indices)
{
	GENRESULT gr = DirectX8_lock_ib(this, ib_handle, start_index, out_data, num_indices);
	return gr;
}

GENRESULT DirectX8::unlock_ib(IRP_INDEXBUFFERHANDLE ib_handle)
{
	GENRESULT gr = DirectX8_unlock_ib(this, ib_handle);
	return gr;
}

GENRESULT DirectX8::select_ib(IRP_INDEXBUFFERHANDLE ib_handle, U32 base_index, UNKNOWN a4, UNKNOWN a5)
{
	GENRESULT gr = DirectX8_select_ib(this, ib_handle, base_index, a4, a5);
	return gr;
}

GENRESULT DirectX8::get_ib_count(IRP_INDEXBUFFERHANDLE ib_handle, U32* out_count)
{
	GENRESULT gr = DirectX8_get_ib_count(this, ib_handle, out_count);
	return gr;
}

BOOL32 DirectX8::is_ib_valid(IRP_INDEXBUFFERHANDLE ib_handle)
{
	BOOL32 gr = DirectX8_is_ib_valid(this, ib_handle);
	return gr;
}

GENRESULT DirectX8::create_vb(U32 vertex_format, U32 num_verts, IRP_VERTEXBUFFERHANDLE* out_vb_handle, U8 irp_vbf_flags)
{
	GENRESULT gr = DirectX8_create_vb(this, vertex_format, num_verts, out_vb_handle, irp_vbf_flags);
	return gr;
}

GENRESULT DirectX8::destroy_vb(IRP_VERTEXBUFFERHANDLE& vb_handle)
{
	GENRESULT gr = DirectX8_destroy_vb(this, vb_handle);
	return gr;
}

GENRESULT DirectX8::ressize_vb(IRP_VERTEXBUFFERHANDLE vb_handle, U32 format, U32 num_verts)
{
	GENRESULT gr = DirectX8_ressize_vb(this, vb_handle, format, num_verts);
	return gr;
}

GENRESULT DirectX8::copy_vertices(IRP_VERTEXBUFFERHANDLE vb_handle, U32* offset, VertexBufferDesc* src_vb_desc, U32 start_vertex, U32 num_vertices)
{
	GENRESULT gr = DirectX8_copy_vertices(this, vb_handle, offset, src_vb_desc, start_vertex, num_vertices);
	return gr;
}

GENRESULT DirectX8::lock_vb(IRP_VERTEXBUFFERHANDLE vb_handle, U32* start_index, void*& out_data, U32 num_verts)
{
	GENRESULT gr = DirectX8_lock_vb(this, vb_handle, start_index, out_data, num_verts);
	return gr;
}

GENRESULT DirectX8::unlock_vb(IRP_VERTEXBUFFERHANDLE vb_handle)
{
	GENRESULT gr = DirectX8_unlock_vb(this, vb_handle);
	return gr;
}

GENRESULT DirectX8::RPVertexBuffer_Unknown24(UNKNOWN a2)
{
	GENRESULT gr = DirectX8_RPVertexBuffer_Unknown24(this, a2);
	return gr;
}

GENRESULT DirectX8::select_vb(IRP_VERTEXBUFFERHANDLE vb_handle)
{
	GENRESULT gr = DirectX8_select_vb(this, vb_handle);
	return gr;
}

GENRESULT DirectX8::get_vb_count(IRP_VERTEXBUFFERHANDLE vb_handle, U32* out_vertex_format, U32* out_num_verts)
{
	GENRESULT gr = DirectX8_get_vb_count(this, vb_handle, out_vertex_format, out_num_verts);
	return gr;
}

BOOL32 DirectX8::is_vb_valid(IRP_VERTEXBUFFERHANDLE vb_handle)
{
	BOOL32 gr = DirectX8_is_vb_valid(this, vb_handle);
	return gr;
}

GENRESULT DirectX8::set_gamma_function(IGC_COMPONENT which, float display_gamma, float bias, float slope, float black_offset)
{
	GENRESULT gr = DirectX8_set_gamma_function(this, which, display_gamma, bias, slope, black_offset);
	return gr;
}

GENRESULT DirectX8::set_gamma_ramp(IGC_COMPONENT igc_component, U16* ramp)
{
	GENRESULT gr = DirectX8_set_gamma_ramp(this, igc_component, ramp);
	return gr;
}

GENRESULT DirectX8::get_gamma_ramp(IGC_COMPONENT igc_component, U16* out_ramp)
{
	GENRESULT gr = DirectX8_get_gamma_ramp(this, igc_component, out_ramp);
	return gr;
}

GENRESULT DirectX8::set_calibration_enable(bool enabled)
{
	GENRESULT gr = DirectX8_set_calibration_enable(this, enabled);
	return gr;
}

GENRESULT DirectX8::get_calibration_enable(void)
{
	GENRESULT gr = DirectX8_get_calibration_enable(this);
	return gr;
}

GENRESULT DirectX8::print_screen(IFileSystem* IFS, const char* child)
{
	GENRESULT gr = DirectX8_print_screen(this, IFS, child);
	return gr;
}

GENRESULT DirectX8::load_texture(IFileSystem* IFS, const char* child, IRP_TEXTUREHANDLE* out_htexture)
{
	GENRESULT gr = DirectX8_load_texture(this, IFS, child, out_htexture);
	return gr;
}

GENRESULT DirectX8::load_surface_from_file(UNKNOWN* a2_interface, UNKNOWN a3, UNKNOWN a4, UNKNOWN a5)
{
	GENRESULT gr = DirectX8_load_surface_from_file(this, a2_interface, a3, a4, a5);
	return gr;
}

GENRESULT DirectX8::load_dds_info(DDSInfo* out_info, DWORD membytesize, void* mapped_file_mem)
{
	GENRESULT gr = DirectX8_load_dds_info(this, out_info, membytesize, mapped_file_mem);
	return gr;
}

GENRESULT DirectX8::load_cubemap(IFileSystem* IFS, const char* child, IRP_TEXTUREHANDLE* out_htexture)
{
	GENRESULT gr = DirectX8_load_cubemap(this, IFS, child, out_htexture);
	return gr;
}

GENRESULT DirectX8::Initialize(void)
{
	GENRESULT gr = DirectX8_Initialize(this);
	return gr;
}

extern "C"
{
	IComponentFactory* CreateDirectX8Factory()
	{
		return new DAComponentFactory2<DAComponentAggregate<DirectX8>, AGGDESC>(CLSID_DirectX8);
	}

	void Register_DirectX8()
	{
		GENRESULT gr = GR_GENERIC;
		if (ICOManager* DACOM = DACOM_Acquire())
		{
			if (IComponentFactory* pFactory = CreateDirectX8Factory())
			{
				gr = DACOM->RegisterComponent(pFactory, CLSID_DirectX8, DACOM_LOW_PRIORITY);
				pFactory->Release();
			}
		}
		ASSERT(SUCCEEDED(gr));
		unused(gr);
	}
}
