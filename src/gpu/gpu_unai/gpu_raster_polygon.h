/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
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

//NOTE: You can find the set of original Unai poly routines (disabled now)
// at the bottom end of this file.

//senquack - Original Unai GPU poly routines have been replaced with new
// ones based on DrHell routines. The original routines suffered from
// shifted rows, causing many quads to have their first triangle drawn
// correctly, but the second triangle would randomly have pixels shifted
// either left or right or entire rows not drawn at all. Furthermore,
// some times entire triangles seemed to be either missing or only
// partially drawn (most clearly seen in sky/road textures in NFS3,
// clock tower in beginning of Castlevania SOTN). Pixel gaps were
// prevalent.
//
// Since DrHell GPU didn't seem to exhibit these artifacts at all, I adapted
// its routines to GPU Unai (Unai was probably already originally based on it).
// DrHell uses 22.10 fixed point instead of Unai's 16.16, so gpu_fixedpoint.h
// required modification as well as gpu_inner.h (where gpuPolySpanFn driver
// functions are). I kept GPU_TESTRANGE3() macro, but also added a new
// macro GPU_ADJUSTCOORD3() that comes from gpu_dfxvideo (peops), that
// ensures the 16-bit coordinate values are interpreted as they were on
// the original hardware (as signed 11-bit values).
//
// Originally, I tried to patch up original Unai routines and got as far
// as fixing the shifted rows, but still had other problem of triangles rendered
// wrong (black triangular gaps in NFS3 sky, clock tower in Castlevania SOTN).
// I eventually gave up. Even after rewriting/adapting the routines,
// however, I still had some random pixel droupouts, specifically in
// NFS3 sky texture. I discovered that gpu_inner.h gpuPolySpanFn function
// was taking optimizations to an extreme and packing u/v texture coords
// into one 32-bit word, reducing their accuracy. Only once they were
// handled in full-accuracy individual words was that problem fixed.
//
// NOTE: I also added support for doing divisions using the FPU, either
//  with normal division or multiplication-by-reciprocal.
//  To use float division, GPU_UNAI_USE_FLOATMATH should be defined.
//  To use float mult-by-reciprocal, GPU_UNAI_USE_FLOAT_DIV_MULTINV.
// NOTE 2: Even with MIPS32R2 having FPU recip.s instruction, and it is
//  used when this platform is detected, I found it not to give any
//  noticeable speedup over normal float division (in fact seemed a tiny
//  tiny bit slower). I also found float division to not provide any
//  noticeable speedups versus integer division on MISP32R2 platform.
//  Granted, the differences were all around .5 FPS so I've left
//  new float options disabled by default.
//
// TODO:
// * Determine if it was useful or wise to leave in the rx0/rx1/ry0/ry1
//   blocks of checks in each routine. DrHell didn't use those checks
//   and Peops seems to do things a bit differently. I think for now they
//   probably are useful, if a little hard to understand.
// * See if anything can be done about remaining pixel gaps in Gran
//   Turismo car models, track.
// * See about fixing Silent Hill running animation as a level is loaded
//   (white rectangles bug I already fixed a while ago)
// * Add support for integer-divde-by-approx-reciprocal back into new Unai
//   functions (might lead back to allowing rows to be shifted, though so
//   possible will need more accurate method of approximation or use an
//   ASM fixed-point division routine). (This is only important for
//   platforms lacking both integer division and an FPU)
// * Find better way of passing parameters to gpuPolySpanFn functions than
//   through original Unai method of using global variables u4,v4,du4 etc.
// * Come up with some newer way of drawing rows of pixels than by calling
//   gpuPolySpanFn through function pointer. For every row, at least on
//   MIPS platforms, many registers are having to be pushed/popped from stack
//   on each call, which is strange since MIPS has so many registers.
// * MIPS MXU/ASM optimized gpuPolySpanFn ?
// * Move use of GPU_TESTRANGE3 and GPU_ADJUSTCOORD3() to gpu_command.h, so that
//   quads are having to have less unnecessary work done.

#define GPU_TESTRANGE3() \
{ \
	if(x0<0) { if((x1-x0)>CHKMAX_X) return; if((x2-x0)>CHKMAX_X) return; } \
	if(x1<0) { if((x0-x1)>CHKMAX_X) return; if((x2-x1)>CHKMAX_X) return; } \
	if(x2<0) { if((x0-x2)>CHKMAX_X) return; if((x1-x2)>CHKMAX_X) return; } \
	if(y0<0) { if((y1-y0)>CHKMAX_Y) return; if((y2-y0)>CHKMAX_Y) return; } \
	if(y1<0) { if((y0-y1)>CHKMAX_Y) return; if((y2-y1)>CHKMAX_Y) return; } \
	if(y2<0) { if((y0-y2)>CHKMAX_Y) return; if((y1-y2)>CHKMAX_Y) return; } \
}

//senquack - added (adapted from peops gpu_dfxvideo).. ensures that coordinates
// are stripped down to 11-bit signed values (-1024..+1023), as that is how
// the original hardware interprets them (ignoring bits 15..11)
#define GPU_ADJUSTCOORD3() \
{ \
	x0=((x0<<21)>>21); \
	x1=((x1<<21)>>21); \
	x2=((x2<<21)>>21); \
	y0=((y0<<21)>>21); \
	y1=((y1<<21)>>21); \
	y2=((y2<<21)>>21); \
}

///////////////////////////////////////////////////////////////////////////////
//  GPU internal polygon drawing functions
///////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------
F3 - Flat-shaded, untextured triangle
----------------------------------------------------------------------*/

void gpuDrawF3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);

	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	s32 y0, y1, y2;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2]);
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3]);
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[4]);
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[5]);
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[6]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[7]);
	GPU_ADJUSTCOORD3();
	GPU_TESTRANGE3();

	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	//senquack - This is left in from original UNAI code, but wasn't in DrHell:
	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	PixelData = GPU_RGB16(PacketBuffer.U4[0]);

	if (y0 > y1) {
		SwapValues(x0, x1);
		SwapValues(y0, y1);
	}
	if (y1 > y2) {
		SwapValues(x1, x2);
		SwapValues(y1, y2);
	}
	if (y0 > y1) {
		SwapValues(x0, x1);
		SwapValues(y0, y1);
	}
	ya = y2 - y0;
	yb = y2 - y1;
	dx = (x2 - x1) * ya - (x2 - x0) * yb;

	for (int loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;  yb = y1;
			x3 = x4 = i2x(x0);
			if (dx < 0) {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				dx3 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) * FloatInv(y2 - y0)) : 0;
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) * FloatInv(y1 - y0)) : 0;
#else
				dx3 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) / (float)(y2 - y0)) : 0;
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) / (float)(y1 - y0)) : 0;
#endif
#else
				dx3 = ((y2 - y0) != 0) ? GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0)) : 0;
				dx4 = ((y1 - y0) != 0) ? GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0)) : 0;
#endif
			} else {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				dx3 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) * FloatInv(y1 - y0)) : 0;
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) * FloatInv(y2 - y0)) : 0;
#else
				dx3 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) / (float)(y1 - y0)) : 0;
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) / (float)(y2 - y0)) : 0;
#endif
#else
				dx3 = ((y1 - y0) != 0) ? GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0)) : 0;
				dx4 = ((y2 - y0) != 0) ? GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0)) : 0;
#endif
			}
		} else {
			ya = y1;  yb = y2;
			if (dx < 0) {
				x3 = i2x(x0) + (dx3 * (y1 - y0));
				x4 = i2x(x1);
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) * FloatInv(y2 - y1)) : 0;
#else
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) / (float)(y2 - y1)) : 0;
#endif
#else
				dx4 = ((y2 - y1) != 0) ? GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1)) : 0;
