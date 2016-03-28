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

#include "port.h"
#include "gpu.h"
#include "profiler.h"
#include "debug.h"

#ifdef TIME_IN_MSEC
#define TPS 1000
#else
#define TPS 1000000
#endif

int skipCount = 0; /* frame skip (0,1,2,3...) */
int linesInterlace = 0;  /* internal lines interlace */
int linesInterlace_user = 0; /* Lines interlace */

bool isSkip = false; /* skip frame (according to GPU) */
bool skipFrame = false; /* skip this frame (according to frame skip) */
bool wasSkip = false; /* skip frame old value (according to GPU) */
bool show_fps = false; /* Show FPS statistics */

bool skipGPU = false; /* skip GPU primitives */
bool progressInterlace_flag = false; /* Progressive interlace flag */
bool progressInterlace = false; /* Progressive interlace option*/
bool frameLimit = false; /* frames to wait */

bool fb_dirty = false; /* frame buffer is dirty (according to GPU) */
bool light = true; /* lighting */
bool blend = true; /* blending */
bool FrameToRead = false; /* load image in progress */

bool FrameToWrite = false; /* store image in progress */
u8 BLEND_MODE;
u8 TEXT_MODE;
u8 Masking;

u16 PixelMSB;
u16 PixelData;

///////////////////////////////////////////////////////////////////////////////
//  GPU Global data
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Dma Transfers info
s32		px,py;
s32		x_end,y_end;
u16*  pvram;

u32 GP0;
s32 PacketCount;
s32 PacketIndex;

///////////////////////////////////////////////////////////////////////////////
//  Display status
u32 DisplayArea   [6];

///////////////////////////////////////////////////////////////////////////////
//  Rasterizer status
u32 TextureWindow [4];
u32 DrawingArea   [4];
u32 DrawingOffset [2];

///////////////////////////////////////////////////////////////////////////////
//  Rasterizer status

u16* TBA;
u16* CBA;

///////////////////////////////////////////////////////////////////////////////
//  Inner Loops
s32   u4, du4;
s32   v4, dv4;
s32   r4, dr4;
s32   g4, dg4;
s32   b4, db4;
u32   lInc;
u32   tInc, tMsk;

GPUPacket PacketBuffer;
// FRAME_BUFFER_SIZE is defined in bytes; 512K is guard memory for out of range reads
u16   GPU_FrameBuffer[(FRAME_BUFFER_SIZE+512*1024)/2] __attribute__((aligned(32)));
u32   GPU_GP1;

u32 tw=0; /* texture window */

u32 blit_mask=0; /* blit mask */

u32 *last_dma=NULL; /* last dma pointer */

///////////////////////////////////////////////////////////////////////////////
//  Inner loop driver instanciation file
#include "gpu_inner.h"

///////////////////////////////////////////////////////////////////////////////
//  GPU Raster Macros
#define	GPU_RGB16(rgb)        ((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

#define GPU_EXPANDSIGN(x)  (((s32)(x)<<21)>>21)

#define CHKMAX_X 1024
#define CHKMAX_Y 512

#define	GPU_SWAP(a,b,t)	{(t)=(a);(a)=(b);(b)=(t);}

#define IS_PAL (GPU_GP1&(0x08<<17))

///////////////////////////////////////////////////////////////////////////////
// GPU internal image drawing functions
#include "gpu_raster_image.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal line drawing functions
#include "gpu_raster_line.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal polygon drawing functions
#include "gpu_raster_polygon.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal sprite drawing functions
#include "gpu_raster_sprite.h"

///////////////////////////////////////////////////////////////////////////////
// GPU command buffer execution/store
#include "gpu_command.h"

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuReset(void)
{
	GPU_GP1 = 0x14802000;
	TextureWindow[0] = 0;
	TextureWindow[1] = 0;
	TextureWindow[2] = 255;
	TextureWindow[3] = 255;
	DrawingArea[2] = 256;
	DrawingArea[3] = 240;
	DisplayArea[2] = 256;
	DisplayArea[3] = 240;
	DisplayArea[5] = 240;
	linesInterlace=linesInterlace_user;
	blit_mask=0;
}

