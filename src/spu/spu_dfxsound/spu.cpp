/***************************************************************************
                            spu.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include "spu.h"
#include "port.h"

// globals
bool nullspu=false;

// psx buffer / addresses

unsigned short  regArea[10000];
unsigned short  spuMem[256*1024];
unsigned char * spuMemC;
unsigned char * pSpuIrq=0;
unsigned char * pSpuBuffer;
unsigned char * pMixIrq=0;

// user settings
int             iDisStereo=0; /* 0=stereo, 1=mono */
int             iUseInterpolation=0; /* 0=disabled, 1=enabled */
int             iUseReverb=0; /* 0=disabled, 1=enabled */

// MAIN infos struct for each channel

SPUCHAN         s_chan[MAXCHAN+1];                     // channel + 1 infos (1 is security for fmod handling)
REVERBInfo      rvb;

unsigned long   dwNoiseVal=1;                          // global noise generator
int             iSpuAsyncWait=0;

unsigned short  spuCtrl=0;                             // some vars to store psx reg infos
unsigned short  spuStat=0;
unsigned short  spuIrq=0;
unsigned long   spuAddr=0xffffffff;                    // address into spu mem

unsigned long dwNewChannel=0;                          // flags for faster testing, if new channel starts

// certain globals (were local before, but with the new timeproc I need em global)

static const int f[5][2] = {   {    0,  0  },
                        {   60,  0  },
                        {  115, -52 },
                        {   98, -55 },
                        {  122, -60 } };
int SSumR[NSSIZE];
int SSumL[NSSIZE];
int iFMod[NSSIZE];
int iCycle=0;
short * pS;

static int lastch=-1;      // last channel processed on spu irq in timer mode
static int lastns=0;       // last ns pos
static int iSecureStart=0; // secure start counter

////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

// dirty inline func includes

#include "spu_reverb.h"
#include "spu_regs.h"
#include "spu_dma.h"
#include "spu_adsr.h"
#include "spu_interp.h"
#include "spu_xa.h"

////////////////////////////////////////////////////////////////////////
// START SOUND... called by main thread to setup a new sound on a channel
////////////////////////////////////////////////////////////////////////

INLINE void StartSound(int ch)
{
 StartADSR(ch);
 StartREVERB(ch);

 s_chan[ch].pCurr=s_chan[ch].pStart;                   // set sample start

 s_chan[ch].s_1=0;                                     // init mixing vars
 s_chan[ch].s_2=0;
 s_chan[ch].iSBPos=28;

 s_chan[ch].bNew=0;                                    // init channel flags
 s_chan[ch].bStop=0;
 s_chan[ch].bOn=1;

 s_chan[ch].SB[29]=0;                                  // init our interpolation helpers
 s_chan[ch].SB[30]=0;

 s_chan[ch].spos=0x10000L;s_chan[ch].SB[31]=0;  	   // -> no/simple interpolation starts with one 44100 decoding

 dwNewChannel&=~(1<<ch);                               // clear new channel bit
}

////////////////////////////////////////////////////////////////////////
// ALL KIND OF HELPERS
////////////////////////////////////////////////////////////////////////

INLINE void VoiceChangeFrequency(int ch)
{
 s_chan[ch].iUsedFreq=s_chan[ch].iActFreq;             // -> take it and calc steps
 s_chan[ch].sinc=s_chan[ch].iRawPitch<<4;
 if(!s_chan[ch].sinc) s_chan[ch].sinc=1;
 if(iUseInterpolation) s_chan[ch].SB[32]=1;         // -> freq change in simle imterpolation mode: set flag
}

////////////////////////////////////////////////////////////////////////

INLINE void FModChangeFrequency(int ch,int ns)
{
 int NP=s_chan[ch].iRawPitch;

 NP=((32768L+iFMod[ns])*NP)/32768L;

 if(NP>0x3fff) NP=0x3fff;
 if(NP<0x1)    NP=0x1;

 NP=(44100L*NP)/(4096L);                               // calc frequency

 s_chan[ch].iActFreq=NP;
 s_chan[ch].iUsedFreq=NP;
 s_chan[ch].sinc=(((NP/10)<<16)/4410);
 if(!s_chan[ch].sinc) s_chan[ch].sinc=1;
 if(iUseInterpolation)                              // freq change in simple interpolation mode
 s_chan[ch].SB[32]=1;
 iFMod[ns]=0;
}                    

////////////////////////////////////////////////////////////////////////

// noise handler... just produces some noise data
// surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
// and sometimes the noise will be used as fmod modulation... pfff

