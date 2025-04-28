/* ---------- headers */

#include "DX8Texture.h"
#include "FVF.h"

/* ---------- public code */

#define HRESULT_GET_ERROR_STRING(...) 0

DX8Texture::DX8Texture() :
	texture(),
	unknown4()
{

}

DX8Texture::~DX8Texture()
{
	dispose();
}

HRESULT DX8Texture::create_vb(IDirect3DDevice8* direct3d_device, U32 format, U32 num_verts)
{

	NOT_IMPLEMENTED;
}

void DX8Texture::dispose()
{
	if (texture != nullptr)
	{
		U32 refcount = texture->Release();
		if (refcount > 0)
		{
			GENERAL_WARNING(TEMPSTR("direct3d_texture released with %u references", refcount));
		}
		texture = nullptr;
	}
}

HRESULT DX8Texture::get_subsurface(U32 subsurface, IDirect3DSurface8** direct3d_surface)
{
	GENRESULT gr;
	HRESULT hr;

	D3DRESOURCETYPE resource_type = texture->GetType();

	switch (resource_type)
	{
	case D3DRTYPE_TEXTURE:
	{
		ASSERT(unknown4 == 0);

		IDirect3DTexture8* direct3d_2d_texture;
		if (SUCCEEDED(hr = texture->QueryInterface(
			IID_IDirect3DTexture8,
			reinterpret_cast<void**>(&direct3d_2d_texture))))
		{
			if (FAILED(hr = direct3d_2d_texture->GetSurfaceLevel(subsurface, direct3d_surface)))
			{
				GENERAL_ERROR(TEMPSTR("%s GetSurfaceLevel failed %s", __FUNCTION__, HRESULT_GET_ERROR_STRING(hr)));
				gr = GR_GENERIC;
			}
			else
			{
				gr = GR_OK;
			}
			direct3d_2d_texture->Release();
		}
		else
		{
			GENERAL_ERROR(TEMPSTR("%s QueryInterface for IDirect3DTexture8 failed %s", __FUNCTION__, HRESULT_GET_ERROR_STRING(hr)));
			gr = GR_GENERIC;
		}
	}
	break;
	case D3DRTYPE_CUBETEXTURE:
	{
		ASSERT(unknown4 == 1);

		IDirect3DCubeTexture8* direct3d_cube_texture;
		if (SUCCEEDED(hr = texture->QueryInterface(
			IID_IDirect3DCubeTexture8,
			reinterpret_cast<void**>(&direct3d_cube_texture))))
		{
			D3DCUBEMAP_FACES face = static_cast<D3DCUBEMAP_FACES>(subsurface);
			if (FAILED(hr = direct3d_cube_texture->GetCubeMapSurface(face, 0, direct3d_surface)))
			{
				GENERAL_ERROR(TEMPSTR("%s GetCubeMapSurface failed %s", __FUNCTION__, HRESULT_GET_ERROR_STRING(hr)));
				gr = GR_GENERIC;
			}
			else
			{
				gr = GR_OK;
			}
			direct3d_cube_texture->Release();
		}
		else
		{
			GENERAL_ERROR(TEMPSTR("%s QueryInterface for IDirect3DCubeTexture8 failed %s", __FUNCTION__, HRESULT_GET_ERROR_STRING(hr)));
			gr = GR_GENERIC;
		}
	}
	break;
	default:
	{
		GENERAL_FATAL(TEMPSTR("%s unsupported resource type %u", __FUNCTION__, static_cast<U32>(resource_type)));
		gr = GR_NOT_IMPLEMENTED;
	}
	break;
	}
	return gr;
}
