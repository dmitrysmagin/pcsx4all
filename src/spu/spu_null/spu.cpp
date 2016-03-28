/////////////////////////////////////////////////////////

#define PSE_SPU_ERR_SUCCESS         0
#define PSE_SPU_ERR                 -60
#define PSE_SPU_ERR_NOTCONFIGURED   PSE_SPU_ERR -1
#define PSE_SPU_ERR_INIT            PSE_SPU_ERR -2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spu_regs.h"
#include "profiler.h"
#include "debug.h"
// some ms windows compatibility define
#undef CALLBACK
#define CALLBACK

////////////////////////////////////////////////////////////////////////

unsigned short  regArea[10000];                        // psx buffer
unsigned short  spuMem[256*1024];
unsigned char * spuMemC;
unsigned char * pSpuIrq=0;

unsigned short spuCtrl, spuStat, spuIrq=0;             // some vars to store psx reg infos
unsigned long  spuAddr=0xffffffff;                     // address into spu mem

////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

void CALLBACK SPU_writeRegister(unsigned long reg, unsigned short val)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_writeRegister++;
#endif
 unsigned long r=reg&0xfff;
 regArea[(r-0xc00)>>1] = val;

 if(r>=0x0c00 && r<0x0d80)
  {
   //int ch=(r>>4)-0xc0;
   switch(r&0x0f)
    {
     //------------------------------------------------// l volume
     case 0:                    
       //SetVolumeL(ch,val);
       return;
     //------------------------------------------------// r volume
     case 2:                                           
       //SetVolumeR(ch,val);
       return;
     //------------------------------------------------// pitch
     case 4:                                           
       //SetPitch(ch,val);
       return;
     //------------------------------------------------// start
     case 6:
       //s_chan[ch].pStart=spuMemC+((unsigned long) val<<3);
       return;
     //------------------------------------------------// adsr level 
     case 8:
       return;
     //------------------------------------------------// adsr rate 
     case 10:
      return;
     //------------------------------------------------// adsr volume
     case 12:
       return;
     //------------------------------------------------// loop adr
     case 14:                                          
       return;
     //------------------------------------------------//
    }
   return;
  }

 switch(r)
   {
    //-------------------------------------------------//
    case H_SPUaddr:
        spuAddr = (unsigned long) val<<3;
        return;
    //-------------------------------------------------//
    case H_SPUdata:
        spuMem[spuAddr>>1] = val;
        spuAddr+=2;
        if(spuAddr>0x7ffff) spuAddr=0;
        return;
    //-------------------------------------------------//
    case H_SPUctrl:
        spuCtrl=val;
        return;
    //-------------------------------------------------//
    case H_SPUstat:
        spuStat=val & 0xf800;
        return;
    //-------------------------------------------------//
    case H_SPUirqAddr:
        spuIrq = val;
        pSpuIrq=spuMemC+((unsigned long) val<<3);
        return;
    //-------------------------------------------------//
    case H_SPUon1:
        //SoundOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_SPUon2:
        //SoundOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_SPUoff1:
        //SoundOff(0,16,val);
        return;
    //-------------------------------------------------//
    case H_SPUoff2:
        //SoundOff(16,24,val);
        return;
    //-------------------------------------------------//
    case H_CDLeft:
        //if(cddavCallback) cddavCallback(0,val);
        return;
    case H_CDRight:
        //if(cddavCallback) cddavCallback(1,val);
        return;
    //-------------------------------------------------//
    case H_FMod1:
        //FModOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_FMod2:
        //FModOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_Noise1:
        //NoiseOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_Noise2:
        //NoiseOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_RVBon1:
        //ReverbOn(0,16,val);
        return;
    //-------------------------------------------------//
    case H_RVBon2:
        //ReverbOn(16,24,val);
        return;
    //-------------------------------------------------//
    case H_Reverb:
        return;
   }
}

////////////////////////////////////////////////////////////////////////

unsigned short CALLBACK SPU_readRegister(unsigned long reg)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_readRegister++;
#endif
 unsigned long r=reg&0xfff;

 if(r>=0x0c00 && r<0x0d80)
  {
   switch(r&0x0f)
    {
     case 12:                                          // adsr vol
      {
       //int ch=(r>>4)-0xc0;
       static unsigned short adsr_dummy_vol=0;
       adsr_dummy_vol=!adsr_dummy_vol;
       return adsr_dummy_vol;
      }

     case 14:                                          // return curr loop adr
      {
       //int ch=(r>>4)-0xc0;        
       return 0;
      }
    }
  }

 switch(r)
  {
    case H_SPUctrl:
        return spuCtrl;

    case H_SPUstat:
        return spuStat;
        
    case H_SPUaddr:
        return (unsigned short)(spuAddr>>3);

    case H_SPUdata:
      {
       unsigned short s=spuMem[spuAddr>>1];
       spuAddr+=2;
       if(spuAddr>0x7ffff) spuAddr=0;
       return s;
      }

    case H_SPUirqAddr:
        return spuIrq;
  }
 return regArea[(r-0xc00)>>1];
}
 
