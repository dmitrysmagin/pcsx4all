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

////////////////////////////////////////////////////////////////////////////////
// Blend bgr555 color in 'uSrc' (foreground) with bgr555 color
//  in 'uDst' (background), returning resulting color.
//
// INPUT:
//  'uSrc','uDst' input: -bbbbbgggggrrrrr
//                       ^ bit 16
// OUTPUT:
//           u16 output: 0bbbbbgggggrrrrr
//                       ^ bit 16
// RETURNS:
// Where '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
template <int BLENDMODE>
GPU_INLINE u16 gpuBlending(u16 uSrc, u16 uDst)
{
	u16 mix;

	//	0.5 x Back + 0.5 x Forward
	if (BLENDMODE==0) {
		mix = ((uDst & 0x7BDE) + (uSrc & 0x7BDE)) >> 1;
	}

	//	1.0 x Back + 1.0 x Forward
	if (BLENDMODE==1) {
		u16 b = (uDst & 0x7C00) + (uSrc & 0x7C00);
		u16 g = (uDst & 0x03E0) + (uSrc & 0x03E0);
		u16 r = (uDst & 0x001F) + (uSrc & 0x001F);
		if (b > 0x7C00) b = 0x7C00;
		if (g > 0x03E0) g = 0x03E0;
		if (r > 0x001F) r = 0x001F;
		mix = b | g | r;
	}

	//	1.0 x Back - 1.0 x Forward	*/
	if (BLENDMODE==2) {
		s32 b = (uDst & 0x7C00) - (uSrc & 0x7C00);  if (b < 0) b = 0;
		s32 g = (uDst & 0x03E0) - (uSrc & 0x03E0);  if (g > 0) b |= g;
		s32 r = (uDst & 0x001F) - (uSrc & 0x001F);  if (r > 0) b |= r;
		mix = b;
	}

	//	1.0 x Back + 0.25 x Forward	*/
	if (BLENDMODE==3) {
		uSrc >>= 2;
		u16 b = (uDst & 0x7C00) + (uSrc & 0x1C00);
		u16 g = (uDst & 0x03E0) + (uSrc & 0x00E0);
		u16 r = (uDst & 0x001F) + (uSrc & 0x0007);
		if (b > 0x7C00) b = 0x7C00;
		if (g > 0x03E0) g = 0x03E0;
		if (r > 0x001F) r = 0x001F;
		mix = b | g | r;
	}

	return mix;
}


////////////////////////////////////////////////////////////////////////////////
// Convert bgr555 color in uSrc to padded u32 5.4:5.4:5.4 bgr fixed-pt
//  color triplet suitable for use with HQ 24-bit quantization.
//
// INPUT:
//       'uDst' input: -bbbbbgggggrrrrr
//                     ^ bit 16
// RETURNS:
//         u32 output: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE u32 gpuGetRGB24(u16 uSrc)
{
	return ((uSrc & 0x7C00)<<14)
	     | ((uSrc & 0x03E0)<< 9)
	     | ((uSrc & 0x001F)<< 4);
}


////////////////////////////////////////////////////////////////////////////////
// Blend padded u32 5.4:5.4:5.4 bgr fixed-pt color triplet in 'uSrc24'
//  (foreground color) with bgr555 color in 'uDst' (background color),
//  returning the resulting u32 5.4:5.4:5.4 color.
//
// INPUT:
//     'uSrc24' input: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
//       'uDst' input: -bbbbbgggggrrrrr
//                     ^ bit 16
// RETURNS:
//         u32 output: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
template <int BLENDMODE>
GPU_INLINE u32 gpuBlending24(u32 uSrc24, u16 uDst)
{
	static const u32 uMsk = 0x1FE7F9FE;

	u32 nDst = gpuGetRGB24(uDst);
	u32 mix;

	//	0.5 x Back + 0.5 x Forward
	if (BLENDMODE==0) {
		mix = (((nDst & uMsk) + (uSrc24 & uMsk)) >> 1);
	}

	//	1.0 x Back + 1.0 x Forward
	if (BLENDMODE==1) {
		mix = nDst + uSrc24;

		//clamp to 1FFh
		if (mix & (1<< 9)) mix |= (0x1FF    );
		if (mix & (1<<19)) mix |= (0x1FF<<10);
		if (mix & (1<<29)) mix |= (0x1FF<<20);
	}

	//	1.0 x Back - 1.0 x Forward	*/
	if (BLENDMODE==2) {
		//((color^mask)+mask_one) = -color
		mix = (nDst + ((uSrc24 ^ (0x1FF7FDFF)) + 0x100401));

		//clamp to 0h
		if ((mix & (0x1FF    )) > (nDst & (0x1FF    ))) mix &= ~(0x3FF);
		if ((mix & (0x1FF<<10)) > (nDst & (0x1FF<<10))) mix &= ~(0x3FF << 10);
		if ((mix & (0x1FF<<20)) > (nDst & (0x1FF<<20))) mix &= ~(0x3FF << 20);
	}

	//	1.0 x Back + 0.25 x Forward	*/
	if (BLENDMODE==3) {
		mix = (nDst + ((uSrc24 & 0x1FC7F1FC) >> 2));

		//clamp to 1FFh
		if (mix & (1 << 9)) mix |= (0x1FF      );
		if (mix & (1 <<19)) mix |= (0x1FF << 10);
		if (mix & (1 <<29)) mix |= (0x1FF << 20);
	}

	return mix;
}

#endif  //_OP_BLEND_H_
