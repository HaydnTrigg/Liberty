#pragma once
#ifndef __PIXEL_H__
#define __PIXEL_H__

/* ---------- headers */

#include "typedefs.h"
#include <ddraw.h>
#include <d3d8.h>

/* ---------- constants */

enum PixelFormatFourCC
{
	PF_4CC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'), // D3DFMT_DXT1
	PF_4CC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'), // D3DFMT_DXT2
	PF_4CC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'), // D3DFMT_DXT3
	PF_4CC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'), // D3DFMT_DXT4
	PF_4CC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'), // D3DFMT_DXT5
	PF_4CC_DAOP = MAKEFOURCC('D', 'A', 'O', 'P'), // D3DFMT_R5G6B5
	PF_4CC_DAOT = MAKEFOURCC('D', 'A', 'O', 'T'), // D3DFMT_X8R8G8B8
	PF_4CC_DAAA = MAKEFOURCC('D', 'A', 'A', 'A'), // D3DFMT_A8
	PF_4CC_DAAL = MAKEFOURCC('D', 'A', 'A', 'L'), // D3DFMT_A8L8
	PF_4CC_DAA1 = MAKEFOURCC('D', 'A', 'A', '1'), // D3DFMT_A1R5G5B5
	PF_4CC_DAA4 = MAKEFOURCC('D', 'A', 'A', '4'), // D3DFMT_A4R4G4B4
	PF_4CC_DAA8 = MAKEFOURCC('D', 'A', 'A', '8'), // D3DFMT_A8R8G8B8
};

enum PFenum
{
	PF_UNKNOWN, // D3DFMT_UNKNOWN
	PF_NEW_P8, // D3DFMT_P8
	PF_NEW_R8G8B8, // D3DFMT_R8G8B8
	PF_NEW_R5G6B5, // D3DFMT_R5G6B5
	PF_NEW_X1R5G5B5, // D3DFMT_X1R5G5B5
	PF_NEW_A4R4G4B4, // D3DFMT_A4R4G4B4
	PF_NEW_A1R5G5B5, // D3DFMT_A1R5G5B5
	PF_NEW_A8R8G8B8,  // D3DFMT_A8R8G8B8
	PF_NEW_X8R8G8B8,  // D3DFMT_X8R8G8B8
	PF_NEW_MAX_UNCOMPRESSED, // D3DFMT_UNKNOWN
	PF_NEW_DXT1, // PF_4CC_DXT1 or D3DFMT_DXT1
	PF_NEW_DXT2, // PF_4CC_DXT2 or D3DFMT_DXT2
	PF_NEW_DXT3, // PF_4CC_DXT3 or D3DFMT_DXT3
	PF_NEW_DXT4, // PF_4CC_DXT4 or D3DFMT_DXT4
	PF_NEW_DXT5, // PF_4CC_DXT5 or D3DFMT_DXT5
	PF_NEW_DAOP, // PF_4CC_DAOP
	PF_NEW_DAOT, // PF_4CC_DAOT
	PF_NEW_DAAA, // PF_4CC_DAAA
	PF_NEW_DAAL, // PF_4CC_DAAL
	PF_NEW_DAA1, // PF_4CC_DAA1
	PF_NEW_DAA4, // PF_4CC_DAA4
	PF_NEW_DAA8, // PF_4CC_DAA8
	PF_MAX_VALUE,
};

/* ---------- definitions */

struct PixelFormat
{
	DDPIXELFORMAT	ddpf;
	int				rr, rl, rwidth;
	int				gr, gl, gwidth;
	int				br, bl, bwidth;
	int				ar, al, awidth;

	PixelFormat();
	PixelFormat(U32 bpp, U32 rbits, U32 gbits, U32 bbits, U32 abits);
	PixelFormat(DDPIXELFORMAT _ddpf);
	PixelFormat(PFenum pd);
	PixelFormat(PixelFormatFourCC fourcc);

	void init(U32 bpp, U32 rbits, U32 gbits, U32 bbits, U32 abits);
	void init(DDPIXELFORMAT format);
	void init(PixelFormat& pf);
	void init(U32 raw);
	void init(PFenum pd);
	void init(PixelFormatFourCC fourcc);

	U32 compute(U8 r, U8 g, U8 b, U8 a = 0) const;
	bool is_indexed(void) const;
	bool has_alpha_channel(void) const;
	U32 get_r_mask(void) const;
	U32 get_g_mask(void) const;
	U32 get_b_mask(void) const;
	U32 get_a_mask(void) const;
	U32 num_r_bits(void) const;
	U32 num_g_bits(void) const;
	U32 num_b_bits(void) const;
	U32 num_a_bits(void) const;
	bool is_fourcc() const;
	PixelFormatFourCC get_fourcc() const;

	// determine if the given pixelformat is the same as this one.
	U32 is_equal(const PixelFormat& pf) const;

	// determine if this pixelformat is compatible with
	// the given one.  This pixelformat is 'compatible' 
	// iff this pixel format can describe a surface that
	// can contain pixeldata in the given format without
	// loss of data.
	// i.e. An RGB pixelformat is compatible with an indexed 
	// format because an RGB surface can hold indexed data
	// without loss of data.  However, an indexed format
	// is not compatible with an RGB format because an
	// indexed surface cannot hold RGB data without quantization
	U32 is_compatible(const PixelFormat& pf) const;
	U32 extract(char* src, U8 r, U8 g, U8 b, U8 a = 0) const;
	U32 num_bits(void) const;
	void persist(char* out);
	void unpersist(char* in_src);
};

/* ---------- prototypes */

extern D3DFORMAT pf_to_d3d(PFenum pf);
extern PFenum d3d_to_pf(D3DFORMAT d3d);

/* ---------- globals */

/* ---------- inline code */

#endif // __PIXEL_H__