///////////////////////////////////////////////////////////////////////////////
bool  GPU_init(void)
{
	gpuReset();

	// s_invTable
	for(unsigned int i=1;i<=(1<<TABLE_BITS);++i)
	{
		s_invTable[i-1]=0x7fffffff/i;
	}
	fb_dirty = true;
	last_dma = NULL;
	return (0);
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_shutdown(void)
{
}

///////////////////////////////////////////////////////////////////////////////
long  GPU_freeze(unsigned int bWrite, GPUFreeze_t* p2)
{
	if (!p2) return (0);
	if (p2->Version != 1) return (0);

	if (bWrite)
	{
		p2->GPU_gp1 = GPU_GP1;
		memset(p2->Control, 0, sizeof(p2->Control));
		// save resolution and registers for P.E.Op.S. compatibility
		p2->Control[3] = (3 << 24) | ((GPU_GP1 >> 23) & 1);
		p2->Control[4] = (4 << 24) | ((GPU_GP1 >> 29) & 3);
		p2->Control[5] = (5 << 24) | (DisplayArea[0] | (DisplayArea[1] << 10));
		p2->Control[6] = (6 << 24) | (2560 << 12);
		p2->Control[7] = (7 << 24) | (DisplayArea[4] | (DisplayArea[5] << 10));
		p2->Control[8] = (8 << 24) | ((GPU_GP1 >> 17) & 0x3f) | ((GPU_GP1 >> 10) & 0x40);
		memcpy(p2->FrameBuffer, (u16*)GPU_FrameBuffer, FRAME_BUFFER_SIZE);
		return (1);
	}
	else
	{
		extern void GPU_writeStatus(u32 data);
		GPU_GP1 = p2->GPU_gp1;
		memcpy((u16*)GPU_FrameBuffer, p2->FrameBuffer, FRAME_BUFFER_SIZE);
		GPU_writeStatus((5 << 24) | p2->Control[5]);
		GPU_writeStatus((7 << 24) | p2->Control[7]);
		GPU_writeStatus((8 << 24) | p2->Control[8]);
		gpuSetTexture(GPU_GP1);
		return (1);
	}
	return (0);
}

///////////////////////////////////////////////////////////////////////////////
//  GPU DMA comunication

///////////////////////////////////////////////////////////////////////////////
u8 PacketSize[256] =
{
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		0-15
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		16-31
	3, 3, 3, 3, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 8, 8,	//		32-47
	5, 5, 5, 5, 8, 8, 8, 8, 7, 7, 7, 7, 11, 11, 11, 11,	//	48-63
	2, 2, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,	//		64-79
	3, 3, 3, 3, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4,	//		80-95
	2, 2, 2, 2, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2,	//		96-111
	1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2,	//		112-127
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		128-
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		144
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		160
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	//
};

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuSendPacket()
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_sendPacket++;
#endif
	gpuSendPacketFunction(PacketBuffer.U4[0]>>24);
}

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuCheckPacket(u32 uData)
{
	if (PacketCount)
	{
		PacketBuffer.U4[PacketIndex++] = uData;
		--PacketCount;
	}
	else
	{
		PacketBuffer.U4[0] = uData;
		PacketCount = PacketSize[uData >> 24];
		PacketIndex = 1;
	}
	if (!PacketCount) gpuSendPacket();
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_writeDataMem(u32* dmaAddress, s32 dmaCount)
{
	#ifdef DEBUG_ANALYSIS
		dbg_anacnt_GPU_writeDataMem++;
	#endif
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_writeDataMem(%d)\n",dmaCount);
	#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	u32 data;
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
	GPU_GP1 &= ~0x14000000;

	while (dmaCount) 
	{
		if (FrameToWrite) 
		{
			while (dmaCount) 
			{
				dmaCount--;
				data = *dmaAddress++;
				if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
				pvram[px] = data;
				if (++px>=x_end) 
				{
					px = 0;
					pvram += 1024;
					if (++py>=y_end) 
					{
						FrameToWrite = false;
						GPU_GP1 &= ~0x08000000;
						fb_dirty = true;
						break;
					}
				}
				if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
				pvram[px] = data>>16;
				if (++px>=x_end) 
				{
					px = 0;
					pvram += 1024;
					if (++py>=y_end) 
					{
						FrameToWrite = false;
						GPU_GP1 &= ~0x08000000;
						fb_dirty = true;
						break;
					}
				}
			}
		}
		else
		{
			data = *dmaAddress++;
			dmaCount--;
			gpuCheckPacket(data);
		}
	}

	GPU_GP1 = (GPU_GP1 | 0x14000000) & ~0x60000000;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
}

long GPU_dmaChain(u32 *rambase, u32 start_addr)
{
	#ifdef DEBUG_ANALYSIS
		dbg_anacnt_GPU_dmaChain++;
	#endif
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_dmaChain(0x%x)\n",start_addr);
	#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);

	u32 addr, *list;
	u32 len, count;
	long dma_words = 0;

	if (last_dma) *last_dma |= 0x800000;
	
	GPU_GP1 &= ~0x14000000;
	
	addr = start_addr & 0xffffff;
	for (count = 0; addr != 0xffffff; count++)
	{
		list = rambase + (addr & 0x1fffff) / 4;
		len = list[0] >> 24;
		addr = list[0] & 0xffffff;

		dma_words += 1 + len;

		// add loop detection marker
		list[0] |= 0x800000;

		if (len) GPU_writeDataMem(list + 1, len);

		if (addr & 0x800000)
		{
			#ifdef ENABLE_GPU_LOG_SUPPORT
				fprintf(stdout,"GPU_dmaChain(LOOP)\n");
			#endif
			break;
		}
	}

	// remove loop detection markers
	addr = start_addr & 0x1fffff;
	while (count-- > 0)
	{
		list = rambase + addr / 4;
		addr = list[0] & 0x1fffff;
		list[0] &= ~0x800000;
	}
	
	if (last_dma) *last_dma &= ~0x800000;
	last_dma = rambase + (start_addr & 0x1fffff) / 4;

	GPU_GP1 = (GPU_GP1 | 0x14000000) & ~0x60000000;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);

	return dma_words;
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_writeData(u32 data)
{
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
	#ifdef DEBUG_ANALYSIS
		dbg_anacnt_GPU_writeData++;
	#endif
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_writeData()\n");
	#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	GPU_GP1 &= ~0x14000000;

	if (FrameToWrite)
	{
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		pvram[px]=(u16)data;
		if (++px>=x_end)
		{
			px = 0;
			pvram += 1024;
			if (++py>=y_end) 
			{
				FrameToWrite = false;
				GPU_GP1 &= ~0x08000000;
				fb_dirty = true;
			}
		}
		if (FrameToWrite)
		{
			if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
			pvram[px]=data>>16;
			if (++px>=x_end)
			{
				px = 0;
				pvram += 1024;
				if (++py>=y_end) 
				{
					FrameToWrite = false;
					GPU_GP1 &= ~0x08000000;
					fb_dirty = true;
				}
			}
		}
	}
	else
	{
		gpuCheckPacket(data);
	}
	GPU_GP1 |= 0x14000000;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);

}


