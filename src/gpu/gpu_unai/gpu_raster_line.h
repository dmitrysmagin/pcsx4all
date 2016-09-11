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

///////////////////////////////////////////////////////////////////////////////
//  GPU internal line drawing functions

#define GPU_DIGITS  16
#define GPU_DIGITSC (GPU_DIGITS+3)

///////////////////////////////////////////////////////////////////////////////
void gpuDrawLF(PtrUnion packet, const PD gpuPixelDriver)
{
	s32 xmin, xmax;
	s32 ymin, ymax;
	s32 x0, x1, dx;
	s32 y0, y1, dy;

	x0 = packet.S2[2] + DrawingOffset[0];
	y0 = packet.S2[3] + DrawingOffset[1];
	x1 = packet.S2[4] + DrawingOffset[0];
	y1 = packet.S2[5] + DrawingOffset[1];

	dy = (y1 - y0);
	if (dy < 0) dy = -dy;
	dx = (x1 - x0);
	if (dx < 0) dx = -dx;

	// Maximum absolute distance between any two X coords: 1023, Y: 511
	//  (PSX hardware will not render anything violating this rule)
	if (dx >= CHKMAX_X || dy >= CHKMAX_Y)
		return;

	xmin = DrawingArea[0];	xmax = DrawingArea[2];
	ymin = DrawingArea[1];	ymax = DrawingArea[3];
	const u16 pixeldata = GPU_RGB16(packet.U4[0]);

	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	
	if (dx > dy) {
		if (x0 > x1) {
			SwapValues(x0, x1);
			SwapValues(y0, y1);
		}
		y1 = GPU_DIV((y1 - y0) << GPU_DIGITS, dx);
		y0 <<= GPU_DIGITS;
		s32 temp = xmin - x0;
		if (temp > 0) {
			x0 = xmin;
			y0 += (y1 * temp);
		}
		if (x1 > xmax) x1 = xmax;
		x1 -= x0;
		if (x1 < 0) x1 = 0;

		for (; x1; x1--) {
			s32 temp = y0 >> GPU_DIGITS;
			if ((0==(temp&li))&&((temp&pi)!=pif))  {
				if ((u32) (temp - ymin) < (u32) (ymax - ymin)) {
					gpuPixelDriver(&((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0, temp)],pixeldata);
				}
			}
			x0++;
			y0 += y1;
		}
	} else if (dy) {
		if (y0 > y1) {
			SwapValues(x0, x1);
			SwapValues(y0, y1);
		}
		x1 = GPU_DIV((x1 - x0) << GPU_DIGITS, dy);
		x0 <<= GPU_DIGITS;
		s32 temp = ymin - y0;
		if (temp > 0) {
			y0 = ymin;
			x0 += (x1 * temp);
		}
		if (y1 > ymax) y1 = ymax;
		y1 -= y0;
		if (y1 < 0) y1 = 0;
		
		for (; y1; y1--) {
			if ((0==(y0&li))&&((y0&pi)!=pif))  {
				s32 temp = x0 >> GPU_DIGITS;
				if ((u32) (temp - xmin) < (u32) (xmax - xmin)) {
					gpuPixelDriver(&((u16*)GPU_FrameBuffer)[FRAME_OFFSET(temp, y0)],pixeldata);
				}
			}
			y0++;
			x0 += x1;
		}
		
	} else {
		if ((0==(y0&li))&&((y0&pi)!=pif))  {
			if ((u32) (x0 - xmin) < (u32) (xmax - xmin)) {
				if ((u32) (y0 - ymin) < (u32) (ymax - ymin)) {
					gpuPixelDriver(&((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0, y0)],pixeldata);
				}
			}
		}
	}
}

