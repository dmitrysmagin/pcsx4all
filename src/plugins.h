/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "psxcommon.h"
#include "r3000a.h"

#ifndef _WIN32
#define CALLBACK
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "psemu_plugin_defs.h"
#include "decode_xa.h"

int  LoadPlugins(void);
void ReleasePlugins(void);

// GPU structures

typedef struct {
	uint32_t ulFreezeVersion;
	uint32_t ulStatus;
	uint32_t ulControl[256];
	unsigned char psxVRam[1024*512*2];
} GPUFreeze_t;

/// GPU functions

long GPU_init(void);
long GPU_shutdown(void);
void GPU_writeStatus(uint32_t);
void GPU_writeData(uint32_t);
void GPU_writeDataMem(uint32_t *, int);
uint32_t GPU_readStatus(void);
uint32_t GPU_readData(void);
void GPU_readDataMem(uint32_t *, int);
long GPU_dmaChain(uint32_t *,uint32_t);
void GPU_updateLace(void);
long GPU_freeze(uint32_t, GPUFreeze_t *);
void GPU_requestScreenRedraw(void);

#ifdef USE_GPULIB
void GPU_vBlank(int is_vblank, int lcf);
#endif

// CDROM structures

struct CdrStat {
	uint32_t Type;
	uint32_t Status;
	unsigned char Time[3];
};

//senquack - updated to newer PCSX Reloaded/Rearmed code:
struct SubQ {
	char res0[12];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	unsigned char CRC[2];
	char res1[72];
};

// CDROM functions

long CDR_init(void);
long CDR_shutdown(void);
long CDR_open(void);
long CDR_close(void);
long CDR_getTN(unsigned char *);
long CDR_getTD(unsigned char , unsigned char *);
long CDR_readTrack(unsigned char *);
extern unsigned char *(*CDR_getBuffer)(void);
long CDR_play(unsigned char *);
long CDR_stop(void);
long CDR_getStatus(struct CdrStat *);
unsigned char *CDR_getBufferSub(void);

// SPU structures

typedef struct {
	unsigned char PluginName[8];
	uint32_t PluginVersion;
	uint32_t Size;
	unsigned char SPUPorts[0x200];
	unsigned char SPURam[0x80000];
	xa_decode_t xa;
	unsigned char *SPUInfo;
} SPUFreeze_t;

// SPU functions

//senquack - if using spu_pcsxrearmed plugin adapted from PCSX ReArmed, use
//           PSX4ALL->PCSX_ReARMED SPU wrapper header
#ifdef spu_pcsxrearmed
#include "spu/spu_pcsxrearmed/spu_pcsxrearmed_wrapper.h"
#else
long SPU_init(void);
long SPU_shutdown(void);
void SPU_writeRegister(unsigned long, unsigned short);
unsigned short SPU_readRegister(unsigned long);
void SPU_writeDMA(unsigned short);
unsigned short SPU_readDMA(void);
void SPU_writeDMAMem(unsigned short *, int);
void SPU_readDMAMem(unsigned short *, int);
void SPU_playADPCMchannel(xa_decode_t *);
long SPU_freeze(uint32_t, SPUFreeze_t *);
void SPU_async(void);
void SPU_playCDDAchannel(unsigned char *, int);
#endif

//senquack - added these two functions, see notes in plugins.cpp
#ifdef __cplusplus
extern "C" {
#endif

void CALLBACK Trigger_SPU_IRQ(void);
void CALLBACK Schedule_SPU_IRQ(unsigned int cycles_after);

#ifdef __cplusplus
}
#endif


// PAD functions

unsigned char PAD1_startPoll(void);
unsigned char PAD2_startPoll(void);
unsigned char PAD1_poll(void);
unsigned char PAD2_poll(void);

// ISO functions

void SetIsoFile(const char *filename);
const char *GetIsoFile(void);
boolean UsingIso(void);
void SetCdOpenCaseTime(s64 time);
s64 GetCdOpenCaseTime(void);
int ReloadCdromPlugin();

#endif /* __PLUGINS_H__ */
