// RenderDebugger.cpp
//
// 
//

//

#pragma warning( disable : 4018 )	// signed/unsigned mismatch
#pragma warning( disable : 4100 )	// unreferenced formal parameter
#pragma warning( disable : 4239 )
#pragma warning( disable : 4245 )	// conversion from int to long
#pragma warning( disable : 4530 )   // exceptions disabled
#pragma warning( disable : 4702 )   // unreachable code
#pragma warning( disable : 4710 )   // function 'foo' not inlined
#pragma warning( disable : 4786 )   // identifier truncated

//

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
//#include <ddraw.h>
#include <d3d8.h>
#include <io.h>

#include <string>
#include <list>


#include "FVF.h"

//

#include "RenderDebugger.h"

//

#if RP_RD_DEBUG

//

typedef std::list<std::string> rp_rd_string_buffer;

//

struct RPRD_DEBUGGERSTATE
{
	U32  trace_enabled;				// enable/disable bits for scene capture for tracing
	bool frustum_view_enabled;		// enable/disable extended frustum viewing
	
	bool screen_capture_at_swap;		// a screen capture has been scheduled for the next swap
	char filename[MAX_PATH];			// filename to save screen capture into
	LPDIRECTDRAWSURFACE7 render_target;	// target to save screen capture from (NOT AddRef()'d)

	unsigned long trace_time_stamp;	// time stamp used when logging trace events
	char trace_output_dir[MAX_PATH];	// directory to prepend to output filenames

	unsigned long frame_count;		// number of swap_buffer calls
	unsigned long scene_count;		// number of end_scene_calls;

	unsigned long tab_count;		// number of indenting tabs in output

	float sb_delay_ms;				// swap_buffers delay in milliseconds.
	float dp_delay_ms;				// draw_*_primitive_* delay in milliseconds

	U32 width;
	U32 height;
	U32 color_bpp;
	U32 depth_bpp;
	U32 stencil_bpp;

	RenderStateArray		*render_state_info;
	TextureStageStateArray	*texture_stage_state_info;

	unsigned long dp_calls;			// number of dp calls
	unsigned long dip_calls;		// number of dip calls
	unsigned long dp_vb_calls;		// number of dp vb calls
	unsigned long dip_vb_calls;		// number of dip vb calls

	unsigned long dp_prims;			// number of dp prims
	unsigned long dip_prims;		// number of dip prims
	unsigned long dp_vb_prims;		// number of dp vb prims
	unsigned long dip_vb_prims;		// number of dip vb prims
};

rp_rd_string_buffer string_buffer;

//

RPRD_DEBUGGERSTATE rp_rd_state ;

//

const char *primitive_types[] =
{
	"invalid",
	"points",
	"linelist",
	"linestrip",
	"trilist",
	"tristrip",
	"trifan"
};

//

struct PRIMITIVETYPEINFO {
	U32 vert_fixed_cost;		//	           (num_vertices - vert_fixed_cost) 
								// num_prims =  ------------------------------           
	U32 vert_per_prim_cost;		//                    vert_per_prim_cost
	D3DPRIMITIVETYPE type;		// type used when rendering
	U32	vert_count;				// count used when rendering
};

static PRIMITIVETYPEINFO primitive_type_info[7] = {
	{1,1,D3DPT_POINTLIST,1},
	{0,1,D3DPT_POINTLIST,1},
	{0,2,D3DPT_LINELIST,2},
	{1,1,D3DPT_LINELIST,2},
	{0,3,D3DPT_TRIANGLELIST,3},
	{2,1,D3DPT_TRIANGLELIST,3},
	{2,1,D3DPT_TRIANGLELIST,3}
};


//

void rp_rd_init( RenderStateArray *rs_info, TextureStageStateArray *tss_info )
{
	string_buffer.clear();

	memset( &rp_rd_state, 0, sizeof(rp_rd_state) );

	rp_rd_state.render_state_info = rs_info;
	rp_rd_state.texture_stage_state_info = tss_info;
}

//

void rp_rd_cleanup( void )
{
	string_buffer.clear();

	// just to be deterministic, clear the state
	//
	memset( &rp_rd_state, 0, sizeof(rp_rd_state) );
}

//

inline unsigned long rp_rd_trace_time()
{
	return rp_rd_state.trace_time_stamp++;
}

//

void rp_rd_clear_prim_counts( void )
{
	rp_rd_state.dp_calls = 0 ;
	rp_rd_state.dp_vb_calls = 0 ;
	rp_rd_state.dip_calls = 0 ;
	rp_rd_state.dip_vb_calls = 0 ;

	rp_rd_state.dp_prims = 0 ;
	rp_rd_state.dp_vb_prims = 0 ;
	rp_rd_state.dip_prims = 0 ;
	rp_rd_state.dip_vb_prims = 0 ;
}

//

void rp_rd_get_prim_counts( U32 *out_dp_calls, U32 *out_dp_prims, U32 *out_dip_calls, U32 *out_dip_prims, U32 *out_dp_vb_calls, U32 *out_dp_vb_prims, U32 *out_dip_vb_calls, U32 *out_dip_vb_prims ) 
{
	*out_dp_calls = rp_rd_state.dp_calls;
	*out_dp_vb_calls = rp_rd_state.dp_vb_calls;
	*out_dip_calls = rp_rd_state.dip_calls;
	*out_dip_vb_calls = rp_rd_state.dip_vb_calls;

	*out_dp_prims = rp_rd_state.dp_prims;
	*out_dp_vb_prims = rp_rd_state.dp_vb_prims;
	*out_dip_prims = rp_rd_state.dip_prims;
	*out_dip_vb_prims = rp_rd_state.dip_vb_prims;
}

//

IRD_STATETYPE rp_rd_set_trace_enable( IRD_STATETYPE state_traces ) 
{
	IRD_STATETYPE old = rp_rd_state.trace_enabled ;
	
	rp_rd_state.trace_enabled = state_traces;
	
	return old;
}

//