/*----------------------------------------------------------------------
GF
----------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////
void gpuDrawLG(PtrUnion packet, const PD gpuPixelDriver)
{
	s32 xmin, xmax;
	s32 ymin, ymax;
	s32 x0, x1, dx;
	s32 y0, y1, dy;
	s32 r0, r1;
	s32 g0, g1;
	s32 b0, b1;

	x0 = packet.S2[2] + DrawingOffset[0];
	y0 = packet.S2[3] + DrawingOffset[1];
	x1 = packet.S2[6] + DrawingOffset[0];
	y1 = packet.S2[7] + DrawingOffset[1];

	dy = (y1 - y0);
	if (dy < 0) dy = -dy;
	dx = (x1 - x0);
	if (dx < 0) dx = -dx;

	// Maximum absolute distance between any two X coords: 1023, Y: 511
	//  (PSX hardware will not render anything violating this rule)
	if (dx >= CHKMAX_X || dy >= CHKMAX_Y)
		return;

	r0 = packet.U1[0];  g0 = packet.U1[1];  b0 = packet.U1[2];
	r1 = packet.U1[8];  g1 = packet.U1[9];  b1 = packet.U1[10];

	xmin = DrawingArea[0];	xmax = DrawingArea[2];
	ymin = DrawingArea[1];	ymax = DrawingArea[3];

	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	
	if (dx > dy) {
		if (x0 > x1) {
			SwapValues(x0, x1);
			SwapValues(y0, y1);
			SwapValues(r0, r1);
			SwapValues(g0, g1);
			SwapValues(b0, b1);
		}
		y1 = GPU_DIV((y1 - y0) << GPU_DIGITS, dx);
		r1 = GPU_DIV((r1 - r0) << GPU_DIGITS, dx);
		g1 = GPU_DIV((g1 - g0) << GPU_DIGITS, dx);
		b1 = GPU_DIV((b1 - b0) << GPU_DIGITS, dx);
		y0 <<= GPU_DIGITS;
		r0 <<= GPU_DIGITS;
		g0 <<= GPU_DIGITS;
		b0 <<= GPU_DIGITS;
		s32 temp = xmin - x0;
		if (temp > 0) {
			x0 = xmin;
			y0 += (y1 * temp);
			r0 += (r1 * temp);
			g0 += (g1 * temp);
			b0 += (b1 * temp);
		}
		if (x1 > xmax) x1 = xmax;
		x1 -= x0;
		if (x1 < 0) x1 = 0;
		
		for (; x1; x1--) {
			s32 temp = y0 >> GPU_DIGITS;
			if ((0==(temp&li))&&((temp&pi)!=pif))  {
				if ((u32) (temp - ymin) < (u32) (ymax - ymin)) {
					gpuPixelDriver (
						&((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0, temp)],
						(((b0>>GPU_DIGITSC)&0x1F)<<10) | (((g0>>GPU_DIGITSC)&0x1F)<< 5) | ((r0>>GPU_DIGITSC)&0x1F)
					);
				}
			}
			x0++;
			y0 += y1;
			r0 += r1;
			g0 += g1;
			b0 += b1;
		}
	} else if (dy) {
		if (y0 > y1) {
			SwapValues(x0, x1);
			SwapValues(y0, y1);
			SwapValues(r0, r1);
			SwapValues(g0, g1);
			SwapValues(b0, b1);
		}
		x1 = GPU_DIV((x1 - x0) << GPU_DIGITS, dy);
		r1 = GPU_DIV((r1 - r0) << GPU_DIGITS, dy);
		g1 = GPU_DIV((g1 - g0) << GPU_DIGITS, dy);
		b1 = GPU_DIV((b1 - b0) << GPU_DIGITS, dy);
		x0 <<= GPU_DIGITS;
		r0 <<= GPU_DIGITS;
		g0 <<= GPU_DIGITS;
		b0 <<= GPU_DIGITS;
		s32 temp = ymin - y0;
		if (temp > 0) {
			y0 = ymin;
			x0 += (x1 * temp);
			r0 += (r1 * temp);
			g0 += (g1 * temp);
			b0 += (b1 * temp);
		}
		if (y1 > ymax) y1 = ymax;
		y1 -= y0;
		if (y1 < 0) y1 = 0;
		
		for (; y1; y1--) {
			if ((0==(y0&li))&&((y0&pi)!=pif))  {
				s32 temp = x0 >> GPU_DIGITS;
				if ((u32) (temp - xmin) < (u32) (xmax - xmin)) {
					gpuPixelDriver (
						&((u16*)GPU_FrameBuffer)[FRAME_OFFSET(temp, y0)],
						(((b0>>GPU_DIGITSC)&0x1F)<<10) | (((g0>>GPU_DIGITSC)&0x1F)<< 5) | ((r0>>GPU_DIGITSC)&0x1F)
					);
				}
			}
			y0++;
			x0 += x1;
			r0 += r1;
			g0 += g1;
			b0 += b1;
		}
	} else {
		if ((0==(y0&li))&&((y0&pi)!=pif))  {
			if ((u32) (x0 - xmin) < (u32) (xmax - xmin)) {
				if ((u32) (y0 - ymin) < (u32) (ymax - ymin)) {
					gpuPixelDriver (
						&((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0, y0)],
						(((b0>>GPU_DIGITSC)&0x1F)<<10) | (((g0>>GPU_DIGITSC)&0x1F)<< 5) | ((r0>>GPU_DIGITSC)&0x1F)
					);
				}
			}
		}
	}
}
