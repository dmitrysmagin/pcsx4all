/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*   Copyright (C) 2016 Senquack (dansilsby <AT> gmail <DOT> com)          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Inner loop driver instantiation file
//
// 2016 - Fixes and improvements by Senquack (dansilsby@gmail.com)

///////////////////////////////////////////////////////////////////////////////
//  Option Masks (CF template paramter)
#define  CF_LIGHT     ((CF>> 0)&1) // Lighting
#define  CF_BLEND     ((CF>> 1)&1) // Blending
#define  CF_MASKCHECK ((CF>> 2)&1) // Mask bit check
#define  CF_BLENDMODE ((CF>> 3)&3) // Blend mode   0..3
#define  CF_TEXTMODE  ((CF>> 5)&3) // Texture mode 1..3 (0: texturing disabled)
#define  CF_GOURAUD   ((CF>> 7)&1) // Gouraud shading
#define  CF_MASKSET   ((CF>> 8)&1) // Mask bit set
#define  CF_DITHER    ((CF>> 9)&1) // Dithering
#define  CF_BLITMASK  ((CF>>10)&1) // blit_mask check (skip rendering pixels
                                   //  that wouldn't end up displayed on
                                   //  low-res screen using simple downscaler)

#ifdef __arm__
#ifndef ENABLE_GPU_ARMV7
/* ARMv5 */
#include "gpu_inner_blend_arm5.h"
#else
/* ARMv7 optimized */
#include "gpu_inner_blend_arm7.h"
#endif
#else
#include "gpu_inner_blend.h"
#endif

#include "gpu_inner_quantization.h"
#include "gpu_inner_light.h"

// If defined, Gouraud colors are fixed-point 5.11, otherwise they are 8.16
// This is only for debugging/verification of low-precision colors in C.
// Low-precision Gouraud is intended for use by SIMD-optimized inner drivers
// which get/use Gouraud colors in SIMD registers.
//#define GPU_GOURAUD_LOW_PRECISION

// How many bits of fixed-point precision GouraudColor uses
#ifdef GPU_GOURAUD_LOW_PRECISION
#define GPU_GOURAUD_FIXED_BITS 11
#else
#define GPU_GOURAUD_FIXED_BITS 16
#endif

// Used to pass Gouraud colors to the inner loop drivers.
// TODO: adapt all the inner drivers to get their color values through
//       pointers to these rather than through global variables.
struct GouraudColor {
#ifdef GPU_GOURAUD_LOW_PRECISION
	u16 r, g, b;
	s16 r_incr, g_incr, b_incr;
#else
	u32 r, g, b;
	s32 r_incr, g_incr, b_incr;
#endif
};

static inline u16 gpuGouraudColor15bpp(u32 r, u32 g, u32 b)
{
	r >>= GPU_GOURAUD_FIXED_BITS;
	g >>= GPU_GOURAUD_FIXED_BITS;
	b >>= GPU_GOURAUD_FIXED_BITS;

#ifndef GPU_GOURAUD_LOW_PRECISION
	// High-precision Gouraud colors are 8-bit + fractional
	r >>= 3;  g >>= 3;  b >>= 3;
#endif

	return r | (g << 5) | (b << 10);
}