INLINE int iGetNoiseVal(int ch)
{
 int fa;

 if((dwNoiseVal<<=1)&0x80000000L)
  {
   dwNoiseVal^=0x0040001L;
   fa=((dwNoiseVal>>2)&0x7fff);
   fa=-fa;
  }
 else fa=(dwNoiseVal>>2)&0x7fff;

 // mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
 fa=s_chan[ch].iOldNoise+((fa-s_chan[ch].iOldNoise)/((0x001f-((spuCtrl&0x3f00)>>9))+1));
 if(fa>32767L)  fa=32767L;
 else if(fa<-32767L) fa=-32767L;              
 s_chan[ch].iOldNoise=fa;

 s_chan[ch].SB[29] = fa;                               // -> store noise val in "current sample" slot
 return fa;
}                                 

////////////////////////////////////////////////////////////////////////

INLINE void StoreInterpolationVal(int ch,int fa)
{
 if(s_chan[ch].bFMod==2)                               // fmod freq channel
  s_chan[ch].SB[29]=fa;
 else
  {
   if((spuCtrl&0x4000)==0) fa=0;                       // muted?
   else                                                // else adjust
    {
     if(fa>32767L)  fa=32767L;
     else if(fa<-32767L) fa=-32767L;              
    }

   if(iUseInterpolation)                            // simple interpolation
    {
     s_chan[ch].SB[28] = 0;                    
     s_chan[ch].SB[29] = s_chan[ch].SB[30];            // -> helpers for simple linear interpolation: delay real val for two slots, and calc the two deltas, for a 'look at the future behaviour'
     s_chan[ch].SB[30] = s_chan[ch].SB[31];
     s_chan[ch].SB[31] = fa;
     s_chan[ch].SB[32] = 1;                            // -> flag: calc new interolation
    }
   else s_chan[ch].SB[29]=fa;                          // no interpolation
  }
}

////////////////////////////////////////////////////////////////////////

INLINE int iGetInterpolationVal(int ch)
{
 int fa;

 if(s_chan[ch].bFMod==2) return s_chan[ch].SB[29];

  if(iUseInterpolation)
  {   
     if(s_chan[ch].sinc<0x10000L)                      // -> upsampling?
          InterpolateUp(ch);                           // --> interpolate up
     else InterpolateDown(ch);                         // --> else down
     fa=s_chan[ch].SB[29];
  }
  else
  {
     fa=s_chan[ch].SB[29];                  
  }

 return fa;
}

////////////////////////////////////////////////////////////////////////
// MAIN SPU FUNCTION
// here is the main job handler... thread, timer or direct func call
// basically the whole sound processing is done in this fat func!
////////////////////////////////////////////////////////////////////////

