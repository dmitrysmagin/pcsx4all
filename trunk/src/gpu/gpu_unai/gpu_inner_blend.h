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

#endif  //_OP_BLEND_H_
