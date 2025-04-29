/* ---------- headers */

#include "Pixel.h"

#include <Core.h>
#include <FDump.h>

/* ---------- constants */

/* ---------- definitions */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

PixelFormat::PixelFormat()
{
	memset(&ddpf, 0, sizeof(ddpf));
	ddpf.dwSize = sizeof(ddpf);
	rr = rl = rwidth = 0;
	gr = gl = gwidth = 0;
	br = bl = bwidth = 0;
	ar = al = awidth = 0;
}

PixelFormat::PixelFormat(U32 bpp, U32 rbits, U32 gbits, U32 bbits, U32 abits)
{
	init(bpp, rbits, gbits, bbits, abits);
}

PixelFormat::PixelFormat(DDPIXELFORMAT _ddpf)
{
	init(_ddpf);
}

PixelFormat::PixelFormat(PFenum pd)
{
	init(pd);
}

PixelFormat::PixelFormat(PixelFormatFourCC fourcc)
{
	init(fourcc);
}

void PixelFormat::init(U32 bpp, U32 rbits, U32 gbits, U32 bbits, U32 abits)
{
	DDPIXELFORMAT _ddpf;

	memset(&_ddpf, 0, sizeof(_ddpf));
	_ddpf.dwSize = sizeof(_ddpf);

	if (bpp == 8) {
		U32 total = rbits + gbits + bbits + abits;
		if (total == 0 || total == 24 || total == 32) {
			_ddpf.dwFlags = DDPF_PALETTEINDEXED8;
			_ddpf.dwRGBBitCount = 8;
		}
		else {
			_ddpf.dwFlags = DDPF_ALPHA;
			_ddpf.dwRGBBitCount = bpp;
			_ddpf.dwRBitMask = ((1 << rbits) - 1);
			_ddpf.dwGBitMask = 0;
			_ddpf.dwBBitMask = 0;

			if (abits) {
				_ddpf.dwFlags &= ~(DDPF_ALPHA);
				_ddpf.dwFlags |= DDPF_LUMINANCE | DDPF_ALPHAPIXELS;
				_ddpf.dwRGBAlphaBitMask = ((1 << abits) - 1) << (rbits);
			}
		}
	}
	else {
		_ddpf.dwFlags = DDPF_RGB;
		_ddpf.dwRGBBitCount = bpp;
		_ddpf.dwRBitMask = ((1 << rbits) - 1) << (gbits + bbits);
		_ddpf.dwGBitMask = ((1 << gbits) - 1) << (bbits);
		_ddpf.dwBBitMask = ((1 << bbits) - 1);

		if (abits) {
			_ddpf.dwFlags |= DDPF_ALPHAPIXELS;
			_ddpf.dwRGBAlphaBitMask = ((1 << abits) - 1) << (rbits + gbits + bbits);
		}
	}

	init(_ddpf);
}

void PixelFormat::init(DDPIXELFORMAT format)
{
	ddpf = format;
	if (!is_indexed())
	{
		U32 rmask = format.dwRBitMask;
		U32 gmask = format.dwGBitMask;
		U32 bmask = format.dwBBitMask;
		U32 amask = format.dwRGBAlphaBitMask;
		int i;
		for (i = 31; i >= 0; i--)
		{
			if (rmask & (1 << i))
			{
				rl = i;
			}
			if (gmask & (1 << i))
			{
				gl = i;
			}
			if (bmask & (1 << i))
			{
				bl = i;
			}
			if (amask & (1 << i))
			{
				al = i;
			}
		}

		for (i = 0; i <= 31; i++)
		{
			if (rmask & (1 << i))
			{
				rwidth = i - rl + 1;
			}

			if (gmask & (1 << i))
			{
				gwidth = i - gl + 1;
			}

			if (bmask & (1 << i))
			{
				bwidth = i - bl + 1;
			}

			if (amask & (1 << i))
			{
				awidth = i - al + 1;
			}
		}

		rr = 8 - rwidth;
		gr = 8 - gwidth;
		br = 8 - bwidth;
		ar = 8 - awidth;

		if (!rmask) {
			rl = rr = rwidth = 0;
		}
		if (!gmask) {
			gl = gr = gwidth = 0;
		}
		if (!bmask) {
			bl = br = bwidth = 0;
		}
		if (!has_alpha_channel())
		{
			al = ar = awidth = 0;
		}
	}
}

