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
//  GPU internal sprite drawing functions

///////////////////////////////////////////////////////////////////////////////
void gpuDrawS(const PS gpuSpriteSpanDriver)
{
	s32 x0, x1, y0, y1;
	s32 u0, v0;
	x1 = x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2]) + DrawingOffset[0];
	y1 = y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3]) + DrawingOffset[1];
	x1 += (PacketBuffer.U2[6] & 0x3ff); // Max width is 1023
	y1 += (PacketBuffer.U2[7] & 0x1ff); // Max height is 511

	s32 xmin, xmax, ymin, ymax;
	xmin = DrawingArea[0];	xmax = DrawingArea[2];
	ymin = DrawingArea[1];	ymax = DrawingArea[3];

	u0 = PacketBuffer.U1[8];
	v0 = PacketBuffer.U1[9];

	r4 = s32(PacketBuffer.U1[0]);
	g4 = s32(PacketBuffer.U1[1]);
	b4 = s32(PacketBuffer.U1[2]);

	s32 temp;
	temp = ymin - y0;
	if (temp > 0) { y0 = ymin; v0 += temp; }
	if (y1 > ymax) y1 = ymax;
	if (y1 <= y0) return;

	temp = xmin - x0;
	if (temp > 0) { x0 = xmin; u0 += temp; }
	if (x1 > xmax) x1 = xmax;
	x1 -= x0;
	if (x1 <= 0) return;

	u16 *Pixel = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0, y0)];
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);
	const u32 masku=TextureWindow[2];
	const u32 maskv=TextureWindow[3];

	// senquack-Fixed Xenogears text box border glitches cause by careless
	//  setting of texture pointer when in 4bpp texture mode.
	//  NOTE: sprite inner drivers now take u8* texture pointer as parameter
	u0 &= masku;
	v0 &= maskv;
	unsigned int tmode = TEXT_MODE >> 5;
	unsigned int u0_off;

	switch (tmode) {
		default:
		case 3: // 16bpp texture mode
			u0_off = u0 << 1;
			break;
		case 2: // 8bpp texture mode
			u0_off = u0;
			break;
		case 1: // 4bpp texture mode
			u0_off = u0 >> 1;
			break;
	}

	u8* pTxt_base = (u8*)TBA + u0_off;

	for (; y0<y1; ++y0) {
		u8* pTxt = pTxt_base + (v0 * 2048);
		if ((0==(y0&li))&&((y0&pi)!=pif))
			gpuSpriteSpanDriver(Pixel, x1, pTxt, masku);
		Pixel += FRAME_WIDTH;
		v0 = (v0+1) & maskv;
	}
}

#ifdef __arm__
#include "gpu_arm.h"

/* Notaz 4bit sprites optimization */
void gpuDrawS16(void)
{
	s32 x0, y0;
	s32 u0, v0;
	s32 xmin, xmax;
	s32 ymin, ymax;
	u32 h = 16;

	x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2]) + DrawingOffset[0];
	y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3]) + DrawingOffset[1];

	xmin = DrawingArea[0];	xmax = DrawingArea[2];
	ymin = DrawingArea[1];	ymax = DrawingArea[3];
	u0 = PacketBuffer.U1[8];
	v0 = PacketBuffer.U1[9];

	if (x0 > xmax - 16 || x0 < xmin ||
	    ((u0 | v0) & 15) || !(TextureWindow[2] & TextureWindow[3] & 8)) {
		// send corner cases to general handler
		PacketBuffer.U4[3] = 0x00100010;
		gpuDrawS(gpuSpriteSpanFn<0x20>);
		return;
	}

	if (y0 >= ymax || y0 <= ymin - 16)
		return;
	if (y0 < ymin) {
		h -= ymin - y0;
		v0 += ymin - y0;
		y0 = ymin;
	}
	else if (ymax - y0 < 16)
		h = ymax - y0;

	draw_spr16_full(&GPU_FrameBuffer[FRAME_OFFSET(x0, y0)], &TBA[FRAME_OFFSET(u0/4, v0)], CBA, h);
}
#endif // __arm__

///////////////////////////////////////////////////////////////////////////////
void gpuDrawT(const PT gpuTileSpanDriver)
{
	s32 x0, x1, y0, y1;
	x1 = x0 = GPU_EXPANDSIGN(PacketBuffer.S2[2]) + DrawingOffset[0];
	y1 = y0 = GPU_EXPANDSIGN(PacketBuffer.S2[3]) + DrawingOffset[1];
	x1 += (PacketBuffer.U2[4] & 0x3ff); // Max width is 1023
	y1 += (PacketBuffer.U2[5] & 0x1ff); // Max height is 511

	s32 xmin, xmax, ymin, ymax;
	xmin = DrawingArea[0];	xmax = DrawingArea[2];
	ymin = DrawingArea[1];	ymax = DrawingArea[3];

	if (y0 < ymin) y0 = ymin;
	if (y1 > ymax) y1 = ymax;
	if (y1 <= y0) return;

	if (x0 < xmin) x0 = xmin;
	if (x1 > xmax) x1 = xmax;
	x1 -= x0;
	if (x1 <= 0) return;

	u16 *Pixel = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0, y0)];
	const u16 Data = GPU_RGB16(PacketBuffer.U4[0]);
	const int li=linesInterlace;
	const int pi=(progressInterlace?(linesInterlace+1):0);
	const int pif=(progressInterlace?(progressInterlace_flag?(linesInterlace+1):0):1);

	for (; y0<y1; ++y0) {
		if ((0==(y0&li))&&((y0&pi)!=pif)) gpuTileSpanDriver(Pixel,x1,Data);
		Pixel += FRAME_WIDTH;
	}
}
