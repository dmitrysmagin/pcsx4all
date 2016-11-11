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

#include "gpu.h"

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

struct gpu_unai_t {
	u32 GPU_GP1;
	GPUPacket PacketBuffer;
	u16 *vram;

	////////////////////////////////////////////////////////////////////////////
	// Variables used only by older standalone version of gpu_unai (gpu.cpp)
#ifndef USE_GPULIB
	u32  GPU_GP0;
	u32  tex_window;       // Current texture window vals (set by GP0(E2h) cmd)
	s32  PacketCount;
	s32  PacketIndex;
	bool fb_dirty;         // Framebuffer is dirty (according to GPU)

	//  Display status
	//  NOTE: Standalone older gpu_unai didn't care about horiz display range
	u16  DisplayArea[6];   // [0] : Start of display area (in VRAM) X
	                       // [1] : Start of display area (in VRAM) Y
	                       // [2] : Display mode resolution HORIZONTAL
	                       // [3] : Display mode resolution VERTICAL
	                       // [4] : Vertical display range (on TV) START
	                       // [5] : Vertical display range (on TV) END

	////////////////////////////////////////////////////////////////////////////
	//  Dma Transfers info
	struct {
		s32  px,py;
		s32  x_end,y_end;
		u16* pvram;
		u32 *last_dma;     // Last dma pointer
		bool FrameToRead;  // Load image in progress
		bool FrameToWrite; // Store image in progress
	} dma;

	////////////////////////////////////////////////////////////////////////////
	//  Frameskip
	struct {
		int  skipCount;    // Frame skip (0,1,2,3...)
		bool isSkip;       // Skip frame (according to GPU)
		bool skipFrame;    // Skip this frame (according to frame skip)
		bool wasSkip;      // Skip frame old value (according to GPU)
		bool skipGPU;      // Skip GPU primitives
	} frameskip;
#endif
	// END of standalone gpu_unai variables
	////////////////////////////////////////////////////////////////////////////

	u32 TextureWindowCur;  // Current setting from last GP0(0xE2) cmd (raw form)
	u8  TextureWindow[4];  // [0] : Texture window offset X
	                       // [1] : Texture window offset Y
	                       // [2] : Texture window mask X
	                       // [3] : Texture window mask Y

	u16 DrawingArea[4];    // [0] : Drawing area top left X
	                       // [1] : Drawing area top left Y
	                       // [2] : Drawing area bottom right X
	                       // [3] : Drawing area bottom right Y

	s16 DrawingOffset[2];  // [0] : Drawing offset X (signed)
	                       // [1] : Drawing offset Y (signed)

	u16* TBA;              // Ptr to current texture in VRAM
	u16* CBA;              // Ptr to current CLUT in VRAM

	//  Inner Loop parameters
	//  TODO: Pass these through their own struct?
	u16 PixelData;
	s32   u4, du4;
	s32   v4, dv4;
	s32   r4, dr4;
	s32   g4, dg4;
	s32   b4, db4;
	u32   lInc;

	//senquack - u4,v4 were originally packed into one word in gpuPolySpanFn in
	//           gpu_inner.h, and these two vars were used to pack du4,dv4 into
	//           another word and increment both u4,v4 with one instruction.
	//           During the course of fixing gpu_unai's polygon rendering issues,
	//           I ultimately found it was impossible to keep them combined like
	//           that, as pixel dropouts would always occur, particularly in NFS3
	//           sky background, perhaps from the reduced accuracy. Now they are
	//           incremented individually with du4 and dv4, and masked separetely,
	//           using new vars u4_msk, v4_msk, which store fixed-point masks.
	//           These are updated whenever TextureWindow[2] or [3] are changed.
	//u32   tInc, tMsk;
	u32   u4_msk, v4_msk;

	u32 blit_mask;          // Determines what pixels to skip when rendering.
	                        //  Only useful on low-resolution devices using
	                        //  a simple pixel-dropping downscaler for PS1
                            //  high-res modes. See 'pixel_skip' option.

	u8 ilace_mask;          // Determines what lines to skip when rendering.
	                        //  Normally 0 when PS1 240 vertical res is in
	                        //  use and ilace_force is 0. When running in
	                        //  PS1 480 vertical res on a low-resolution
	                        //  device (320x240), will usually be set to 1
	                        //  so odd lines are not rendered. (Unless future
	                        //  full-screen scaling option is in use ..TODO)

	bool prog_ilace_flag;   // Tracks successive frames for 'prog_ilace' option

	u8 BLEND_MODE;
	u8 TEXT_MODE;
	u8 Masking;

	u16 PixelMSB;

	gpu_unai_config_t config;
};

static gpu_unai_t gpu_unai;

// Global config that frontend can alter.. Values are read in GPU_init().
// TODO: if frontend menu modifies a setting, add a function that can notify
// GPU plugin to use new setting.
gpu_unai_config_t gpu_unai_config_ext;

///////////////////////////////////////////////////////////////////////////////
// Internal inline funcs to get option status: (Allows flexibility)
static inline bool LightingEnabled()
{
	return gpu_unai.config.lighting;
}

static inline bool BlendingEnabled()
{
	return gpu_unai.config.blending;
}

static inline bool ProgressiveInterlaceEnabled()
{
#ifdef USE_GPULIB
	// Using this old option greatly decreases quality of image. Disabled
	//  for now when using new gpulib, since it also adds more work in loops.
	return false;
#else
	return gpu_unai.config.prog_ilace;
#endif
}

static inline bool PixelSkipEnabled()
{
	return gpu_unai.config.pixel_skip;
}

#endif // GPU_UNAI_GPU_UNAI_H
