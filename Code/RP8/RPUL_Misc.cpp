/* ---------- headers */

#include "RPUL_Misc.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string.h>
#include <stdio.h>
#include <FDump.h>

/* ---------- prototypes */

static void rp_dd_surface_put_pixel(U8* dst_bits, PixelFormat& dst_format, U8 r, U8 g, U8 b, U8 a);

/* ---------- public code */

HRESULT mem_bitblt(	void *dst_bits,
						int dst_width,
						int dst_height,
						int dst_stride,
						PixelFormat & dst_format,
						const void *src_bits,
						int src_width,
						int src_height,
						int src_stride,
						const PixelFormat & src_format_in,
						const RGB *src_palette,
						const U8 *src_alpha )
{
	U32			 dst_bd = ((dst_format.ddpf.dwRGBBitCount+7)>>3);	// dst byte depth
	U8			*dst_bits_u8 = (U8*)dst_bits;
	U32			 dst_past_stride = dst_stride - dst_width * dst_bd;  // dst_past_stride is the number of bytes *beyond* the width of the dst surface

	PixelFormat  src_format( src_format_in );
	U8			*src_bits_u8 = (U8*)src_bits;
	U8			*src_bits_line = NULL;
	U32			 src_alpha_stride;
	const U8	*src_alpha_map = NULL;
	U32			 src_bd = ((src_format.ddpf.dwRGBBitCount+7)>>3);		// src byte depth
	U32			 src_pxl_u32;
	U8			*src_bits_u8_p;

	U8 r,g,b,a = 0;

	const int max_alpha_width = 2048;
	static bool init_default_alpha_map = false;
	static U8 default_alpha_map[max_alpha_width];

	U32 r_mask = src_format.get_r_mask();
	U32 g_mask = src_format.get_g_mask();
	U32 b_mask = src_format.get_b_mask();
	U32 a_mask = src_format.get_a_mask();

	// Fixup 8bit true textures to pull r g & b out of r
	//
	if( src_format.num_r_bits() == 8 && src_format.num_g_bits() == 0 && src_format.num_b_bits() == 0 && !src_format.is_indexed() ) {
		g_mask			= b_mask		= r_mask;
		src_format.gl	= src_format.bl = src_format.rl;
		src_format.gr	= src_format.br = src_format.rr;
	}
	
	// Set up source alpha map
	//
	if( src_alpha ) {
		src_alpha_map = src_alpha;
		src_alpha_stride = src_width;
	}
	else {
		if( a_mask == 0 && init_default_alpha_map == false ) {
			ASSERT( src_width < max_alpha_width );
			init_default_alpha_map = true; 
			memset( default_alpha_map, 0xFF, max_alpha_width );
		}
		src_alpha_map = default_alpha_map;
		src_alpha_stride = 0;
	}


	if( dst_width == src_width && dst_height == src_height ) {
		
		if( dst_format.is_equal( src_format ) && dst_format.num_bits() == src_format.num_bits() ) {
			if( dst_stride == src_stride ) {
				// Do block copy
				//
				memcpy( dst_bits_u8, src_bits_u8, src_stride*src_height );
			}
			else {
				// Do block copy, mind the pitch
				//
				for( int y=0; y<dst_height; y++ ) {
					memcpy( dst_bits_u8, src_bits_u8, src_width*src_bd );
					src_bits_u8 += src_stride;
					dst_bits_u8 += dst_stride;
				}
			}
		}
		else {
			// Do copy with format conversion
			//

			// change src_stride to be the number of bytes 
			// *beyond* the width of the src surface
			//
            src_stride -= src_width * src_bd;						

			if( src_format.is_indexed() ) {
				// Indexed source to RGBA 8/16/24/32 bit dest.
				//
				for( int y=0; y < dst_height; y++ ) {
										
					for( int x=0; x < dst_width; x++ ) {
						r = src_palette[*src_bits_u8].r;
						g = src_palette[*src_bits_u8].g;
						b = src_palette[*src_bits_u8].b;
						a = src_alpha_map[x];
						src_bits_u8++;	

						rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
						dst_bits_u8 += dst_bd;					
					}

					src_alpha_map += src_alpha_stride;
					src_bits_u8 += src_stride;
					dst_bits_u8 += dst_past_stride;
				}
			}
			else {
				// 8/16/24/32 bit source to 8/16/24/32 bit dest
				//
				for(int y=0;y < dst_height;y++)
				{
					for(int x=0;x < dst_width;x++)
					{
						a			=src_alpha_map[x];
						src_pxl_u32	=*((U32*)src_bits_u8);

						r	=((src_pxl_u32 & r_mask) >> src_format.rl);
						r	=(r << src_format.rr) | (r & ((1<<src_format.rr)-1));
						g	=((src_pxl_u32 & g_mask) >> src_format.gl);
						g	=(g << src_format.gr) | (g & ((1<<src_format.gr)-1));
						b	=((src_pxl_u32 & b_mask) >> src_format.bl);
						b	=(b << src_format.br) | (b & ((1<<src_format.br)-1));

						if(a_mask)
						{
							a	=((src_pxl_u32 & a_mask) >> src_format.al);
							a	=(a << src_format.ar) | (a & ((1<<src_format.ar)-1));
						}
						
						src_bits_u8	+=src_bd;
						
						rp_dd_surface_put_pixel(dst_bits_u8, dst_format, r, g, b, a);
						dst_bits_u8	+=dst_bd;
					}

					src_alpha_map	+=src_alpha_stride;
					src_bits_u8		+=src_stride;
					dst_bits_u8		+=dst_past_stride;
				}
			}
		}

	}
	else {	

		// Do stretch/shrink with format conversion
		//

		U32  u = 0, u32;
		U32  v = 0, v32;
		U32 du = (U32)( (float(src_width)  / float(dst_width)) * float(1<<16) );
		U32 dv = (U32)( (float(src_height)  / float(dst_height)) * float(1<<16) );

		if( src_format.is_indexed() ) {
			// Indexed source to RGBA 8/16/24/32 bit dest.
			//
			for( int y=0; y < dst_height; y++, v+=dv ) {

				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x < dst_width; x++, u+=du ) {

					u32 = (u>>16);
					src_bits_u8_p = &src_bits_line[ src_bd * u32 ];

					r = src_palette[*src_bits_u8_p].r;
					g = src_palette[*src_bits_u8_p].g;
					b = src_palette[*src_bits_u8_p].b;
					a = src_alpha_map[x];

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map += src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			}
		}
		else {
			// RGBA 8/16/24/32 bit src to RGBA 8/16/24/32 bit dest.
			//

			for( int y=0; y<dst_height; y++, v+=dv ) {
				
				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x<dst_width; x++, u+=du ) {

					u32 = (u>>16);

					a = src_alpha_map[x];

					src_pxl_u32 = *((U32*)&src_bits_line[ src_bd * u32 ]);

					r = ((src_pxl_u32 & r_mask) >> src_format.rl);
					r = (r << src_format.rr) | (r & ((1<<src_format.rr)-1));
					g = ((src_pxl_u32 & g_mask) >> src_format.gl);
					g = (g << src_format.gr) | (g & ((1<<src_format.gr)-1));
					b = ((src_pxl_u32 & b_mask) >> src_format.bl);
					b = (b << src_format.br) | (b & ((1<<src_format.br)-1));
					if( a_mask ) {
						a = ((src_pxl_u32 & a_mask) >> src_format.al);
						a = (a << src_format.ar) | (a & ((1<<src_format.ar)-1));
					}

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map += src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			}
		}
	}	

	return S_OK;
}