#endif
			} else {
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				dx3 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) * FloatInv(y2 - y1)) : 0;
#else
				dx3 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) / (float)(y2 - y1)) : 0;
#endif
#else
				dx3 = ((y2 - y1) != 0) ? GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1)) : 0;
#endif
			}
		}
		if ((ymin - ya) > 0) {
			x3 += (dx3 * (ymin - ya));
			x4 += (dx4 * (ymin - ya));
			ya = ymin;
		}

		if (yb > ymax) yb = ymax;

		u16* PixelBase;
		int loop1 = yb - ya;
		if (loop1 > 0)
			PixelBase = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		else
			loop1 = 0;

		for (; loop1; --loop1, ya++, PixelBase += FRAME_WIDTH,
				x3 += dx3, x4 += dx4 )
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;

			xa = FixedCeilToInt(x3);  xb = FixedCeilToInt(x4);
			if ((xmin - xa) > 0)
				xa = xmin;
			if (xb > xmax) xb = xmax;
			if ((xb - xa) > 0) gpuPolySpanDriver(PixelBase + xa, (xb - xa));
		}
	}
}

/*----------------------------------------------------------------------
FT3 - Flat-shaded, textured triangle
----------------------------------------------------------------------*/

void gpuDrawFT3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	s32 y0, y1, y2;
	s32 u0, u1, u2, u3, du3;
	s32 v0, v1, v2, v3, dv3;
	
	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[6] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[7] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[10]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[11]);
	GPU_ADJUSTCOORD3();
	GPU_TESTRANGE3();

	x0 += DrawingOffset[0];  x1 += DrawingOffset[0];  x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];  y1 += DrawingOffset[1];  y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	//senquack - This is left in from original UNAI code, but wasn't in DrHell:
	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}

	u0 = PacketBuffer.U1[8];   v0 = PacketBuffer.U1[9];
	u1 = PacketBuffer.U1[16];  v1 = PacketBuffer.U1[17];
	u2 = PacketBuffer.U1[24];  v2 = PacketBuffer.U1[25];

	//senquack - TODO: find better way of passing parameters to
	//           gpuPolySpan functions than through globals:
	r4 = s32(PacketBuffer.U1[0]);
	g4 = s32(PacketBuffer.U1[1]);
	b4 = s32(PacketBuffer.U1[2]);
	dr4 = dg4 = db4 = 0;

	if (y0 > y1) {
		SwapValues(x0, x1);
		SwapValues(y0, y1);
		SwapValues(u0, u1);
		SwapValues(v0, v1);
	}
	if (y1 > y2) {
		SwapValues(x1, x2);
		SwapValues(y1, y2);
		SwapValues(u1, u2);
		SwapValues(v1, v2);
	}
	if (y0 > y1) {
		SwapValues(x0, x1);
		SwapValues(y0, y1);
		SwapValues(u0, u1);
		SwapValues(v0, v1);
	}

	ya = y2 - y0;
	yb = y2 - y1;
	dx4 = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;
	dx = dx4;
	if (dx4 < 0) {
		dx4 = -dx4;
		du4 = -du4;
		dv4 = -dv4;
	}

#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
	if (dx4 != 0) {
		float finv = FloatInv(dx4);
		du4 = (fixed)((du4 << FIXED_BITS) * finv);
		dv4 = (fixed)((dv4 << FIXED_BITS) * finv);
	} else {
		du4 = dv4 = 0;
	}
#else
	if (dx4 != 0) {
		float fdiv = dx4;
		du4 = (fixed)((du4 << FIXED_BITS) / fdiv);
		dv4 = (fixed)((dv4 << FIXED_BITS) / fdiv);
	} else {
		du4 = dv4 = 0;
	}
#endif
#else
	if (dx4 != 0) {
		du4 = GPU_FAST_DIV(du4 << FIXED_BITS, dx4);
		dv4 = GPU_FAST_DIV(dv4 << FIXED_BITS, dx4);
	} else {
		du4 = dv4 = 0;
	}
#endif

	//senquack - TODO: why is it always going through 2 iterations when sometimes one would suffice here?
	//			 (SAME ISSUE ELSEWHERE)
	for (s32 loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;  yb = y1;
			x3 = x4 = i2x(x0);
			u3 = i2x(u0);  v3 = i2x(v0);
			if (dx < 0) {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y2 - y0) != 0) {
					float finv = FloatInv(y2 - y0);
					dx3 = (fixed)(((x2 - x0) << FIXED_BITS) * finv);
					du3 = (fixed)(((u2 - u0) << FIXED_BITS) * finv);
					dv3 = (fixed)(((v2 - v0) << FIXED_BITS) * finv);
				} else {
					dx3 = du3 = dv3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) * FloatInv(y1 - y0)) : 0;
#else
				if ((y2 - y0) != 0) {
					float fdiv = y2 - y0;
					dx3 = (fixed)(((x2 - x0) << FIXED_BITS) / fdiv);
					du3 = (fixed)(((u2 - u0) << FIXED_BITS) / fdiv);
					dv3 = (fixed)(((v2 - v0) << FIXED_BITS) / fdiv);
				} else {
					dx3 = du3 = dv3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) / (float)(y1 - y0)) : 0;
#endif
#else
				if ((y2 - y0) != 0) {
					dx3 = GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0));
					du3 = GPU_FAST_DIV((u2 - u0) << FIXED_BITS, (y2 - y0));
					dv3 = GPU_FAST_DIV((v2 - v0) << FIXED_BITS, (y2 - y0));
				} else {
					dx3 = du3 = dv3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0)) : 0;
#endif
			} else {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y1 - y0) != 0) {
					float finv = FloatInv(y1 - y0);
					dx3 = (fixed)(((x1 - x0) << FIXED_BITS) * finv);
					du3 = (fixed)(((u1 - u0) << FIXED_BITS) * finv);
					dv3 = (fixed)(((v1 - v0) << FIXED_BITS) * finv);
				} else {
					dx3 = du3 = dv3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) * FloatInv(y2 - y0)) : 0;
#else
				if ((y1 - y0) != 0) {
					float fdiv = y1 - y0;
					dx3 = (fixed)(((x1 - x0) << FIXED_BITS) / fdiv);
					du3 = (fixed)(((u1 - u0) << FIXED_BITS) / fdiv);
					dv3 = (fixed)(((v1 - v0) << FIXED_BITS) / fdiv);
				} else {
					dx3 = du3 = dv3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) / (float)(y2 - y0)) : 0;
#endif
#else
				if ((y1 - y0) != 0) {
					dx3 = GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0));
					du3 = GPU_FAST_DIV((u1 - u0) << FIXED_BITS, (y1 - y0));
					dv3 = GPU_FAST_DIV((v1 - v0) << FIXED_BITS, (y1 - y0));
				} else {
					dx3 = du3 = dv3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0)) : 0;
#endif
			}
		} else {
			ya = y1;  yb = y2;
			if (dx < 0) {
				x3 = i2x(x0);
				x4 = i2x(x1);
				u3 = i2x(u0);
				v3 = i2x(v0);
				if ((y1 - y0) != 0) {
					x3 += (dx3 * (y1 - y0));
					u3 += (du3 * (y1 - y0));
					v3 += (dv3 * (y1 - y0));
				}
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) * FloatInv(y2 - y1)) : 0;
#else
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) / (float)(y2 - y1)) : 0;
#endif
#else
				dx4 = ((y2 - y1) != 0) ? GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1)) : 0;