void rp_rd_set_trace_output_dir( const char *directory )
{
	strcpy( rp_rd_state.trace_output_dir, directory );
}

//

void rp_rd_add_trace_message( const char *fmt, ... ) 
{
	if( rp_rd_state.trace_enabled == 0 ) {
		return;
	}

	static char output[4096];
	static char buffer[4096];

	va_list args;

	va_start( args, fmt );
	vsprintf( buffer, fmt, args );
	va_end( args );

	unsigned long t;

	for( t=0; t<rp_rd_state.tab_count; t++ ) {
		output[t] = '\t';
	}
	
	strcpy( &output[t], buffer );

	string_buffer.push_back( std::string( output ) ) ;

#if 0
	FILE* file = fopen( "\\rp_rd_test.lua", "at" );
	if( file ) {
		fprintf( file, "\t%s\n", output );
		fclose( file );
	}
#endif

}

//

void rp_rd_clear_trace_capture( void ) 
{
	string_buffer.clear();
}

//

void rp_rd_save_trace_capture( const char *filename ) 
{
	rp_rd_string_buffer::iterator beg = string_buffer.begin();
	rp_rd_string_buffer::iterator end = string_buffer.end();
	rp_rd_string_buffer::iterator line;

	if( beg != end ) {
		FILE *file;

		if( (file = fopen( filename, "wt+" )) == NULL ) {
			return;
		}

		for( line = beg; line != end; line++ ) {
			fprintf( file, "%s\n", (*line).c_str() );
		}

		fclose( file );
	}
}

//

void rp_rd_begin_trace_section( const char *key ) 
{
	rp_rd_add_trace_message( "" );
	rp_rd_add_trace_message( "function %s( )", key );
	
	rp_rd_state.tab_count++;
}

//

void rp_rd_end_trace_section( void ) 
{
	rp_rd_state.tab_count--;

	rp_rd_add_trace_message( "end" );
	rp_rd_add_trace_message( "" );
}

//

void rp_rd_set_frustum_view_enable( bool onoff ) 
{
	rp_rd_state.frustum_view_enabled = onoff;
}

//

void rp_rd_set_frustum_view_options	( void ) 
{

}

//

void rp_rd_set_sb_delay( float milliseconds )
{
	rp_rd_state.sb_delay_ms = milliseconds;
}

//

void rp_rd_set_dp_delay( float milliseconds ) 	
{
	rp_rd_state.dp_delay_ms = milliseconds;
}

//

void rp_rd_save_surface_bmp( const char *filename, LPDIRECTDRAWSURFACE7 surface ) 
{
	/*DDSURFACEDESC2 ddsd;
	unsigned char *rgb_data, *in_data, *out_data;
	unsigned int src_pxl_u32;
	unsigned int size=0;

	memset( &ddsd, 0, sizeof(ddsd) );
	ddsd.dwSize = sizeof( ddsd );

	if( SUCCEEDED( surface->Lock( NULL, &ddsd, DDLOCK_WAIT, 0 ) ) ) {

		size = ddsd.dwWidth * ddsd.dwHeight * 4;

		PixelFormat_OLD pf( ddsd.ddpfPixelFormat_OLD );

		if( (rgb_data = (unsigned char*)malloc( size )) != NULL ) {

			out_data = rgb_data;
	
			int src_bd = (pf.num_bits() + 7 ) / 8;

			unsigned char r,g,b,a;

			for( int y=ddsd.dwHeight-1; y>0; y-- ) {
				
				in_data = ((unsigned char*)ddsd.lpSurface) + y*ddsd.lPitch;

				for( int x=0; x<ddsd.dwWidth; x++ ) {

					src_pxl_u32	= *((U32*)in_data);

					r = ((src_pxl_u32 & pf.get_r_mask()) >> pf.rl) << pf.rr;
					g = ((src_pxl_u32 & pf.get_g_mask()) >> pf.gl) << pf.gr;
					b = ((src_pxl_u32 & pf.get_b_mask()) >> pf.bl) << pf.br;
					a = 0;

					*out_data = b; out_data++;
					*out_data = g; out_data++;
					*out_data = r; out_data++;
					*out_data = a; out_data++;

					in_data += src_bd;
				}
			}
			
		}

		surface->Unlock( NULL );
	}


	FILE *file;
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	short bcookie = 0x4d42;

	if( (file = fopen( filename, "wb" )) == NULL ) {
		return ;
	}

	memset( &bfh, 0, sizeof(bfh) );
	bfh.bfType		= bcookie;
	bfh.bfSize		= sizeof(bfh) + sizeof(bih) + size;
	bfh.bfOffBits	= sizeof(bfh) + sizeof(bih);

	fwrite( &bfh, sizeof(bfh), 1, file );

	memset( &bih, 0, sizeof(bih) );
	bih.biSize		   = sizeof(bih);
	bih.biWidth        = ddsd.dwWidth;
	bih.biHeight       = ddsd.dwHeight;
	bih.biPlanes       = 1;
	bih.biBitCount     = 32;
	bih.biCompression  = BI_RGB;
	bih.biSizeImage    = size;

	fwrite( &bih, sizeof(bih), 1, file );
	
	fwrite( rgb_data, 1, size, file );
	
	free( rgb_data );

	fclose( file );*/
	NOT_IMPLEMENTED;
}

//