///////////////////////////////////////////////////////////////////////////////
//  GPU Pixel span operations generator gpuPixelSpanFn<>
//  Oct 2016: Created/adapted from old gpuPixelFn by senquack:
//  Original gpuPixelFn was used to draw lines one pixel at a time. I wrote
//  new line algorithms that draw lines using horizontal/vertical/diagonal
//  spans of pixels, necessitating new pixel-drawing function that could
//  not only render spans of pixels, but gouraud-shade them as well.
//  This speeds up line rendering and would allow tile-rendering (untextured
//  rectangles) to use the same set of functions. Since tiles are always
//  monochrome, they simply wouldn't use the extra set of 32 gouraud-shaded
//  gpuPixelSpanFn functions (TODO?).
//
// NOTE: While the PS1 framebuffer is 16 bit, we use 8-bit pointers here,
//       so that pDst can be incremented directly by 'incr' parameter
//       without having to shift it before use.
template<const int CF>
static u8* gpuPixelSpanFn(u8* pDst, uintptr_t data, ptrdiff_t incr, size_t len)
{
	u16 col;
	struct GouraudColor * gcPtr;
	u32 r, g, b;
	s32 r_incr, g_incr, b_incr;

	if (CF_GOURAUD) {
		gcPtr = (GouraudColor*)data;
		r = gcPtr->r;  r_incr = gcPtr->r_incr;
		g = gcPtr->g;  g_incr = gcPtr->g_incr;
		b = gcPtr->b;  b_incr = gcPtr->b_incr;
	} else {
		col = (u16)data;
	}

	do {
		if (!CF_GOURAUD)
		{   // NO GOURAUD
			if (!CF_MASKCHECK && !CF_BLEND) {
				if (CF_MASKSET) { *(u16*)pDst = col | 0x8000; }
				else            { *(u16*)pDst = col;          }
			} else if (CF_MASKCHECK && !CF_BLEND) {
				if (!(*(u16*)pDst & 0x8000)) {
					if (CF_MASKSET) { *(u16*)pDst = col | 0x8000; }
					else            { *(u16*)pDst = col;          }
				}
			} else {
				u16 uDst = *(u16*)pDst;
				if (CF_MASKCHECK) { if (uDst & 0x8000) goto endpixel; }
				u16 uSrc = col;
				u32 uMsk; if (CF_BLENDMODE==0) uMsk=0x7BDE;
				if (CF_BLENDMODE==0) gpuBlending00(uSrc, uDst);
				if (CF_BLENDMODE==1) gpuBlending01(uSrc, uDst);
				if (CF_BLENDMODE==2) gpuBlending02(uSrc, uDst);
				if (CF_BLENDMODE==3) gpuBlending03(uSrc, uDst);

				if (CF_MASKSET) { *(u16*)pDst = uSrc | 0x8000; }
				else            { *(u16*)pDst = uSrc;          }
			}

		} else
		{   // GOURAUD

			if (!CF_MASKCHECK && !CF_BLEND) {
				col = gpuGouraudColor15bpp(r, g, b);
				if (CF_MASKSET) { *(u16*)pDst = col | 0x8000; }
				else            { *(u16*)pDst = col;          }
			} else if (CF_MASKCHECK && !CF_BLEND) {
				col = gpuGouraudColor15bpp(r, g, b);
				if (!(*(u16*)pDst & 0x8000)) {
					if (CF_MASKSET) { *(u16*)pDst = col | 0x8000; }
					else            { *(u16*)pDst = col;          }
				}
			} else {
				u16 uDst = *(u16*)pDst;
				if (CF_MASKCHECK) { if (uDst & 0x8000) goto endpixel; }
				col = gpuGouraudColor15bpp(r, g, b);
				u16 uSrc = col;
				u32 uMsk; if (CF_BLENDMODE==0) uMsk=0x7BDE;
				if (CF_BLENDMODE==0) gpuBlending00(uSrc, uDst);
				if (CF_BLENDMODE==1) gpuBlending01(uSrc, uDst);
				if (CF_BLENDMODE==2) gpuBlending02(uSrc, uDst);
				if (CF_BLENDMODE==3) gpuBlending03(uSrc, uDst);

				if (CF_MASKSET) { *(u16*)pDst = uSrc | 0x8000; }
				else            { *(u16*)pDst = uSrc;          }
			}
		}

endpixel:
		if (CF_GOURAUD) {
			r += r_incr;
			g += g_incr;
			b += b_incr;
		}
		pDst += incr;
	} while (len-- > 1);

	// Note from senquack: Normally, I'd prefer to write a 'do {} while (--len)'
	//  loop, or even a for() loop, however, on MIPS platforms anything but the
	//  'do {} while (len-- > 1)' tends to generate very unoptimal asm, with
	//  many unneeded MULs/ADDs/branches at the ends of these functions.
	//  If you change the loop structure above, be sure to compare the quality
	//  of the generated code!!

	if (CF_GOURAUD) {
		gcPtr->r = r;
		gcPtr->g = g;
		gcPtr->b = b;
	}
	return pDst;
}

static u8* PixelSpanNULL(u8* pDst, uintptr_t data, ptrdiff_t incr, size_t len)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"PixelSpanNULL()\n");
	#endif
	return pDst;
}