#endif
			} else {
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));
				u3 = i2x(u1);
				v3 = i2x(v1);
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y2 - y1) != 0) {
					float finv = FloatInv(y2 - y1);
					dx3 = (fixed)(((x2 - x1) << FIXED_BITS) * finv);
					du3 = (fixed)(((u2 - u1) << FIXED_BITS) * finv);
					dv3 = (fixed)(((v2 - v1) << FIXED_BITS) * finv);
				} else {
					dx3 = du3 = dv3 = 0;
				}
#else
				if ((y2 - y1) != 0) {
					float fdiv = y2 - y1;
					dx3 = (fixed)(((x2 - x1) << FIXED_BITS) / fdiv);
					du3 = (fixed)(((u2 - u1) << FIXED_BITS) / fdiv);
					dv3 = (fixed)(((v2 - v1) << FIXED_BITS) / fdiv);
				} else {
					dx3 = du3 = dv3 = 0;
				}
#endif
#else 
				if ((y2 - y1) != 0) {
					dx3 = GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1));
					du3 = GPU_FAST_DIV((u2 - u1) << FIXED_BITS, (y2 - y1));
					dv3 = GPU_FAST_DIV((v2 - v1) << FIXED_BITS, (y2 - y1));
				} else {
					dx3 = du3 = dv3 = 0;
				}
#endif
			}
		}
		if ((ymin - ya) > 0) {
			x3 += dx3 * (ymin - ya);
			x4 += dx4 * (ymin - ya);
			u3 += du3 * (ymin - ya);
			v3 += dv3 * (ymin - ya);
			ya = ymin;
		}

		if (yb > ymax) yb = ymax;

		int loop1 = yb - ya;
		u16* PixelBase;
		if (loop1 > 0)
			PixelBase = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		else
			loop1 = 0;

		for (; loop1; --loop1, ++ya, PixelBase += FRAME_WIDTH,
				x3 += dx3, x4 += dx4,
				u3 += du3, v3 += dv3 )
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;

			xa = FixedCeilToInt(x3);  xb = FixedCeilToInt(x4);
			u4 = u3;  v4 = v3;

			fixed itmp = i2x(xa) - x3;
			if (itmp != 0) {
				u4 += (du4 * itmp) >> FIXED_BITS;
				v4 += (dv4 * itmp) >> FIXED_BITS;
			}

			u4 += fixed_HALF;  v4 += fixed_HALF;

			if ((xmin - xa) > 0) {
				u4 += du4 * (xmin - xa);
				v4 += dv4 * (xmin - xa);
				xa = xmin;
			}

			if (xb > xmax) xb = xmax;
			if ((xb - xa) > 0) gpuPolySpanDriver(PixelBase + xa, (xb - xa));
		}
	}
}

/*----------------------------------------------------------------------
G3 - Gouraud-shaded, untextured triangle
----------------------------------------------------------------------*/

void gpuDrawG3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	s32 y0, y1, y2;
	s32 r0, r1, r2, r3, dr3;
	s32 g0, g1, g2, g3, dg3;
	s32 b0, b1, b2, b3, db3;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[6] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[7] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[10]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[11]);
	GPU_ADJUSTCOORD3();
	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];  x1 += DrawingOffset[0];  x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];  y1 += DrawingOffset[1];  y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	//senquack - This is left in from original UNAI code, but wasn't in DrHell:
	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	r0 = PacketBuffer.U1[0];   g0 = PacketBuffer.U1[1];   b0 = PacketBuffer.U1[2];
	r1 = PacketBuffer.U1[8];   g1 = PacketBuffer.U1[9];   b1 = PacketBuffer.U1[10];
	r2 = PacketBuffer.U1[16];  g2 = PacketBuffer.U1[17];  b2 = PacketBuffer.U1[18];

	if (y0 > y1) {
		SwapValues(x0, x1); SwapValues(y0, y1);
		SwapValues(r0, r1); SwapValues(g0, g1); SwapValues(b0, b1);
	}
	if (y1 > y2) {
		SwapValues(x1, x2); SwapValues(y1, y2);
		SwapValues(r1, r2); SwapValues(g1, g2); SwapValues(b1, b2);
	}
	if (y0 > y1) {
		SwapValues(x0, x1); SwapValues(y0, y1);
		SwapValues(r0, r1); SwapValues(g0, g1); SwapValues(b0, b1);
	}

	ya = y2 - y0;
	yb = y2 - y1;
	dx4 = (x2 - x1) * ya - (x2 - x0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;
	dx = dx4;
	if (dx4 < 0) {
		dx4 = -dx4;
		dr4 = -dr4;
		dg4 = -dg4;
		db4 = -db4;
	}

#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
	if (dx4 != 0) {
		float finv = FloatInv(dx4);
		dr4 = (fixed)((dr4 << FIXED_BITS) * finv);
		dg4 = (fixed)((dg4 << FIXED_BITS) * finv);
		db4 = (fixed)((db4 << FIXED_BITS) * finv);
	} else {
		dr4 = dg4 = db4 = 0;
	}
#else
	if (dx4 != 0) {
		float fdiv = dx4;
		dr4 = (fixed)((dr4 << FIXED_BITS) / fdiv);
		dg4 = (fixed)((dg4 << FIXED_BITS) / fdiv);
		db4 = (fixed)((db4 << FIXED_BITS) / fdiv);
	} else {
		dr4 = dg4 = db4 = 0;
	}
#endif
#else
	if (dx4 != 0) {
		dr4 = GPU_FAST_DIV(dr4 << FIXED_BITS, dx4);
		dg4 = GPU_FAST_DIV(dg4 << FIXED_BITS, dx4);
		db4 = GPU_FAST_DIV(db4 << FIXED_BITS, dx4);
	} else {
		dr4 = dg4 = db4 = 0;
	}
#endif

	// Adapted old Unai code: (New routines use 22.10 fixed point, Unai 16.16)
	// Used by gouraud versions of gpuPolySpanDriver() to increment all three at once:
	u32 dr = (u32)(dr4<< 14)&(0xffffffff<<21);   if(dr4<0) dr+= 1<<21;
	u32 dg = (u32)(dg4<<  3)&(0xffffffff<<10);   if(dg4<0) dg+= 1<<10;
	u32 db = (u32)(db4>>  8)&(0xffffffff    );   if(db4<0) db+= 1<< 0;
	lInc = db + dg + dr;

	for (s32 loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;
			yb = y1;
			x3 = x4 = i2x(x0);
			r3 = i2x(r0);
			g3 = i2x(g0);
			b3 = i2x(b0);
			if (dx < 0) {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y2 - y0) != 0) {
					float finv = FloatInv(y2 - y0);
					dx3 = (fixed)(((x2 - x0) << FIXED_BITS) * finv);
					dr3 = (fixed)(((r2 - r0) << FIXED_BITS) * finv);
					dg3 = (fixed)(((g2 - g0) << FIXED_BITS) * finv);
					db3 = (fixed)(((b2 - b0) << FIXED_BITS) * finv);
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) * FloatInv(y1 - y0)) : 0;
#else
				if ((y2 - y0) != 0) {
					float fdiv = y2 - y0;
					dx3 = (fixed)(((x2 - x0) << FIXED_BITS) / fdiv);
					dr3 = (fixed)(((r2 - r0) << FIXED_BITS) / fdiv);
					dg3 = (fixed)(((g2 - g0) << FIXED_BITS) / fdiv);
					db3 = (fixed)(((b2 - b0) << FIXED_BITS) / fdiv);
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) / (float)(y1 - y0)) : 0;
#endif
#else
				if ((y2 - y0) != 0) {
					dx3 = GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0));
					dr3 = GPU_FAST_DIV((r2 - r0) << FIXED_BITS, (y2 - y0));
					dg3 = GPU_FAST_DIV((g2 - g0) << FIXED_BITS, (y2 - y0));
					db3 = GPU_FAST_DIV((b2 - b0) << FIXED_BITS, (y2 - y0));
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0)) : 0;
#endif
			} else {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y1 - y0) != 0) {
					float finv = FloatInv(y1 - y0);
					dx3 = (fixed)(((x1 - x0) << FIXED_BITS) * finv);
					dr3 = (fixed)(((r1 - r0) << FIXED_BITS) * finv);
					dg3 = (fixed)(((g1 - g0) << FIXED_BITS) * finv);
					db3 = (fixed)(((b1 - b0) << FIXED_BITS) * finv);
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) * FloatInv(y2 - y0)) : 0;
#else
				if ((y1 - y0) != 0) {
					float fdiv = y1 - y0;
					dx3 = (fixed)(((x1 - x0) << FIXED_BITS) / fdiv);
					dr3 = (fixed)(((r1 - r0) << FIXED_BITS) / fdiv);
					dg3 = (fixed)(((g1 - g0) << FIXED_BITS) / fdiv);
					db3 = (fixed)(((b1 - b0) << FIXED_BITS) / fdiv);
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) / (float)(y2 - y0)) : 0;
#endif
#else
				if ((y1 - y0) != 0) {
					dx3 = GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0));
					dr3 = GPU_FAST_DIV((r1 - r0) << FIXED_BITS, (y1 - y0));
					dg3 = GPU_FAST_DIV((g1 - g0) << FIXED_BITS, (y1 - y0));
					db3 = GPU_FAST_DIV((b1 - b0) << FIXED_BITS, (y1 - y0));
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0)) : 0;
#endif
			}
		} else {
			ya = y1;  yb = y2;
			if (dx < 0) {
				x3 = i2x(x0);  x4 = i2x(x1);
				r3 = i2x(r0);  g3 = i2x(g0);  b3 = i2x(b0);

				if ((y1 - y0) != 0) {
					x3 += (dx3 * (y1 - y0));
					r3 += (dr3 * (y1 - y0));
					g3 += (dg3 * (y1 - y0));
					b3 += (db3 * (y1 - y0));
				}

#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) * FloatInv(y2 - y1)) : 0;
#else
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) / (float)(y2 - y1)) : 0;
#endif
#else
				dx4 = ((y2 - y1) != 0) ? GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1)) : 0;
