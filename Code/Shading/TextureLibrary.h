#pragma once

#ifndef TEXTURELIBRARY_H
#define TEXTURELIBRARY_H

#include "ITextureLibrary.h"

/*
 * TextureLibrary.h
 *
 * The DACOM "TextureLibrary" component, implementing ITextureLibrary2 (and thus
 * ITextureLibraryA). A plain DACOM component (the IDAComponent / aggregation
 * plumbing is supplied by the DAComponentAggregate<> wrapper); the registration
 * hooks are the hand-written Register_/Shutdown_TextureLibrary in
 * TextureLibrary.cpp (declared in ITextureLibrary.h).
 */
#define CLSID_TextureLibrary "TextureLibrary"
struct TextureLibrary : public ITextureLibrary2
{
	BEGIN_DACOM_MAP_INBOUND(TextureLibrary)
	DACOM_INTERFACE_ENTRY2(IID_ITextureLibraryA, ITextureLibraryA)
	DACOM_INTERFACE_ENTRY2(IID_ITextureLibrary2, ITextureLibrary2)
	END_DACOM_MAP()

	// Called by the DACOM factory immediately after construction.
	GENRESULT init(AGGDESC* info);

	// ITextureLibraryA

	DEFMETHOD(set_library_state)(ITL_STATE state, U32 value);
	DEFMETHOD(get_library_state)(ITL_STATE state, U32* out_value);
	DEFMETHOD(load_library)(IFileSystem* IFS, const char* library_name);
	DEFMETHOD(load_texture)(IFileSystem* IFS, const char* texture_name);
	DEFMETHOD(free_library)(BOOL release_all);
	DEFMETHOD(update)(SINGLE dt);
	DEFMETHOD(set_unknown_string)(DWORD unknown, const char* unknown_string);
	DEFMETHOD(get_unknown_string)(DWORD unknown, char* out_unknown_string, U32 max_buf_size);
	DEFMETHOD(get_texture_id)(const char* texture_name, ITL_TEXTURE_ID* out_texture_id);
	DEFMETHOD(get_texture_id2)(CRCString* crc_texture_name /* $todo correct type here */, ITL_TEXTURE_ID* out_texture_id);
	DEFMETHOD(has_texture_id)(const char* texture_name);
	DEFMETHOD(add_ref_texture_id)(ITL_TEXTURE_ID texture_id, ITL_TEXTURE_REF_ID* out_texture_ref_id);
	DEFMETHOD(release_texture_id)(ITL_TEXTURE_ID texture_id);
	DEFMETHOD(add_ref_texture_ref)(ITL_TEXTURE_REF_ID texture_id);
	DEFMETHOD(release_texture_ref)(ITL_TEXTURE_REF_ID texture_ref_id);
	DEFMETHOD(get_texture_name)(ITL_TEXTURE_ID texture_id, char* out_texture_name, U32 max_buf_size);
	DEFMETHOD(set_texture_name)(ITL_TEXTURE_ID texture_id, const char* texture_name);
	DEFMETHOD(set_texture_frame_texture)(ITL_TEXTURE_ID texture_id, U32 txm_id_idx, U32 rp_texture_id);
	DEFMETHOD(set_texture_frame_id)(ITL_TEXTURE_ID texture_id, U32 frame_num, U32 txm_id_idx);
	DEFMETHOD(set_texture_frame_rect)(ITL_TEXTURE_ID texture_id, U32 frame_num, float u0, float v0, float u1, float v1);
	DEFMETHOD(get_texture_frame)(ITL_TEXTURE_ID texture_id, U32 frame_num, ITL_TEXTUREFRAME_IRP* out_frame);
	DEFMETHOD(get_texture_format)(ITL_TEXTURE_ID texture_id, U32 frame_num, PixelFormat* out_texture_format);
	DEFMETHOD(set_texture_frame_rate)(ITL_TEXTURE_ID texture_id, float fps_rate);
	DEFMETHOD(get_texture_frame_rate)(ITL_TEXTURE_ID texture_id, float* out_fps_rate);
	DEFMETHOD(get_texture_frame_count)(ITL_TEXTURE_ID texture_id, U32* out_frame_count);
	DEFMETHOD(get_texture_ref_texture_id)(ITL_TEXTURE_REF_ID texture_ref_id, ITL_TEXTURE_ID* out_texture_id);
	DEFMETHOD(get_texture_ref_frame)(ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num, ITL_TEXTUREFRAME_IRP* out_frame);
	DEFMETHOD(set_texture_ref_frame_time)(ITL_TEXTURE_REF_ID texture_ref_id, float frame_time);
	DEFMETHOD(get_texture_ref_frame_time)(ITL_TEXTURE_REF_ID texture_ref_id, float* out_frame_time);
	DEFMETHOD(set_texture_ref_frame_num)(ITL_TEXTURE_REF_ID texture_ref_id, U32 frame_num);
	DEFMETHOD(get_texture_ref_frame_num)(ITL_TEXTURE_REF_ID texture_ref_id, U32* out_frame_num);
	DEFMETHOD(set_texture_ref_frame_rate)(ITL_TEXTURE_REF_ID texture_ref_id, float fps_rate);
	DEFMETHOD(get_texture_ref_frame_rate)(ITL_TEXTURE_REF_ID texture_ref_id, float* out_fps_rate);
	DEFMETHOD(set_texture_ref_play_mode)(ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND play_command);
	DEFMETHOD(get_texture_ref_play_mode)(ITL_TEXTURE_REF_ID texture_ref_id, ITL_PLAYCOMMAND* out_play_command);
	DEFMETHOD(update_texture_ref)(ITL_TEXTURE_REF_ID texture_ref_id, SINGLE dt);
	DEFMETHOD(get_texture_count)(U32* out_num_textures);
	DEFMETHOD(get_texture)(U32 texture_num, ITL_TEXTURE_ID* out_texture_id);

	// ITextureLibrary2

	DEFMETHOD(sub_6EC2300)(ITL_TEXTURE_REF_ID texture_ref_id, void*);
	DEFMETHOD(sub_6EC23A0)(ITL_TEXTURE_REF_ID texture_ref_id, DWORD, void*);
	DEFMETHOD(sub_6EC2490)(ITL_TEXTURE_REF_ID texture_ref_id, DWORD);
	DEFMETHOD(sub_6EC2680)(ITL_TEXTURE_REF_ID texture_ref_id, void*);
	DEFMETHOD(sub_6EC37F0)(void*, void*);
	DEFMETHOD(sub_6EC1950)(void*);
};

/*
 * Component registration hooks, invoked from shading.dll's entry point
 * (DllMain.cpp). SHADING_DEC is the host module's export/import decoration.
 */
extern "C"
{
	SHADING_DEC void Register_TextureLibrary();
	SHADING_DEC void Shutdown_TextureLibrary();
}

#endif // TEXTURELIBRARY_H
