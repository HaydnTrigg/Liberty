#pragma once

#ifndef __RPDRAW_H__
#define __RPDRAW_H__

#include <DACOM.h>

#include <d3d8types.h>

//--------------------------------------------------------------------------//
//-------------------------------IRPDraw Interface--------------------------//
//--------------------------------------------------------------------------//

typedef struct IRPDraw* LPRPDRAW;

#define IID_IRPDraw DACOM_MAKE_IID("IRPDraw")
struct DACOM_NO_VTABLE IRPDraw : public IDAComponent
{
	// IDAComponent methods

	DEFMETHOD(QueryInterface)(const C8* interface_name, void** instance) = 0;
	DEFMETHOD_(U32, AddRef)(void) = 0;
	DEFMETHOD_(U32, Release)(void) = 0;

	// IRPDraw methods

	DEFMETHOD_(HRESULT, draw_indexed_primitive)(D3DPRIMITIVETYPE type, U32 min_index, U32 num_verts, U32 start_index, U32 count);
};

#endif
