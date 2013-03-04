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

#include "psxcommon.h"
#include "spu.h"
#include "port.h"
#include "profiler.h"

// globals

// psx buffer / addresses

unsigned short  regArea[10000];
unsigned short  spuMem[256*1024];
unsigned char * spuMemC;
unsigned char * pSpuIrq=0;
unsigned char * pSpuBuffer;

// MAIN infos struct for each channel

SPUCHAN         s_chan[MAXCHAN+1];                     // channel + 1 infos (1 is security for fmod handling)

unsigned short  spuCtrl=0;                             // some vars to store psx reg infos
unsigned short  spuStat=0;
unsigned short  spuIrq=0;
unsigned long   spuAddr=0xffffffff;                    // address into spu mem

unsigned int dwNewChannel=0;                           // flags for faster testing, if new channel starts
unsigned int dwChannelOn=0;
unsigned long dwPendingChanOff=0;

// certain globals (were local before, but with the new timeproc I need em global)

static const int f[5][2] = {   {    0,  0  },
                        {   60,  0  },
                        {  115, -52 },
                        {   98, -55 },
                        {  122, -60 } };
signed int SSumL[NSSIZE];
signed int iFMod[NSSIZE];
signed short * pS;

// configuration
bool nullspu=false; // no sound

////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

// dirty inline func includes

#include "spu_regs.h"
#include "spu_dma.h"
#include "spu_adsr.h"
#include "spu_xa.h"

////////////////////////////////////////////////////////////////////////
// START SOUND... called by main thread to setup a new sound on a channel
////////////////////////////////////////////////////////////////////////

INLINE void StartSound(SPUCHAN *l_chan)
{
	l_chan->ADSRX.State=0;
	l_chan->ADSRX.EnvelopeVol=0;
	l_chan->SB[26]=0;                                     // init mixing vars
	l_chan->SB[27]=0;
	l_chan->iSBPos=28;
	l_chan->SB[29]=0;                                  // init our interpolation helpers
	l_chan->spos=0x10000L;
}

////////////////////////////////////////////////////////////////////////
// ALL KIND OF HELPERS
////////////////////////////////////////////////////////////////////////

INLINE void VoiceChangeFrequency(SPUCHAN *l_chan)
{
	l_chan->iUsedFreq=l_chan->iActFreq;             // -> take it and calc steps
	l_chan->sinc=l_chan->iRawPitch<<4;
	if(!l_chan->sinc) l_chan->sinc=1;
	l_chan->sinc<<=1;
}

////////////////////////////////////////////////////////////////////////

INLINE void FModChangeFrequency(SPUCHAN *l_chan,int ns,signed int *mod)
{
	int NP=l_chan->iRawPitch;

	NP=SDIV(((32768L+mod[ns])*NP),32768L);

	if(NP>0x3fff) NP=0x3fff;
	if(NP<0x1)    NP=0x1;

	NP=(44100L*NP)/(4096L);                               // calc frequency

	l_chan->iActFreq=NP;
	l_chan->iUsedFreq=NP;
	l_chan->sinc=UDIV(((NP/10)<<16),4410);
	if(!l_chan->sinc) l_chan->sinc=1;
	l_chan->sinc<<=1;
	mod[ns]=0;
}                    

////////////////////////////////////////////////////////////////////////

// noise handler... just produces some noise data
// surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
// and sometimes the noise will be used as fmod modulation... pfff

INLINE int iGetNoiseVal(SPUCHAN *l_chan)
{
	static unsigned long   dwNoiseVal=1;                          // global noise generator
	int fa;

	if((dwNoiseVal<<=1)&0x80000000L)
	{
		dwNoiseVal^=0x0040001L;
		fa=((dwNoiseVal>>2)&0x7fff);
		fa=-fa;
	}
	else fa=(dwNoiseVal>>2)&0x7fff;

	// mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
	fa=l_chan->iOldNoise+SDIV((fa-l_chan->iOldNoise),((0x001f-((spuCtrl&0x3f00)>>9))+1));
	CLIP_SHORT(fa);
	l_chan->iOldNoise=fa;

	l_chan->SB[29] = fa;                               // -> store noise val in "current sample" slot
	return fa;
}                                 