static void SoundUpdate(void)
{
 int s_1,s_2,fa,ns;
 unsigned char * start;unsigned int nSample;
 int ch,predict_nr,shift_factor,flags,d,s;
 int bIRQReturn=0;

   if(dwNewChannel)                                    // new channel should start immedately?
    {                                                  // (at least one bit 0 ... MAXCHANNEL is set?)
     iSecureStart++;                                   // -> set iSecure
     if(iSecureStart>5) iSecureStart=0;                //    (if it is set 5 times - that means on 5 tries a new samples has been started - in a row, we will reset it, to give the sound update a chance)
    }
   else iSecureStart=0;                                // 0: no new channel should start

   if (!iSecureStart &&                 			   // no new start?
         (sound_get()>TESTSIZE))           // and still enuff data in sound buffer?
    {
     iSecureStart=0;                                   // reset secure

     return;                           			   	   // linux no-thread mode? bye
    }

   //--------------------------------------------------// continue from irq handling in timer mode? 

   if(lastch>=0)                                       // will be -1 if no continue is pending
    {
     ch=lastch; ns=lastns; lastch=-1;                  // -> setup all kind of vars to continue
     goto GOON;                                        // -> directly jump to the continue point
    }

   //--------------------------------------------------//
   //- main channel loop                              -// 
   //--------------------------------------------------//
    {
     for(ch=0;ch<MAXCHAN;ch++)                         // loop em all... we will collect 1 ms of sound of each playing channel
      {
       if(s_chan[ch].bNew) StartSound(ch);             // start new sound
       if(!s_chan[ch].bOn) continue;                   // channel not playing? next

       if(s_chan[ch].iActFreq!=s_chan[ch].iUsedFreq)   // new psx frequency?
        VoiceChangeFrequency(ch);

       ns=0;
       while(ns<NSSIZE)                                // loop until 1 ms of data is reached
        {
         if(s_chan[ch].bFMod==1 && iFMod[ns])          // fmod freq channel
          FModChangeFrequency(ch,ns);

         while(s_chan[ch].spos>=0x10000L)
          {
           if(s_chan[ch].iSBPos==28)                   // 28 reached?
            {
             start=s_chan[ch].pCurr;                   // set up the current pos

             if (start == (unsigned char*)-1)          // special "stop" sign
              {
               s_chan[ch].bOn=0;                       // -> turn everything off
               s_chan[ch].ADSRX.lVolume=0;
               s_chan[ch].ADSRX.EnvelopeVol=0;
               goto ENDX;                              // -> and done for this channel
              }

             s_chan[ch].iSBPos=0;

             //////////////////////////////////////////// spu irq handler here? mmm... do it later

             s_1=s_chan[ch].s_1;
             s_2=s_chan[ch].s_2;

             predict_nr=(int)*start;start++;
             shift_factor=predict_nr&0xf;
             predict_nr >>= 4;
             flags=(int)*start;start++;

             // -------------------------------------- // 

             for (nSample=0;nSample<28;start++)      
              {
               d=(int)*start;
               s=((d&0xf)<<12);
               if(s&0x8000) s|=0xffff0000;

               fa=(s >> shift_factor);
               fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
               s_2=s_1;s_1=fa;
               s=((d & 0xf0) << 8);

               s_chan[ch].SB[nSample++]=fa;

               if(s&0x8000) s|=0xffff0000;
               fa=(s>>shift_factor);
               fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
               s_2=s_1;s_1=fa;

               s_chan[ch].SB[nSample++]=fa;
              }

             //////////////////////////////////////////// flag handler

             if((flags&4) && (!s_chan[ch].bIgnoreLoop))
              s_chan[ch].pLoop=start-16;               // loop adress

             if(flags&1)                               // 1: stop/loop
              {
               // We play this block out first...
               //if(!(flags&2))                          // 1+2: do loop... otherwise: stop
               if(flags!=3 || s_chan[ch].pLoop==NULL)  // PETE: if we don't check exactly for 3, loop hang ups will happen (DQ4, for example)
                {                                      // and checking if pLoop is set avoids crashes, yeah
                 start = (unsigned char*)-1;
                }
               else
                {
                 start = s_chan[ch].pLoop;
                }
              }

             s_chan[ch].pCurr=start;                   // store values for next cycle
             s_chan[ch].s_1=s_1;
             s_chan[ch].s_2=s_2;

             if(bIRQReturn)                            // special return for "spu irq - wait for cpu action"
              {
               bIRQReturn=0;
               lastch=ch; 
               lastns=ns;
               return;
              }

GOON: ;
            }

           fa=s_chan[ch].SB[s_chan[ch].iSBPos++];      // get sample data

           StoreInterpolationVal(ch,fa);               // store val for later interpolation

           s_chan[ch].spos -= 0x10000L;
          }

         if(s_chan[ch].bNoise)
              fa=iGetNoiseVal(ch);                     // get noise val
         else fa=iGetInterpolationVal(ch);             // get sample val

         s_chan[ch].sval = (MixADSR(ch) * fa) / 1023;  // mix adsr

         if(s_chan[ch].bFMod==2)                       // fmod freq channel
          iFMod[ns]=s_chan[ch].sval;                   // -> store 1T sample data, use that to do fmod on next channel
         else                                          // no fmod freq channel
          {
           //////////////////////////////////////////////
           // ok, left/right sound volume (psx volume goes from 0 ... 0x3fff)

           if(s_chan[ch].iMute) 
            s_chan[ch].sval=0;                         // debug mute
           else
            {
             SSumL[ns]+=(s_chan[ch].sval*s_chan[ch].iLeftVolume)/0x4000L;
             SSumR[ns]+=(s_chan[ch].sval*s_chan[ch].iRightVolume)/0x4000L;
            }

           //////////////////////////////////////////////
           // now let us store sound data for reverb    

           if(s_chan[ch].bRVBActive) StoreREVERB(ch,ns);
          }

         ////////////////////////////////////////////////
         // ok, go on until 1 ms data of this channel is collected

         ns++;
         s_chan[ch].spos += s_chan[ch].sinc;

        }
ENDX:   ;
      }
    }

  //---------------------------------------------------//
  //- here we have another 1 ms of sound data
  //---------------------------------------------------//
  // mix XA infos (if any)

  MixXA();
  
  ///////////////////////////////////////////////////////
  // mix all channels (including reverb) into one buffer

  if(iDisStereo)                                       // no stereo?
   {
    int dl,dr;
    for(ns=0;ns<NSSIZE;ns++)
     {
      SSumL[ns]+=MixREVERBLeft(ns);

      dl=SSumL[ns];SSumL[ns]=0;
      if(dl<-32767) dl=-32767; else if(dl>32767) dl=32767;

      SSumR[ns]+=MixREVERBRight();

      dr=SSumR[ns];SSumR[ns]=0;
      if(dr<-32767) dr=-32767; else if(dr>32767) dr=32767;
      *pS++=(dl+dr)/2;
     }
   }
  else                                                 // stereo:
  for (ns = 0; ns < NSSIZE; ns++)
   {
    SSumL[ns] += MixREVERBLeft(ns);

    d = SSumL[ns]; SSumL[ns] = 0;
    if (d < -32767) d = -32767; else if (d > 32767) d = 32767;
    *pS++ = d;

    SSumR[ns] += MixREVERBRight();

    d = SSumR[ns]; SSumR[ns] = 0;
    if(d < -32767) d = -32767; else if(d > 32767) d = 32767;
    *pS++ = d;
   }

  // feed the sound
  // wanna have around 1/60 sec (16.666 ms) updates
  if (iCycle++ > 16)
   {
    sound_set((unsigned char *)pSpuBuffer,
                        ((unsigned char *)pS) - ((unsigned char *)pSpuBuffer));
    pS = (short *)pSpuBuffer;
    iCycle = 0;
   }

 return;
}