void rp_rd_save_surface_dds( const char *filename, LPDIRECTDRAWSURFACE7 surface ) 
{
//#pragma message( "TODO: support cube maps in rp_rd_save_surface" )
//#pragma message( "TODO: support multiple saves (dynamic textures) rp_rd_save_surface" )

	// dump DDS file, code massaged from dxtex tool code
	//

	FILE *file ;
	HRESULT hr;
	DDSURFACEDESC2 ddsd;
	DWORD dwMagic;
	DDSCAPS2 ddsCaps;
	LPDIRECTDRAWSURFACE7 pdds;
	LPDIRECTDRAWSURFACE7 pdds2;

	ZeroMemory( &ddsd, sizeof(ddsd) );
	ddsd.dwSize = sizeof(ddsd);
	
	if( FAILED( surface->GetSurfaceDesc( &ddsd ) ) ) {
		return;
	}

	if( (file = fopen( filename, "wb" )) == NULL ) {
		return;
	}

	dwMagic = MAKEFOURCC('D','D','S',' ');

	fwrite( &dwMagic, sizeof(dwMagic), 1, file );
	
	// Mask out all caps indicating image uses, and only
	// keep those intrinsic to the image
	// e.g., remove DDSCAPS_SYSTEMMEMORY, but keep DDSCAPS_MIPMAP
	ddsd.ddsCaps.dwCaps &= (DDSCAPS_ALPHA | DDSCAPS_COMPLEX | DDSCAPS_PALETTE | DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_OPTIMIZED);
	ddsd.ddsCaps.dwCaps2 &= (DDSCAPS2_CUBEMAP);
	ddsd.ddsCaps.dwCaps3 &= ( 0 );
	ddsd.ddsCaps.dwCaps4 &= ( 0 );

	fwrite( &ddsd, sizeof(ddsd), 1, file );

	// Write out top level, along with any mips

	pdds = surface;
	pdds->AddRef();

	ZeroMemory( &ddsCaps, sizeof(ddsCaps) );
	ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;

	while( TRUE ) {

		ZeroMemory( &ddsd, sizeof(ddsd) );
		ddsd.dwSize = sizeof(ddsd);
		
		if( SUCCEEDED( hr = pdds->Lock( NULL, &ddsd, DDLOCK_WAIT, NULL ) ) ) {

			if( ddsd.dwFlags & DDSD_LINEARSIZE ) {
				fwrite( ddsd.lpSurface, 1, ddsd.dwLinearSize, file );
			}
			else 
			{
				DWORD yp;
				BYTE* pbSrc = (BYTE*)ddsd.lpSurface;
				LONG dataBytesPerRow = ddsd.dwWidth * ddsd.ddpfPixelFormat.dwRGBBitCount / 8;
				
				for( yp = 0; yp < ddsd.dwHeight; yp++ ) {
					fwrite( pbSrc, 1, dataBytesPerRow, file );
					pbSrc += ddsd.lPitch;
				}
			}
		
			pdds->Unlock( NULL );
		}
	
		if( FAILED( hr = pdds->GetAttachedSurface( &ddsCaps, &pdds2 ) ) ) {
			if( pdds ) {
				pdds->Release();
				pdds = NULL;
			}
			break;
		}

		if( pdds ) {
			pdds->Release();
			pdds = NULL;
		}

		pdds = pdds2;
	}

	fclose( file );

	return;
}

//

void rp_rd_save_screen_capture( const char *filename, LPDIRECTDRAWSURFACE7 render_target, bool at_next_swap_buffers  ) 
{

	if( !at_next_swap_buffers ) {
		rp_rd_save_surface_bmp( filename, render_target );
	}
	else {
		rp_rd_state.screen_capture_at_swap = true;
		rp_rd_state.render_target = render_target;
		strcpy( rp_rd_state.filename, filename );
	}
}

//

void rp_rd_create_buffers( U32 width, U32 height, U32 color_bpp, U32 depth_bpp, U32 stencil_bpp )
{
	rp_rd_state.width = width;
	rp_rd_state.height = height;
	rp_rd_state.color_bpp = color_bpp;
	rp_rd_state.depth_bpp = depth_bpp;
	rp_rd_state.stencil_bpp = stencil_bpp;

	rp_rd_add_trace_message( "fbconfig( %d, %d, %d, %d, %d );", width, height, color_bpp, depth_bpp, stencil_bpp );
}

//

void rp_rd_clear( D3DRECT *rect, U32 flags, U32 color, float depth, U32 stencil )
{
	rp_rd_add_trace_message( "clear( { %d, %d, %d, %d }, %d, %d, %5.3f, %d ); -- flags: %08X clr:%08X stencil:%08X",
							 rect->x1, rect->y1, rect->x2, rect->y2,
							 flags,
							 color,
							 depth,
							 stencil,
							 flags, 
							 color,
							 stencil );
}

//

void rp_rd_begin_scene( void )
{
	rp_rd_add_trace_message( "beginscene( %d );", rp_rd_state.scene_count );
}

//

void rp_rd_end_scene( void )
{
	rp_rd_add_trace_message( "endscene( %d );", rp_rd_state.scene_count );
	
	rp_rd_state.scene_count++;
}

//

void rp_rd_swap( void )
{
	rp_rd_add_trace_message( "swap(); -- delay = %5.3f ms, frame = %d", rp_rd_state.sb_delay_ms, rp_rd_state.frame_count );

	rp_rd_add_trace_message( "" );
	rp_rd_add_trace_message( "" );
	rp_rd_add_trace_message( "" );
	
	if( rp_rd_state.sb_delay_ms > 0.0f ) {
		Sleep( rp_rd_state.sb_delay_ms );
	}

	if( rp_rd_state.screen_capture_at_swap ) {
		rp_rd_save_surface_bmp( rp_rd_state.filename, rp_rd_state.render_target );
		rp_rd_state.screen_capture_at_swap = false;
	}

	rp_rd_state.frame_count++;
}

//

void rp_rd_viewport( const D3DVIEWPORT8 *value )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_TRANSFORM) ) {
		return ;
	}

	rp_rd_add_trace_message( "viewport( %5d, %5d, %5d, %5d, %5.3f, %5.3f );", 
							 value->X,
							 value->Y,
							 value->Width,
							 value->Height,
							 value->MinZ,
							 value->MaxZ );
}

//

void rp_rd_ortho( float left, float right, float bottom, float top, float nearval, float farval )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_TRANSFORM) ) {
		return ;
	}

	rp_rd_add_trace_message( "-- ortho( %5.3f, %5.3f, %5.3f, %5.3f, %5.3f, %5.3f );", left, right, bottom, top, nearval, farval );
}

//