///////////////////////////////////////////////////////////////////////////////
void  GPU_readDataMem(u32* dmaAddress, s32 dmaCount)
{
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
	#ifdef DEBUG_ANALYSIS
		dbg_anacnt_GPU_readDataMem++;
	#endif
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_readDataMem(%d)\n",dmaCount);
	#endif
	if(!FrameToRead) return;

	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	GPU_GP1 &= ~0x14000000;
	do 
	{
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		// lower 16 bit
		u32 data = (unsigned long)pvram[px];

		if (++px>=x_end) 
		{
			px = 0;
			pvram += 1024;
		}

		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		// higher 16 bit (always, even if it's an odd width)
		data |= (unsigned long)(pvram[px])<<16;
		
		*dmaAddress++ = data;

		if (++px>=x_end) 
		{
			px = 0;
			pvram += 1024;
			if (++py>=y_end) 
			{
				FrameToRead = false;
				GPU_GP1 &= ~0x08000000;
				break;
			}
		}
	} while (--dmaCount);

	GPU_GP1 = (GPU_GP1 | 0x14000000) & ~0x60000000;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
}



///////////////////////////////////////////////////////////////////////////////
u32  GPU_readData(void)
{
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
	#ifdef DEBUG_ANALYSIS
		dbg_anacnt_GPU_readData++;
	#endif
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_readData()\n");
	#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_READ);
	GPU_GP1 &= ~0x14000000;
	if (FrameToRead)
	{
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		GP0 = pvram[px];
		if (++px>=x_end)
		{
			px = 0;
			pvram += 1024;
			if (++py>=y_end) 
			{
				FrameToRead = false;
				GPU_GP1 &= ~0x08000000;
			}
		}
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		GP0 |= pvram[px]<<16;
		if (++px>=x_end)
		{
			px = 0;
			pvram +=1024;
			if (++py>=y_end) 
			{
				FrameToRead = false;
				GPU_GP1 &= ~0x08000000;
			}
		}

	}
	GPU_GP1 |= 0x14000000;

	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_READ);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
	return (GP0);
}