typedef u8* (*PSD)(u8* dst, uintptr_t data, ptrdiff_t incr, size_t len);
const PSD gpuPixelSpanDrivers[64] =
{ 
	// Array index | 'CF' template field | Field value
	// ------------+---------------------+----------------
	// Bit 0       | CF_BLEND            | off (0), on (1)
	// Bit 1       | CF_MASKCHECK        | off (0), on (1)
	// Bit 3:2     | CF_BLENDMODE        | 0..3
	// Bit 4       | CF_MASKSET          | off (0), on (1)
	// Bit 5       | CF_GOURAUD          | off (0), on (1)
	//
	// NULL entries are ones for which blending is disabled and blend-mode
	//  field is non-zero, which is obviously invalid.

	// Flat-shaded
	gpuPixelSpanFn<0x00<<1>,         gpuPixelSpanFn<0x01<<1>,         gpuPixelSpanFn<0x02<<1>,         gpuPixelSpanFn<0x03<<1>,
	PixelSpanNULL,                   gpuPixelSpanFn<0x05<<1>,         PixelSpanNULL,                   gpuPixelSpanFn<0x07<<1>,
	PixelSpanNULL,                   gpuPixelSpanFn<0x09<<1>,         PixelSpanNULL,                   gpuPixelSpanFn<0x0B<<1>,
	PixelSpanNULL,                   gpuPixelSpanFn<0x0D<<1>,         PixelSpanNULL,                   gpuPixelSpanFn<0x0F<<1>,

	// Flat-shaded + PixelMSB (CF_MASKSET)
	gpuPixelSpanFn<(0x00<<1)|0x100>, gpuPixelSpanFn<(0x01<<1)|0x100>, gpuPixelSpanFn<(0x02<<1)|0x100>, gpuPixelSpanFn<(0x03<<1)|0x100>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x05<<1)|0x100>, PixelSpanNULL,                   gpuPixelSpanFn<(0x07<<1)|0x100>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x09<<1)|0x100>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0B<<1)|0x100>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x0D<<1)|0x100>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0F<<1)|0x100>,

	// Gouraud-shaded (CF_GOURAUD)
	gpuPixelSpanFn<(0x00<<1)|0x80>,  gpuPixelSpanFn<(0x01<<1)|0x80>,  gpuPixelSpanFn<(0x02<<1)|0x80>,  gpuPixelSpanFn<(0x03<<1)|0x80>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x05<<1)|0x80>,  PixelSpanNULL,                   gpuPixelSpanFn<(0x07<<1)|0x80>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x09<<1)|0x80>,  PixelSpanNULL,                   gpuPixelSpanFn<(0x0B<<1)|0x80>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x0D<<1)|0x80>,  PixelSpanNULL,                   gpuPixelSpanFn<(0x0F<<1)|0x80>,

	// Gouraud-shaded (CF_GOURAUD) + PixelMSB (CF_MASKSET)
	gpuPixelSpanFn<(0x00<<1)|0x180>, gpuPixelSpanFn<(0x01<<1)|0x180>, gpuPixelSpanFn<(0x02<<1)|0x180>, gpuPixelSpanFn<(0x03<<1)|0x180>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x05<<1)|0x180>, PixelSpanNULL,                   gpuPixelSpanFn<(0x07<<1)|0x180>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x09<<1)|0x180>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0B<<1)|0x180>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x0D<<1)|0x180>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0F<<1)|0x180>
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Tiles innerloops generator

template<const int CF>
static void gpuTileSpanFn(u16 *pDst, u32 count, u16 data)
{
	if (!CF_MASKCHECK && !CF_BLEND) {
		if (CF_MASKSET) { data = data | 0x8000; }
		do { *pDst++ = data; } while (--count);
	} else if (CF_MASKCHECK && !CF_BLEND) {
		if (CF_MASKSET) { data = data | 0x8000; }
		do { if (!(*pDst&0x8000)) { *pDst = data; } pDst++; } while (--count);
	} else
	{
		u16 uSrc;
		u16 uDst;
		u32 uMsk; if (CF_BLENDMODE==0) uMsk=0x7BDE;
		do
		{
			//  MASKING
			uDst = *pDst;
			if (CF_MASKCHECK) { if (uDst&0x8000) goto endtile; }
			uSrc = data;

			//  BLEND
			if (CF_BLENDMODE==0) gpuBlending00(uSrc, uDst);
			if (CF_BLENDMODE==1) gpuBlending01(uSrc, uDst);
			if (CF_BLENDMODE==2) gpuBlending02(uSrc, uDst);
			if (CF_BLENDMODE==3) gpuBlending03(uSrc, uDst);

			if (CF_MASKSET) { *pDst = uSrc | 0x8000; }
			else            { *pDst = uSrc;          }

			//senquack - Did not apply "Silent Hill" mask-bit fix to here.
			// It is hard to tell from scarce documentation available and
			//  lack of comments in code, but I believe the tile-span
			//  functions here should not bother to preserve any source MSB,
			//  as they are not drawing from a texture.
endtile:
			pDst++;
		}
		while (--count);
	}
}