void PixelFormat::init(PixelFormat& pf)
{
	memcpy(&ddpf, &pf.ddpf, sizeof(ddpf));

	rr = pf.rr;
	rl = pf.rl;
	rwidth = pf.rwidth;

	gr = pf.gr;
	gl = pf.gl;
	gwidth = pf.gwidth;

	br = pf.br;
	bl = pf.bl;
	bwidth = pf.bwidth;

	ar = pf.ar;
	al = pf.al;
	awidth = pf.awidth;
}

void PixelFormat::init(U32 raw)
{
	if (raw < PF_MAX_VALUE)
	{
		PFenum pf = static_cast<PFenum>(raw);
		init(pf);
	}
	else
	{
		switch (raw)
		{
		case PF_4CC_DXT1:
		case PF_4CC_DXT2:
		case PF_4CC_DXT3:
		case PF_4CC_DXT4:
		case PF_4CC_DXT5:
		case PF_4CC_DAOP:
		case PF_4CC_DAOT:
		case PF_4CC_DAAA:
		case PF_4CC_DAAL:
		case PF_4CC_DAA1:
		case PF_4CC_DAA4:
		case PF_4CC_DAA8:
		{
			PixelFormatFourCC fourcc = static_cast<PixelFormatFourCC>(raw);
			init(fourcc);
		}
		break;
		default:
			UNREACHABLE;
		}
	}
}

void PixelFormat::init(PFenum pd)
{
	DDPIXELFORMAT ddpf =
	{
		.dwSize = sizeof(ddpf)
	};

	switch (pd)
	{
	case PF_NEW_P8:
	{
		ddpf.dwFlags = DDPF_PALETTEINDEXED8;
		ddpf.dwRGBBitCount = 8;
		init(ddpf);
	}
	break;
	case PF_NEW_R8G8B8:
	{
		ddpf.dwFlags = DDPF_RGB;
		ddpf.dwRGBBitCount = 24;
		ddpf.dwRBitMask = 0x000000FF;
		ddpf.dwGBitMask = 0x0000FF00;
		ddpf.dwBBitMask = 0x00FF0000;
		ddpf.dwRGBAlphaBitMask = 0x00000000;
		init(ddpf);
	}
	break;
	case PF_NEW_X8R8G8B8:
	{
		ddpf.dwFlags = DDPF_RGB;
		ddpf.dwRGBBitCount = 32;
		ddpf.dwRBitMask = 0x000000FF;
		ddpf.dwGBitMask = 0x0000FF00;
		ddpf.dwBBitMask = 0x00FF0000;
		ddpf.dwRGBAlphaBitMask = 0x00000000;
		init(ddpf);
	}
	break;
	case PF_NEW_A4R4G4B4:
	{
		ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		ddpf.dwRGBBitCount = 16;
		ddpf.dwRBitMask = 0x0000000F;
		ddpf.dwGBitMask = 0x000000F0;
		ddpf.dwBBitMask = 0x00000F00;
		ddpf.dwRGBAlphaBitMask = 0x0000F000;
		init(ddpf);
	}
	break;
	case PF_NEW_A8R8G8B8:
	{
		ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		ddpf.dwRGBBitCount = 32;
		ddpf.dwRBitMask = 0x000000FF;
		ddpf.dwGBitMask = 0x0000FF00;
		ddpf.dwBBitMask = 0x00FF0000;
		ddpf.dwRGBAlphaBitMask = 0xFF000000;
		init(ddpf);
	}
	break;
	case PF_NEW_R5G6B5:
	{
		ddpf.dwFlags = DDPF_RGB;
		ddpf.dwRGBBitCount = 16;
		ddpf.dwRBitMask = 0x0000F800;
		ddpf.dwGBitMask = 0x000007E0;
		ddpf.dwBBitMask = 0x0000001F;
		ddpf.dwRGBAlphaBitMask = 0x00000000;
		init(ddpf);
	}
	break;
	case PF_NEW_X1R5G5B5:
	{
		ddpf.dwFlags = DDPF_RGB;
		ddpf.dwRGBBitCount = 16;
		ddpf.dwBBitMask = 0x00007C00;
		ddpf.dwGBitMask = 0x000003E0;
		ddpf.dwRBitMask = 0x0000001F;
		ddpf.dwRGBAlphaBitMask = 0x00000000;
		init(ddpf);
	}
	break;
	case PF_NEW_A1R5G5B5:
	{
		ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		ddpf.dwRGBBitCount = 16;
		ddpf.dwBBitMask = 0x00007C00;
		ddpf.dwGBitMask = 0x000003E0;
		ddpf.dwRBitMask = 0x0000001F;
		ddpf.dwRGBAlphaBitMask = 0x00008000;
		init(ddpf);
	}
	break;
	case PF_NEW_DXT1:
	{
		init(PF_4CC_DXT1);
	}
	break;
	case PF_NEW_DXT2:
	{
		init(PF_4CC_DXT2);
	}
	break;
	case PF_NEW_DXT3:
	{
		init(PF_4CC_DXT3);
	}
	break;
	case PF_NEW_DXT4:
	{
		init(PF_4CC_DXT4);
	}
	break;
	case PF_NEW_DXT5:
	{
		init(PF_4CC_DXT5);
	}
	break;
	case PF_NEW_DAOP:
	{
		init(PF_4CC_DAOP);
	}
	break;
	case PF_NEW_DAOT:
	{
		init(PF_4CC_DAOT);
	}
	break;
	case PF_NEW_DAAA:
	{
		init(PF_4CC_DAAA);
	}
	break;
	case PF_NEW_DAAL:
	{
		init(PF_4CC_DAAL);
	}
	break;
	case PF_NEW_DAA1:
	{
		init(PF_4CC_DAA1);
	}
	break;
	case PF_NEW_DAA4:
	{
		init(PF_4CC_DAA4);
	}
	break;
	case PF_NEW_DAA8:
	{
		init(PF_4CC_DAA8);
	}
	break;
	default:
	{
		UNREACHABLE;
	}
	break;
	}

}

