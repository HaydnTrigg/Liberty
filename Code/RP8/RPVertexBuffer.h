#pragma once

#ifndef __RPVERTEXBUFFER_H__
#define __RPVERTEXBUFFER_H__

#include <DACOM.h>

#include <d3d8types.h>

//--------------------------------------------------------------------------//
//---------------------------IRPVertexBuffer Interface----------------------//
//--------------------------------------------------------------------------//

typedef struct IRPVertexBuffer* LPRPVERTEXBUFFER;
DECLARE_HANDLE(IRP_VERTEXBUFFERHANDLE);
#define IRP_INVALID_VB_HANDLE ((IRP_VERTEXBUFFERHANDLE)-1)

#include "VertexBufferDesc.h"

#define IID_IRPVertexBuffer DACOM_MAKE_IID("IRPVertexBuffer")
struct DACOM_NO_VTABLE IRPVertexBuffer : public IDAComponent
{
	// IDAComponent methods

	DEFMETHOD(QueryInterface)(const C8* interface_name, void** instance) = 0;
	DEFMETHOD_(U32, AddRef)(void) = 0;
	DEFMETHOD_(U32, Release)(void) = 0;

	// IRPVertexBuffer methods

	DEFMETHOD(create_vb)(U32 vertex_format, U32 num_verts, IRP_VERTEXBUFFERHANDLE* out_vb_handle, U8 irp_vbf_flags);
	DEFMETHOD(destroy_vb)(IRP_VERTEXBUFFERHANDLE& vb_handle);
	DEFMETHOD(ressize_vb)(IRP_VERTEXBUFFERHANDLE vb_handle, U32 format, U32 num_verts);
	DEFMETHOD(copy_vertices)(IRP_VERTEXBUFFERHANDLE vb_handle, U32* offset, VertexBufferDesc* src_vb_desc, U32 start_vertex, U32 num_vertices);
	DEFMETHOD(lock_vb)(IRP_VERTEXBUFFERHANDLE vb_handle, U32* start_index, void*& out_data, U32 num_verts);
	DEFMETHOD(unlock_vb)(IRP_VERTEXBUFFERHANDLE vb_handle);
	DEFMETHOD(RPVertexBuffer_Unknown24)(UNKNOWN);
	DEFMETHOD(select_vb)(IRP_VERTEXBUFFERHANDLE vb_handle);
	DEFMETHOD(get_vb_count)(IRP_VERTEXBUFFERHANDLE vb_handle, U32* out_vertex_format, U32* out_num_verts);
	DEFMETHOD_(BOOL32, is_vb_valid)(IRP_VERTEXBUFFERHANDLE vb_handle);
};

#endif