// SPU ASYNC... even newer epsxe func
//  1 time every 'cycle' cycles... harhar

void CALLBACK SPU_async(void)
{
 if (nullspu) return;
 if(iSpuAsyncWait)
  {
   iSpuAsyncWait++;
   if(iSpuAsyncWait<=64) return;
   iSpuAsyncWait=0;
  }

   SoundUpdate();                                      // -> linux high-compat mode
}

// XA AUDIO

void CALLBACK SPU_playADPCMchannel(xa_decode_t *xap)
{
 if (nullspu) return;
 if(!xap)       return;
 if(!xap->freq) return;                                // no xa freq ? bye

 FeedXA(xap);                                          // call main XA feeder
}

// CDDA AUDIO
void CALLBACK SPU_playCDDAchannel(unsigned char *pcm, int nbytes)
{
 if (nullspu) return;
 if (!pcm)      return;
 if (nbytes<=0) return;

 FeedCDDA((unsigned char *)pcm, nbytes);
}

// INIT/EXIT STUFF

// SETUPTIMER: init of certain buffers and threads/timers
INLINE void SetupTimer(void)
{
 memset(SSumR,0,NSSIZE*sizeof(int));                   // init some mixing buffers
 memset(SSumL,0,NSSIZE*sizeof(int));
 memset(iFMod,0,NSSIZE*sizeof(int));
 pS=(short *)pSpuBuffer;                               // setup soundbuffer pointer
}

// SETUPSTREAMS: init most of the spu buffers
INLINE void SetupStreams(void)
{ 
 int i;

 pSpuBuffer=(unsigned char *)malloc(32768);            // alloc mixing buffer

 if(iUseReverb) i=88200*2;
 else              i=NSSIZE*2;

 sRVBStart = (int *)malloc(i*4);                       // alloc reverb buffer
 memset(sRVBStart,0,i*4);
 sRVBEnd  = sRVBStart + i;
 sRVBPlay = sRVBStart;

 XAStart =                                             // alloc xa buffer
  (uint32_t *)malloc(44100 * sizeof(uint32_t));
 XAEnd   = XAStart + 44100;
 XAPlay  = XAStart;
 XAFeed  = XAStart;

 CDDAStart =                                           // alloc cdda buffer
  (uint32_t *)malloc(16384 * sizeof(uint32_t));
 CDDAEnd   = CDDAStart + 16384;
 CDDAPlay  = CDDAStart;
 CDDAFeed  = CDDAStart + 1;

 for(i=0;i<MAXCHAN;i++)                                // loop sound channels
  {
// we don't use mutex sync... not needed, would only 
// slow us down:
//   s_chan[i].hMutex=CreateMutex(NULL,FALSE,NULL);
   s_chan[i].ADSRX.SustainLevel = 1024;                // -> init sustain
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
   s_chan[i].pLoop=spuMemC;
   s_chan[i].pStart=spuMemC;
   s_chan[i].pCurr=spuMemC;
  }

  pMixIrq=spuMemC;                                     // enable decoded buffer irqs by setting the address
}