static void TileNULL(u16 *pDst, u32 count, u16 data)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"TileNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Tiles innerloops driver
typedef void (*PT)(u16 *pDst, u32 count, u16 data);

// Template instantiation helper macros
#define TI(cf) gpuTileSpanFn<(cf)>
#define TN     TileNULL
#define TIBLOCK(ub) \
	TI((ub)|0x00), TI((ub)|0x02), TI((ub)|0x04), TI((ub)|0x06), \
	TN,            TI((ub)|0x0a), TN,            TI((ub)|0x0e), \
	TN,            TI((ub)|0x12), TN,            TI((ub)|0x16), \
	TN,            TI((ub)|0x1a), TN,            TI((ub)|0x1e)

const PT gpuTileSpanDrivers[32] = {
	TIBLOCK(0<<8), TIBLOCK(1<<8)
};

#undef TI
#undef TN
#undef TIBLOCK


///////////////////////////////////////////////////////////////////////////////
//  GPU Sprites innerloops generator

template<const int CF>
static void gpuSpriteSpanFn(u16 *pDst, u32 count, u8* pTxt, const u32 mask)
{
	u16 uSrc, uDst;
	u32 u0 = 0;
	const u16 *CBA_; if (CF_TEXTMODE!=3) CBA_ = gpu_unai.CBA;
	u32 lCol;
	if (CF_LIGHT) {
		lCol = ((u32)(gpu_unai.b4 <<  2)&(0x03ff))     |
		       ((u32)(gpu_unai.g4 << 13)&(0x07ff<<10)) |
		       ((u32)(gpu_unai.r4 << 24)&(0x07ff<<21));
	}
	u8 rgb; if (CF_TEXTMODE==1) rgb = pTxt[u0>>1];
	u32 uMsk; if (CF_BLEND && CF_BLENDMODE==0) uMsk=0x7BDE;

	//senquack - 'Silent Hill' white rectangles around characters fix:
	// MSB of pixel from source texture was not preserved across calls to
	// gpuBlendingXX() macros.. we must save it if calling them:
	// NOTE: it was gpuPolySpanFn() that cause the Silent Hill glitches, but
	// from reading the few PSX HW docs available, MSB should be transferred
	// whenever a texture is being read.
	u16 srcMSB;

	do
	{
		if (CF_MASKCHECK) {
			uDst = *pDst;
			if (uDst&0x8000) { u0=(u0+1)&mask; goto endsprite; }
		}

		if (CF_TEXTMODE==1) {  //  4bpp (CLUT)
			if (!(u0&1)) rgb = pTxt[u0>>1];
			uSrc = CBA_[(rgb>>((u0&1)<<2))&0xf];
			u0=(u0+1)&mask;
		}
		if (CF_TEXTMODE==2) {  //  8bpp (CLUT)
			uSrc = CBA_[pTxt[u0]];
			u0=(u0+1)&mask;
		}
		if (CF_TEXTMODE==3) {  // 16bpp
			uSrc = ((u16*)pTxt)[u0];
			u0=(u0+1)&mask;
		}
		if (!uSrc) goto endsprite;

		//senquack - save source MSB, as blending or lighting macros will not
		if (!CF_MASKSET && (CF_BLEND || CF_LIGHT)) { srcMSB = uSrc & 0x8000; }
		
		if (CF_BLEND)
		{
			if (uSrc&0x8000) {
				if (CF_LIGHT) gpuLightingTXT(uSrc, lCol);

				if (!CF_MASKCHECK) uDst = *pDst;
				if (CF_BLENDMODE==0) gpuBlending00(uSrc, uDst);
				if (CF_BLENDMODE==1) gpuBlending01(uSrc, uDst);
				if (CF_BLENDMODE==2) gpuBlending02(uSrc, uDst);
				if (CF_BLENDMODE==3) gpuBlending03(uSrc, uDst);
			} else {
				if (CF_LIGHT) gpuLightingTXT(uSrc, lCol);
			}
		} else {
			//senquack - While fixing Silent Hill white-rectangles bug, I
			// noticed uSrc was being masked unnecessarily here:
			//if(CF_LIGHT)  { gpuLightingTXT(uSrc, lCol);   } else
			//{ if(!CF_MASKSET) uSrc&= 0x7fff;               }
			if (CF_LIGHT) gpuLightingTXT(uSrc, lCol);
		}

		if (CF_MASKSET)                { *pDst = uSrc | 0x8000; }
		else if (CF_BLEND || CF_LIGHT) { *pDst = uSrc | srcMSB; }
		else                           { *pDst = uSrc;          }

endsprite:
		pDst++;
	}
	while (--count);
}