////////////////////////////////////////////////////////////////////////

unsigned short CALLBACK SPU_readDMA(void)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_readDMA++;
#endif
 unsigned short s=spuMem[spuAddr>>1];
 spuAddr+=2;
 if(spuAddr>0x7ffff) spuAddr=0;
 return s;
}

////////////////////////////////////////////////////////////////////////

void CALLBACK SPU_writeDMA(unsigned short val)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_writeDMA++;
#endif
 spuMem[spuAddr>>1] = val;                             // spu addr got by writeregister
 spuAddr+=2;                                           // inc spu addr
 if(spuAddr>0x7ffff) spuAddr=0;                        // wrap
}

////////////////////////////////////////////////////////////////////////

void CALLBACK SPU_writeDMAMem(unsigned short * pusPSXMem,int iSize)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_writeDMAMem++;
#endif
 int i;
 for(i=0;i<iSize;i++)
  {
   spuMem[spuAddr>>1] = *pusPSXMem++;                  // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }
}

////////////////////////////////////////////////////////////////////////

void CALLBACK SPU_readDMAMem(unsigned short * pusPSXMem,int iSize)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_readDMAMem++;
#endif
 int i;
 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr>>1];                    // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }
}

////////////////////////////////////////////////////////////////////////
// XA AUDIO
////////////////////////////////////////////////////////////////////////

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

void CALLBACK SPU_playADPCMchannel(xa_decode_t *xap)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_playADPCM++;
#endif
}

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// INIT/EXIT STUFF
////////////////////////////////////////////////////////////////////////

long CALLBACK SPU_init(void)
{
 spuMemC=(unsigned char *)spuMem;                      // just small setup
 return 0;
}

////////////////////////////////////////////////////////////////////////

long CALLBACK SPU_shutdown(void)
{
 return 0;
}

////////////////////////////////////////////////////////////////////////

typedef struct
{
 char          szSPUName[8];
 unsigned long ulFreezeVersion;
 unsigned long ulFreezeSize;
 unsigned char cSPUPort[0x200];
 unsigned char cSPURam[0x80000];
 xa_decode_t   xaS;     
} SPUFreeze_t;

typedef struct
{
 unsigned long Future[256];

} SPUNULLFreeze_t;

////////////////////////////////////////////////////////////////////////

long CALLBACK SPU_freeze(unsigned int ulFreezeMode,SPUFreeze_t * pF)
{
 int i;

 if(!pF) return 0;

 if(ulFreezeMode)
  {
   if(ulFreezeMode==1)
    memset(pF,0,sizeof(SPUFreeze_t)+sizeof(SPUNULLFreeze_t));

   strcpy(pF->szSPUName,"PBNUL");
   pF->ulFreezeVersion=1;
   pF->ulFreezeSize=sizeof(SPUFreeze_t)+sizeof(SPUNULLFreeze_t);

   if(ulFreezeMode==2) return 1;

   memcpy(pF->cSPURam,spuMem,0x80000);
   memcpy(pF->cSPUPort,regArea,0x200);
   // dummy:
   memset(&pF->xaS,0,sizeof(xa_decode_t));
   return 1;
  }

 if(ulFreezeMode!=0) return 0;

 memcpy(spuMem,pF->cSPURam,0x80000);
 memcpy(regArea,pF->cSPUPort,0x200);

 for(i=0;i<0x100;i++)
  {
   if(i!=H_SPUon1-0xc00 && i!=H_SPUon2-0xc00)
    SPU_writeRegister(0x1f801c00+i*2,regArea[i]);
  }
 SPU_writeRegister(H_SPUon1,regArea[(H_SPUon1-0xc00)/2]);
 SPU_writeRegister(H_SPUon2,regArea[(H_SPUon2-0xc00)/2]);

 return 1;
}

void CALLBACK SPU_async(void)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_async++;
#endif
}

void CALLBACK SPU_playCDDAchannel(unsigned char *pcm, int nbytes)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_playCDDA++;
#endif
}
