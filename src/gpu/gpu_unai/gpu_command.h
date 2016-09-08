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
void gpuSetTexture(u16 tpage)
{
	u32 tmode, tx, ty;
	GPU_GP1 = (GPU_GP1 & ~0x1FF) | (tpage & 0x1FF);
	TextureWindow[0]&= ~TextureWindow[2];
	TextureWindow[1]&= ~TextureWindow[3];

	tmode = (tpage >> 7) & 3;  // 16bpp, 8bpp, or 4bpp texture colors?
	                           // 0: 4bpp     1: 8bpp     2/3: 16bpp

	// Nocash PSX docs state setting of 3 is same as setting of 2 (16bpp):
	// Note: DrHell assumes 3 is same as 0.. TODO: verify which is correct?
	if (tmode == 3) tmode = 2;

	tx = (tpage & 0x0F) << 6;
	ty = (tpage & 0x10) << 4;

	tx += (TextureWindow[0] >> (2 - tmode));
	ty += TextureWindow[1];
	
	BLEND_MODE  = ((tpage>>5) & 3) << 3;
	TEXT_MODE   = (tmode + 1) << 5; // TEXT_MODE should be values 1..3, so add one
	TBA = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(tx, ty)];
}

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuSetCLUT(u16 clut)
{
	CBA = &((u16*)GPU_FrameBuffer)[(clut & 0x7FFF) << 4];
}

#ifdef  ENABLE_GPU_NULL_SUPPORT
#define NULL_GPU() break
#else
#define NULL_GPU()
#endif

#ifdef  ENABLE_GPU_LOG_SUPPORT
#define DO_LOG(expr) printf expr
#else
#define DO_LOG(expr) {}
#endif

#define Blending (((PRIM&0x2)&&(blend))?(PRIM&0x2):0)
#define Blending_Mode (((PRIM&0x2)&&(blend))?BLEND_MODE:0)
#define Lighting (((~PRIM)&0x1)&&(light))