static void SpriteNULL(u16 *pDst, u32 count, u8* pTxt, const u32 mask)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"SpriteNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Sprite innerloops driver
typedef void (*PS)(u16 *pDst, u32 count, u8* pTxt, const u32 mask);

// Template instantiation helper macros
#define TI(cf) gpuSpriteSpanFn<(cf)>
#define TN     SpriteNULL
#define TIBLOCK(ub) \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TI((ub)|0x20), TI((ub)|0x21), TI((ub)|0x22), TI((ub)|0x23), TI((ub)|0x24), TI((ub)|0x25), TI((ub)|0x26), TI((ub)|0x27), \
	TN,            TN,            TI((ub)|0x2a), TI((ub)|0x2b), TN,            TN,            TI((ub)|0x2e), TI((ub)|0x2f), \
	TN,            TN,            TI((ub)|0x32), TI((ub)|0x33), TN,            TN,            TI((ub)|0x36), TI((ub)|0x37), \
	TN,            TN,            TI((ub)|0x3a), TI((ub)|0x3b), TN,            TN,            TI((ub)|0x3e), TI((ub)|0x3f), \
	TI((ub)|0x40), TI((ub)|0x41), TI((ub)|0x42), TI((ub)|0x43), TI((ub)|0x44), TI((ub)|0x45), TI((ub)|0x46), TI((ub)|0x47), \
	TN,            TN,            TI((ub)|0x4a), TI((ub)|0x4b), TN,            TN,            TI((ub)|0x4e), TI((ub)|0x4f), \
	TN,            TN,            TI((ub)|0x52), TI((ub)|0x53), TN,            TN,            TI((ub)|0x56), TI((ub)|0x57), \
	TN,            TN,            TI((ub)|0x5a), TI((ub)|0x5b), TN,            TN,            TI((ub)|0x5e), TI((ub)|0x5f), \
	TI((ub)|0x60), TI((ub)|0x61), TI((ub)|0x62), TI((ub)|0x63), TI((ub)|0x64), TI((ub)|0x65), TI((ub)|0x66), TI((ub)|0x67), \
	TN,            TN,            TI((ub)|0x6a), TI((ub)|0x6b), TN,            TN,            TI((ub)|0x6e), TI((ub)|0x6f), \
	TN,            TN,            TI((ub)|0x72), TI((ub)|0x73), TN,            TN,            TI((ub)|0x76), TI((ub)|0x77), \
	TN,            TN,            TI((ub)|0x7a), TI((ub)|0x7b), TN,            TN,            TI((ub)|0x7e), TI((ub)|0x7f)

const PS gpuSpriteSpanDrivers[256] = {
	TIBLOCK(0<<8), TIBLOCK(1<<8)
};

#undef TI
#undef TN
#undef TIBLOCK

///////////////////////////////////////////////////////////////////////////////
//  GPU Polygon innerloops generator