void rp_rd_perspective( float fovy, float aspect, float znear, float zfar )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_TRANSFORM) ) {
		return ;
	}

	rp_rd_add_trace_message( "-- perspective( %5.3f, %5.3f, %5.3f, %5.3f );", fovy, aspect, znear, zfar );
}

//

void rp_rd_transform( D3DTRANSFORMSTATETYPE which, const D3DMATRIX *value )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_TRANSFORM) ) {
		return ;
	}

	static const char *names[] =
	{
		"invalid",
		"world",
		"view",
		"projection"
	};

	static const char *spaces[] =
	{
		"       ",
		"     ",
		"    ",
		"          "
	};

	if( which <= D3DTS_PROJECTION ) {
		
		rp_rd_add_trace_message(	"%s( % 5.3f, % 5.3f, % 5.3f, % 5.3f,",  names[which],  value->m[0][0], value->m[0][1], value->m[0][2], value->m[0][3] );
		rp_rd_add_trace_message(	"%s  % 5.3f, % 5.3f, % 5.3f, % 5.3f,",  spaces[which], value->m[1][0], value->m[1][1], value->m[1][2], value->m[1][3] );
		rp_rd_add_trace_message(	"%s  % 5.3f, % 5.3f, % 5.3f, % 5.3f,",  spaces[which], value->m[2][0], value->m[2][1], value->m[2][2], value->m[2][3] );
		rp_rd_add_trace_message(	"%s  % 5.3f, % 5.3f, % 5.3f, % 5.3f );",spaces[which], value->m[3][0], value->m[3][1], value->m[3][2], value->m[3][3] );
	}
		
	if( (which >= D3DTS_TEXTURE0) && (which <= D3DTS_TEXTURE7) ) {

		U32 num = which - D3DTS_TEXTURE0;

		rp_rd_add_trace_message(	"texturetransform( %d, % 5.3f, % 5.3f, % 5.3f, % 5.3f,", num, value->m[0][0], value->m[0][1], value->m[0][2], value->m[0][3] );
		rp_rd_add_trace_message(	"                     % 5.3f, % 5.3f, % 5.3f, % 5.3f,",      value->m[1][0], value->m[1][1], value->m[1][2], value->m[1][3] );
		rp_rd_add_trace_message(	"                     % 5.3f, % 5.3f, % 5.3f, % 5.3f,",      value->m[2][0], value->m[2][1], value->m[2][2], value->m[2][3] );
		rp_rd_add_trace_message(	"                     % 5.3f, % 5.3f, % 5.3f, % 5.3f );",    value->m[3][0], value->m[3][1], value->m[3][2], value->m[3][3] );
	}
}

//

void rp_rd_render_state( D3DRENDERSTATETYPE state, U32 value )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_RENDER_STATE) ) {
		return ;
	}

	rp_rd_add_trace_message( "renderstate( % 4d, %d ); -- %s = 0x%08X", state, value, (*rp_rd_state.render_state_info)[state].key_name, value );
}

//

void rp_rd_texture( U32 stage, IDirectDrawSurface7 *texture )
{
	if( texture ) {

		char filename[MAX_PATH];
		sprintf( filename, "%s\\tex%p.dds", rp_rd_state.trace_output_dir, texture );

		if( rp_rd_state.trace_enabled & IRD_ST_TEXTURE ) {
			rp_rd_add_trace_message( "texture( %d, [[%s]] );", stage, filename );
		}
		if( rp_rd_state.trace_enabled & IRD_ST_TEXTURE_DATA ) {
			rp_rd_save_surface_dds( filename, texture );
		}
	}
	else {

		rp_rd_add_trace_message( "nulltexture( %d );", stage );

	}
}

//

void rp_rd_texture_state( U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_TEXTURE_STATE) ) {
		return ;
	}

	rp_rd_add_trace_message( "texturestate( %d, % 3d, %d ); -- %s = 0x%08X", stage, state, value, (*rp_rd_state.texture_stage_state_info)[state].key_name, value );
}

//

void rp_rd_light( IRP_LIGHTHANDLE handle, const D3DLIGHT8 *value )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_LIGHT) ) {
		return ;
	}

	static const char *names[] = 
	{
		"invalid",
		"point",
		"spot",
		"directional"
	};

	rp_rd_add_trace_message( "light( %p, {  -- %s", handle, names[value->Type] );
	rp_rd_add_trace_message( "              Type = %d,", value->Type );
	rp_rd_add_trace_message( "              Position = { % 5.3f, % 5.3f, % 5.3f },", value->Position.x, value->Position.y, value->Position.z );
	rp_rd_add_trace_message( "              Direction= { % 5.3f, % 5.3f, % 5.3f },", value->Direction.x, value->Direction.y, value->Direction.z );
	rp_rd_add_trace_message( "              Ambient  = { %5.3f, %5.3f, %5.3f, %5.3f },", value->Ambient.r, value->Ambient.g, value->Ambient.b, value->Ambient.a );
	rp_rd_add_trace_message( "              Diffuse  = { %5.3f, %5.3f, %5.3f, %5.3f },", value->Diffuse.r, value->Diffuse.g, value->Diffuse.b, value->Diffuse.a );
	rp_rd_add_trace_message( "              Specular = { %5.3f, %5.3f, %5.3f, %5.3f },", value->Specular.r, value->Specular.g, value->Specular.b, value->Specular.a );
	rp_rd_add_trace_message( "              Attenuation = { % 5.3f, % 5.3f, % 5.3f },", value->Attenuation0, value->Attenuation1, value->Attenuation2 );
	rp_rd_add_trace_message( "              Range = %5.3f,", value->Range );
	rp_rd_add_trace_message( "              Falloff = %5.3f,", value->Falloff );
	rp_rd_add_trace_message( "              Theta = %5.3f,", value->Theta );
	rp_rd_add_trace_message( "              Phi = %5.3f ", value->Phi );
	rp_rd_add_trace_message( "           } );" ); 
}

//