//senquack - now handled by Rearmed's gpulib and gpu_unai/gpulib_if.cpp:
#ifndef USE_GPULIB
void gpuSendPacketFunction(const int PRIM)
{
	//printf("0x%x\n",PRIM);

	//senquack - TODO: optimize this (packet pointer union as prim draw parameter
	// introduced as optimization for gpulib command-list processing)
	PtrUnion packet = { .ptr = (void*)&PacketBuffer };

	switch (PRIM)
	{
		case 0x02: {
			NULL_GPU();
			gpuClearImage();    //  prim handles updateLace && skip
			fb_dirty = true;
			DO_LOG(("gpuClearImage(0x%x)\n",PRIM));
		} break;

		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23: {          // Monochrome 3-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				PP driver = gpuPolySpanDrivers[(blit_mask?512:0) | Blending_Mode | Masking | Blending | PixelMSB];
				gpuDrawF3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawF3(0x%x)\n",PRIM));
			}
		} break;

		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27: {          // Textured 3-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[4] >> 16);
				u32 driver_idx = (blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | PixelMSB;
				if (!((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F)))
					driver_idx |= Lighting;
				PP driver = gpuPolySpanDrivers[driver_idx];
				gpuDrawFT3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawFT3(0x%x)\n",PRIM));
			}
		} break;

		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B: {          // Monochrome 4-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				PP driver = gpuPolySpanDrivers[(blit_mask?512:0) | Blending_Mode | Masking | Blending | PixelMSB];
				//--PacketBuffer.S2[6];
				gpuDrawF3(driver);
				PacketBuffer.U4[1] = PacketBuffer.U4[4];
				//--PacketBuffer.S2[2];
				gpuDrawF3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawF4(0x%x)\n",PRIM));
			}
		} break;

		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F: {          // Textured 4-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[4] >> 16);
				u32 driver_idx = (blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | PixelMSB;
				if (!((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F)))
					driver_idx |= Lighting;
				PP driver = gpuPolySpanDrivers[driver_idx];
				//--PacketBuffer.S2[6];
				gpuDrawFT3(driver);
				PacketBuffer.U4[1] = PacketBuffer.U4[7];
				PacketBuffer.U4[2] = PacketBuffer.U4[8];
				//--PacketBuffer.S2[2];
				gpuDrawFT3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawFT4(0x%x)\n",PRIM));
			}
		} break;

		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33: {          // Gouraud-shaded 3-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				PP driver = gpuPolySpanDrivers[(blit_mask?512:0) | Blending_Mode | Masking | Blending | 129 | PixelMSB];
				gpuDrawG3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawG3(0x%x)\n",PRIM));
			}
		} break;

		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37: {          // Gouraud-shaded, textured 3-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[5] >> 16);
				PP driver = gpuPolySpanDrivers[(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | ((Lighting)?129:0) | PixelMSB];
				gpuDrawGT3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawGT3(0x%x)\n",PRIM));
			}
		} break;

		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B: {          // Gouraud-shaded 4-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				PP driver = gpuPolySpanDrivers[(blit_mask?512:0) | Blending_Mode | Masking | Blending | 129 | PixelMSB];
				//--PacketBuffer.S2[6];
				gpuDrawG3(driver);
				PacketBuffer.U4[0] = PacketBuffer.U4[6];
				PacketBuffer.U4[1] = PacketBuffer.U4[7];
				//--PacketBuffer.S2[2];
				gpuDrawG3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawG4(0x%x)\n",PRIM));
			}
		} break;

		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F: {          // Gouraud-shaded, textured 4-pt poly
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (PacketBuffer.U4[5] >> 16);
				PP driver = gpuPolySpanDrivers[(blit_mask?512:0) | Blending_Mode | TEXT_MODE | Masking | Blending | ((Lighting)?129:0) | PixelMSB];
				//--PacketBuffer.S2[6];
				gpuDrawGT3(driver);
				PacketBuffer.U4[0] = PacketBuffer.U4[9];
				PacketBuffer.U4[1] = PacketBuffer.U4[10];
				PacketBuffer.U4[2] = PacketBuffer.U4[11];
				//--PacketBuffer.S2[2];
				gpuDrawGT3(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawGT4(0x%x)\n",PRIM));
			}
		} break;

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43: {          // Monochrome line
			if (!skipGPU)
			{
				NULL_GPU();
				PD driver = gpuPixelDrivers[(Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1];
				gpuDrawLF(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawLF(0x%x)\n",PRIM));
			}
		} break;

		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F: { // Monochrome line strip
			if (!skipGPU)
			{
				NULL_GPU();
				PD driver = gpuPixelDrivers[(Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1];
				gpuDrawLF(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawLF(0x%x)\n",PRIM));
			}
			if ((PacketBuffer.U4[3] & 0xF000F000) != 0x50005000)
			{
				PacketBuffer.U4[1] = PacketBuffer.U4[2];
				PacketBuffer.U4[2] = PacketBuffer.U4[3];
				PacketCount = 1;
				PacketIndex = 3;
			}
		} break;

		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53: {          // Gouraud-shaded line
			if (!skipGPU)
			{
				NULL_GPU();
				PD driver = gpuPixelDrivers[(Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1];
				gpuDrawLG(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawLG(0x%x)\n",PRIM));
			}
		} break;

		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F: { // Gouraud-shaded line strip
			if (!skipGPU)
			{
				NULL_GPU();
				PD driver = gpuPixelDrivers[(Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1];
				gpuDrawLG(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawLG(0x%x)\n",PRIM));
			}
			if ((PacketBuffer.U4[4] & 0xF000F000) != 0x50005000)
			{
				PacketBuffer.U1[3 + (2 * 4)] = PacketBuffer.U1[3 + (0 * 4)];
				PacketBuffer.U4[0] = PacketBuffer.U4[2];
				PacketBuffer.U4[1] = PacketBuffer.U4[3];
				PacketBuffer.U4[2] = PacketBuffer.U4[4];
				PacketCount = 2;
				PacketIndex = 3;
			}
		} break;

		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63: {          // Monochrome rectangle (variable size)
			if (!skipGPU)
			{
				NULL_GPU();
				PT driver = gpuTileSpanDrivers[Blending_Mode | Masking | Blending | (PixelMSB>>3)];
				gpuDrawT(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67: {          // Textured rectangle (variable size)
			if (!skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				u32 driver_idx = Blending_Mode | TEXT_MODE | Masking | Blending | (PixelMSB>>1);

				// This fixes Silent Hill running animation on loading screens:
				// (On PSX, color values 0x00-0x7F darken the source texture's color,
				//  0x81-FF lighten textures (ultimately clamped to 0x1F),
				//  0x80 leaves source texture color unchanged, HOWEVER,
				//   gpu_unai uses a simple lighting LUT whereby only the upper
				//   5 bits of an 8-bit color are used, so 0x80-0x87 all behave as
				//   0x80.
				// 
				// NOTE: I've changed all textured sprite draw commands here and
				//  elsewhere to use proper behavior, but left poly commands
				//  alone, I don't want to slow rendering down too much. (TODO)
				//if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
				// Strip lower 3 bits of each color and determine if lighting should be used:
				if ((PacketBuffer.U4[0] & 0xF8F8F8) != 0x808080)
					driver_idx |= Lighting;
				PS driver = gpuSpriteSpanDrivers[driver_idx];
				gpuDrawS(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
		} break;

		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B: {          // Monochrome rectangle (1x1 dot)
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[2] = 0x00010001;
				PT driver = gpuTileSpanDrivers[Blending_Mode | Masking | Blending | (PixelMSB>>3)];
				gpuDrawT(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73: {          // Monochrome rectangle (8x8)
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[2] = 0x00080008;
				PT driver = gpuTileSpanDrivers[Blending_Mode | Masking | Blending | (PixelMSB>>3)];
				gpuDrawT(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77: {          // Textured rectangle (8x8)
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[3] = 0x00080008;
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				u32 driver_idx = Blending_Mode | TEXT_MODE | Masking | Blending | (PixelMSB>>1);

				//senquack - Only color 808080h-878787h allows skipping lighting calculation:
				//if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
				// Strip lower 3 bits of each color and determine if lighting should be used:
				if ((PacketBuffer.U4[0] & 0xF8F8F8) != 0x808080)
					driver_idx |= Lighting;
				PS driver = gpuSpriteSpanDrivers[driver_idx];
				gpuDrawS(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
		} break;

		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B: {          // Monochrome rectangle (16x16)
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[2] = 0x00100010;
				PT driver = gpuTileSpanDrivers[Blending_Mode | Masking | Blending | (PixelMSB>>3)];
				gpuDrawT(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x7C:
		case 0x7D:
			#ifdef __arm__
			/* Notaz 4bit sprites optimization */
			if ((!skipGPU) && (!(GPU_GP1&0x180)) && (!(Masking|PixelMSB)))
			{
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				gpuDrawS16();
				fb_dirty = true;
				break;
			}
			#endif
		case 0x7E:
		case 0x7F: {          // Textured rectangle (16x16)
			if (!skipGPU)
			{
				NULL_GPU();
				PacketBuffer.U4[3] = 0x00100010;
				gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
				gpuSetTexture (GPU_GP1);
				u32 driver_idx = Blending_Mode | TEXT_MODE | Masking | Blending | (PixelMSB>>1);

				//senquack - Only color 808080h-878787h allows skipping lighting calculation:
				//if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
				// Strip lower 3 bits of each color and determine if lighting should be used:
				if ((PacketBuffer.U4[0] & 0xF8F8F8) != 0x808080)
					driver_idx |= Lighting;
				PS driver = gpuSpriteSpanDrivers[driver_idx];
				gpuDrawS(driver);
				fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
		} break;

		case 0x80:          //  vid -> vid
			gpuMoveImage();   //  prim handles updateLace && skip
			if ((!skipCount) && (DisplayArea[3] == 480)) // Tekken 3 hack
			{
				if (!skipGPU) fb_dirty = true;
			}
			else
			{
				fb_dirty = true;
			}
			DO_LOG(("gpuMoveImage(0x%x)\n",PRIM));
			break;
		case 0xA0:          //  sys ->vid
			gpuLoadImage();   //  prim handles updateLace && skip
			DO_LOG(("gpuLoadImage(0x%x)\n",PRIM));
			break;
		case 0xC0:          //  vid -> sys
			gpuStoreImage();  //  prim handles updateLace && skip
			DO_LOG(("gpuStoreImage(0x%x)\n",PRIM));
			break;
		case 0xE1:
			{
				const u32 temp = PacketBuffer.U4[0];
				GPU_GP1 = (GPU_GP1 & ~0x000007FF) | (temp & 0x000007FF);
				gpuSetTexture(temp);
				DO_LOG(("gpuSetTexture(0x%x)\n",PRIM));
			}
			break;
		case 0xE2:	  
			{
				static const u8  TextureMask[32] = {
					255, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7,	//
					127, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7	  //
				};
				const u32 temp = PacketBuffer.U4[0];
				tw=temp&0xFFFFF;
				TextureWindow[0] = ((temp >> 10) & 0x1F) << 3;
				TextureWindow[1] = ((temp >> 15) & 0x1F) << 3;
				TextureWindow[2] = TextureMask[(temp >> 0) & 0x1F];
				TextureWindow[3] = TextureMask[(temp >> 5) & 0x1F];
				//senquack - new vars must be updated whenever texture window is changed:
				u4_msk = i2x(TextureWindow[2]) | ((1 << FIXED_BITS) - 1);
				v4_msk = i2x(TextureWindow[3]) | ((1 << FIXED_BITS) - 1);
				gpuSetTexture(GPU_GP1);
				DO_LOG(("TextureWindow(0x%x)\n",PRIM));
			}
			break;
		case 0xE3:
			{
				const u32 temp = PacketBuffer.U4[0];
				DrawingArea[0] = temp         & 0x3FF;
				DrawingArea[1] = (temp >> 10) & 0x3FF;
				DO_LOG(("DrawingArea_Pos(0x%x)\n",PRIM));
			}
			break;
		case 0xE4:
			{
				const u32 temp = PacketBuffer.U4[0];
				DrawingArea[2] = (temp         & 0x3FF) + 1;
				DrawingArea[3] = ((temp >> 10) & 0x3FF) + 1;
				DO_LOG(("DrawingArea_Size(0x%x)\n",PRIM));
			}
			break;
		case 0xE5:
			{
				const u32 temp = PacketBuffer.U4[0];
				DrawingOffset[0] = ((s32)temp<<(32-11))>>(32-11);
				DrawingOffset[1] = ((s32)temp<<(32-22))>>(32-11);
				DO_LOG(("DrawingOffset(0x%x)\n",PRIM));
			}
			break;
		case 0xE6:
			{
				const u32 temp = PacketBuffer.U4[0];
				//GPU_GP1 = (GPU_GP1 & ~0x00001800) | ((temp&3) << 11);
				Masking = (temp & 0x2) <<  1;
				PixelMSB =(temp & 0x1) <<  8;
				DO_LOG(("SetMask(0x%x)\n",PRIM));
			}
			break;
	}
}
#endif //USE_GPULIB