//senquack - Newer version with following changes:
//           * Adapted to work with new poly routings in gpu_raster_polygon.h
//             adapted from DrHell GPU. They are less glitchy and use 22.10
//             fixed-point instead of original UNAI's 16.16.
//           * Texture coordinates are no longer packed together into one
//             unsigned int. This seems to lose too much accuracy (they each
//             end up being only 8.7 fixed-point that way) and pixel-droupouts
//             were noticeable both with original code and current DrHell
//             adaptations. An example would be the sky in NFS3. Now, they are
//             stored in separate ints, using separate masks.
//           * Function is no longer INLINE, as it was always called
//             through a function pointer.
//           * Function now ensures the mask bit of source texture is preserved
//             across calls to blending functions (Silent Hill rectangles fix)
// (see README_senquack.txt)
template<const int CF>
static void gpuPolySpanFn(u16 *pDst, u32 count)
{
	static const u16 uMsk = 0x7BDE;

	if (!CF_TEXTMODE)
	{	
		// NO TEXTURE
		if (!CF_GOURAUD)
		{
			u32 lCol;
			u32 uSrc;
			u16 uDst;

			if (!CF_LIGHT)
			{
				do {
					uSrc = gpu_unai.PixelData;

					if (CF_BLEND || CF_MASKCHECK) uDst = *pDst;
					if (CF_MASKCHECK) { if (uDst&0x8000) { goto endpolynotextnogou; } }

					if (CF_BLEND) {
						if (CF_BLENDMODE==0) gpuBlending00(uSrc, uDst);
						if (CF_BLENDMODE==1) gpuBlending01(uSrc, uDst);
						if (CF_BLENDMODE==2) gpuBlending02(uSrc, uDst);
						if (CF_BLENDMODE==3) gpuBlending03(uSrc, uDst);
					}

					if (CF_MASKSET) { uSrc = uSrc | 0x8000; }
					*pDst = uSrc;
endpolynotextnogou:
					pDst++;

				} while(--count);
			}
			else
			{
				u32 sSrc;
				getLightNoGouraud(lCol);
				gpuLightingRGB24(sSrc, lCol);

				do {
					uSrc = sSrc;

					if (CF_BLEND) {
						uDst = *pDst;
						if (CF_MASKCHECK) { if (uDst&0x8000) { pDst++; continue; } }

						doGpuPolyBlend<CF_BLENDMODE>(uSrc, uDst);
					}

					gpuColorQuantization<CF_DITHER>(uSrc, pDst);

					if (CF_MASKSET) { *pDst++ = ((u16)uSrc) | 0x8000; }
					else            { *pDst++ = ((u16)uSrc);          }

				} while(--count);
			}
		}
		else
		{
			// GOURAUD
			u16 uDst;
			u32 uSrc;
			u32 linc=gpu_unai.lInc;

			//senquack - DRHELL poly code uses 22.10 fixed point, while UNAI used 16.16:
			//u32 lCol=((u32)(gpu_unai.b4>>14)&(0x03ff)) | ((u32)(gpu_unai.g4>>3)&(0x07ff<<10)) | ((u32)(gpu_unai.r4<<8)&(0x07ff<<21));
			u32 lCol; getLightGouraud(lCol);

			u32 bMsk; if (CF_BLITMASK) bMsk=gpu_unai.blit_mask;
			do
			{
				uDst = *pDst;

				if (CF_BLITMASK) { if ((bMsk>>((((uintptr_t)pDst)>>1)&7))&1) goto endpolynotextgou; }
				if (CF_MASKCHECK) { if (uDst&0x8000) goto endpolynotextgou; }

				gpuLightingRGB24(uSrc,lCol);

				if (CF_BLEND) {
					doGpuPolyBlend<CF_BLENDMODE>(uSrc, uDst);
				}

				gpuColorQuantization<CF_DITHER>(uSrc, pDst);

				if (CF_MASKSET) { *pDst = ((u16)uSrc) | 0x8000; }
				else            { *pDst = ((u16)uSrc);          }

endpolynotextgou:
				pDst++;
				lCol=(lCol+linc);
			}
			while (--count);
		}
	}
	else
	{
		// TEXTURED

		//senquack - 'Silent Hill' white rectangles around characters fix:
		// MSB of pixel from source texture was not preserved across calls to
		// gpuBlendingXX() macros.. we must save it if calling them:
		u16 srcMSB;

		// TEXTURE
		u16 uDst;
		u32 uSrc;
		u32 linc; if (CF_LIGHT && CF_GOURAUD) linc=gpu_unai.lInc;

		//senquack - note: original UNAI code had gpu_unai.{u4/v4} packed into
		// one 32-bit unsigned int, but this proved to lose too much accuracy
		// (pixel drouputs noticeable in NFS3 sky), so now are separate vars.
		// TODO: pass these as parameters rather than through globals?
		u32 l_u4_msk = gpu_unai.u4_msk;      u32 l_v4_msk = gpu_unai.v4_msk;
		u32 l_u4 = gpu_unai.u4 & l_u4_msk;   u32 l_v4 = gpu_unai.v4 & l_v4_msk;
		u32 l_du4 = gpu_unai.du4;            u32 l_dv4 = gpu_unai.dv4;

		const u16* TBA_ = gpu_unai.TBA;
		const u16* CBA_; if (CF_TEXTMODE!=3) CBA_ = gpu_unai.CBA;
		u32 lCol;

		//senquack - After adapting DrHell poly code to replace Unai's glitchy
		// routines, it was necessary to adjust for the differences in fixed
		// point: DrHell is 22.10, Unai used 16.16. In the following two lines,
		// the top line expected gpu_unai.{b4,g4,r4} to be integers, as it is
		// not using Gouraud shading, so it needed no adjustment. The bottom
		// line needed no adjustment for fixed point changes.
		if (CF_LIGHT && !CF_GOURAUD) getLightNoGouraud(lCol);
		if (CF_LIGHT &&  CF_GOURAUD) getLightGouraud(lCol);

		u32 bMsk; if (CF_BLITMASK) bMsk=gpu_unai.blit_mask;

		do
		{
			if (CF_BLITMASK) { if ((bMsk>>((((uintptr_t)pDst)>>1)&7))&1) goto endpolytext; }
			if (CF_MASKCHECK || CF_BLEND) { uDst = *pDst; }
			if (CF_MASKCHECK) if (uDst&0x8000) goto endpolytext;

			//senquack - adapted to work with new 22.10 fixed point routines:
			//           (UNAI originally used 16.16)
			if (CF_TEXTMODE==1) {  //  4bpp (CLUT)
				u32 tu=(l_u4>>10);
				u32 tv=(l_v4<<1)&(0xff<<11);
				u8 rgb=((u8*)TBA_)[tv+(tu>>1)];
				uSrc=CBA_[(rgb>>((tu&1)<<2))&0xf];
				if(!uSrc) goto endpolytext;
			}
			if (CF_TEXTMODE==2) {  //  8bpp (CLUT)
				uSrc = CBA_[(((u8*)TBA_)[(l_u4>>10)+((l_v4<<1)&(0xff<<11))])];
				if (!uSrc) goto endpolytext;
			}
			if (CF_TEXTMODE==3) {  // 16bpp
				uSrc = TBA_[(l_u4>>10)+((l_v4)&(0xff<<10))];
				if (!uSrc) goto endpolytext;
			}

			//senquack - save source MSB, as blending or lighting macros will not
			srcMSB = uSrc & 0x8000;

			// LIGHT &&  BLEND => dither
			// LIGHT && !BLEND => dither
			//!LIGHT &&  BLEND => no dither
			//!LIGHT && !BLEND => no dither

			if (CF_LIGHT)
			{
				if (!CF_BLEND || uSrc&0x8000)
				{
					gpuLightingTXT24(uSrc, lCol);
					if (CF_BLEND) doGpuPolyBlend<CF_BLENDMODE>(uSrc, uDst);
					gpuColorQuantization<CF_DITHER>(uSrc, pDst);
				}
			}
			else
			{
				if (CF_BLEND && (srcMSB&0x8000)) {
					if (CF_BLENDMODE==0) gpuBlending00(uSrc, uDst);
					if (CF_BLENDMODE==1) gpuBlending01(uSrc, uDst);
					if (CF_BLENDMODE==2) gpuBlending02(uSrc, uDst);
					if (CF_BLENDMODE==3) gpuBlending03(uSrc, uDst);
				}
			}

			if (CF_MASKSET)                { *pDst = uSrc | 0x8000; }
			else if (CF_BLEND || CF_LIGHT) { *pDst = uSrc | srcMSB; }
			else                           { *pDst = uSrc;          }
endpolytext:
			pDst++;
			l_u4 = (l_u4 + l_du4) & l_u4_msk;
			l_v4 = (l_v4 + l_dv4) & l_v4_msk;
			if (CF_LIGHT && CF_GOURAUD) lCol=(lCol+linc);
		}
		while (--count);
	}
}