#endif
			} else {
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));

				r3 = i2x(r1);  g3 = i2x(g1);  b3 = i2x(b1);

#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y2 - y1) != 0) {
					float finv = FloatInv(y2 - y1);
					dx3 = (fixed)(((x2 - x1) << FIXED_BITS) * finv);
					dr3 = (fixed)(((r2 - r1) << FIXED_BITS) * finv);
					dg3 = (fixed)(((g2 - g1) << FIXED_BITS) * finv);
					db3 = (fixed)(((b2 - b1) << FIXED_BITS) * finv);
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
#else
				if ((y2 - y1) != 0) {
					float fdiv = y2 - y1;
					dx3 = (fixed)(((x2 - x1) << FIXED_BITS) / fdiv);
					dr3 = (fixed)(((r2 - r1) << FIXED_BITS) / fdiv);
					dg3 = (fixed)(((g2 - g1) << FIXED_BITS) / fdiv);
					db3 = (fixed)(((b2 - b1) << FIXED_BITS) / fdiv);
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}

#endif
#else
				if ((y2 - y1) != 0) {
					dx3 = GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1));
					dr3 = GPU_FAST_DIV((r2 - r1) << FIXED_BITS, (y2 - y1));
					dg3 = GPU_FAST_DIV((g2 - g1) << FIXED_BITS, (y2 - y1));
					db3 = GPU_FAST_DIV((b2 - b1) << FIXED_BITS, (y2 - y1));
				} else {
					dx3 = dr3 = dg3 = db3 = 0;
				}
#endif
			}
		}
		if ((ymin - ya) > 0) {
			x3 += (dx3 * (ymin - ya));
			x4 += (dx4 * (ymin - ya));
			r3 += (dr3 * (ymin - ya));
			g3 += (dg3 * (ymin - ya));
			b3 += (db3 * (ymin - ya));
			ya = ymin;
		}

		if (yb > ymax) yb = ymax;

		int loop1 = yb - ya;
		u16* PixelBase;
		if (loop1 > 0)
			PixelBase = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		else
			loop1 = 0;

		for (; loop1; --loop1, ++ya, PixelBase += FRAME_WIDTH,
				x3 += dx3, x4 += dx4,
				r3 += dr3, g3 += dg3, b3 += db3 )
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;

			xa = FixedCeilToInt(x3);  xb = FixedCeilToInt(x4);
			r4 = r3;  g4 = g3;  b4 = b3;

			fixed itmp = i2x(xa) - x3;
			if (itmp != 0) {
				r4 += (dr4 * itmp) >> FIXED_BITS;
				g4 += (dg4 * itmp) >> FIXED_BITS;
				b4 += (db4 * itmp) >> FIXED_BITS;
			}

			r4 += fixed_HALF;  g4 += fixed_HALF;  b4 += fixed_HALF;

			if ((xmin - xa) > 0) {
				r4 += (dr4 * (xmin - xa));
				g4 += (dg4 * (xmin - xa));
				b4 += (db4 * (xmin - xa));
				xa = xmin;
			}

			if (xb > xmax) xb = xmax;
			if ((xb - xa) > 0) gpuPolySpanDriver(PixelBase + xa, (xb - xa));
		}
	}
}

/*----------------------------------------------------------------------
GT3 - Gouraud-shaded, textured triangle
----------------------------------------------------------------------*/

void gpuDrawGT3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	s32 y0, y1, y2;
	s32 u0, u1, u2, u3, du3;
	s32 v0, v1, v2, v3, dv3;
	s32 r0, r1, r2, r3, dr3;
	s32 g0, g1, g2, g3, dg3;
	s32 b0, b1, b2, b3, db3;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[8] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[9] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[14]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[15]);
	GPU_ADJUSTCOORD3();
	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];  x1 += DrawingOffset[0];  x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];  y1 += DrawingOffset[1];  y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	//senquack - This is left in from original UNAI code, but wasn't in DrHell:
	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}

	r0 = PacketBuffer.U1[0];   g0 = PacketBuffer.U1[1];   b0 = PacketBuffer.U1[2];
	u0 = PacketBuffer.U1[8];   v0 = PacketBuffer.U1[9];
	r1 = PacketBuffer.U1[12];  g1 = PacketBuffer.U1[13];  b1 = PacketBuffer.U1[14];
	u1 = PacketBuffer.U1[20];  v1 = PacketBuffer.U1[21];
	r2 = PacketBuffer.U1[24];  g2 = PacketBuffer.U1[25];  b2 = PacketBuffer.U1[26];
	u2 = PacketBuffer.U1[32];  v2 = PacketBuffer.U1[33];

	if (y0 > y1) {
		SwapValues(x0, x1); SwapValues(y0, y1);
		SwapValues(u0, u1); SwapValues(v0, v1);
		SwapValues(r0, r1); SwapValues(g0, g1); SwapValues(b0, b1);
	}
	if (y1 > y2) {
		SwapValues(x1, x2); SwapValues(y1, y2);
		SwapValues(u1, u2); SwapValues(v1, v2);
		SwapValues(r1, r2); SwapValues(g1, g2); SwapValues(b1, b2);
	}
	if (y0 > y1) {
		SwapValues(x0, x1); SwapValues(y0, y1);
		SwapValues(u0, u1); SwapValues(v0, v1);
		SwapValues(r0, r1); SwapValues(g0, g1); SwapValues(b0, b1);
	}

	ya = y2 - y0;
	yb = y2 - y1;
	dx4 = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;
	dx = dx4;
	if (dx4 < 0) {
		dx4 = -dx4;
		du4 = -du4;
		dv4 = -dv4;
		dr4 = -dr4;
		dg4 = -dg4;
		db4 = -db4;
	}