void rp_rd_light_enable( IRP_LIGHTHANDLE handle, U32 value )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_LIGHT) ) {
		return ;
	}

	rp_rd_add_trace_message( "lightenable( %p, %d );", handle, value );
}

//

void rp_rd_material( const D3DMATERIAL8 *value )
{
	if( !(rp_rd_state.trace_enabled & IRD_ST_LIGHT) ) {
		return ;
	}

	rp_rd_add_trace_message( "material( { -- #%08X", value );
	rp_rd_add_trace_message( "             Ambient  = { %5.3f, %5.3f, %5.3f, %5.3f },", value->Ambient.r, value->Ambient.g, value->Ambient.b, value->Ambient.a );
	rp_rd_add_trace_message( "             Emissive = { %5.3f, %5.3f, %5.3f, %5.3f },", value->Emissive.r, value->Emissive.g, value->Emissive.b, value->Emissive.a );
	rp_rd_add_trace_message( "             Diffuse  = { %5.3f, %5.3f, %5.3f, %5.3f },", value->Diffuse.r, value->Diffuse.g, value->Diffuse.b, value->Diffuse.a );
	rp_rd_add_trace_message( "             Specular = { %5.3f, %5.3f, %5.3f, %5.3f },", value->Specular.r, value->Specular.g, value->Specular.b, value->Specular.a );
	rp_rd_add_trace_message( "             Power = %5.3f", value->Power );
	rp_rd_add_trace_message( "          } ); " );

}

//

const char *rp_rd_vertex_format_string( U32 vertex_format )
{
	static char buffer[MAX_PATH];
	char *out;

	out = buffer;
	buffer[0] = 0;

	if( vertex_format & D3DFVF_XYZ ) {
		*out++ = 'P';
	}
	else if( vertex_format & D3DFVF_XYZRHW ) {
		*out++ = 'P';
		*out++ = '|';
		*out++ = 'R';
		*out++ = 'H';
		*out++ = 'W';
	}

	if( vertex_format & D3DFVF_NORMAL ) {
		*out++ = '|';
		*out++ = 'N';
	}

	if( vertex_format & D3DFVF_DIFFUSE ) {
		*out++ = '|';
		*out++ = 'C';
		*out++ = '0';
	}

	if( vertex_format & D3DFVF_SPECULAR ) {
		*out++ = '|';
		*out++ = 'C';
		*out++ = '1';
	}

	if( (vertex_format & D3DFVF_TEXCOUNT_MASK) != 0 ) {
		*out++ = '|';
		*out++ = 'U';
		*out++ = 'V';
		*out++ = '0' + ((vertex_format & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);
	}

	*out++ = 0;

	return buffer;
}

//

void rp_rd_save_vertex_data( const char *filename, U32 vertex_format, const void *verts, int num_verts )
{
	FILE *file ;
	DWORD dwMagic;

	if( (file = fopen( filename, "wb" )) == NULL ) {
		return;
	}

	dwMagic = MAKEFOURCC('D','D','V','B');

	fwrite( &dwMagic, sizeof(dwMagic), 1, file );
	fwrite( &vertex_format, sizeof(vertex_format), 1, file );
	fwrite( &num_verts, sizeof(num_verts), 1, file );
	fwrite( verts, 1, FVF_SIZEOF_VERT(vertex_format) * num_verts, file );

	fclose( file );
}

//

void rp_rd_dump_vertex_data( U32 handle, U32 vertex_format, const void *verts, int num_verts )
{
	rp_rd_add_trace_message( "" );
	rp_rd_add_trace_message( "vb%08X = {", handle );
	rp_rd_add_trace_message( "            Name   = %d,", handle );
	rp_rd_add_trace_message( "            Format = %d, -- %s", vertex_format, rp_rd_vertex_format_string( vertex_format ) );
	rp_rd_add_trace_message( "            Count  = %d,", num_verts );
	rp_rd_add_trace_message( "            Data   = {" );

	// dump the vertex data as byte values, this code is ugly!
	//
	char *vert = (char*)verts;
	U32 sizeof_vert = FVF_SIZEOF_VERT( vertex_format );
	U32 dword_count = (num_verts*sizeof_vert) / 4;
	U32 extra_count = (num_verts*sizeof_vert) % 4;
	U32 b;
	
	for( b = 0; b < (dword_count-1)*4; b+=4 ) {
		rp_rd_add_trace_message( "                        %d, %d, %d, %d, ", vert[b+0], vert[b+1], vert[b+2], vert[b+3] ); 
	}

	switch( extra_count ) {

	case 0:	
		rp_rd_add_trace_message( "                        %d, %d, %d, %d  ", vert[b+0], vert[b+1], vert[b+2], vert[b+3] ); 
		break;
		
	case 1:	
		rp_rd_add_trace_message( "                        %d, %d, %d, %d, ", vert[b+0], vert[b+1], vert[b+2], vert[b+3] ); 
		b += 4;
		rp_rd_add_trace_message( "                        %d ", vert[b+0] );
		break;
	
	case 2:	
		rp_rd_add_trace_message( "                        %d, %d, %d, %d, ", vert[b+0], vert[b+1], vert[b+2], vert[b+3] ); 
		b += 4;
		rp_rd_add_trace_message( "                        %d, %d ", vert[b+0], vert[b+1] );
		break;

	case 3:	
		rp_rd_add_trace_message( "                        %d, %d, %d, %d, ", vert[b+0], vert[b+1], vert[b+2], vert[b+3] ); 
		b += 4;
		rp_rd_add_trace_message( "                        %d, %d, %d ", vert[b+0], vert[b+1], vert[b+2] );
		break;

	}

	rp_rd_add_trace_message( "            } -- end of Data" );
	rp_rd_add_trace_message( "}; -- end of vb%08X", handle );
	rp_rd_add_trace_message( "" );
	
}

//

void rp_rd_save_index_data( const char *filename, const WORD *indices, int num_indices )
{
	FILE *file ;
	DWORD dwMagic;

	if( (file = fopen( filename, "wb" )) == NULL ) {
		return;
	}

	dwMagic = MAKEFOURCC('D','D','I','B');

	fwrite( &dwMagic, sizeof(dwMagic), 1, file );
	fwrite( &num_indices, sizeof(num_indices), 1, file );
	fwrite( indices, sizeof(U16), num_indices, file );

	fclose( file );
}

//

void rp_rd_dump_index_data( U32 handle, const U16 *indices, int num_indices )
{
	rp_rd_add_trace_message( "" );
	rp_rd_add_trace_message( "ib%08X = {", handle );
	rp_rd_add_trace_message( "            Name   = %d,", handle );
	rp_rd_add_trace_message( "            Count  = %d,", num_indices );
	rp_rd_add_trace_message( "            Data   = {" );

	int i;

	for( i=0; i<num_indices-1; i++ ) {
		rp_rd_add_trace_message( "                        %d, ", indices[i] );
	}

	rp_rd_add_trace_message( "                        %d ", indices[i] );

	rp_rd_add_trace_message( "            } -- end of Data" );
	rp_rd_add_trace_message( "}; -- end of ib%08X", handle );
	rp_rd_add_trace_message( "" );
	
}

//

void rp_rd_dp( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, U32 flags ) 
{
	rp_rd_state.dp_calls++;

	rp_rd_state.dp_prims +=		(num_verts - primitive_type_info[type].vert_fixed_cost) / 
								    primitive_type_info[type].vert_per_prim_cost;


	if( rp_rd_state.trace_enabled & IRD_ST_PRIMITIVE ) {

		U32 timestamp = rp_rd_trace_time();

		char vbname[MAX_PATH];

		sprintf( vbname, "%s\\vb%08X.vbd", rp_rd_state.trace_output_dir, timestamp );
		rp_rd_save_vertex_data( vbname, vertex_format, verts, num_verts );

		rp_rd_add_trace_message( "%svb( [[%s]], %d, %d, %d );", primitive_types[type], vbname, 0, num_verts, flags );
		rp_rd_add_trace_message( "" );
	}

	if( rp_rd_state.dp_delay_ms > 0.0f ) {
		Sleep( rp_rd_state.dp_delay_ms );
	}
}

//

void rp_rd_dp_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, U32 flags ) 
{
	rp_rd_state.dp_vb_calls++;

	rp_rd_state.dp_vb_prims +=	(num_verts - primitive_type_info[type].vert_fixed_cost) / 
								    primitive_type_info[type].vert_per_prim_cost;


	if( rp_rd_state.trace_enabled & IRD_ST_PRIMITIVE ) {

		U32 timestamp = rp_rd_trace_time();

		char vbname[MAX_PATH];

		sprintf( vbname, "%s\\vb%08X.vbd", rp_rd_state.trace_output_dir, timestamp );
	//	rp_rd_save_vertex_data( vbname, vertex_format, verts, num_verts );

		rp_rd_add_trace_message( "%svb( [[%s]], %d, %d, %d );", primitive_types[type], vbname, start_vert, num_verts, flags );
		rp_rd_add_trace_message( "" );

	}

	if( rp_rd_state.dp_delay_ms > 0.0f ) {
		Sleep( rp_rd_state.dp_delay_ms );
	}
}