///////////////////////////////////////////////////////////////////////////////
u32     GPU_readStatus(void)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_readStatus++;
#endif
	return GPU_GP1;
}

INLINE void GPU_NoSkip(void)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_NoSkip()\n");
	#endif
	wasSkip = isSkip;
	if (isSkip)
	{
		isSkip = false;
		skipGPU = false;
	}
	else
	{
		isSkip = skipFrame;
		skipGPU = skipFrame;
	}
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_writeStatus(u32 data)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_writeStatus++;
#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"GPU_writeStatus(%d,%d)\n",data>>24,data & 0xff);
	#endif
	switch (data >> 24) {
	case 0x00:
		gpuReset();
		break;
	case 0x01:
		GPU_GP1 &= ~0x08000000;
		PacketCount = 0; FrameToRead = FrameToWrite = false;
		break;
	case 0x02:
		GPU_GP1 &= ~0x08000000;
		PacketCount = 0; FrameToRead = FrameToWrite = false;
		break;
	case 0x03:
		GPU_GP1 = (GPU_GP1 & ~0x00800000) | ((data & 1) << 23);
		break;
	case 0x04:
		if (data == 0x04000000)	PacketCount = 0;
		GPU_GP1 = (GPU_GP1 & ~0x60000000) | ((data & 3) << 29);
		break;
	case 0x05:
		DisplayArea[0] = (data & 0x000003FF); //(short)(data & 0x3ff);
		DisplayArea[1] = ((data & 0x0007FC00)>>10); //(data & 0x000FFC00) >> 10; //(short)((data>>10)&0x1ff);
		GPU_NoSkip();
		break;
	case 0x07:
		{
			u32 v1=data & 0x000003FF; //(short)(data & 0x3ff);
			u32 v2=(data & 0x000FFC00) >> 10; //(short)((data>>10) & 0x3ff);
			if ((DisplayArea[4]!=v1)||(DisplayArea[5]!=v2))
			{
				DisplayArea[4] = v1;
				DisplayArea[5] = v2;
				#ifdef ENABLE_GPU_LOG_SUPPORT
					fprintf(stdout,"video_clear(CHANGE_Y)\n");
				#endif
				video_clear();
			}
		}
		break;
	case 0x08:
		{
			static const u32 HorizontalResolution[8] = { 256, 368, 320, 384, 512, 512, 640, 640 };
			static const u32 VerticalResolution[4] = { 240, 480, 256, 480 };
			u32 old_gp1=GPU_GP1;
			GPU_GP1 = (GPU_GP1 & ~0x007F0000) | ((data & 0x3F) << 17) | ((data & 0x40) << 10);
			#ifdef ENABLE_GPU_LOG_SUPPORT
				fprintf(stdout,"GPU_writeStatus(RES=%dx%d,BITS=%d,PAL=%d)\n",HorizontalResolution[(GPU_GP1 >> 16) & 7],
						VerticalResolution[(GPU_GP1 >> 19) & 3],(GPU_GP1&0x00200000?24:15),(IS_PAL?1:0));
			#endif
			// Video mode change
			if (GPU_GP1!=old_gp1)
			{
				// Update width
				DisplayArea[2] = HorizontalResolution[(GPU_GP1 >> 16) & 7];
				switch (DisplayArea[2])
				{
					case 256: blit_mask=0x00; break;
					case 320: blit_mask=0x00; break;
					case 368: blit_mask=0x00; break;
					case 384: blit_mask=0x00; break;
					case 512: blit_mask=0xa4; break; // GPU_BlitWWSWWSWS
					case 640: blit_mask=0xaa; break; // GPU_BlitWS
				}
				// Update height
				DisplayArea[3] = VerticalResolution[(GPU_GP1 >> 19) & 3];
				if (DisplayArea[3] == 480)
				{
					if (linesInterlace_user) linesInterlace = 3; // 1/4 of lines
					else linesInterlace = 1; // if 480 we only need half of lines
				}
				else if (linesInterlace != linesInterlace_user)
				{
					linesInterlace = linesInterlace_user; // resolution changed from 480 to lower one
				}
				#ifdef ENABLE_GPU_LOG_SUPPORT
					fprintf(stdout,"video_clear(CHANGE_RES)\n");
				#endif
				video_clear();
			}

		}
		break;
	case 0x10:
		switch (data & 0xff) {
			case 2: GP0 = tw; break;
			case 3: GP0 = (DrawingArea[1] << 10) | DrawingArea[0]; break;
			case 4: GP0 = ((DrawingArea[3]-1) << 10) | (DrawingArea[2]-1); break;
			case 5: case 6:	GP0 = (DrawingOffset[1] << 11) | DrawingOffset[0]; break;
			case 7: GP0 = 2; break;
			case 8: case 15: GP0 = 0xBFC03720; break;
		}
		break;
	}
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
}