#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
	if (dx4 != 0) {
		float finv = FloatInv(dx4);
		du4 = (fixed)((du4 << FIXED_BITS) * finv);
		dv4 = (fixed)((dv4 << FIXED_BITS) * finv);
		dr4 = (fixed)((dr4 << FIXED_BITS) * finv);
		dg4 = (fixed)((dg4 << FIXED_BITS) * finv);
		db4 = (fixed)((db4 << FIXED_BITS) * finv);
	} else {
		du4 = dv4 = dr4 = dg4 = db4 = 0;
	}
#else
	if (dx4 != 0) {
		float fdiv = dx4;
		du4 = (fixed)((du4 << FIXED_BITS) / fdiv);
		dv4 = (fixed)((dv4 << FIXED_BITS) / fdiv);
		dr4 = (fixed)((dr4 << FIXED_BITS) / fdiv);
		dg4 = (fixed)((dg4 << FIXED_BITS) / fdiv);
		db4 = (fixed)((db4 << FIXED_BITS) / fdiv);
	} else {
		du4 = dv4 = dr4 = dg4 = db4 = 0;
	}
#endif
#else
	if (dx4 != 0) {
		du4 = GPU_FAST_DIV(du4 << FIXED_BITS, dx4);
		dv4 = GPU_FAST_DIV(dv4 << FIXED_BITS, dx4);
		dr4 = GPU_FAST_DIV(dr4 << FIXED_BITS, dx4);
		dg4 = GPU_FAST_DIV(dg4 << FIXED_BITS, dx4);
		db4 = GPU_FAST_DIV(db4 << FIXED_BITS, dx4);
	} else {
		du4 = dv4 = dr4 = dg4 = db4 = 0;
	}
#endif

	// Adapted old Unai code: (New routines use 22.10 fixed point, Unai 16.16)
	// Used by gouraud versions of gpuPolySpanDriver() to increment all three at once:
	u32 dr = (u32)(dr4<< 14)&(0xffffffff<<21);   if(dr4<0) dr+= 1<<21;
	u32 dg = (u32)(dg4<<  3)&(0xffffffff<<10);   if(dg4<0) dg+= 1<<10;
	u32 db = (u32)(db4>>  8)&(0xffffffff    );   if(db4<0) db+= 1<< 0;
	lInc = db + dg + dr;

	for (s32 loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;  yb = y1;
			x3 = x4 = i2x(x0);
			u3 = i2x(u0);  v3 = i2x(v0);
			r3 = i2x(r0);  g3 = i2x(g0);  b3 = i2x(b0);
			if (dx < 0) {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y2 - y0) != 0) {
					float finv = FloatInv(y2 - y0);
					dx3 = (fixed)(((x2 - x0) << FIXED_BITS) * finv);
					du3 = (fixed)(((u2 - u0) << FIXED_BITS) * finv);
					dv3 = (fixed)(((v2 - v0) << FIXED_BITS) * finv);
					dr3 = (fixed)(((r2 - r0) << FIXED_BITS) * finv);
					dg3 = (fixed)(((g2 - g0) << FIXED_BITS) * finv);
					db3 = (fixed)(((b2 - b0) << FIXED_BITS) * finv);
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) * FloatInv(y1 - y0)) : 0;
#else
				if ((y2 - y0) != 0) {
					float fdiv = y2 - y0;
					dx3 = (fixed)(((x2 - x0) << FIXED_BITS) / fdiv);
					du3 = (fixed)(((u2 - u0) << FIXED_BITS) / fdiv);
					dv3 = (fixed)(((v2 - v0) << FIXED_BITS) / fdiv);
					dr3 = (fixed)(((r2 - r0) << FIXED_BITS) / fdiv);
					dg3 = (fixed)(((g2 - g0) << FIXED_BITS) / fdiv);
					db3 = (fixed)(((b2 - b0) << FIXED_BITS) / fdiv);
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? (fixed)(((x1 - x0) << FIXED_BITS) / (float)(y1 - y0)) : 0;
#endif
#else
				if ((y2 - y0) != 0) {
					dx3 = GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0));
					du3 = GPU_FAST_DIV((u2 - u0) << FIXED_BITS, (y2 - y0));
					dv3 = GPU_FAST_DIV((v2 - v0) << FIXED_BITS, (y2 - y0));
					dr3 = GPU_FAST_DIV((r2 - r0) << FIXED_BITS, (y2 - y0));
					dg3 = GPU_FAST_DIV((g2 - g0) << FIXED_BITS, (y2 - y0));
					db3 = GPU_FAST_DIV((b2 - b0) << FIXED_BITS, (y2 - y0));
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y1 - y0) != 0) ? GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0)) : 0;
#endif
			} else {
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y1 - y0) != 0) {
					float finv = FloatInv(y1 - y0);
					dx3 = (fixed)(((x1 - x0) << FIXED_BITS) * finv);
					du3 = (fixed)(((u1 - u0) << FIXED_BITS) * finv);
					dv3 = (fixed)(((v1 - v0) << FIXED_BITS) * finv);
					dr3 = (fixed)(((r1 - r0) << FIXED_BITS) * finv);
					dg3 = (fixed)(((g1 - g0) << FIXED_BITS) * finv);
					db3 = (fixed)(((b1 - b0) << FIXED_BITS) * finv);
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) * FloatInv(y2 - y0)) : 0;
#else
				if ((y1 - y0) != 0) {
					float fdiv = y1 - y0;
					dx3 = (fixed)(((x1 - x0) << FIXED_BITS) / fdiv);
					du3 = (fixed)(((u1 - u0) << FIXED_BITS) / fdiv);
					dv3 = (fixed)(((v1 - v0) << FIXED_BITS) / fdiv);
					dr3 = (fixed)(((r1 - r0) << FIXED_BITS) / fdiv);
					dg3 = (fixed)(((g1 - g0) << FIXED_BITS) / fdiv);
					db3 = (fixed)(((b1 - b0) << FIXED_BITS) / fdiv);
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? (fixed)(((x2 - x0) << FIXED_BITS) / float(y2 - y0)) : 0;
#endif

#else
				if ((y1 - y0) != 0) {
					dx3 = GPU_FAST_DIV((x1 - x0) << FIXED_BITS, (y1 - y0));
					du3 = GPU_FAST_DIV((u1 - u0) << FIXED_BITS, (y1 - y0));
					dv3 = GPU_FAST_DIV((v1 - v0) << FIXED_BITS, (y1 - y0));
					dr3 = GPU_FAST_DIV((r1 - r0) << FIXED_BITS, (y1 - y0));
					dg3 = GPU_FAST_DIV((g1 - g0) << FIXED_BITS, (y1 - y0));
					db3 = GPU_FAST_DIV((b1 - b0) << FIXED_BITS, (y1 - y0));
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
				dx4 = ((y2 - y0) != 0) ? GPU_FAST_DIV((x2 - x0) << FIXED_BITS, (y2 - y0)) : 0;
#endif
			}
		} else {
			ya = y1;  yb = y2;
			if (dx < 0) {
				x3 = i2x(x0);  x4 = i2x(x1);
				u3 = i2x(u0);  v3 = i2x(v0);
				r3 = i2x(r0);  g3 = i2x(g0);  b3 = i2x(b0);

				if ((y1 - y0) != 0) {
					x3 += (dx3 * (y1 - y0));
					u3 += (du3 * (y1 - y0));
					v3 += (dv3 * (y1 - y0));
					r3 += (dr3 * (y1 - y0));
					g3 += (dg3 * (y1 - y0));
					b3 += (db3 * (y1 - y0));
				}

#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) * FloatInv(y2 - y1)) : 0;
#else
				dx4 = ((y2 - y1) != 0) ? (fixed)(((x2 - x1) << FIXED_BITS) / (float)(y2 - y1)) : 0;
#endif
#else
				dx4 = ((y2 - y1) != 0) ? GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1)) : 0;