////////////////////////////////////////////////////////////////////////

INLINE void StoreInterpolationVal(SPUCHAN *l_chan,int fa)
{
	if(l_chan->bFMod==2) l_chan->SB[29]=fa; // fmod freq channel
	else
	{
		if((spuCtrl&0x4000)==0) fa=0;                       // muted?
		else                                                // else adjust
		{
			CLIP_SHORT(fa);
		}
		l_chan->SB[29]=fa;                          // no interpolation
	}
}

////////////////////////////////////////////////////////////////////////
// MAIN SPU FUNCTION
// here is the main job handler... thread, timer or direct func call
// basically the whole sound processing is done in this fat func!
////////////////////////////////////////////////////////////////////////

static void SoundUpdate(void)
{
	int fa,ns,ch;
	int nssize=NSSIZE;
	signed int *sum=SSumL;
	signed int *mod=iFMod;
	int sval;
	
	// main channel loop
	for(ch=0;ch<MAXCHAN;ch++)                         // loop em all... we will collect 1 ms of sound of each playing channel
	{
		SPUCHAN *l_chan=&s_chan[ch];

		if(dwNewChannel&(1<<ch))
		{
			StartSound(l_chan); // start new sound
			dwNewChannel&=~(1<<ch); // clear new channel bit
		}
		if(!(dwChannelOn&(1<<ch))) continue;         // channel not playing? next
		if(l_chan->iActFreq!=l_chan->iUsedFreq) VoiceChangeFrequency(l_chan); // new psx frequency

		ns=0;
		while(ns<nssize)                                // loop until 1 ms of data is reached
		{
			if(l_chan->bFMod==1 && mod[ns])          // fmod freq channel
				FModChangeFrequency(l_chan,ns,mod);

			while(l_chan->spos>=0x10000L)
			{
				if(l_chan->iSBPos==28)                   // 28 reached?
				{
					unsigned char *start=l_chan->pCurr;                   // set up the current pos

					if (start==(unsigned char*)-1||(dwPendingChanOff&(1<<ch))) // special "stop" sign
					{
						dwChannelOn&=~(1<<ch);                  // -> turn everything off
						dwPendingChanOff&=~(1<<ch);
						s_chan[ch].bStop=true;
						l_chan->ADSRX.EnvelopeVol=0;
						goto ENDX;                              // -> and done for this channel
					}

					l_chan->iSBPos=0;
					{
						int *dest=l_chan->SB;
						int s_1=dest[27];
						int s_2=dest[26];
						int predict_nr;
						int shift_factor;
						int flags;
						predict_nr=(int)*start; start++;
						shift_factor=predict_nr&0xf; predict_nr >>= 4;
						flags=(int)*start; start++;
						if(predict_nr>4) predict_nr=0;
						{
							unsigned int nSample;
							int d,s;

							for (nSample=0;nSample<28;start++)      
							{
								d=(int)*start;

								s = (int)(signed short)((d & 0x0f) << 12);
								fa = (s>>shift_factor);
								fa += ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
								s_2=s_1;s_1=fa;

								dest[nSample++]=fa;

								s = (int)(signed short)((d & 0xf0) << 8);
								fa = (s>>shift_factor);
								fa += ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
								s_2=s_1;s_1=fa;

								dest[nSample++]=fa;
							}
						}

						// flag handler
						if(flags&4) l_chan->pLoop=start-16; // loop adress
						if(flags&1) // 1: stop/loop
						{
							if(!(flags&2)) dwPendingChanOff|=1<<ch;
							start = l_chan->pLoop;
						}
						if (start - spuMemC >= 0x80000) start = (unsigned char*)-1;
						l_chan->pCurr=start; // store values for next cycle
					}
				}

				fa=l_chan->SB[l_chan->iSBPos++];      // get sample data
				StoreInterpolationVal(l_chan,fa);               // store val for later interpolation
				l_chan->spos -= 0x10000L;
			}

			if(l_chan->bNoise) fa=iGetNoiseVal(l_chan); // get noise val
			else fa=l_chan->SB[29]; // get sample val

			sval = fa>>sound_scale[(MixADSR(ch)>>1)];  // mix adsr

			if(l_chan->bFMod==2) mod[ns]=sval; // store 1T sample data, use that to do fmod on next channel
			else sum[ns]+=sval>>l_chan->iLeftVolume; // left sound volume (psx volume goes from 0 ... 0x3fff)

			// ok, go on until 1 ms data of this channel is collected
			ns++;
			l_chan->spos += l_chan->sinc;
		}
ENDX:;
	}

	// here we have another 1 ms of sound data

	// mix XA infos (if any)
	MixXA(nssize,sum);

	// mix all channels into one buffer
	#ifndef __arm__
	{
		signed int dl;
		signed short *_pS=pS;
		for(ns=0;ns<nssize;ns++)
		{
			dl=sum[ns];
			sum[ns]=0;
			CLIP_SHORT(dl);
			*_pS++=((signed short)dl);
		}
		pS=_pS;
	}
	#else
	{
		signed int a;
		signed int b;
		const int m=0x7fff;
		signed short *_pS=pS;
		do
		{
			b=*sum; *sum++=0;
			asm ("mov %1,%2,asr#15;teq %1,%2,asr#31;eorne %0,%3,%2,asr#31" : "=r" (b) , "=r" (a) : "0" (b), "r"(m) : "cc" );	
			*_pS++=b;
		}
		while(--nssize);
		pS=_pS;
	}
	#endif

	// feed the sound: wanna have around 1/60 sec (16.666 ms) updates
	{
		static int iCycle=0;
		if (iCycle++ > CYCLES)
		{
			sound_set((unsigned char *)pSpuBuffer,((unsigned char *)pS)-((unsigned char *)pSpuBuffer));
			pS = (short *)(void *)pSpuBuffer;
			iCycle = 0;
		}
	}
	
	return;
}