void PixelFormat::init(PixelFormatFourCC fourcc)
{
	ddpf =
	{
		.dwFlags = DDPF_FOURCC,
		.dwFourCC = static_cast<DWORD>(fourcc),
	};
}

U32 PixelFormat::compute(U8 r, U8 g, U8 b, U8 a) const
{
	U32 result;
	if (is_indexed())
	{
		result = 0xffffffff;
	}
	else
	{
		result = (((r >> rr) << rl) |
			((g >> gr) << gl) |
			((b >> br) << bl));
		if (awidth)
		{
			result |= ((a >> ar) << al);
		}
	}
	return result;
}

bool PixelFormat::is_indexed(void) const
{
	return ((ddpf.dwFlags & DDPF_PALETTEINDEXED8) != 0);
}

bool PixelFormat::has_alpha_channel(void) const
{
	return ((ddpf.dwFlags & DDPF_ALPHAPIXELS) != 0);
}

U32 PixelFormat::get_r_mask(void) const
{
	return ddpf.dwRBitMask;
}

U32 PixelFormat::get_g_mask(void) const
{
	return ddpf.dwGBitMask;
}

U32 PixelFormat::get_b_mask(void) const
{
	return ddpf.dwBBitMask;
}

U32 PixelFormat::get_a_mask(void) const
{
	return ddpf.dwRGBAlphaBitMask;
}

U32 PixelFormat::num_r_bits(void) const
{
	return rwidth;
}

U32 PixelFormat::num_g_bits(void) const
{
	return gwidth;
}

U32 PixelFormat::num_b_bits(void) const
{
	return bwidth;
}

U32 PixelFormat::num_a_bits(void) const
{
	return awidth;
}