#endif
			} else {
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));

				u3 = i2x(u1);  v3 = i2x(v1);
				r3 = i2x(r1);  g3 = i2x(g1);  b3 = i2x(b1);
#ifdef GPU_UNAI_USE_FLOATMATH
#ifdef GPU_UNAI_USE_FLOAT_DIV_MULTINV
				if ((y2 - y1) != 0) {
					float finv = FloatInv(y2 - y1);
					dx3 = (fixed)(((x2 - x1) << FIXED_BITS) * finv);
					du3 = (fixed)(((u2 - u1) << FIXED_BITS) * finv);
					dv3 = (fixed)(((v2 - v1) << FIXED_BITS) * finv);
					dr3 = (fixed)(((r2 - r1) << FIXED_BITS) * finv);
					dg3 = (fixed)(((g2 - g1) << FIXED_BITS) * finv);
					db3 = (fixed)(((b2 - b1) << FIXED_BITS) * finv);
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
#else
				if ((y2 - y1) != 0) {
					float fdiv = y2 - y1;
					dx3 = (fixed)(((x2 - x1) << FIXED_BITS) / fdiv);
					du3 = (fixed)(((u2 - u1) << FIXED_BITS) / fdiv);
					dv3 = (fixed)(((v2 - v1) << FIXED_BITS) / fdiv);
					dr3 = (fixed)(((r2 - r1) << FIXED_BITS) / fdiv);
					dg3 = (fixed)(((g2 - g1) << FIXED_BITS) / fdiv);
					db3 = (fixed)(((b2 - b1) << FIXED_BITS) / fdiv);
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
#endif

#else
				if ((y2 - y1) != 0) {
					dx3 = GPU_FAST_DIV((x2 - x1) << FIXED_BITS, (y2 - y1));
					du3 = GPU_FAST_DIV((u2 - u1) << FIXED_BITS, (y2 - y1));
					dv3 = GPU_FAST_DIV((v2 - v1) << FIXED_BITS, (y2 - y1));
					dr3 = GPU_FAST_DIV((r2 - r1) << FIXED_BITS, (y2 - y1));
					dg3 = GPU_FAST_DIV((g2 - g1) << FIXED_BITS, (y2 - y1));
					db3 = GPU_FAST_DIV((b2 - b1) << FIXED_BITS, (y2 - y1));
				} else {
					dx3 = du3 = dv3 = dr3 = dg3 = db3 = 0;
				}
#endif
			}
		}

		if ((ymin - ya) > 0) {
			x3 += (dx3 * (ymin - ya));
			x4 += (dx4 * (ymin - ya));
			u3 += (du3 * (ymin - ya));
			v3 += (dv3 * (ymin - ya));
			r3 += (dr3 * (ymin - ya));
			g3 += (dg3 * (ymin - ya));
			b3 += (db3 * (ymin - ya));
			ya = ymin;
		}

		if (yb > ymax) yb = ymax;

		int loop1 = yb - ya;
		u16* PixelBase;
		if (loop1 > 0)
			PixelBase = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		else
			loop1 = 0;

		for (; loop1; --loop1, ++ya, PixelBase += FRAME_WIDTH,
				x3 += dx3, x4 += dx4,
				u3 += du3, v3 += dv3,
				r3 += dr3, g3 += dg3, b3 += db3 )
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;

			xa = FixedCeilToInt(x3);  xb = FixedCeilToInt(x4);
			u4 = u3;  v4 = v3;
			r4 = r3;  g4 = g3;  b4 = b3;

			fixed itmp = i2x(xa) - x3;
			if (itmp != 0) {
				u4 += (du4 * itmp) >> FIXED_BITS;
				v4 += (dv4 * itmp) >> FIXED_BITS;
				r4 += (dr4 * itmp) >> FIXED_BITS;
				g4 += (dg4 * itmp) >> FIXED_BITS;
				b4 += (db4 * itmp) >> FIXED_BITS;
			}

			u4 += fixed_HALF;  v4 += fixed_HALF;
			r4 += fixed_HALF;  g4 += fixed_HALF;  b4 += fixed_HALF;

			if ((xmin - xa) > 0) {
				u4 += du4 * (xmin - xa);
				v4 += dv4 * (xmin - xa);
				r4 += dr4 * (xmin - xa);
				g4 += dg4 * (xmin - xa);
				b4 += db4 * (xmin - xa);
				xa = xmin;
			}

			if (xb > xmax) xb = xmax;
			if ((xb - xa) > 0) gpuPolySpanDriver(PixelBase + xa, (xb - xa));
		}
	}
}




//////////////////////////////////////////////////////////////////////////
//senquack - Disabled original Unai poly routines left here for reference:
//////////////////////////////////////////////////////////////////////////
#if 0
/*----------------------------------------------------------------------
F3
----------------------------------------------------------------------*/

void gpuDrawF3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2]);
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3]);
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[4]);
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[5]);
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[6]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[7]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	PixelData = GPU_RGB16(PacketBuffer.U4[0]);

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);
			GPU_SWAP(y1, y2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
		}
	}

	ya = y2 - y0;
	yb = y2 - y1;
	dx =(x2 - x1) * ya - (x2 - x0) * yb;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				dx3 = xLoDivx((x2 - x0), (y2 - y0));
				dx4 = xLoDivx((x1 - x0), (y1 - y0));
			}
			else
			{
				dx3 = xLoDivx((x1 - x0), (y1 - y0));
				dx4 = xLoDivx((x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				x4  = i2x(x1);
				x3  = i2x(x0) + (dx3 * (y1 - y0));
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				x3  = i2x(x1);
				x4  = i2x(x0) + (dx4 * (y1 - y0));
				dx3 = xLoDivx((x2 - x1), (y2 - y1));
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;
			x4 += dx4*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;
		x4+= fixed_HALF;

		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		
		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin) ) continue;
			if(xa < xmin) xa = xmin;
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}

/*----------------------------------------------------------------------
FT3
----------------------------------------------------------------------*/

