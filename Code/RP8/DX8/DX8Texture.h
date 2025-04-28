#pragma once
#ifndef __DX8TEXTURE_H__
#define __DX8TEXTURE_H__

/* ---------- headers */

#include "RPTexture.h"
#include <d3d8.h>

/* ---------- definitions */

struct IDirect3DDevice8;
struct IDirect3DTexture8;

struct DX8Texture
{
public:
	/* ---------- member variables */

	IDirect3DBaseTexture8* texture;
	UNKNOWN unknown4;

	/* ---------- member functions */

	DX8Texture();
	DX8Texture(DX8Texture&) = delete;
	~DX8Texture();

	[[nodiscard]] HRESULT create_vb(
		// The D3D device used to create the index buffer
		IDirect3DDevice8* direct3d_device,
		// Flexible Vertex Format
		U32 format,
		// The number of vertices to allocate
		U32 num_verts);
	void dispose();
	[[nodiscard]] HRESULT get_subsurface(U32 subsurface, IDirect3DSurface8** direct3d_surface);
};

#endif // __DX8TEXTURE_H__