// REMOVESTREAMS: free most buffer
void RemoveStreams(void)
{ 
 free(pSpuBuffer);                                     // free mixing buffer
 pSpuBuffer = NULL;
 free(sRVBStart);                                      // free reverb buffer
 sRVBStart = NULL;
 free(XAStart);                                        // free XA buffer
 XAStart = NULL;
 free(CDDAStart);                                      // free CDDA buffer
 CDDAStart = NULL;
}

// SPU_init: called by main emu after init
long CALLBACK SPU_init(void)
{
 spuMemC=(unsigned char *)spuMem;                      // just small setup
 if (nullspu) return 0;
 memset((void *)s_chan,0,MAXCHAN*sizeof(SPUCHAN));
 memset((void *)&rvb,0,sizeof(REVERBInfo));
 InitADSR();

 iReverbOff = -1;
 spuIrq = 0;
 spuAddr = 0xffffffff;
 spuMemC = (unsigned char *)spuMem;
 pMixIrq = 0;
 memset((void *)s_chan, 0, (MAXCHAN + 1) * sizeof(SPUCHAN));
 pSpuIrq = 0;

 sound_init();                                         // setup sound (before init!)

 SetupStreams();                                       // prepare streaming

 SetupTimer();                                         // timer for feeding data

 return PSE_SPU_ERR_SUCCESS;
}

// SPU_shutdown: called by main emu on final exit
long CALLBACK SPU_shutdown(void)
{
 if (nullspu) return 0;
 sound_close();                                        // no more sound handling
 RemoveStreams();                                      // no more streaming
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
 uint32_t   dummy0;
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

 for(i=0;i<MAXCHAN;i++)
  {
   memcpy((void *)&s_chan[i],(void *)&pFO->s_chan[i],sizeof(SPUCHAN));

   s_chan[i].pStart+=(unsigned long)spuMemC;
   s_chan[i].pCurr+=(unsigned long)spuMemC;
   s_chan[i].pLoop+=(unsigned long)spuMemC;
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
  }

  spuAddr = 0xffffffff;
}

////////////////////////////////////////////////////////////////////////

INLINE void LoadStateUnknown(SPUFreeze_t * pF)
{
 int i;

 for(i=0;i<MAXCHAN;i++)
  {
   s_chan[i].bOn=0;
   s_chan[i].bNew=0;
   s_chan[i].bStop=0;
   s_chan[i].ADSR.lVolume=0;
   s_chan[i].pLoop=spuMemC;
   s_chan[i].pStart=spuMemC;
   s_chan[i].pLoop=spuMemC;
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
  }

 dwNewChannel=0;
 pSpuIrq=0;

 for(i=0;i<0xc0;i++)
  {
   SPU_writeRegister(0x1f801c00+i*2,regArea[i]);
  }
}

////////////////////////////////////////////////////////////////////////
// SPU_freeze: called by main emu on savestate load/save
////////////////////////////////////////////////////////////////////////

long CALLBACK SPU_freeze(uint32_t ulFreezeMode,SPUFreeze_t * pF)
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

   if (nullspu) {
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

   SetupTimer();                                       // sound processing on again

   return 1;
   //--------------------------------------------------//
  }
                                                       
 if(ulFreezeMode!=0) return 0;                         // bad mode? bye

 memcpy(spuMem,pF->cSPURam,0x80000);                   // get ram
 memcpy(regArea,pF->cSPUPort,0x200);

 if (nullspu) {
	for(i=0;i<0x100;i++) {
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

 // repair some globals
 for(i=0;i<=62;i+=2)
  SPU_writeRegister(H_Reverb+i,regArea[(H_Reverb+i-0xc00)>>1]);
 SPU_writeRegister(H_SPUReverbAddr,regArea[(H_SPUReverbAddr-0xc00)>>1]);
 SPU_writeRegister(H_SPUrvolL,regArea[(H_SPUrvolL-0xc00)>>1]);
 SPU_writeRegister(H_SPUrvolR,regArea[(H_SPUrvolR-0xc00)>>1]);

 SPU_writeRegister(H_SPUctrl,(unsigned short)(regArea[(H_SPUctrl-0xc00)>>1]|0x4000));
 SPU_writeRegister(H_SPUstat,regArea[(H_SPUstat-0xc00)>>1]);
 SPU_writeRegister(H_CDLeft,regArea[(H_CDLeft-0xc00)>>1]);
 SPU_writeRegister(H_CDRight,regArea[(H_CDRight-0xc00)>>1]);

 // fix to prevent new interpolations from crashing
 for(i=0;i<MAXCHAN;i++) s_chan[i].SB[28]=0;

 SetupTimer();                                         // start sound processing again

 return 1;
}