HRESULT mem_bitblt_invert(	void *dst_bits,
						int dst_width,
						int dst_height,
						int dst_stride,
						PixelFormat & dst_format,
						const void *src_bits,
						int src_width,
						int src_height,
						int src_stride,
						const PixelFormat & src_format_in,
						const RGB *src_palette,
						const U8 *src_alpha )
{
	U32			 dst_bd = ((dst_format.ddpf.dwRGBBitCount+7)>>3);	// dst byte depth
	U8			*dst_bits_u8 = (U8*)dst_bits;
	U32			 dst_past_stride = dst_stride - dst_width * dst_bd;  // dst_past_stride is the number of bytes *beyond* the width of the dst surface

	PixelFormat  src_format( src_format_in );
	U8			*src_bits_u8 = (U8*)src_bits + (src_stride * (src_height-1));	//adjust to last line
	U8			*src_bits_line = NULL;
	U32			 src_alpha_stride;
	const U8	*src_alpha_map = NULL;
	U32			 src_bd = ((src_format.ddpf.dwRGBBitCount+7)>>3);		// src byte depth
	U32			 src_pxl_u32;
	U8			*src_bits_u8_p;

	U8 r,g,b,a = 0;

	const int max_alpha_width = 2048;
	static bool init_default_alpha_map = false;
	static U8 default_alpha_map[max_alpha_width];

	U32 r_mask = src_format.get_r_mask();
	U32 g_mask = src_format.get_g_mask();
	U32 b_mask = src_format.get_b_mask();
	U32 a_mask = src_format.get_a_mask();

	// Fixup 8bit true textures to pull r g & b out of r
	//
	if( src_format.num_r_bits() == 8 && src_format.num_g_bits() == 0 && src_format.num_b_bits() == 0 && !src_format.is_indexed() ) {
		g_mask			= b_mask		= r_mask;
		src_format.gl	= src_format.bl = src_format.rl;
		src_format.gr	= src_format.br = src_format.rr;
	}
	
	// Set up source alpha map
	//
	if( src_alpha ) {
		src_alpha_map = src_alpha + (src_stride * (src_height-1));	//adjust to last line
		src_alpha_stride = src_width;
	}
	else {
		if( a_mask == 0 && init_default_alpha_map == false ) {
			ASSERT( src_width < max_alpha_width );
			init_default_alpha_map = true; 
			memset( default_alpha_map, 0xFF, max_alpha_width );
		}
		src_alpha_map = default_alpha_map;
		src_alpha_stride = 0;
	}


	if(dst_width == src_width && dst_height == src_height)
	{
		if(dst_format.is_equal(src_format) && dst_format.num_bits() == src_format.num_bits())
		{
			for(int y = 0;y < dst_height;y++)
			{
				memcpy(dst_bits_u8, src_bits_u8, src_width * src_bd);
				src_bits_u8	-=src_stride;
				dst_bits_u8	+=dst_stride;
			}
		}
		else
		{
			// Do copy with format conversion
			//

			// change src_stride to be the number of bytes 
			// *beyond* the width of the src surface + original stride
			//
            src_stride += src_width * src_bd;						

			if( src_format.is_indexed() ) {
				// Indexed source to RGBA 8/16/24/32 bit dest.
				//
				for( int y=0; y < dst_height; y++ ) {
										
					for( int x=0; x < dst_width; x++ ) {
						r = src_palette[*src_bits_u8].r;
						g = src_palette[*src_bits_u8].g;
						b = src_palette[*src_bits_u8].b;
						a = src_alpha_map[x];
						src_bits_u8++;	

						rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
						dst_bits_u8 += dst_bd;					
					}

					src_alpha_map -= src_alpha_stride;
					src_bits_u8 -= src_stride;
					dst_bits_u8 += dst_past_stride;
				}
			}
			else {
				// 8/16/24/32 bit source to 8/16/24/32 bit dest
				//
				for(int y=0;y < dst_height;y++)
				{
					for(int x=0;x < dst_width;x++)
					{
						a			=src_alpha_map[x];
						src_pxl_u32	=*((U32*)src_bits_u8);

						r	=((src_pxl_u32 & r_mask) >> src_format.rl);
						r	=(r << src_format.rr) | (r & ((1<<src_format.rr)-1));
						g	=((src_pxl_u32 & g_mask) >> src_format.gl);
						g	=(g << src_format.gr) | (g & ((1<<src_format.gr)-1));
						b	=((src_pxl_u32 & b_mask) >> src_format.bl);
						b	=(b << src_format.br) | (b & ((1<<src_format.br)-1));

						if(a_mask)
						{
							a	=((src_pxl_u32 & a_mask) >> src_format.al);
							a	=(a << src_format.ar) | (a & ((1<<src_format.ar)-1));
						}
						
						src_bits_u8	+=src_bd;
						
						rp_dd_surface_put_pixel(dst_bits_u8, dst_format, r, g, b, a);
						dst_bits_u8	+=dst_bd;
					}

					src_alpha_map	-=src_alpha_stride;
					src_bits_u8		-=src_stride;
					dst_bits_u8		+=dst_past_stride;
				}
			}
		}

	}
	else {	

		// Do stretch/shrink with format conversion
		//

		U32  u = 0, u32;
		U32  v = src_height - 1, v32;
		U32 du = (U32)( (float(src_width)  / float(dst_width)) * float(1<<16) );
		U32 dv = (U32)( (float(src_height)  / float(dst_height)) * float(1<<16) );

		if( src_format.is_indexed() ) {
			// Indexed source to RGBA 8/16/24/32 bit dest.
			//
			for( int y=0; y < dst_height; y++, v+=dv ) {

				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x < dst_width; x++, u+=du ) {

					u32 = (u>>16);
					src_bits_u8_p = &src_bits_line[ src_bd * u32 ];

					r = src_palette[*src_bits_u8_p].r;
					g = src_palette[*src_bits_u8_p].g;
					b = src_palette[*src_bits_u8_p].b;
					a = src_alpha_map[x];

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map -= src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			}
		}
		else {
			// RGBA 8/16/24/32 bit src to RGBA 8/16/24/32 bit dest.

			for( int y=0; y<dst_height; y++, v+=dv ) {
				
				v32 = (v>>16);
				src_bits_line = &src_bits_u8[src_stride * v32];
				u = 0;

				for( int x=0; x<dst_width; x++, u+=du ) {

					u32 = (u>>16);

					a = src_alpha_map[x];

					src_pxl_u32 = *((U32*)&src_bits_line[ src_bd * u32 ]);

					r = ((src_pxl_u32 & r_mask) >> src_format.rl);
					r = (r << src_format.rr) | (r & ((1<<src_format.rr)-1));
					g = ((src_pxl_u32 & g_mask) >> src_format.gl);
					g = (g << src_format.gr) | (g & ((1<<src_format.gr)-1));
					b = ((src_pxl_u32 & b_mask) >> src_format.bl);
					b = (b << src_format.br) | (b & ((1<<src_format.br)-1));
					if( a_mask ) {
						a = ((src_pxl_u32 & a_mask) >> src_format.al);
						a = (a << src_format.ar) | (a & ((1<<src_format.ar)-1));
					}

					rp_dd_surface_put_pixel( dst_bits_u8, dst_format, r, g, b, a );
					dst_bits_u8 += dst_bd;					
				}

				src_alpha_map -= src_alpha_stride;
				dst_bits_u8 += dst_past_stride;
			}
		}
	}	

	return S_OK;
}

/* ---------- private code */

static void rp_dd_surface_put_pixel(U8* dst_bits, PixelFormat& dst_format, U8 r, U8 g, U8 b, U8 a)
{
#if 0
	switch (dst_format.num_bits()) {
	case 32: dst_bits[3] = ((char*)&dst_pxl)[3];
	case 24: dst_bits[2] = ((char*)&dst_pxl)[2];
	case 16: dst_bits[1] = ((char*)&dst_pxl)[1];
	case  8: dst_bits[0] = ((char*)&dst_pxl)[0];
	}
#else
	U32 dst_pxl = dst_format.compute(r, g, b, a);
	switch (dst_format.num_bits()) {

	case 32: 	dst_bits[3] = ((char*)&dst_pxl)[3];
	case 24: 	dst_bits[2] = ((char*)&dst_pxl)[2];
		dst_bits[1] = ((char*)&dst_pxl)[1];
		dst_bits[0] = ((char*)&dst_pxl)[0];
		break;

	case  8: 	dst_bits[0] = r;
		break;

	case 16:	*((U16*)dst_bits) = (U16)dst_pxl;
		break;
	}
#endif
}
