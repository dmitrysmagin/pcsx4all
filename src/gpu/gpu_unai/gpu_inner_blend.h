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

#ifndef _OP_BLEND_H_
#define _OP_BLEND_H_

//  GPU Blending operations functions

#define gpuBlending00(uSrc,uDst) \
{ \
	uSrc = (((uDst & uMsk) + (uSrc & uMsk)) >> 1); \
}

//	1.0 x Back + 1.0 x Forward
#define gpuBlending01(uSrc,uDst) \
{ \
	u16 rr, gg, bb; \
	bb = (uDst & 0x7C00) + (uSrc & 0x7C00);   if (bb > 0x7C00)  bb = 0x7C00; \
	gg = (uDst & 0x03E0) + (uSrc & 0x03E0);   if (gg > 0x03E0)  gg = 0x03E0;  bb |= gg; \
	rr = (uDst & 0x001F) + (uSrc & 0x001F);   if (rr > 0x001F)  rr = 0x001F;  bb |= rr; \
	uSrc = bb; \
}

//	1.0 x Back - 1.0 x Forward	*/
#define gpuBlending02(uSrc,uDst) \
{ \
	s32 rr, gg, bb; \
	bb = (uDst & 0x7C00) - (uSrc & 0x7C00);   if (bb < 0)  bb  =  0; \
	gg = (uDst & 0x03E0) - (uSrc & 0x03E0);   if (gg > 0)  bb |= gg; \
	rr = (uDst & 0x001F) - (uSrc & 0x001F);   if (rr > 0)  bb |= rr; \
	uSrc = bb; \
}

//	1.0 x Back + 0.25 x Forward	*/
#define gpuBlending03(uSrc,uDst) \
{ \
	u16 rr, gg, bb; \
	uSrc >>= 2; \
	bb = (uDst & 0x7C00) + (uSrc & 0x1C00);   if (bb > 0x7C00)  bb = 0x7C00; \
	gg = (uDst & 0x03E0) + (uSrc & 0x00E0);   if (gg > 0x03E0)  gg = 0x03E0;  bb |= gg; \
	rr = (uDst & 0x001F) + (uSrc & 0x0007);   if (rr > 0x001F)  rr = 0x001F;  bb |= rr; \
	uSrc = bb; \
}

INLINE void gpuGetRGB24(u32 &uDst, u16 &uSrc)
{
	uDst = ((uSrc & 0x7C00)<<14)
	     | ((uSrc & 0x03E0)<< 9)
	     | ((uSrc & 0x001F)<< 4);
}

template <const int BLENDMODE>
INLINE void doGpuPolyBlend(u32 &uSrc, u16 &uDst)
{
	static const u32 uMsk = 0x1FE7F9FE;

	u32 nDst; gpuGetRGB24(nDst, uDst);

	//	0.5 x Back + 0.5 x Forward
	if (BLENDMODE==0) {
		uSrc = (((nDst & uMsk) + (uSrc & uMsk)) >> 1);
	}

	//	1.0 x Back + 1.0 x Forward
	if (BLENDMODE==1) {
		uSrc = nDst + uSrc;

		//clamp to 1FFh
		if (uSrc & (1<< 9)) uSrc |= (0x1FF    );
		if (uSrc & (1<<19)) uSrc |= (0x1FF<<10);
		if (uSrc & (1<<29)) uSrc |= (0x1FF<<20);
	}

	//	1.0 x Back - 1.0 x Forward	*/
	if (BLENDMODE==2) {
		//((color^mask)+mask_one) = -color
		u32 mix = (nDst + ((uSrc ^ (0x1FF7FDFF)) + 0x100401));

		//clamp to 0h
		if ((mix & (0x1FF    )) > (nDst & (0x1FF    ))) mix &= ~(0x3FF);
		if ((mix & (0x1FF<<10)) > (nDst & (0x1FF<<10))) mix &= ~(0x3FF << 10);
		if ((mix & (0x1FF<<20)) > (nDst & (0x1FF<<20))) mix &= ~(0x3FF << 20);

		uSrc = mix;
	}

	//	1.0 x Back + 0.25 x Forward	*/
	if (BLENDMODE==3) {
		uSrc = (nDst + ((uSrc & 0x1FC7F1FC) >> 2));

		//clamp to 1FFh
		if (uSrc & (1 << 9)) uSrc |= (0x1FF      );
		if (uSrc & (1 <<19)) uSrc |= (0x1FF << 10);
		if (uSrc & (1 <<29)) uSrc |= (0x1FF << 20);
	}
}

#endif  //_OP_BLEND_H_