//

void rp_rd_dip( D3DPRIMITIVETYPE type, U32 vertex_format, const void *verts, int num_verts, const U16 * indices, U32 num_indices, U32 flags ) 
{
	rp_rd_state.dip_calls++;

	rp_rd_state.dip_prims +=	(num_indices - primitive_type_info[type].vert_fixed_cost) / 
								    primitive_type_info[type].vert_per_prim_cost;

	if( rp_rd_state.trace_enabled & IRD_ST_PRIMITIVE ) {

		U32 timestamp = rp_rd_trace_time();

		char vbname[MAX_PATH];
		char ibname[MAX_PATH];

		sprintf( vbname, "%s\\vb%08X.vbd", rp_rd_state.trace_output_dir, timestamp );
		rp_rd_save_vertex_data( vbname, vertex_format, verts, num_verts );

		sprintf( ibname, "%s\\ib%08X.ibd", rp_rd_state.trace_output_dir, timestamp );
		rp_rd_save_index_data( ibname, indices, num_indices );

		rp_rd_add_trace_message( "indexed%svb( [[%s]], %d, %d, [[%s]], %d, %d );", primitive_types[type], vbname, 0, num_verts, ibname, num_indices, flags );
		rp_rd_add_trace_message( "" );
	}

	if( rp_rd_state.dp_delay_ms > 0.0f ) {
		Sleep( rp_rd_state.dp_delay_ms );
	}
}

//

void rp_rd_dip_vb( D3DPRIMITIVETYPE type, IRP_VERTEXBUFFERHANDLE vb_handle, U32 start_vert, U32 num_verts, const U16 * indices, U32 num_indices, U32 flags ) 
{
	rp_rd_state.dip_vb_calls++;

	rp_rd_state.dip_vb_prims +=	(num_indices - primitive_type_info[type].vert_fixed_cost) / 
								    primitive_type_info[type].vert_per_prim_cost;

	if( rp_rd_state.trace_enabled & IRD_ST_PRIMITIVE ) {

		U32 timestamp = rp_rd_trace_time();

		char vbname[MAX_PATH];
		char ibname[MAX_PATH];

		sprintf( vbname, "%s\\vb%08X.vbd", rp_rd_state.trace_output_dir, timestamp );
	//	rp_rd_save_vertex_data( vbname, vertex_format, verts, num_verts );

		sprintf( ibname, "%s\\ib%08X.ibd", rp_rd_state.trace_output_dir, timestamp );
		rp_rd_save_index_data( ibname, indices, num_indices );

		rp_rd_add_trace_message( "indexed%svb( vb%08X, %d, %d, ib%08X, %d, %d );", primitive_types[type], vbname, start_vert, num_verts, ibname, num_indices, flags );
		rp_rd_add_trace_message( "" );

	}

	if( rp_rd_state.dp_delay_ms > 0.0f ) {
		Sleep( rp_rd_state.dp_delay_ms );
	}
}