U32 PixelFormat::is_equal(const PixelFormat& pf) const
{
	if (pf.is_indexed()) {
		return is_indexed();
	}
	else if (!is_indexed()) {

		if ((num_r_bits() == pf.num_r_bits()) &&
			(get_r_mask() == pf.get_r_mask()) &&
			(num_g_bits() == pf.num_g_bits()) &&
			(get_g_mask() == pf.get_g_mask()) &&
			(num_b_bits() == pf.num_b_bits()) &&
			(get_b_mask() == pf.get_b_mask()) &&
			(num_a_bits() == pf.num_a_bits()) &&
			(get_a_mask() == pf.get_a_mask()) &&
			ddpf.dwBumpDuBitMask == pf.ddpf.dwBumpDuBitMask &&
			ddpf.dwBumpDvBitMask == pf.ddpf.dwBumpDvBitMask &&
			ddpf.dwBumpLuminanceBitMask == pf.ddpf.dwBumpLuminanceBitMask) {
			return 1;

		}
	}

	return 0;
}

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
//
U32 PixelFormat::is_compatible(const PixelFormat& pf) const
{
	if (is_indexed() && pf.is_indexed()) {
		// both indexed
		return 1;
	}
	else if ((!is_indexed() && pf.is_indexed())) { //&& !has_alpha_channel()) ) {
		// this is rgb, that is indexed
		return 1;
	}
	else {
#if 0
		// both are rgb(a), this must have at least as many bits as that.
		// if that has alpha, this must have it too and at least as many
		// bits.
		yup = (num_r_bits() >= pf.num_r_bits()) &&
			(num_g_bits() >= pf.num_g_bits()) &&
			(num_b_bits() >= pf.num_b_bits());

		if (yup && pf.has_alpha_channel()) {
			return yup && (num_a_bits() >= pf.num_a_bits());
		}
#endif
		return (has_alpha_channel() == pf.has_alpha_channel());
	}

	return 0;
}

U32 PixelFormat::extract(char* src, U8 r, U8 g, U8 b, U8 a) const
{
	unused(src); // bonk the 'src' : unreferenced formal parameter

	U32 result;
	if (is_indexed())
	{
		result = 0xffffffff;
	}
	else
	{
		result = (((r >> rr) << rl) |
			((g >> gr) << gl) |
			((b >> br) << bl));
		if (awidth)
		{
			result |= ((a >> ar) << al);
		}
	}
	return result;
}

U32 PixelFormat::num_bits(void) const
{
	return ddpf.dwRGBBitCount;
}

void PixelFormat::persist(char* out)
{
	if (is_indexed()) {
		strcpy(out, "Format_PAL8_3__8_8_8");
	}
	else {
		char sz[25 + 1];
		int num_comps = 0;

		if (get_r_mask() != 0) {
			num_comps++;
		}
		if (get_g_mask() != 0) {
			num_comps++;
		}
		if (get_b_mask() != 0) {
			num_comps++;
		}
		if (get_a_mask() != 0) {
			num_comps++;
		}

		wsprintf(out, "Format_TRUE_%d_", num_comps);

		if (get_r_mask() != 0) {
			wsprintf(sz, "_%d", num_r_bits());
			strcat(out, sz);
		}
		if (get_g_mask() != 0) {
			wsprintf(sz, "_%d", num_g_bits());
			strcat(out, sz);
		}
		if (get_b_mask() != 0) {
			wsprintf(sz, "_%d", num_b_bits());
			strcat(out, sz);
		}
		if (get_a_mask() != 0) {
			wsprintf(sz, "_%d", num_a_bits());
			strcat(out, sz);
		}
	}
}

void PixelFormat::unpersist(char* in_src)
{

	if (strncmp(in_src, "Format_PAL8", strlen("Format_PAL8")) == 0) {
		init(8, 8, 8, 8, 0);
	}
	else {
		char in[255 + 1];
		strcpy(in, in_src);
		char* p = &in[12];	// Eat up "Format_TRUE_"
		char* t;

		t = p;
		while (*p && *p != '_') p++;
		if (*p) {
			*p = 0;
			p++;
		}

		int num_comps = atoi(t);
		int sizes[4] = { 0,0,0,0 };

		p++; // eat extra leading '_'
		for (int n = 0; n < num_comps; n++) {
			t = p;
			while (*p && *p != '_') p++;
			if (*p) {
				*p = 0;
				p++;
			}
			else if (n < num_comps - 1) {
				break;	// break out of the for loop
			}

			sizes[n] = atoi(t);
		}

		if (num_comps == 2) {
			sizes[3] = sizes[1];
			sizes[1] = 0;
		}

		U32 bpp = sizes[0] + sizes[1] + sizes[2] + sizes[3];

		switch (bpp) {
		case 24:	init(PF_NEW_R8G8B8); break;
		case 32:	init(PF_NEW_A8R8G8B8); break;
		default:	init(bpp, sizes[0], sizes[1], sizes[2], sizes[3]); break;
		}
	}

}

