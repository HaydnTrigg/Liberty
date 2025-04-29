/* ---------- headers */

#include "CachedGeometry.h"
#include "RenderDebugger.h"

#include "DX8/DX8IndexBuffer.h"

#include <typedefs.h>
#include <d3d8.h>

/* ---------- public code */

GENRESULT CACHED_GEOMETRY::set_vertex_buffer(IRP_VERTEXBUFFERHANDLE hvertexbuffer, U32 vertex_format)
{
	if (current_vertex_buffer_handle != hvertexbuffer || vertex_format != current_vertex_format)
	{
		vertex_dirty = true;
		current_vertex_buffer_handle = hvertexbuffer;
		current_vertex_format = vertex_format;
	}
	return GR_OK;
}

GENRESULT CACHED_GEOMETRY::set_index_buffer(IRP_INDEXBUFFERHANDLE hindexbuffer, U32 base_index)
{
	if (current_index_buffer_handle != hindexbuffer || base_index != current_base_index)
	{
		index_dirty = true;
		current_index_buffer_handle = hindexbuffer;
		current_base_index = base_index;
	}
	return GR_OK;
}

GENRESULT CACHED_GEOMETRY::flush_indices(IDirect3DDevice8* direct3d_device)
{
	DX8IndexBuffer* index_buffer = reinterpret_cast<DX8IndexBuffer*>(current_index_buffer_handle);

	GENRESULT gr = GR_OK;
	if (index_dirty)
	{
		HRESULT hr;
		if (current_index_buffer_handle)
		{
			hr = direct3d_device->SetIndices(index_buffer->buffer, current_base_index);
		}
		else
		{
			hr = direct3d_device->SetIndices(NULL, 0);
		}
		if (FAILED(hr))
		{
			GENERAL_ERROR(TEMPSTR("%s: %s", __FUNCTION__, HRESULT_TO_STRING(hr)));
			gr = GR_GENERIC;
		}
	}
	return gr;
}

GENRESULT CACHED_GEOMETRY::invalidate()
{
	current_vertex_format = D3DFMT_UNKNOWN;
	current_vertex_buffer_handle = nullptr;
	current_index_buffer_handle = nullptr;
	current_base_index = UINT_MAX;
	index_dirty = false;
	vertex_dirty = false;

	return GR_OK;
}
