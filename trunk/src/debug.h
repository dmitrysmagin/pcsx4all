/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Chui                                               *
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

#include <stdio.h>
#include <stdlib.h>
#include "port.h"

#ifndef DEBUG_PCSX4ALL_H
#define DEBUG_PCSX4ALL_H

#ifdef DEBUG_PCSX4ALL

extern int dbg_now;
extern int dbg_frame;
extern unsigned dbg_biosptr;
#ifdef DEBUG_FILE
extern FILE *DEBUG_STR_FILE;
#else
#define DEBUG_STR_FILE stdout
#endif

#define isdbg() dbg_now
#define isdbg_frame() dbg_frame
#define dbg_enable() dbg_now=1
#define dbg_disable() dbg_now=0
#define dbg_enable_frame() dbg_frame=1

#ifdef DEBUG_PCSX4ALL_FFLUSH
#define PCSX4ALL_FFLUSH fflush(DEBUG_STR_FILE);
#else
#define PCSX4ALL_FFLUSH
#endif

#ifdef DEBUG_FILE
#define dbg(TEXTO) \
	if (dbg_now) \
	{ \
		fprintf(DEBUG_STR_FILE,"%s\n",TEXTO); \
		PCSX4ALL_FFLUSH \
	}


#define dbgf(FORMATO, RESTO...) \
	if (dbg_now) \
	{ \
		fprintf (DEBUG_STR_FILE,FORMATO , ## RESTO); \
		PCSX4ALL_FFLUSH \
	}
#else
#define dbg(TEXTO) \
	if (dbg_now) \
	{ \
		puts(TEXTO); \
		PCSX4ALL_FFLUSH \
	}


#define dbgf(FORMATO, RESTO...) \
	if (dbg_now) \
	{ \
		printf(FORMATO , ## RESTO); \
		PCSX4ALL_FFLUSH \
	}
#endif


void dbgsum(char *str, void *buff, unsigned len);
void dbgregs(void *regs);
void dbgregsCop(void *r);
void dbgpsxregs(void);
void dbgpsxregsCop(void);
void dbg_opcode(unsigned _pc, unsigned _opcode, unsigned _cycle_add, unsigned block);
void dbg_print_analysis(void);
#ifdef DEBUG_BIOS
#define dbg_bioscheck(BRPC) { \
	if (isdbg() && ((BRPC)==0xA0||(BRPC)==0xB0||(BRPC)==0xC0)) \
		dbg_bioscall((BRPC)); \
	/*else if (dbg_biosptr==(BRPC))*/ \
		/*dbg_biosreturn();*/ \
	else	dbgpsxregs(); \
}
#define dbg_bioscheckopcode(OPPC) { \
	if ((OPPC)==dbg_biosptr) { \
		dbg_biosreturn(); \
	} \
}
void dbg_bioscall(unsigned);
void dbg_biosreturn();
#define dbg_biosin() (dbg_biosptr!=0xFFFFFFFF)
#define dbg_biosunset() dbg_biosptr=0xFFFFFFFF
#define dbg_biosset(PTR) dbg_biosptr=(PTR)
#else
#define dbg_bioscheck(BRPC)
#define dbg_bioscheckopcode(OPPC)
#define dbg_bioscall(A)
#define dbg_biosreturn()
#define dbg_biosin() 0
#define dbg_biosunset()
#define dbg_biosset(PTR)
#endif

