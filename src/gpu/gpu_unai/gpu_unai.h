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

#ifndef GPU_UNAI_GPU_UNAI_H
#define GPU_UNAI_GPU_UNAI_H

// Header shared between both standalone gpu_unai (gpu.cpp) and new
// gpulib-compatible gpu_unai (gpulib_if.cpp)
// -> Anything here should be for gpu_unai's private use. <-

///////////////////////////////////////////////////////////////////////////////
//  Compile Options

//#define ENABLE_GPU_NULL_SUPPORT   // Enables NullGPU support
//#define ENABLE_GPU_LOG_SUPPORT    // Enables gpu logger, very slow only for windows debugging
//#define ENABLE_GPU_ARMV7			// Enables ARMv7 optimized assembly

//Poly routine options (default is integer math and accurate division)
//#define GPU_UNAI_USE_FLOATMATH         // Use float math in poly routines
//#define GPU_UNAI_USE_FLOAT_DIV_MULTINV // If GPU_UNAI_USE_FLOATMATH is defined,
                                         //  use multiply-by-inverse for division
//#define GPU_UNAI_USE_INT_DIV_MULTINV   // If GPU_UNAI_USE_FLOATMATH is *not*
                                         //  defined, use old inaccurate division


#define u8  uint8_t
#define s8  int8_t
#define u16 uint16_t
#define s16 int16_t
#define u32 uint32_t
#define s32 int32_t
#define s64 int64_t

union PtrUnion
{
	u32  *U4;
	s32  *S4;
	u16  *U2;
	s16  *S2;
	u8   *U1;
	s8   *S1;
	void *ptr;
};

union GPUPacket
{
	u32 U4[16];
	s32 S4[16];
	u16 U2[32];
	s16 S2[32];
	u8  U1[64];
	s8  S1[64];
};

template<class T> static inline void SwapValues(T &x, T &y)
{
	T tmp(x);  x = y;  y = tmp;
}

template<typename T>
static inline T Min2 (const T a, const T b)
{
	return (a<b)?a:b;
}

template<typename T>
static inline T Min3 (const T a, const T b, const T c)
{
	return  Min2(Min2(a,b),c);
}

template<typename T>
static inline T Max2 (const T a, const T b)
{
	return  (a>b)?a:b;
}

template<typename T>
static inline T Max3 (const T a, const T b, const T c)
{
	return  Max2(Max2(a,b),c);
}


///////////////////////////////////////////////////////////////////////////////
//  GPU Raster Macros

// Convert 24bpp color parameter of GPU command to 16bpp (15bpp + mask bit)
#define	GPU_RGB16(rgb) ((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

// Sign-extend 11-bit coordinate command param
#define GPU_EXPANDSIGN(x) (((s32)(x)<<(32-11))>>(32-11))

// Max difference between any two X or Y primitive coordinates
#define CHKMAX_X 1024
#define CHKMAX_Y 512

#define	FRAME_BUFFER_SIZE	(1024*512*2)
#define	FRAME_WIDTH			  1024
#define	FRAME_HEIGHT		  512
#define	FRAME_OFFSET(x,y)	(((y)<<10)+(x))
#define FRAME_BYTE_STRIDE     2048
#define FRAME_BYTES_PER_PIXEL 2

///////////////////////////////////////////////////////////////////////////
// Division helpers (SDIV should be defined somewhere in port headers,
//  allowing use of ASM routine on platforms lacking hardware divide)
static inline s32 GPU_DIV(s32 rs, s32 rt)
{
	return rt ? (SDIV(rs,rt)) : (0);
}

// 'Unsafe' version of above that doesn't check for div-by-zero
#define GPU_FAST_DIV(rs, rt) SDIV((rs),(rt))

#endif // GPU_UNAI_GPU_UNAI_H
