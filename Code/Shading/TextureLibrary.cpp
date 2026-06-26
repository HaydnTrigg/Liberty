#include <DACOM.h>

#include "TextureLibrary.h"

/*
 * TextureLibrary.cpp
 *
 * shading.dll - the DACOM "TextureLibrary" component.
 */

/*
 * One-time initialization, driven by the DACOM factory.
 */
GENRESULT TextureLibrary::init(AGGDESC* info)
{
    return GR_OK;
}

// ITextureLibraryA

GENRESULT __stdcall TextureLibrary::set_library_state(ITL_STATE state, U32 value)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_library_state(ITL_STATE state, U32* out_value)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::load_library(IFileSystem* IFS, const char* library_name)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::load_texture(IFileSystem* IFS, const char* texture_name)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::free_library(BOOL release_all)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::update(SINGLE dt)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_unknown_string(DWORD unknown, const char* unknown_string)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_unknown_string(DWORD unknown, char* out_unknown_string, U32 max_buf_size)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_id(const char* texture_name, ITL_TEXTURE_ID* out_texture_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_id2(CRCString* crc_texture_name /* $todo correct type here */, ITL_TEXTURE_ID* out_texture_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::has_texture_id(const char* texture_name)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::add_ref_texture_id(ITL_TEXTURE_ID texture_id, ITL_TEXTURE_REF_ID* out_texture_ref_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::release_texture_id(ITL_TEXTURE_ID texture_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::add_ref_texture_ref(ITL_TEXTURE_REF_ID texture_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::release_texture_ref(ITL_TEXTURE_REF_ID texture_ref_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_name(ITL_TEXTURE_ID texture_id, char* out_texture_name, U32 max_buf_size)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_name(ITL_TEXTURE_ID texture_id, const char* texture_name)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_frame_texture(ITL_TEXTURE_ID texture_id, U32 txm_id_idx, U32 rp_texture_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_frame_id(ITL_TEXTURE_ID texture_id, U32 frame_num, U32 txm_id_idx)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_frame_rect(ITL_TEXTURE_ID texture_id, U32 frame_num, float u0, float v0, float u1, float v1)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_frame(ITL_TEXTURE_ID texture_id, U32 frame_num, ITL_TEXTUREFRAME_IRP* out_frame)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_format(ITL_TEXTURE_ID texture_id, U32 frame_num, PixelFormat* out_texture_format)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_frame_rate(ITL_TEXTURE_ID texture_id, float fps_rate)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_frame_rate(ITL_TEXTURE_ID texture_id, float* out_fps_rate)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_frame_count(ITL_TEXTURE_ID texture_id, U32* out_frame_count)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_ref_texture_id(ITL_TEXTURE_REF_ID texture_ref_id, ITL_TEXTURE_ID* out_texture_id)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_ref_frame(ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num, ITL_TEXTUREFRAME_IRP* out_frame)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_ref_frame_time(ITL_TEXTURE_REF_ID texture_ref_id, float frame_time)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_ref_frame_time(ITL_TEXTURE_REF_ID texture_ref_id, float* out_frame_time)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_ref_frame_num(ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_ref_frame_num(ITL_TEXTURE_REF_ID texture_ref_id, U32* out_frame_num)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_ref_frame_rate(ITL_TEXTURE_REF_ID texture_ref_id, float fps_rate)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_ref_frame_rate(ITL_TEXTURE_REF_ID texture_ref_id, float* out_fps_rate)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::set_texture_ref_play_mode(ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND play_command)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_ref_play_mode(ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND* out_play_command)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::update_texture_ref(ITL_TEXTURE_REF_ID texture_ref_id, SINGLE dt)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture_count(U32* out_num_textures)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::get_texture(U32 texture_num, ITL_TEXTURE_ID* out_texture_id)
{
    NOT_IMPLEMENTED;
}

// ITextureLibrary2

GENRESULT __stdcall TextureLibrary::sub_6EC2300(ITL_TEXTURE_REF_ID texture_ref_id, void*)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::sub_6EC23A0(ITL_TEXTURE_REF_ID texture_ref_id, DWORD, void*)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::sub_6EC2490(ITL_TEXTURE_REF_ID texture_ref_id, DWORD)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::sub_6EC2680(ITL_TEXTURE_REF_ID texture_ref_id, void*)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::sub_6EC37F0(void*, void*)
{
    NOT_IMPLEMENTED;
}

GENRESULT __stdcall TextureLibrary::sub_6EC1950(void*)
{
    NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------//
// Component registration
//--------------------------------------------------------------------------//
//
// DllMain (Shading/DllMain.cpp) calls these registration hooks.

// Factory registered with DACOM; retained so Shutdown can unregister it.
static IComponentFactory* g_textureLibraryFactory = nullptr;

extern "C"
{
	/*
	 * Registers the TextureLibrary component factory with the component manager.
	 */
	void Register_TextureLibrary()
	{
		g_textureLibraryFactory = RegisterComponentFactory<DAComponentAggregate<TextureLibrary>>(DACOM_LIBRARY_NAME, CLSID_TextureLibrary, DACOM_LOW_PRIORITY);
	}

	/*
	 * Unregisters the TextureLibrary component factory.
	 */
	void Shutdown_TextureLibrary()
	{
		if (g_textureLibraryFactory != nullptr)
		{
			UnregisterComponentFactory(DACOM_LIBRARY_NAME, g_textureLibraryFactory, CLSID_TextureLibrary);
			g_textureLibraryFactory = nullptr;
		}
	}
}