void gpuDrawFT3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;
	s32 u0, u1, u2, u3, du3=0;
	s32 v0, v1, v2, v3, dv3=0;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[6] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[7] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[10]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[11]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	u0 = PacketBuffer.U1[8];  v0 = PacketBuffer.U1[9];
	u1 = PacketBuffer.U1[16]; v1 = PacketBuffer.U1[17];
	u2 = PacketBuffer.U1[24]; v2 = PacketBuffer.U1[25];

	r4 = s32(PacketBuffer.U1[0]);
	g4 = s32(PacketBuffer.U1[1]);
	b4 = s32(PacketBuffer.U1[2]);
	dr4 = dg4 = db4 = 0;

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);
			GPU_SWAP(v0, v1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);
			GPU_SWAP(y1, y2, temp);
			GPU_SWAP(u1, u2, temp);
			GPU_SWAP(v1, v2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);
			GPU_SWAP(v0, v1, temp);
		}
	}

	ya  = y2 - y0;
	yb  = y2 - y1;
	dx  = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;

	s32 iF,iS;
	xInv( dx, iF, iS);
	du4 = xInvMulx( du4, iF, iS);
	dv4 = xInvMulx( dv4, iF, iS);
	tInc = ((u32)(du4<<7)&0x7fff0000) | ((u32)(dv4>>9)&0x00007fff);
	tMsk = (TextureWindow[2]<<23) | (TextureWindow[3]<<7) | 0x00ff00ff;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			u3 = i2x(u0);
			v3 = i2x(v0);
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				xInv( (y2 - y0), iF, iS);
				dx3 = xInvMulx( (x2 - x0), iF, iS);
				du3 = xInvMulx( (u2 - u0), iF, iS);
				dv3 = xInvMulx( (v2 - v0), iF, iS);
				dx4 = xLoDivx ( (x1 - x0), (y1 - y0));
			}
			else
			{
				xInv( (y1 - y0), iF, iS);
				dx3 = xInvMulx( (x1 - x0), iF, iS);
				du3 = xInvMulx( (u1 - u0), iF, iS);
				dv3 = xInvMulx( (v1 - v0), iF, iS);
				dx4 = xLoDivx ( (x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				temp = y1 - y0;
				u3 = i2x(u0) + (du3 * temp);
				v3 = i2x(v0) + (dv3 * temp);
				x3 = i2x(x0) + (dx3 * temp);
				x4 = i2x(x1);
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				u3 = i2x(u1);
				v3 = i2x(v1);
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));
				xInv( (y2 - y1), iF, iS);
				dx3 = xInvMulx( (x2 - x1), iF, iS);
				du3 = xInvMulx( (u2 - u1), iF, iS);
				dv3 = xInvMulx( (v2 - v1), iF, iS);
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;
			x4 += dx4*temp;
			u3 += du3*temp;
			v3 += dv3*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;
		x4+= fixed_HALF;
		u3+= fixed_HALF;
		v4+= fixed_HALF;

		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];

		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4, u3+=du3, v3+=dv3)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin) ) continue;

			temp = xmin - xa;
			if(temp > 0)
			{
				xa  = xmin;
				u4 = u3 + du4*temp;
				v4 = v3 + dv4*temp;
			}
			else
			{
				u4 = u3;
				v4 = v3;
			}
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}

/*----------------------------------------------------------------------
G3
----------------------------------------------------------------------*/

void gpuDrawG3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;
	s32 r0, r1, r2, r3, dr3=0;
	s32 g0, g1, g2, g3, dg3=0;
	s32 b0, b1, b2, b3, db3=0;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[6] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[7] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[10]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[11]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];  xmax = DrawingArea[2];
	ymin = DrawingArea[1];  ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}
	
	r0 = PacketBuffer.U1[0];	g0 = PacketBuffer.U1[1];	b0 = PacketBuffer.U1[2];
	r1 = PacketBuffer.U1[8];	g1 = PacketBuffer.U1[9];	b1 = PacketBuffer.U1[10];
	r2 = PacketBuffer.U1[16];	g2 = PacketBuffer.U1[17];	b2 = PacketBuffer.U1[18];

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(r0, r1, temp);		GPU_SWAP(g0, g1, temp);		GPU_SWAP(b0, b1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);		GPU_SWAP(y1, y2, temp);
			GPU_SWAP(r1, r2, temp);		GPU_SWAP(g1, g2, temp);   GPU_SWAP(b1, b2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(r0, r1, temp);   GPU_SWAP(g0, g1, temp);		GPU_SWAP(b0, b1, temp);
		}
	}

	ya  = y2 - y0;
	yb  = y2 - y1;
	dx  = (x2 - x1) * ya - (x2 - x0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;

	s32 iF,iS;
	xInv(            dx, iF, iS);
	dr4 = xInvMulx( dr4, iF, iS);
	dg4 = xInvMulx( dg4, iF, iS);
	db4 = xInvMulx( db4, iF, iS);
	u32 dr = (u32)(dr4<< 8)&(0xffffffff<<21);   if(dr4<0) dr+= 1<<21;
	u32 dg = (u32)(dg4>> 3)&(0xffffffff<<10);   if(dg4<0) dg+= 1<<10;
	u32 db = (u32)(db4>>14)&(0xffffffff    );   if(db4<0) db+= 1<< 0;
	lInc = db + dg + dr;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			r3 = i2x(r0);
			g3 = i2x(g0);
			b3 = i2x(b0);
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				xInv(           (y2 - y0), iF, iS);
				dx3 = xInvMulx( (x2 - x0), iF, iS);
				dr3 = xInvMulx( (r2 - r0), iF, iS);
				dg3 = xInvMulx( (g2 - g0), iF, iS);
				db3 = xInvMulx( (b2 - b0), iF, iS);
				dx4 = xLoDivx ( (x1 - x0), (y1 - y0));
			}
			else
			{
				xInv(           (y1 - y0), iF, iS);
				dx3 = xInvMulx( (x1 - x0), iF, iS);
				dr3 = xInvMulx( (r1 - r0), iF, iS);
				dg3 = xInvMulx( (g1 - g0), iF, iS);
				db3 = xInvMulx( (b1 - b0), iF, iS);
				dx4 = xLoDivx ( (x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				temp = y1 - y0;
				r3  = i2x(r0) + (dr3 * temp);
				g3  = i2x(g0) + (dg3 * temp);
				b3  = i2x(b0) + (db3 * temp);
				x3  = i2x(x0) + (dx3 * temp);
				x4  = i2x(x1);
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				r3 = i2x(r1);
				g3 = i2x(g1);
				b3 = i2x(b1);
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));

				xInv(           (y2 - y1), iF, iS);
				dx3 = xInvMulx( (x2 - x1), iF, iS);
				dr3 = xInvMulx( (r2 - r1), iF, iS);
				dg3 = xInvMulx( (g2 - g1), iF, iS);
				db3 = xInvMulx( (b2 - b1), iF, iS);
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;   x4 += dx4*temp;
			r3 += dr3*temp;   g3 += dg3*temp;   b3 += db3*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;  x4+= fixed_HALF;
		r3+= fixed_HALF;  g3+= fixed_HALF;  b3+= fixed_HALF;

		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		
		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4, r3+=dr3, g3+=dg3, b3+=db3)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin) ) continue;

			temp = xmin - xa;
			if(temp > 0)
			{
				xa  = xmin;
				r4 = r3 + dr4*temp;   g4 = g3 + dg4*temp;   b4 = b3 + db4*temp;
			}
			else
			{
				r4 = r3;  g4 = g3;  b4 = b3;
			}
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}

/*----------------------------------------------------------------------
GT3
----------------------------------------------------------------------*/

