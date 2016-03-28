/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis                                            *
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

#ifndef _PSX_SPU_
#define _PSX_SPU_

#include <stdio.h>
#include <stdlib.h>
//#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h> 
#include <sys/time.h>  
#include <math.h>  
#include <stdint.h>

typedef struct
{
	long	y0, y1;
} ADPCM_Decode_t;

typedef struct
{                                                                   
	int				freq;
	int				nbits;
	int				stereo;
	int				nsamples;
	ADPCM_Decode_t	left, right;
	short			pcm[16384];
} xa_decode_t;

////////////////////////////////////////////////////////////////////////
// spu defines
////////////////////////////////////////////////////////////////////////

// num of channels
#define MAXCHAN     24

// ~ 1 ms of data
#define NSSIZE 32
#define CYCLES 16

#define H_SPUirqAddr     0x0da4
#define H_SPUaddr        0x0da6
#define H_SPUdata        0x0da8
#define H_SPUctrl        0x0daa
#define H_SPUstat        0x0dae
#define H_SPUmvolL       0x0d80
#define H_SPUmvolR       0x0d82
#define H_SPUon1         0x0d88
#define H_SPUon2         0x0d8a
#define H_SPUoff1        0x0d8c
#define H_SPUoff2        0x0d8e
#define H_FMod1          0x0d90
#define H_FMod2          0x0d92
#define H_Noise1         0x0d94
#define H_Noise2         0x0d96
#define H_SPUMute1       0x0d9c
#define H_SPUMute2       0x0d9e
#define H_CDLeft         0x0db0
#define H_SPUPitch0      0x0c04
#define H_SPUPitch1      0x0c14
#define H_SPUPitch2      0x0c24
#define H_SPUPitch3      0x0c34
#define H_SPUPitch4      0x0c44
#define H_SPUPitch5      0x0c54
#define H_SPUPitch6      0x0c64
#define H_SPUPitch7      0x0c74
#define H_SPUPitch8      0x0c84
#define H_SPUPitch9      0x0c94
#define H_SPUPitch10     0x0ca4
#define H_SPUPitch11     0x0cb4
#define H_SPUPitch12     0x0cc4
#define H_SPUPitch13     0x0cd4
#define H_SPUPitch14     0x0ce4
#define H_SPUPitch15     0x0cf4
#define H_SPUPitch16     0x0d04
#define H_SPUPitch17     0x0d14
#define H_SPUPitch18     0x0d24
#define H_SPUPitch19     0x0d34
#define H_SPUPitch20     0x0d44
#define H_SPUPitch21     0x0d54
#define H_SPUPitch22     0x0d64
#define H_SPUPitch23     0x0d74

#define H_SPUStartAdr0   0x0c06
#define H_SPUStartAdr1   0x0c16
#define H_SPUStartAdr2   0x0c26
#define H_SPUStartAdr3   0x0c36
#define H_SPUStartAdr4   0x0c46
#define H_SPUStartAdr5   0x0c56
#define H_SPUStartAdr6   0x0c66
#define H_SPUStartAdr7   0x0c76
#define H_SPUStartAdr8   0x0c86
#define H_SPUStartAdr9   0x0c96
#define H_SPUStartAdr10  0x0ca6
#define H_SPUStartAdr11  0x0cb6
#define H_SPUStartAdr12  0x0cc6
#define H_SPUStartAdr13  0x0cd6
#define H_SPUStartAdr14  0x0ce6
#define H_SPUStartAdr15  0x0cf6
#define H_SPUStartAdr16  0x0d06
#define H_SPUStartAdr17  0x0d16
#define H_SPUStartAdr18  0x0d26
#define H_SPUStartAdr19  0x0d36
#define H_SPUStartAdr20  0x0d46
#define H_SPUStartAdr21  0x0d56
#define H_SPUStartAdr22  0x0d66
#define H_SPUStartAdr23  0x0d76

#define H_SPULoopAdr0   0x0c0e
#define H_SPULoopAdr1   0x0c1e
#define H_SPULoopAdr2   0x0c2e
#define H_SPULoopAdr3   0x0c3e
#define H_SPULoopAdr4   0x0c4e
#define H_SPULoopAdr5   0x0c5e
#define H_SPULoopAdr6   0x0c6e
#define H_SPULoopAdr7   0x0c7e
#define H_SPULoopAdr8   0x0c8e
#define H_SPULoopAdr9   0x0c9e
#define H_SPULoopAdr10  0x0cae
#define H_SPULoopAdr11  0x0cbe
#define H_SPULoopAdr12  0x0cce
#define H_SPULoopAdr13  0x0cde
#define H_SPULoopAdr14  0x0cee
#define H_SPULoopAdr15  0x0cfe
#define H_SPULoopAdr16  0x0d0e
#define H_SPULoopAdr17  0x0d1e
#define H_SPULoopAdr18  0x0d2e
#define H_SPULoopAdr19  0x0d3e
#define H_SPULoopAdr20  0x0d4e
#define H_SPULoopAdr21  0x0d5e
#define H_SPULoopAdr22  0x0d6e
#define H_SPULoopAdr23  0x0d7e