// Blitting functions
#include "gpu_blit.h"

static void gpuVideoOutput(void)
{
	int h0, x0, y0, w0, h1;

	x0 = DisplayArea[0];
	y0 = DisplayArea[1];

	w0 = DisplayArea[2];
	h0 = DisplayArea[3];  // video mode

	h1 = DisplayArea[5] - DisplayArea[4]; // display needed
	if (h0 == 480) h1 = Min2(h1*2,480);

	u16* dest_screen16 = SCREEN;
	u16* src_screen16  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0,y0)];
	bool isRGB24 = (GPU_GP1 & 0x00200000 ? true : false);

	//  Height centering
	int sizeShift = 1;
	if(h0==256) h0 = 240; else if(h0==480) sizeShift = 2;
	if(h1>h0) { src_screen16 += ((h1-h0)>>sizeShift)*1024; h1 = h0; }
	else if(h1<h0) dest_screen16 += ((h0-h1)>>sizeShift)*VIDEO_WIDTH;

	/* Main blitter */
	int incY = (h0==480) ? 2 : 1;
	h0=(h0==480 ? 2048 : 1024);

	{
		const int li=linesInterlace;
		bool pi=progressInterlace;
		bool pif=progressInterlace_flag;
		switch ( w0 )
		{
			case 256:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWDWW(	src_screen16,	dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 368:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWWWWWWWS(	src_screen16,	dest_screen16, isRGB24, 4);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 320:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWW(	src_screen16,	dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 384:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWWWWS(	src_screen16,	dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 512:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWSWWSWS(src_screen16,dest_screen16,isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 640:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWS(	src_screen16, dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
		}
		progressInterlace_flag=!progressInterlace_flag;
	}
	video_flip();
}

// Update frames-skip each second>>3 (8 times per second)
#define GPU_FRAMESKIP_UPDATE 3

static void GPU_frameskip (bool show)
{
	u32 now=get_ticks(); // current frame

	// Show FPS
	if (show_fps)
	{
		static u32 frames_fps=0; // frames counter
		static u32 prev_fps=now; // previous fps calculation
		static char msg[36]="RES=000x000x00 FPS=000/00 SPD=000%"; // fps information
		frames_fps++;
		if ((now-prev_fps)>=TPS)
		{
			if (IS_PAL)	sprintf(msg,"RES=%3dx%3dx%2d FPS=%3d/50 SPD=%3d%%",DisplayArea[2],DisplayArea[3],(GPU_GP1&0x00200000?24:15),frames_fps,(frames_fps<<1));
			else 		sprintf(msg,"RES=%3dx%3dx%2d FPS=%3d/60 SPD=%3d%%",DisplayArea[2],DisplayArea[3],(GPU_GP1&0x00200000?24:15),frames_fps,((frames_fps*1001)/600));
			frames_fps=0;
			prev_fps=now;
#ifndef __arm__
			port_printf(5,5,msg);
#endif
		}
#ifdef __arm__
		port_printf(5,5,msg);
#endif
	}

	// Update frameskip
	if (skipCount==0) skipFrame=false; // frameskip off
	else if (skipCount==7) { if (show) skipFrame=!skipFrame; } // frameskip medium
	else if (skipCount==8) skipFrame=true; // frameskip maximum
	else
	{
		static u32 spd=100; // speed %
		static u32 frames=0; // frames counter
		static u32 prev=now; // previous fps calculation
		frames++;
		if ((now-prev)>=(TPS>>GPU_FRAMESKIP_UPDATE))
		{
			if (IS_PAL) spd=(frames<<1);
			else spd=((frames*1001)/600);
			spd<<=GPU_FRAMESKIP_UPDATE;
			frames=0;
			prev=now;
		}
		switch(skipCount)
		{
			case 1: if (spd<50) skipFrame=true; else skipFrame=false; break; // frameskip on (spd<50%)
			case 2: if (spd<60) skipFrame=true; else skipFrame=false; break; // frameskip on (spd<60%)
			case 3: if (spd<70) skipFrame=true; else skipFrame=false; break; // frameskip on (spd<70%)
			case 4: if (spd<80) skipFrame=true; else skipFrame=false; break; // frameskip on (spd<80%)
			case 5: if (spd<90) skipFrame=true; else skipFrame=false; break; // frameskip on (spd<90%)
		}
	}

	// Limit FPS
	if (frameLimit)
	{
		static u32 next=now; // next frame
		if (show) { if (now<next) wait_ticks(next-now); }
		next+=(IS_PAL?(TPS/50):((u32)(((double)TPS)/59.94)));
	}
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_updateLace(void)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_updateLace++;
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_COUNTERS);
#ifdef PROFILER_PCSX4ALL
	pcsx4all_prof_frames++;
#endif
#ifdef DEBUG_FRAME
	if(isdbg_frame())
	{
		static int passed=0;
		if (!passed) dbg_enable();
		else pcsx4all_exit();
		passed++;
	}
#endif

	// Interlace bit toggle
	GPU_GP1 ^= 0x80000000;

	// Update display?
	if ((fb_dirty) && (!wasSkip) && (!(GPU_GP1&0x00800000)))
	{
		// Display updated
		gpuVideoOutput();
		GPU_frameskip(true);
		#ifdef ENABLE_GPU_LOG_SUPPORT
			fprintf(stdout,"GPU_updateLace(UPDATE)\n");
		#endif
	}
	else
	{
		GPU_frameskip(false);
		#ifdef ENABLE_GPU_LOG_SUPPORT
			fprintf(stdout,"GPU_updateLace(SKIP)\n");
		#endif
	}

	if ((!skipCount) && (DisplayArea[3] == 480)) skipGPU=true; // Tekken 3 hack

	fb_dirty=false;
	last_dma = NULL;

	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_COUNTERS);
}