static void PolyNULL(u16 *pDst, u32 count)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"PolyNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Polygon innerloops driver
typedef void (*PP)(u16 *pDst, u32 count);

// Template instantiation helper macros
#define TI(cf) gpuPolySpanFn<(cf)>
#define TN     PolyNULL
#define TIBLOCK(ub) \
	TI((ub)|0x00), TI((ub)|0x01), TI((ub)|0x02), TI((ub)|0x03), TI((ub)|0x04), TI((ub)|0x05), TI((ub)|0x06), TI((ub)|0x07), \
	TN,            TN,            TI((ub)|0x0a), TI((ub)|0x0b), TN,            TN,            TI((ub)|0x0e), TI((ub)|0x0f), \
	TN,            TN,            TI((ub)|0x12), TI((ub)|0x13), TN,            TN,            TI((ub)|0x16), TI((ub)|0x17), \
	TN,            TN,            TI((ub)|0x1a), TI((ub)|0x1b), TN,            TN,            TI((ub)|0x1e), TI((ub)|0x1f), \
	TI((ub)|0x20), TI((ub)|0x21), TI((ub)|0x22), TI((ub)|0x23), TI((ub)|0x24), TI((ub)|0x25), TI((ub)|0x26), TI((ub)|0x27), \
	TN,            TN,            TI((ub)|0x2a), TI((ub)|0x2b), TN,            TN,            TI((ub)|0x2e), TI((ub)|0x2f), \
	TN,            TN,            TI((ub)|0x32), TI((ub)|0x33), TN,            TN,            TI((ub)|0x36), TI((ub)|0x37), \
	TN,            TN,            TI((ub)|0x3a), TI((ub)|0x3b), TN,            TN,            TI((ub)|0x3e), TI((ub)|0x3f), \
	TI((ub)|0x40), TI((ub)|0x41), TI((ub)|0x42), TI((ub)|0x43), TI((ub)|0x44), TI((ub)|0x45), TI((ub)|0x46), TI((ub)|0x47), \
	TN,            TN,            TI((ub)|0x4a), TI((ub)|0x4b), TN,            TN,            TI((ub)|0x4e), TI((ub)|0x4f), \
	TN,            TN,            TI((ub)|0x52), TI((ub)|0x53), TN,            TN,            TI((ub)|0x56), TI((ub)|0x57), \
	TN,            TN,            TI((ub)|0x5a), TI((ub)|0x5b), TN,            TN,            TI((ub)|0x5e), TI((ub)|0x5f), \
	TI((ub)|0x60), TI((ub)|0x61), TI((ub)|0x62), TI((ub)|0x63), TI((ub)|0x64), TI((ub)|0x65), TI((ub)|0x66), TI((ub)|0x67), \
	TN,            TN,            TI((ub)|0x6a), TI((ub)|0x6b), TN,            TN,            TI((ub)|0x6e), TI((ub)|0x6f), \
	TN,            TN,            TI((ub)|0x72), TI((ub)|0x73), TN,            TN,            TI((ub)|0x76), TI((ub)|0x77), \
	TN,            TN,            TI((ub)|0x7a), TI((ub)|0x7b), TN,            TN,            TI((ub)|0x7e), TI((ub)|0x7f), \
	TN,            TI((ub)|0x81), TN,            TI((ub)|0x83), TN,            TI((ub)|0x85), TN,            TI((ub)|0x87), \
	TN,            TN,            TN,            TI((ub)|0x8b), TN,            TN,            TN,            TI((ub)|0x8f), \
	TN,            TN,            TN,            TI((ub)|0x93), TN,            TN,            TN,            TI((ub)|0x97), \
	TN,            TN,            TN,            TI((ub)|0x9b), TN,            TN,            TN,            TI((ub)|0x9f), \
	TN,            TI((ub)|0xa1), TN,            TI((ub)|0xa3), TN,            TI((ub)|0xa5), TN,            TI((ub)|0xa7), \
	TN,            TN,            TN,            TI((ub)|0xab), TN,            TN,            TN,            TI((ub)|0xaf), \
	TN,            TN,            TN,            TI((ub)|0xb3), TN,            TN,            TN,            TI((ub)|0xb7), \
	TN,            TN,            TN,            TI((ub)|0xbb), TN,            TN,            TN,            TI((ub)|0xbf), \
	TN,            TI((ub)|0xc1), TN,            TI((ub)|0xc3), TN,            TI((ub)|0xc5), TN,            TI((ub)|0xc7), \
	TN,            TN,            TN,            TI((ub)|0xcb), TN,            TN,            TN,            TI((ub)|0xcf), \
	TN,            TN,            TN,            TI((ub)|0xd3), TN,            TN,            TN,            TI((ub)|0xd7), \
	TN,            TN,            TN,            TI((ub)|0xdb), TN,            TN,            TN,            TI((ub)|0xdf), \
	TN,            TI((ub)|0xe1), TN,            TI((ub)|0xe3), TN,            TI((ub)|0xe5), TN,            TI((ub)|0xe7), \
	TN,            TN,            TN,            TI((ub)|0xeb), TN,            TN,            TN,            TI((ub)|0xef), \
	TN,            TN,            TN,            TI((ub)|0xf3), TN,            TN,            TN,            TI((ub)|0xf7), \
	TN,            TN,            TN,            TI((ub)|0xfb), TN,            TN,            TN,            TI((ub)|0xff)

const PP gpuPolySpanDrivers[2048] = {
	TIBLOCK(0<<8), TIBLOCK(1<<8), TIBLOCK(2<<8), TIBLOCK(3<<8),
	TIBLOCK(4<<8), TIBLOCK(5<<8), TIBLOCK(6<<8), TIBLOCK(7<<8)
};

#undef TI
#undef TN
#undef TIBLOCK
