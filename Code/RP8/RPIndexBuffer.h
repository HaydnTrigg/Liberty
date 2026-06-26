#pragma once

#ifndef __RPINDEXBUFFER_H__
#define __RPINDEXBUFFER_H__

#include <DACOM.h>

#include <d3d8types.h>

//--------------------------------------------------------------------------//
//---------------------------IRPIndexBuffer Interface-----------------------//
//--------------------------------------------------------------------------//

typedef struct IRPIndexBuffer* LPRPINDEXBUFFER;
DECLARE_HANDLE(IRP_INDEXBUFFERHANDLE);
#define IRP_INVALID_IB_HANDLE ((IRP_INDEXBUFFERHANDLE)-1)

#define IID_IRPIndexBuffer DACOM_MAKE_IID("IRPIndexBuffer")
struct DACOM_NO_VTABLE IRPIndexBuffer : public IDAComponent
{
	// IDAComponent methods

	DEFMETHOD(QueryInterface)(const C8* interface_name, void** instance) = 0;
	DEFMETHOD_(U32, AddRef)(void) = 0;
	DEFMETHOD_(U32, Release)(void) = 0;

	// IRPIndexBuffer methods

	DEFMETHOD(create_index_buffer)(U32 num_indices, IRP_INDEXBUFFERHANDLE* out_ib_handle, U8 irp_ibf_flags);
	DEFMETHOD(destroy_index_buffer)(IRP_INDEXBUFFERHANDLE* ib_handle);
	DEFMETHOD_(HRESULT, create_ib)(IRP_INDEXBUFFERHANDLE ib_handle, U32 num_indices);
	DEFMETHOD(copy_indices)(IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, U16 const* indices, U32 num_indices);
	DEFMETHOD(lock_ib)(IRP_INDEXBUFFERHANDLE ib_handle, U32* start_index, void*& out_data, U32 num_indices);
	DEFMETHOD(unlock_ib)(IRP_INDEXBUFFERHANDLE ib_handle);
	DEFMETHOD(select_ib)(IRP_INDEXBUFFERHANDLE ib_handle, U32 base_index, UNKNOWN a4, UNKNOWN a5);
	DEFMETHOD(get_ib_count)(IRP_INDEXBUFFERHANDLE ib_handle, U32* out_count);
	DEFMETHOD_(BOOL32, is_ib_valid)(IRP_INDEXBUFFERHANDLE ib_handle);
};

#endif
