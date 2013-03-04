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

#define CALLBACK

#include "psemu_plugin_defs.h"
#include "decode_xa.h"

extern int  LoadPlugins(void);
extern void ReleasePlugins(void);

// GPU structures

typedef struct {
	uint32_t ulFreezeVersion;
	uint32_t ulStatus;
	uint32_t ulControl[256];
	unsigned char psxVRam[1024*512*2];
} GPUFreeze_t;

/// GPU functions

extern long GPU_init(void);
extern long GPU_shutdown(void);
extern void GPU_writeStatus(uint32_t);
extern void GPU_writeData(uint32_t);
extern void GPU_writeDataMem(uint32_t *, int);
extern uint32_t GPU_readStatus(void);
extern uint32_t GPU_readData(void);
extern void GPU_readDataMem(uint32_t *, int);
extern long GPU_dmaChain(uint32_t *,uint32_t);
extern void GPU_updateLace(void);
extern long GPU_freeze(uint32_t, GPUFreeze_t *);

// CDROM structures

struct CdrStat {
	uint32_t Type;
	uint32_t Status;
	unsigned char Time[3];
};

struct SubQ {
	char res0[12];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	char res1[72];
};

// CDROM functions

extern long CDR_init(void);
extern long CDR_shutdown(void);
extern long CDR_open(void);
extern long CDR_close(void);
extern long CDR_getTN(unsigned char *);
extern long CDR_getTD(unsigned char , unsigned char *);
extern long CDR_readTrack(unsigned char *);
extern unsigned char *CDR_getBuffer(void);
extern long CDR_play(unsigned char *);
extern long CDR_stop(void);
extern long CDR_getStatus(struct CdrStat *);
extern unsigned char *CDR_getBufferSub(void);

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

extern long SPU_init(void);				
extern long SPU_shutdown(void);	
extern void SPU_writeRegister(unsigned long, unsigned short);
extern unsigned short SPU_readRegister(unsigned long);
extern void SPU_writeDMA(unsigned short);
extern unsigned short SPU_readDMA(void);
extern void SPU_writeDMAMem(unsigned short *, int);
extern void SPU_readDMAMem(unsigned short *, int);
extern void SPU_playADPCMchannel(xa_decode_t *);
extern long SPU_freeze(uint32_t, SPUFreeze_t *);
extern void SPU_async(void);
extern void SPU_playCDDAchannel(unsigned char *, int);

// PAD functions

extern unsigned char PAD1_startPoll(void);
extern unsigned char PAD2_startPoll(void);
extern unsigned char PAD1_poll(void);
extern unsigned char PAD2_poll(void);

// ISO functions

extern void SetIsoFile(const char *filename);
extern const char *GetIsoFile(void);
extern boolean UsingIso(void);
extern void SetCdOpenCaseTime(s64 time);
extern s64 GetCdOpenCaseTime(void);

#endif /* __PLUGINS_H__ */