void gpuDrawGT3(const PP gpuPolySpanDriver)
{
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	s32 temp;
	s32 xa, xb, xmin, xmax;
	s32 ya, yb, ymin, ymax;
	s32 x0, x1, x2, x3, dx3=0, x4, dx4=0, dx;
	s32 y0, y1, y2;
	s32 u0, u1, u2, u3, du3=0;
	s32 v0, v1, v2, v3, dv3=0;
	s32 r0, r1, r2, r3, dr3=0;
	s32 g0, g1, g2, g3, dg3=0;
	s32 b0, b1, b2, b3, db3=0;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2] );
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3] );
	x1 = GPU_EXPANDSIGN(PacketBuffer.S2[8] );
	y1 = GPU_EXPANDSIGN(PacketBuffer.S2[9] );
	x2 = GPU_EXPANDSIGN(PacketBuffer.S2[14]);
	y2 = GPU_EXPANDSIGN(PacketBuffer.S2[15]);

	GPU_TESTRANGE3();
	
	x0 += DrawingOffset[0];   x1 += DrawingOffset[0];   x2 += DrawingOffset[0];
	y0 += DrawingOffset[1];   y1 += DrawingOffset[1];   y2 += DrawingOffset[1];

	xmin = DrawingArea[0];	xmax = DrawingArea[2];
	ymin = DrawingArea[1];	ymax = DrawingArea[3];

	{
		int rx0 = Max2(xmin,Min3(x0,x1,x2));
		int ry0 = Max2(ymin,Min3(y0,y1,y2));
		int rx1 = Min2(xmax,Max3(x0,x1,x2));
		int ry1 = Min2(ymax,Max3(y0,y1,y2));
		if( rx0>=rx1 || ry0>=ry1) return;
	}

	r0 = PacketBuffer.U1[0];	g0 = PacketBuffer.U1[1];	b0 = PacketBuffer.U1[2];
	u0 = PacketBuffer.U1[8];	v0 = PacketBuffer.U1[9];
	r1 = PacketBuffer.U1[12];	g1 = PacketBuffer.U1[13];	b1 = PacketBuffer.U1[14];
	u1 = PacketBuffer.U1[20];	v1 = PacketBuffer.U1[21];
	r2 = PacketBuffer.U1[24];	g2 = PacketBuffer.U1[25];	b2 = PacketBuffer.U1[26];
	u2 = PacketBuffer.U1[32];	v2 = PacketBuffer.U1[33];

	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);		GPU_SWAP(v0, v1, temp);
			GPU_SWAP(r0, r1, temp);		GPU_SWAP(g0, g1, temp);   GPU_SWAP(b0, b1, temp);
		}
	}
	if (y1 >= y2)
	{
		if( y1!=y2 || x1>x2 )
		{
			GPU_SWAP(x1, x2, temp);		GPU_SWAP(y1, y2, temp);
			GPU_SWAP(u1, u2, temp);		GPU_SWAP(v1, v2, temp);
			GPU_SWAP(r1, r2, temp);   GPU_SWAP(g1, g2, temp);		GPU_SWAP(b1, b2, temp);
		}
	}
	if (y0 >= y1)
	{
		if( y0!=y1 || x0>x1 )
		{
			GPU_SWAP(x0, x1, temp);		GPU_SWAP(y0, y1, temp);
			GPU_SWAP(u0, u1, temp);		GPU_SWAP(v0, v1, temp);
			GPU_SWAP(r0, r1, temp);		GPU_SWAP(g0, g1, temp);		GPU_SWAP(b0, b1, temp);
		}
	}

	ya  = y2 - y0;
	yb  = y2 - y1;
	dx  = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;

	s32 iF,iS;

	xInv(            dx, iF, iS);
	du4 = xInvMulx( du4, iF, iS);
	dv4 = xInvMulx( dv4, iF, iS);
	dr4 = xInvMulx( dr4, iF, iS);
	dg4 = xInvMulx( dg4, iF, iS);
	db4 = xInvMulx( db4, iF, iS);
	u32 dr = (u32)(dr4<< 8)&(0xffffffff<<21);   if(dr4<0) dr+= 1<<21;
	u32 dg = (u32)(dg4>> 3)&(0xffffffff<<10);   if(dg4<0) dg+= 1<<10;
	u32 db = (u32)(db4>>14)&(0xffffffff    );   if(db4<0) db+= 1<< 0;
	lInc = db + dg + dr;
	tInc = ((u32)(du4<<7)&0x7fff0000) | ((u32)(dv4>>9)&0x00007fff);
	tMsk = (TextureWindow[2]<<23) | (TextureWindow[3]<<7) | 0x00ff00ff;

	for (s32 loop0 = 2; loop0; --loop0)
	{
		if (loop0 == 2)
		{
			ya = y0;
			yb = y1;
			u3 = i2x(u0);
			v3 = i2x(v0);
			r3 = i2x(r0);
			g3 = i2x(g0);
			b3 = i2x(b0);
			x3 = i2x(x0);
			x4 = y0!=y1 ? x3 : i2x(x1);
			if (dx < 0)
			{
				xInv(           (y2 - y0), iF, iS);
				dx3 = xInvMulx( (x2 - x0), iF, iS);
				du3 = xInvMulx( (u2 - u0), iF, iS);
				dv3 = xInvMulx( (v2 - v0), iF, iS);
				dr3 = xInvMulx( (r2 - r0), iF, iS);
				dg3 = xInvMulx( (g2 - g0), iF, iS);
				db3 = xInvMulx( (b2 - b0), iF, iS);
				dx4 = xLoDivx ( (x1 - x0), (y1 - y0));
			}
			else
			{
				xInv(           (y1 - y0), iF, iS);
				dx3 = xInvMulx( (x1 - x0), iF, iS);
				du3 = xInvMulx( (u1 - u0), iF, iS);
				dv3 = xInvMulx( (v1 - v0), iF, iS);
				dr3 = xInvMulx( (r1 - r0), iF, iS);
				dg3 = xInvMulx( (g1 - g0), iF, iS);
				db3 = xInvMulx( (b1 - b0), iF, iS);
				dx4 = xLoDivx ( (x2 - x0), (y2 - y0));
			}
		}
		else
		{
			ya = y1;
			yb = y2;
			if (dx < 0)
			{
				temp = y1 - y0;
				u3  = i2x(u0) + (du3 * temp);
				v3  = i2x(v0) + (dv3 * temp);
				r3  = i2x(r0) + (dr3 * temp);
				g3  = i2x(g0) + (dg3 * temp);
				b3  = i2x(b0) + (db3 * temp);
				x3  = i2x(x0) + (dx3 * temp);
				x4  = i2x(x1);
				dx4 = xLoDivx((x2 - x1), (y2 - y1));
			}
			else
			{
				u3 = i2x(u1);
				v3 = i2x(v1);
				r3 = i2x(r1);
				g3 = i2x(g1);
				b3 = i2x(b1);
				x3 = i2x(x1);
				x4 = i2x(x0) + (dx4 * (y1 - y0));

				xInv(           (y2 - y1), iF, iS);
				dx3 = xInvMulx( (x2 - x1), iF, iS);
				du3 = xInvMulx( (u2 - u1), iF, iS);
				dv3 = xInvMulx( (v2 - v1), iF, iS);
				dr3 = xInvMulx( (r2 - r1), iF, iS);
				dg3 = xInvMulx( (g2 - g1), iF, iS);
				db3 = xInvMulx( (b2 - b1), iF, iS);
			}
		}

		temp = ymin - ya;
		if (temp > 0)
		{
			ya  = ymin;
			x3 += dx3*temp;   x4 += dx4*temp;
			u3 += du3*temp;   v3 += dv3*temp;
			r3 += dr3*temp;   g3 += dg3*temp;   b3 += db3*temp;
		}
		if (yb > ymax) yb = ymax;
		if (ya>=yb) continue;

		x3+= fixed_HALF;  x4+= fixed_HALF;
		u3+= fixed_HALF;  v4+= fixed_HALF;
		r3+= fixed_HALF;  g3+= fixed_HALF;  b3+= fixed_HALF;
		u16* PixelBase  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(0, ya)];
		
		for(;ya<yb;++ya, PixelBase += FRAME_WIDTH, x3+=dx3, x4+=dx4, u3+=du3, v3+=dv3, r3+=dr3, g3+=dg3,	b3+=db3)
		{
			if (ya&li) continue;
			if ((ya&pi)==pif) continue;
			xa = x2i(x3);
			xb = x2i(x4);
			if( (xa>xmax) || (xb<xmin))	continue;

			temp = xmin - xa;
			if(temp > 0)
			{
				xa  = xmin;
				u4 = u3 + du4*temp;   v4 = v3 + dv4*temp;
				r4 = r3 + dr4*temp;   g4 = g3 + dg4*temp;   b4 = b3 + db4*temp;
			}
			else
			{
				u4 = u3;  v4 = v3;
				r4 = r3;  g4 = g3;  b4 = b3;
			}
			if(xb > xmax) xb = xmax;
			xb-=xa;
			if(xb>0) gpuPolySpanDriver(PixelBase + xa,xb);
		}
	}
}
#endif