//

#endif // RP_RD_DEBUG


// rp_rd_ddmessage
//
//
//
const char *rp_rd_ddmessage( HRESULT error )
{
	switch( error )  {

	case DD_OK:
		return "No error.\0";
	case DDERR_ALREADYINITIALIZED:
		return "This object is already initialized.\0";
	case DDERR_BLTFASTCANTCLIP:
		return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
	case DDERR_CANNOTATTACHSURFACE:
		return "This surface can not be attached to the requested surface.\0";
	case DDERR_CANNOTDETACHSURFACE:
		return "This surface can not be detached from the requested surface.\0";
	case DDERR_CANTCREATEDC:
		return "Windows can not create any more DCs.\0";
	case DDERR_CANTDUPLICATE:
		return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
	case DDERR_CLIPPERISUSINGHWND:
		return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
	case DDERR_COLORKEYNOTSET:
		return "No src color key specified for this operation.\0";
	case DDERR_CURRENTLYNOTAVAIL:
		return "Support is currently not available.\0";
	case DDERR_DIRECTDRAWALREADYCREATED:
		return "A DirectDraw object representing this driver has already been created for this process.\0";
	case DDERR_EXCEPTION:
		return "An exception was encountered while performing the requested operation.\0";
	case DDERR_EXCLUSIVEMODEALREADYSET:
		return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
	case DDERR_GENERIC:
		return "Generic failure.\0";
	case DDERR_HEIGHTALIGN:
		return "Height of rectangle provided is not a multiple of reqd alignment.\0";
	case DDERR_HWNDALREADYSET:
		return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
	case DDERR_HWNDSUBCLASSED:
		return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
	case DDERR_IMPLICITLYCREATED:
		return "This surface can not be restored because it is an implicitly created surface.\0";
	case DDERR_INCOMPATIBLEPRIMARY:
		return "Unable to match primary surface creation request with existing primary surface.\0";
	case DDERR_INVALIDCAPS:
		return "One or more of the caps bits passed to the callback are incorrect.\0";
	case DDERR_INVALIDCLIPLIST:
		return "DirectDraw does not support the provided cliplist.\0";
	case DDERR_INVALIDDIRECTDRAWGUID:
		return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
	case DDERR_INVALIDMODE:
		return "DirectDraw does not support the requested mode.\0";
	case DDERR_INVALIDOBJECT:
		return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
	case DDERR_INVALIDPARAMS:
		return "One or more of the parameters passed to the function are incorrect.\0";
	case DDERR_INVALIDPIXELFORMAT:
		return "The pixel format was invalid as specified.\0";
	case DDERR_INVALIDPOSITION:
		return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
	case DDERR_INVALIDRECT:
		return "Rectangle provided was invalid.\0";
	case DDERR_LOCKEDSURFACES:
		return "Operation could not be carried out because one or more surfaces are locked.\0";
	case DDERR_NO3D:
		return "There is no 3D present.\0";
	case DDERR_NOALPHAHW:
		return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
	case DDERR_NOBLTHW:
		return "No blitter hardware present.\0";
	case DDERR_NOCLIPLIST:
		return "No cliplist available.\0";
	case DDERR_NOCLIPPERATTACHED:
		return "No clipper object attached to surface object.\0";
	case DDERR_NOCOLORCONVHW:
		return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
	case DDERR_NOCOLORKEY:
		return "Surface doesn't currently have a color key\0";
	case DDERR_NOCOLORKEYHW:
		return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
	case DDERR_NOCOOPERATIVELEVELSET:
		return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
	case DDERR_NODC:
		return "No DC was ever created for this surface.\0";
	case DDERR_NODDROPSHW:
		return "No DirectDraw ROP hardware.\0";
	case DDERR_NODIRECTDRAWHW:
		return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
	case DDERR_NOEMULATION:
		return "Software emulation not available.\0";
	case DDERR_NOEXCLUSIVEMODE:
		return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
	case DDERR_NOFLIPHW:
		return "Flipping visible surfaces is not supported.\0";
	case DDERR_NOGDI:
		return "There is no GDI present.\0";
	case DDERR_NOHWND:
		return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
	case DDERR_NOMIPMAPHW:
		return "Hardware doesn't support mipmapping.\0";
	case DDERR_NOMIRRORHW:
		return "Operation could not be carried out because there is no hardware present or available.\0";
	case DDERR_NOOVERLAYDEST:
		return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
	case DDERR_NOOVERLAYHW:
		return "Operation could not be carried out because there is no overlay hardware present or available.\0";
	case DDERR_NOPALETTEATTACHED:
		return "No palette object attached to this surface.\0";
	case DDERR_NOPALETTEHW:
		return "No hardware support for 16 or 256 color palettes.\0";
	case DDERR_NORASTEROPHW:
		return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
	case DDERR_NOROTATIONHW:
		return "Operation could not be carried out because there is no rotation hardware present or available.\0";
	case DDERR_NOSTRETCHHW:
		return "Operation could not be carried out because there is no hardware support for stretching.\0";
	case DDERR_NOT4BITCOLOR:
		return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
	case DDERR_NOT4BITCOLORINDEX:
		return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
	case DDERR_NOT8BITCOLOR:
		return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
	case DDERR_NOTAOVERLAYSURFACE:
		return "Returned when an overlay member is called for a non-overlay surface.\0";
	case DDERR_NOTEXTUREHW:
		return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
	case DDERR_NOTFLIPPABLE:
		return "An attempt has been made to flip a surface that is not flippable.\0";
	case DDERR_NOTFOUND:
		return "Requested item was not found.\0";
	case DDERR_NOTLOCKED:
		return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
	case DDERR_NOTPALETTIZED:
		return "The surface being used is not a palette-based surface.\0";
	case DDERR_NOVSYNCHW:
		return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
	case DDERR_NOZBUFFERHW:
		return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
	case DDERR_NOZOVERLAYHW:
		return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
	case DDERR_OUTOFCAPS:
		return "The hardware needed for the requested operation has already been allocated.\0";
	case DDERR_OUTOFMEMORY:
		return "DirectDraw does not have enough memory to perform the operation.\0";
	case DDERR_OUTOFVIDEOMEMORY:
		return "DirectDraw does not have enough memory to perform the operation.\0";
	case DDERR_OVERLAYCANTCLIP:
		return "The hardware does not support clipped overlays.\0";
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
		return "Can only have ony color key active at one time for overlays.\0";
	case DDERR_OVERLAYNOTVISIBLE:
		return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
	case DDERR_PALETTEBUSY:
		return "Access to this palette is being refused because the palette is already locked by another thread.\0";
	case DDERR_PRIMARYSURFACEALREADYEXISTS:
		return "This process already has created a primary surface.\0";
	case DDERR_REGIONTOOSMALL:
		return "Region passed to Clipper::GetClipList is too small.\0";
	case DDERR_SURFACEALREADYATTACHED:
		return "This surface is already attached to the surface it is being attached to.\0";
	case DDERR_SURFACEALREADYDEPENDENT:
		return "This surface is already a dependency of the surface it is being made a dependency of.\0";
	case DDERR_SURFACEBUSY:
		return "Access to this surface is being refused because the surface is already locked by another thread.\0";
	case DDERR_SURFACEISOBSCURED:
		return "Access to surface refused because the surface is obscured.\0";
	case DDERR_SURFACELOST:
		return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
	case DDERR_SURFACENOTATTACHED:
		return "The requested surface is not attached.\0";
	case DDERR_TOOBIGHEIGHT:
		return "Height requested by DirectDraw is too large.\0";
	case DDERR_TOOBIGSIZE:
		return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
	case DDERR_TOOBIGWIDTH:
		return "Width requested by DirectDraw is too large.\0";
	case DDERR_UNSUPPORTED:
		return "Action not supported.\0";
	case DDERR_UNSUPPORTEDFORMAT:
		return "FOURCC format requested is unsupported by DirectDraw.\0";
	case DDERR_UNSUPPORTEDMASK:
		return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
	case DDERR_VERTICALBLANKINPROGRESS:
		return "Vertical blank is in progress.\0";
	case DDERR_WASSTILLDRAWING:
		return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
	case DDERR_WRONGMODE:
		return "This surface can not be restored because it was created in a different mode.\0";
	case DDERR_XALIGN:
		return "Rectangle provided was not horizontally aligned on required boundary.\0";
	/*case D3DERR_BADMAJORVERSION:
		return "D3DERR_BADMAJORVERSION\0";
	case D3DERR_BADMINORVERSION:
		return "D3DERR_BADMINORVERSION\0";
	case D3DERR_EXECUTE_LOCKED:
		return "D3DERR_EXECUTE_LOCKED\0";
	case D3DERR_EXECUTE_NOT_LOCKED:
		return "D3DERR_EXECUTE_NOT_LOCKED\0";
	case D3DERR_EXECUTE_CREATE_FAILED:
		return "D3DERR_EXECUTE_CREATE_FAILED\0";
	case D3DERR_EXECUTE_DESTROY_FAILED:
		return "D3DERR_EXECUTE_DESTROY_FAILED\0";
	case D3DERR_EXECUTE_LOCK_FAILED:
		return "D3DERR_EXECUTE_LOCK_FAILED\0";
	case D3DERR_EXECUTE_UNLOCK_FAILED:
		return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
	case D3DERR_EXECUTE_FAILED:
		return "D3DERR_EXECUTE_FAILED\0";
	case D3DERR_EXECUTE_CLIPPED_FAILED:
		return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
	case D3DERR_TEXTURE_NO_SUPPORT:
		return "D3DERR_TEXTURE_NO_SUPPORT\0";
	case D3DERR_TEXTURE_NOT_LOCKED:
		return "D3DERR_TEXTURE_NOT_LOCKED\0";
	case D3DERR_TEXTURE_LOCKED:
		return "D3DERR_TEXTURELOCKED\0";
	case D3DERR_TEXTURE_CREATE_FAILED:
		return "D3DERR_TEXTURE_CREATE_FAILED\0";
	case D3DERR_TEXTURE_DESTROY_FAILED:
		return "D3DERR_TEXTURE_DESTROY_FAILED\0";
	case D3DERR_TEXTURE_LOCK_FAILED:
		return "D3DERR_TEXTURE_LOCK_FAILED\0";
	case D3DERR_TEXTURE_UNLOCK_FAILED:
		return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
	case D3DERR_TEXTURE_LOAD_FAILED:
		return "D3DERR_TEXTURE_LOAD_FAILED\0";
	case D3DERR_MATRIX_CREATE_FAILED:
		return "D3DERR_MATRIX_CREATE_FAILED\0";
	case D3DERR_MATRIX_DESTROY_FAILED:
		return "D3DERR_MATRIX_DESTROY_FAILED\0";
	case D3DERR_MATRIX_SETDATA_FAILED:
		return "D3DERR_MATRIX_SETDATA_FAILED\0";
	case D3DERR_SETVIEWPORTDATA_FAILED:
		return "D3DERR_SETVIEWPORTDATA_FAILED\0";
	case D3DERR_MATERIAL_CREATE_FAILED:
		return "D3DERR_MATERIAL_CREATE_FAILED\0";
	case D3DERR_MATERIAL_DESTROY_FAILED:
		return "D3DERR_MATERIAL_DESTROY_FAILED\0";
	case D3DERR_MATERIAL_SETDATA_FAILED:
		return "D3DERR_MATERIAL_SETDATA_FAILED\0";
	case D3DERR_LIGHT_SET_FAILED:
		return "D3DERR_LIGHT_SET_FAILED\0";
	case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY:
		return "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY\0";*/

	default:
		{
			static char buff[128+1];
			wsprintf( buff, "Unrecognized DirectX error=%X\0", error );
			return buff;
		}
     }
}

// EOF