// SPU ASYNC
void SPU_async(void)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_async++;
#endif
	if (!nullspu) SoundUpdate();
}

// XA AUDIO
void SPU_playADPCMchannel(xa_decode_t *xap)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_playADPCM++;
#endif
	if ((!nullspu) && (xap) && (xap->freq)) FeedXA(xap);
}

// CDDA AUDIO
void SPU_playCDDAchannel(unsigned char *pcm, int nbytes)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_playCDDA++;
#endif
	if ((!nullspu) && (pcm) && (nbytes>0)) FeedCDDA((unsigned char*)pcm, nbytes);
}

// SETUPSTREAMS: init most of the spu buffers
INLINE void SetupStreams(void)
{ 
	int i;

	pSpuBuffer=(unsigned char *)malloc(((CYCLES+2)*NSSIZE*2));            // alloc mixing buffer

	XAStart =                                             // alloc xa buffer
	(uint16_t *)malloc(22050 * sizeof(uint16_t));
	XAEnd   = XAStart + 22050;
	XAPlay  = XAStart;
	XAFeed  = XAStart;

	CDDAStart =                                           // alloc cdda buffer
	(uint16_t *)malloc(8192 * sizeof(uint16_t));
	CDDAEnd   = CDDAStart + 8192;
	CDDAPlay  = CDDAStart;
	CDDAFeed  = CDDAStart;

	for(i=0;i<MAXCHAN;i++)                                // loop sound channels
	{
		s_chan[i].ADSRX.SustainLevel = 0x000f;                // -> init sustain
		s_chan[i].pLoop=spuMemC;
		s_chan[i].pStart=spuMemC;
		s_chan[i].pCurr=spuMemC;
	}
}

// REMOVESTREAMS: free most buffer
void RemoveStreams(void)
{ 
	free(pSpuBuffer);                                     // free mixing buffer
	pSpuBuffer = NULL;
	free(XAStart);                                        // free XA buffer
	XAStart = NULL;
	free(CDDAStart);                                      // free CDDA buffer
	CDDAStart = NULL;
}