bool PixelFormat::is_fourcc() const
{
	bool result = (ddpf.dwFlags & DDPF_FOURCC) != 0;
	return result;
}

PixelFormatFourCC PixelFormat::get_fourcc() const
{
	ASSERT(ddpf.dwFlags & DDPF_FOURCC);
	return static_cast<PixelFormatFourCC>(ddpf.dwFourCC);
}

D3DFORMAT pf_to_d3d(PFenum pf)
{
	switch (pf)
	{
	case PF_UNKNOWN: return D3DFMT_UNKNOWN;
	case PF_NEW_P8: return D3DFMT_P8;
	case PF_NEW_R8G8B8: return D3DFMT_R8G8B8;
	case PF_NEW_R5G6B5: return D3DFMT_R5G6B5;
	case PF_NEW_X1R5G5B5: return D3DFMT_X1R5G5B5;
	case PF_NEW_A4R4G4B4: return D3DFMT_A4R4G4B4;
	case PF_NEW_A1R5G5B5: return D3DFMT_A1R5G5B5;
	case PF_NEW_A8R8G8B8: return D3DFMT_A8R8G8B8;
	case PF_NEW_X8R8G8B8: return D3DFMT_X8R8G8B8;
	case PF_NEW_MAX_UNCOMPRESSED: return D3DFMT_UNKNOWN;
	case PF_NEW_DXT1: return D3DFMT_DXT1;
	case PF_NEW_DXT2: return D3DFMT_DXT2;
	case PF_NEW_DXT3: return D3DFMT_DXT3;
	case PF_NEW_DXT4: return D3DFMT_DXT4;
	case PF_NEW_DXT5: return D3DFMT_DXT5;
	case PF_NEW_DAOP: return D3DFMT_R5G6B5;
	case PF_NEW_DAOT: return D3DFMT_X8R8G8B8;
	case PF_NEW_DAAA: return D3DFMT_A8;
	case PF_NEW_DAAL: return D3DFMT_A8L8;
	case PF_NEW_DAA1: return D3DFMT_A1R5G5B5;
	case PF_NEW_DAA4: return D3DFMT_A4R4G4B4;
	case PF_NEW_DAA8: return D3DFMT_A8R8G8B8;
	default: UNREACHABLE;
	}
}

PFenum d3d_to_pf(D3DFORMAT d3d)
{
	switch (d3d)
	{
	case D3DFMT_UNKNOWN: return PF_UNKNOWN;
	case D3DFMT_P8: return PF_NEW_P8;
	case D3DFMT_R8G8B8: return PF_NEW_R8G8B8;
	case D3DFMT_R5G6B5: return PF_NEW_R5G6B5;
	case D3DFMT_X1R5G5B5: return PF_NEW_X1R5G5B5;
	case D3DFMT_A4R4G4B4: return PF_NEW_A4R4G4B4;
	case D3DFMT_A1R5G5B5: return PF_NEW_A1R5G5B5;
	case D3DFMT_A8R8G8B8: return PF_NEW_A8R8G8B8;
	case D3DFMT_X8R8G8B8: return PF_NEW_X8R8G8B8;
	case D3DFMT_DXT1: return PF_NEW_DXT1;
	case D3DFMT_DXT2: return PF_NEW_DXT2;
	case D3DFMT_DXT3: return PF_NEW_DXT3;
	case D3DFMT_DXT4: return PF_NEW_DXT4;
	case D3DFMT_DXT5: return PF_NEW_DXT5;
	default: UNREACHABLE;
	}
}