#define H_SPU_ADSRLevel0   0x0c08
#define H_SPU_ADSRLevel1   0x0c18
#define H_SPU_ADSRLevel2   0x0c28
#define H_SPU_ADSRLevel3   0x0c38
#define H_SPU_ADSRLevel4   0x0c48
#define H_SPU_ADSRLevel5   0x0c58
#define H_SPU_ADSRLevel6   0x0c68
#define H_SPU_ADSRLevel7   0x0c78
#define H_SPU_ADSRLevel8   0x0c88
#define H_SPU_ADSRLevel9   0x0c98
#define H_SPU_ADSRLevel10  0x0ca8
#define H_SPU_ADSRLevel11  0x0cb8
#define H_SPU_ADSRLevel12  0x0cc8
#define H_SPU_ADSRLevel13  0x0cd8
#define H_SPU_ADSRLevel14  0x0ce8
#define H_SPU_ADSRLevel15  0x0cf8
#define H_SPU_ADSRLevel16  0x0d08
#define H_SPU_ADSRLevel17  0x0d18
#define H_SPU_ADSRLevel18  0x0d28
#define H_SPU_ADSRLevel19  0x0d38
#define H_SPU_ADSRLevel20  0x0d48
#define H_SPU_ADSRLevel21  0x0d58
#define H_SPU_ADSRLevel22  0x0d68
#define H_SPU_ADSRLevel23  0x0d78

#ifndef __arm__
#define CLIP_SHORT(x) { int sign = x >> 31; if (sign != (x >> 15)) x = sign ^ ((1 << 15) - 1); }
#else
INLINE void CLIP_SHORT(int &b) { const int m=0x7fff; int a; asm ("mov %1,%2,asr#15;teq %1,%2,asr#31;eorne %0,%3,%2,asr#31" : "=r" (b) , "=r" (a) : "0" (b), "r"(m) : "cc" ); }
#endif

///////////////////////////////////////////////////////////
// struct defines
///////////////////////////////////////////////////////////

typedef struct
{
	unsigned char  State:2;
	unsigned char  AttackModeExp:1;
	unsigned char  SustainModeExp:1;
	unsigned char  SustainIncrease:1;
	unsigned char  ReleaseModeExp:1;
	unsigned char  AttackRate;
	unsigned char  DecayRate;
	unsigned char  SustainLevel;
	unsigned char  SustainRate;
	unsigned char  ReleaseRate;
	int            EnvelopeVol;
} ADSRInfoEx;

///////////////////////////////////////////////////////////

// MAIN CHANNEL STRUCT
typedef struct
{
	int               iSBPos;                             // mixing stuff
	int               spos;
	int               sinc;
	int               iLeftVolume;                        // left volume (0..16)

	unsigned char *   pStart;                             // start ptr into sound mem
	unsigned char *   pCurr;                              // current pos in sound mem
	unsigned char *   pLoop;                              // loop ptr in sound mem
	
	bool              bStop:1;                            // is channel stopped (sample _can_ still be playing, ADSR Release phase)
	bool              bNoise:1;                           // noise active flag
	unsigned char     bFMod:2;                            // freq mod (0=off, 1=sound channel, 2=freq channel)

	int               iActFreq;                           // current psx pitch
	int               iUsedFreq;                          // current pc pitch
	ADSRInfoEx        ADSRX;                              // next ADSR settings (will be moved to active on sample start)
	int               iRawPitch;                          // raw pitch (0...3fff)
	int               iOldNoise;                          // old noise val for this channel   

	int               SB[30];
} SPUCHAN;

///////////////////////////////////////////////////////////
// SPU.C globals
///////////////////////////////////////////////////////////

// psx buffers / addresses

extern unsigned short  regArea[];                        
extern unsigned short  spuMem[];
extern unsigned char * spuMemC;
extern unsigned char * pSpuIrq;
extern unsigned char * pSpuBuffer;

// MISC

extern SPUCHAN s_chan[];

extern unsigned short spuCtrl;
extern unsigned short spuStat;
extern unsigned short spuIrq;
extern unsigned long  spuAddr;
extern unsigned int dwNewChannel;
extern unsigned int dwChannelOn;

extern int      SSumL[];
extern short *  pS;

///////////////////////////////////////////////////////////
// XA.C globals
///////////////////////////////////////////////////////////

extern xa_decode_t   * xapGlobal;

extern uint16_t * XAFeed;
extern uint16_t * XAPlay;
extern uint16_t * XAStart;
extern uint16_t * XAEnd;

extern uint32_t   XARepeat;

extern uint16_t * CDDAFeed;
extern uint16_t * CDDAPlay;
extern uint16_t * CDDAStart;
extern uint16_t * CDDAEnd;

extern unsigned int iLeftXAVol;

#endif