extern unsigned dbg_anacnt_softCall;
extern unsigned dbg_anacnt_softCall2;
extern unsigned dbg_anacnt_psxInterrupt;
extern unsigned dbg_anacnt_psxBiosException;
extern unsigned dbg_anacnt_psxRcntUpdate;
extern unsigned dbg_anacnt_psxRcntRcount;
extern unsigned dbg_anacnt_psxRcntRmode;
extern unsigned dbg_anacnt_spuInterrupt;
extern unsigned dbg_anacnt_psxDma4;
extern unsigned dbg_anacnt_psxDma2;
extern unsigned dbg_anacnt_gpuInterrupt;
extern unsigned dbg_anacnt_psxDma6;
extern unsigned dbg_anacnt_CDR_playthread;
extern unsigned dbg_anacnt_CDR_stopCDDA;
extern unsigned dbg_anacnt_CDR_startCDDA;
extern unsigned dbg_anacnt_CDR_playCDDA;
extern unsigned dbg_anacnt_CDR_getTN;
extern unsigned dbg_anacnt_CDR_getTD;
extern unsigned dbg_anacnt_CDR_readTrack;
extern unsigned dbg_anacnt_CDR_play;
extern unsigned dbg_anacnt_psxHwRead8;
extern unsigned dbg_anacnt_psxHwRead16;
extern unsigned dbg_anacnt_psxHwRead32;
extern unsigned dbg_anacnt_psxHwWrite8;
extern unsigned dbg_anacnt_psxHwWrite16;
extern unsigned dbg_anacnt_psxHwWrite32;
extern unsigned dbg_anacnt_PAD1_startPoll;
extern unsigned dbg_anacnt_PAD2_startPoll;
extern unsigned dbg_anacnt_PAD1_poll;
extern unsigned dbg_anacnt_PAD2_poll;
extern unsigned dbg_anacnt_hleDummy;
extern unsigned dbg_anacnt_hleA0;
extern unsigned dbg_anacnt_hleB0;
extern unsigned dbg_anacnt_hleC0;
extern unsigned dbg_anacnt_hleBootstrap;
extern unsigned dbg_anacnt_hleExecRet;
extern unsigned dbg_anacnt_cdrInterrupt;
extern unsigned dbg_anacnt_cdrReadInterrupt;
extern unsigned dbg_anacnt_cdrRead0;
extern unsigned dbg_anacnt_cdrWrite0;
extern unsigned dbg_anacnt_cdrRead1;
extern unsigned dbg_anacnt_cdrWrite1;
extern unsigned dbg_anacnt_cdrRead2;
extern unsigned dbg_anacnt_cdrWrite2;
extern unsigned dbg_anacnt_cdrRead3;
extern unsigned dbg_anacnt_cdrWrite3;
extern unsigned dbg_anacnt_psxDma3;
extern unsigned dbg_anacnt_psxException;
extern unsigned dbg_anacnt_psxBranchTest;
extern unsigned dbg_anacnt_psxExecuteBios;
extern unsigned dbg_anacnt_GPU_sendPacket;
extern unsigned dbg_anacnt_GPU_updateLace;
extern unsigned dbg_anacnt_GPU_writeStatus;
extern unsigned dbg_anacnt_GPU_readStatus;
extern unsigned dbg_anacnt_GPU_writeData;
extern unsigned dbg_anacnt_GPU_writeDataMem;
extern unsigned dbg_anacnt_GPU_readData;
extern unsigned dbg_anacnt_GPU_readDataMem;
extern unsigned dbg_anacnt_GPU_dmaChain;
extern unsigned dbg_anacnt_GPU_sendPacket;
extern unsigned dbg_anacnt_GPU_dmaChain;
extern unsigned dbg_anacnt_GPU_writeDataMem;
extern unsigned dbg_anacnt_GPU_writeData;
extern unsigned dbg_anacnt_GPU_readDataMem;
extern unsigned dbg_anacnt_GPU_readData;
extern unsigned dbg_anacnt_GPU_readStatus;
extern unsigned dbg_anacnt_GPU_writeStatus;
extern unsigned dbg_anacnt_GPU_updateLace;
extern unsigned dbg_anacnt_SPU_writeRegister;
extern unsigned dbg_anacnt_SPU_readRegister;
extern unsigned dbg_anacnt_SPU_readDMA;
extern unsigned dbg_anacnt_SPU_writeDMA;
extern unsigned dbg_anacnt_SPU_writeDMAMem;
extern unsigned dbg_anacnt_SPU_readDMAMem;
extern unsigned dbg_anacnt_SPU_playADPCM;
extern unsigned dbg_anacnt_SPU_async;
extern unsigned dbg_anacnt_SPU_playCDDA;
extern unsigned dbg_anacnt_SPU_writeRegister;
extern unsigned dbg_anacnt_SPU_readRegister;
extern unsigned dbg_anacnt_SPU_async;
extern unsigned dbg_anacnt_SPU_playADPCM;
extern unsigned dbg_anacnt_SPU_playCDDA;
extern unsigned dbg_anacnt_SPU_readDMA;
extern unsigned dbg_anacnt_SPU_readDMAMem;
extern unsigned dbg_anacnt_SPU_readDMA;
extern unsigned dbg_anacnt_SPU_writeDMAMem;
extern unsigned dbg_anacnt_SIO_Int;
extern unsigned dbg_anacnt_sioWrite8;
extern unsigned dbg_anacnt_sioWriteStat16;
extern unsigned dbg_anacnt_sioWriteMode16;
extern unsigned dbg_anacnt_sioWriteCtrl16;
extern unsigned dbg_anacnt_sioWriteBaud16;
extern unsigned dbg_anacnt_sioRead8;
extern unsigned dbg_anacnt_sioReadStat16;
extern unsigned dbg_anacnt_sioReadMode16;
extern unsigned dbg_anacnt_sioReadCtrl16;
extern unsigned dbg_anacnt_sioReadBaud16;
extern unsigned dbg_anacnt_sioInterrupt;
extern unsigned dbg_anacnt_mdecWrite0;
extern unsigned dbg_anacnt_mdecRead0;
extern unsigned dbg_anacnt_mdecWrite1;
extern unsigned dbg_anacnt_mdecRead1;
extern unsigned dbg_anacnt_psxDma0;
extern unsigned dbg_anacnt_psxDma1;
extern unsigned dbg_anacnt_mdec1Interrupt;
extern unsigned dbg_anacnt_psxMemRead8;
extern unsigned dbg_anacnt_psxMemRead8_direct;
extern unsigned dbg_anacnt_psxMemRead16;
extern unsigned dbg_anacnt_psxMemRead16_direct;
extern unsigned dbg_anacnt_psxMemRead32;
extern unsigned dbg_anacnt_psxMemRead32_direct;
extern unsigned dbg_anacnt_psxMemWrite8;
extern unsigned dbg_anacnt_psxMemWrite8_direct;
extern unsigned dbg_anacnt_psxMemWrite16;
extern unsigned dbg_anacnt_psxMemWrite16_direct;
extern unsigned dbg_anacnt_psxMemWrite32_error;
extern unsigned dbg_anacnt_psxMemWrite32;
extern unsigned dbg_anacnt_psxMemWrite32_direct;
extern unsigned dbg_anacnt_LoadCdrom;
extern unsigned dbg_anacnt_LoadCdromFile;
extern unsigned dbg_anacnt_CheckCdrom;
extern unsigned dbg_anacnt_Load;

#else

#define isdbg() 0
#define isdbg_frame() 0
#define dbg_enable()
#define dbg_disable()
#define dbg_enable_frame()
#define dbg(TEXTO)
#define dbgf(FORMATO, RESTO...)
#define dbgsum(STR,BUF,LEN)
#define dbgregs(REGS)
#define dbgregsCop(REGS)
#define dbgpsxregs()
#define dbgpsxregsCop()
#define dbg_opcode(A,B,C,D)
#define dbg_print_analysis()
#define dbg_bioscheck(BRPC)
#define dbg_bioscheckopcode(OPPC)
#define dbg_bioscall(A)
#define dbg_biosreturn()
#define dbg_biosin() 0
#define dbg_biosunset()
#define dbg_biosset(PTR)

#endif


#endif
