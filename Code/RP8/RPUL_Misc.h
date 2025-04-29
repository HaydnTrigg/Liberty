#pragma once
#ifndef __RPUL_MISC_H__
#define __RPUL_MISC_H__

/* ---------- headers */

#include "RenderPipeline.h"
#include "Pixel.h"

#include <DACOM.h>

/* ---------- prototypes */

HRESULT mem_bitblt(
	void* dst_bits,
	int dst_width,
	int dst_height,
	int dst_stride,
	PixelFormat& dst_format,
	const void* src_bits,
	int src_width,
	int src_height,
	int src_stride,
	const PixelFormat& src_format,
	const RGB* src_palette,
	const void* src_alpha);

HRESULT mem_bitblt_invert(
	void* dst_bits,
	int dst_width,
	int dst_height,
	int dst_stride,
	PixelFormat& dst_format,
	const void* src_bits,
	int src_width,
	int src_height,
	int src_stride,
	const PixelFormat& src_format,
	const RGB* src_palette,
	const void* src_alpha);

#endif // __RPUL_MISC_H__