// SPU_init: called by main emu after init
long SPU_init(void)
{
	spuMemC=(unsigned char *)spuMem;                      // just small setup
	if (!nullspu)
	{
		memset((void *)s_chan,0,MAXCHAN*sizeof(SPUCHAN));
		InitADSR();

		spuIrq = 0;
		spuAddr = 0xffffffff;
		spuMemC = (unsigned char *)spuMem;
		memset((void *)s_chan, 0, (MAXCHAN + 1) * sizeof(SPUCHAN));
		pSpuIrq = 0;

		sound_init();                                         // setup sound (before init!)

		SetupStreams();                                       // prepare streaming

		memset(SSumL,0,NSSIZE*sizeof(int));
		memset(iFMod,0,NSSIZE*sizeof(int));
		pS=(short *)(void *)pSpuBuffer;                               // setup soundbuffer pointer
	}
	return 0;
}

// SPU_shutdown: called by main emu on final exit
long SPU_shutdown(void)
{
	if (!nullspu)
	{
		sound_close();                                        // no more sound handling
		RemoveStreams();                                      // no more streaming
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
// freeze structs
////////////////////////////////////////////////////////////////////////

typedef struct
{
	char          szSPUName[8];
	uint32_t ulFreezeVersion;
	uint32_t ulFreezeSize;
	unsigned char cSPUPort[0x200];
	unsigned char cSPURam[0x80000];
	xa_decode_t   xaS;     
} SPUFreeze_t;

typedef struct
{
	unsigned short  spuIrq;
	uint32_t   pSpuIrq;
	uint32_t   spuAddr;
	uint32_t   dummy1;
	uint32_t   dummy2;
	uint32_t   dummy3;

	SPUCHAN  s_chan[MAXCHAN];   

} SPUOSSFreeze_t;

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

INLINE void LoadStateV5(SPUFreeze_t * pF)
{
	int i;SPUOSSFreeze_t * pFO;

	pFO=(SPUOSSFreeze_t *)(pF+1);

	spuIrq   = pFO->spuIrq;
	if(pFO->pSpuIrq)  pSpuIrq  = pFO->pSpuIrq+spuMemC;  else pSpuIrq=0;

	if(pFO->spuAddr)
	{
		spuAddr = pFO->spuAddr;
		if (spuAddr == 0xbaadf00d) spuAddr = 0;
	}

	dwNewChannel=0;
	dwChannelOn=0;
	
	for(i=0;i<MAXCHAN;i++)
	{
		memcpy((void *)&s_chan[i],(void *)&pFO->s_chan[i],sizeof(SPUCHAN));

		s_chan[i].pStart+=(unsigned long)spuMemC;
		s_chan[i].pCurr+=(unsigned long)spuMemC;
		s_chan[i].pLoop+=(unsigned long)spuMemC;
	}
}

////////////////////////////////////////////////////////////////////////

INLINE void LoadStateUnknown(SPUFreeze_t * pF)
{
	int i;

	for(i=0;i<MAXCHAN;i++)
	{
		s_chan[i].bStop=false;
		s_chan[i].pLoop=spuMemC;
		s_chan[i].pStart=spuMemC;
		s_chan[i].pLoop=spuMemC;
	}

	dwNewChannel=0;
	dwChannelOn=0;
	pSpuIrq=0;

	for(i=0;i<0xc0;i++)
	{
		SPU_writeRegister(0x1f801c00+i*2,regArea[i]);
	}
}

////////////////////////////////////////////////////////////////////////
// SPU_freeze: called by main emu on savestate load/save
////////////////////////////////////////////////////////////////////////

long SPU_freeze(uint32_t ulFreezeMode,SPUFreeze_t * pF)
{
	int i;SPUOSSFreeze_t * pFO;

	if(!pF) return 0;                                     // first check

	if(ulFreezeMode)                                      // info or save?
	{//--------------------------------------------------//
		if(ulFreezeMode==1)                                 
		memset(pF,0,sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t));

		strcpy(pF->szSPUName,"PBOSS");
		pF->ulFreezeVersion=5;
		pF->ulFreezeSize=sizeof(SPUFreeze_t)+sizeof(SPUOSSFreeze_t);

		if(ulFreezeMode==2) return 1;                       // info mode? ok, bye
		// save mode:
		memcpy(pF->cSPURam,spuMem,0x80000);                 // copy common infos
		memcpy(pF->cSPUPort,regArea,0x200);

		if (nullspu)
		{
			memset(&pF->xaS,0,sizeof(xa_decode_t));
			return 1;
		}

		if(xapGlobal && XAPlay!=XAFeed)                     // some xa
		{
			pF->xaS=*xapGlobal;     
		}
		else 
		memset(&pF->xaS,0,sizeof(xa_decode_t));             // or clean xa

		pFO=(SPUOSSFreeze_t *)(pF+1);                       // store special stuff

		pFO->spuIrq=spuIrq;
		if(pSpuIrq)  pFO->pSpuIrq  = (unsigned long)pSpuIrq-(unsigned long)spuMemC;

		pFO->spuAddr=spuAddr;
		if(pFO->spuAddr==0) pFO->spuAddr=0xbaadf00d;

		for(i=0;i<MAXCHAN;i++)
		{
			memcpy((void *)&pFO->s_chan[i],(void *)&s_chan[i],sizeof(SPUCHAN));
			if(pFO->s_chan[i].pStart)
			pFO->s_chan[i].pStart-=(unsigned long)spuMemC;
			if(pFO->s_chan[i].pCurr)
			pFO->s_chan[i].pCurr-=(unsigned long)spuMemC;
			if(pFO->s_chan[i].pLoop)
			pFO->s_chan[i].pLoop-=(unsigned long)spuMemC;
		}

		memset(SSumL,0,NSSIZE*sizeof(int));
		memset(iFMod,0,NSSIZE*sizeof(int));
		pS=(short *)(void *)pSpuBuffer;                               // setup soundbuffer pointer

		return 1;
		//--------------------------------------------------//
	}
	
	if(ulFreezeMode!=0) return 0;                         // bad mode? bye

	memcpy(spuMem,pF->cSPURam,0x80000);                   // get ram
	memcpy(regArea,pF->cSPUPort,0x200);

	if (nullspu)
	{
		for(i=0;i<0x100;i++)
		{
			if(i!=H_SPUon1-0xc00 && i!=H_SPUon2-0xc00)
			SPU_writeRegister(0x1f801c00+i*2,regArea[i]);
		}
		SPU_writeRegister(H_SPUon1,regArea[(H_SPUon1-0xc00)/2]);
		SPU_writeRegister(H_SPUon2,regArea[(H_SPUon2-0xc00)/2]);

		return 1;
	}

	if(pF->xaS.nsamples<=4032)                            // start xa again
	SPU_playADPCMchannel(&pF->xaS);

	xapGlobal=0;

	if(!strcmp(pF->szSPUName,"PBOSS") &&                  
			pF->ulFreezeVersion==5)
	LoadStateV5(pF);
	else LoadStateUnknown(pF);

	SPU_writeRegister(H_SPUctrl,(unsigned short)(regArea[(H_SPUctrl-0xc00)>>1]|0x4000));
	SPU_writeRegister(H_SPUstat,regArea[(H_SPUstat-0xc00)>>1]);
	SPU_writeRegister(H_CDLeft,regArea[(H_CDLeft-0xc00)>>1]);

	// fix to prevent new interpolations from crashing
	for(i=0;i<MAXCHAN;i++) s_chan[i].SB[28]=0;

	memset(SSumL,0,NSSIZE*sizeof(int));
	memset(iFMod,0,NSSIZE*sizeof(int));
	pS=(short *)(void *)pSpuBuffer;                               // setup soundbuffer pointer

	return 1;
}
