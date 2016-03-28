/***************************************************************************
                         registers.c  -  description
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

#define H_SPUReverbAddr  0x0da2
#define H_SPUirqAddr     0x0da4
#define H_SPUaddr        0x0da6
#define H_SPUdata        0x0da8
#define H_SPUctrl        0x0daa
#define H_SPUstat        0x0dae
#define H_SPUmvolL       0x0d80
#define H_SPUmvolR       0x0d82
#define H_SPUrvolL       0x0d84
#define H_SPUrvolR       0x0d86
#define H_SPUon1         0x0d88
#define H_SPUon2         0x0d8a
#define H_SPUoff1        0x0d8c
#define H_SPUoff2        0x0d8e
#define H_FMod1          0x0d90
#define H_FMod2          0x0d92
#define H_Noise1         0x0d94
#define H_Noise2         0x0d96
#define H_RVBon1         0x0d98
#define H_RVBon2         0x0d9a
#define H_SPUMute1       0x0d9c
#define H_SPUMute2       0x0d9e
#define H_CDLeft         0x0db0
#define H_CDRight        0x0db2
#define H_ExtLeft        0x0db4
#define H_ExtRight       0x0db6
#define H_Reverb         0x0dc0
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

////////////////////////////////////////////////////////////////////////
// SOUND ON register write
////////////////////////////////////////////////////////////////////////

INLINE void SoundOn(int start,int end,unsigned short val)     // SOUND ON PSX COMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if((val&1) && s_chan[ch].pStart)                    // mmm... start has to be set before key on !?!
    {
     s_chan[ch].bIgnoreLoop=0;
     s_chan[ch].bNew=1;
     dwNewChannel|=(1<<ch);                            // bitfield for faster testing
    }
  }
}

////////////////////////////////////////////////////////////////////////
// SOUND OFF register write
////////////////////////////////////////////////////////////////////////

INLINE void SoundOff(int start,int end,unsigned short val)    // SOUND OFF PSX COMMAND
{
 int ch;
 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // && s_chan[i].bOn)  mmm...
    {
     s_chan[ch].bStop=1;
    }                                                  
  }
}

////////////////////////////////////////////////////////////////////////
// FMOD register write
////////////////////////////////////////////////////////////////////////

INLINE void FModOn(int start,int end,unsigned short val)      // FMOD ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> fmod on/off
    {
     if(ch>0) 
      {
       s_chan[ch].bFMod=1;                             // --> sound channel
       s_chan[ch-1].bFMod=2;                           // --> freq channel
      }
    }
   else
    {
     s_chan[ch].bFMod=0;                               // --> turn off fmod
    }
  }
}

////////////////////////////////////////////////////////////////////////
// NOISE register write
////////////////////////////////////////////////////////////////////////

INLINE void NoiseOn(int start,int end,unsigned short val)     // NOISE ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> noise on/off
    {
     s_chan[ch].bNoise=1;
    }
   else 
    {
     s_chan[ch].bNoise=0;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// LEFT VOLUME register write
////////////////////////////////////////////////////////////////////////

// please note: sweep and phase invert are wrong... but I've never seen
// them used

INLINE void SetVolumeL(unsigned char ch,short vol)            // LEFT VOLUME
{
 s_chan[ch].iLeftVolRaw=vol;

 if(vol&0x8000)                                        // sweep?
  {
   short sInc=1;                                       // -> sweep up?
   if(vol&0x2000) sInc=-1;                             // -> or down?
   if(vol&0x1000) vol^=0xffff;                         // -> mmm... phase inverted? have to investigate this
   vol=((vol&0x7f)+1)/2;                               // -> sweep: 0..127 -> 0..64
   vol+=vol/(2*sInc);                                  // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
   vol*=128;
  }
 else                                                  // no sweep:
  {
   if(vol&0x4000)                                      // -> mmm... phase inverted? have to investigate this
    //vol^=0xffff;
    vol=0x3fff-(vol&0x3fff);
  }

 vol&=0x3fff;
 s_chan[ch].iLeftVolume=vol;                           // store volume
}

////////////////////////////////////////////////////////////////////////
// RIGHT VOLUME register write
////////////////////////////////////////////////////////////////////////

INLINE void SetVolumeR(unsigned char ch,short vol)            // RIGHT VOLUME
{
 s_chan[ch].iRightVolRaw=vol;

 if(vol&0x8000)                                        // comments... see above :)
  {
   short sInc=1;
   if(vol&0x2000) sInc=-1;
   if(vol&0x1000) vol^=0xffff;
   vol=((vol&0x7f)+1)/2;        
   vol+=vol/(2*sInc);
   vol*=128;
  }
 else            
  {
   if(vol&0x4000) //vol=vol^=0xffff;
    vol=0x3fff-(vol&0x3fff);
  }

 vol&=0x3fff;

 s_chan[ch].iRightVolume=vol;
}

////////////////////////////////////////////////////////////////////////
// PITCH register write
////////////////////////////////////////////////////////////////////////

INLINE void SetPitch(int ch,unsigned short val)               // SET PITCH
{
 int NP;
 if(val>0x3fff) NP=0x3fff;                             // get pitch val
 else           NP=val;

 s_chan[ch].iRawPitch=NP;

 NP=(44100L*NP)/4096L;                                 // calc frequency
 if(NP<1) NP=1;                                        // some security
 s_chan[ch].iActFreq=NP;                               // store frequency
}

////////////////////////////////////////////////////////////////////////
// REVERB register write
////////////////////////////////////////////////////////////////////////

INLINE void ReverbOn(int start,int end,unsigned short val)    // REVERB ON PSX COMMAND
{
 int ch;

 for(ch=start;ch<end;ch++,val>>=1)                     // loop channels
  {
   if(val&1)                                           // -> reverb on/off
    {
     s_chan[ch].bReverb=1;
    }
   else 
    {
     s_chan[ch].bReverb=0;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// WRITE REGISTERS: called by main emu
////////////////////////////////////////////////////////////////////////

void CALLBACK SPU_writeRegister(unsigned long reg, unsigned short val)
{
 const unsigned long r=reg&0xfff;
 regArea[(r-0xc00)>>1] = val;

 if(r>=0x0c00 && r<0x0d80)                             // some channel info?
  {
   int ch=(r>>4)-0xc0;                                 // calc channel
   switch(r&0x0f)
    {
     //------------------------------------------------// r volume
     case 0:                                           
       SetVolumeL((unsigned char)ch,val);
       break;
     //------------------------------------------------// l volume
     case 2:                                           
       SetVolumeR((unsigned char)ch,val);
       break;
     //------------------------------------------------// pitch
     case 4:                                           
       SetPitch(ch,val);
       break;
     //------------------------------------------------// start
     case 6:      
       s_chan[ch].pStart=spuMemC+((unsigned long) val<<3);
       break;
     //------------------------------------------------// level with pre-calcs
     case 8:
       {
        const unsigned long lval=val;
        //---------------------------------------------//
        s_chan[ch].ADSRX.AttackModeExp=(lval&0x8000)?1:0; 
        s_chan[ch].ADSRX.AttackRate=(lval>>8) & 0x007f;
        s_chan[ch].ADSRX.DecayRate=(lval>>4) & 0x000f;
        s_chan[ch].ADSRX.SustainLevel=lval & 0x000f;
        //---------------------------------------------//
       }
      break;
     //------------------------------------------------// adsr times with pre-calcs
     case 10:
      {
       const unsigned long lval=val;

       //----------------------------------------------//
       s_chan[ch].ADSRX.SustainModeExp = (lval&0x8000)?1:0;
       s_chan[ch].ADSRX.SustainIncrease= (lval&0x4000)?0:1;
       s_chan[ch].ADSRX.SustainRate = (lval>>6) & 0x007f;
       s_chan[ch].ADSRX.ReleaseModeExp = (lval&0x0020)?1:0;
       s_chan[ch].ADSRX.ReleaseRate = lval & 0x001f;
       //----------------------------------------------//
      }
     break;
     //------------------------------------------------// adsr volume... mmm have to investigate this
     case 12:
       break;
     //------------------------------------------------//
     case 14:                                          // loop?
       //WaitForSingleObject(s_chan[ch].hMutex,2000);        // -> no multithread fuckups
       s_chan[ch].pLoop=spuMemC+((unsigned long) val<<3);
       s_chan[ch].bIgnoreLoop=1;
       //ReleaseMutex(s_chan[ch].hMutex);                    // -> oki, on with the thread
       break;
     //------------------------------------------------//
    }
   iSpuAsyncWait=0;
   return;
  }

 switch(r)
   {
    //-------------------------------------------------//
    case H_SPUaddr:
      spuAddr = (unsigned long) val<<3;
      break;
    //-------------------------------------------------//
    case H_SPUdata:
      spuMem[spuAddr>>1] = val;
      spuAddr+=2;
      if(spuAddr>0x7ffff) spuAddr=0;
      break;
    //-------------------------------------------------//
    case H_SPUctrl:
      spuCtrl=val;
      break;
    //-------------------------------------------------//
    case H_SPUstat:
      spuStat=val & 0xf800;
      break;
    //-------------------------------------------------//
    case H_SPUReverbAddr:
      if(val==0xFFFF || val<=0x200)
       {rvb.StartAddr=rvb.CurrAddr=0;}
      else
       {
        const long iv=(unsigned long)val<<2;
        if(rvb.StartAddr!=iv)
         {
          rvb.StartAddr=(unsigned long)val<<2;
          rvb.CurrAddr=rvb.StartAddr;
         }
       }
      break;
    //-------------------------------------------------//
    case H_SPUirqAddr:
      spuIrq = val;
      pSpuIrq=spuMemC+((unsigned long) val<<3);
      break;
    //-------------------------------------------------//
    case H_SPUrvolL:
      rvb.VolLeft=val;
      break;
    //-------------------------------------------------//
    case H_SPUrvolR:
      rvb.VolRight=val;
      break;
    //-------------------------------------------------//
    case H_SPUon1:
      SoundOn(0,16,val);
      break;
    //-------------------------------------------------//
     case H_SPUon2:
      SoundOn(16,24,val);
      break;
    //-------------------------------------------------//
    case H_SPUoff1:
      SoundOff(0,16,val);
      break;
    //-------------------------------------------------//
    case H_SPUoff2:
      SoundOff(16,24,val);
      break;
    //-------------------------------------------------//
    case H_CDLeft:
      iLeftXAVol=val  & 0x7fff;
      break;
    case H_CDRight:
      iRightXAVol=val & 0x7fff;
      break;
    //-------------------------------------------------//
    case H_FMod1:
      FModOn(0,16,val);
      break;
    //-------------------------------------------------//
    case H_FMod2:
      FModOn(16,24,val);
      break;
    //-------------------------------------------------//
    case H_Noise1:
      NoiseOn(0,16,val);
      break;
    //-------------------------------------------------//
    case H_Noise2:
      NoiseOn(16,24,val);
      break;
    //-------------------------------------------------//
    case H_RVBon1:
      ReverbOn(0,16,val);
      break;
    //-------------------------------------------------//
    case H_RVBon2:
      ReverbOn(16,24,val);
      break;
    //-------------------------------------------------//
    case H_Reverb+0:

      rvb.FB_SRC_A=val;

      // OK, here's the fake REVERB stuff...
      // depending on effect we do more or less delay and repeats... bah
      // still... better than nothing :)

      SetREVERB(val);
      break;

	  case H_Reverb+2   : rvb.FB_SRC_B=(short)val;       break;
    case H_Reverb+4   : rvb.IIR_ALPHA=(short)val;      break;
    case H_Reverb+6   : rvb.ACC_COEF_A=(short)val;     break;
    case H_Reverb+8   : rvb.ACC_COEF_B=(short)val;     break;
    case H_Reverb+10  : rvb.ACC_COEF_C=(short)val;     break;
    case H_Reverb+12  : rvb.ACC_COEF_D=(short)val;     break;
    case H_Reverb+14  : rvb.IIR_COEF=(short)val;       break;
    case H_Reverb+16  : rvb.FB_ALPHA=(short)val;       break;
    case H_Reverb+18  : rvb.FB_X=(short)val;           break;
    case H_Reverb+20  : rvb.IIR_DEST_A0=(short)val;    break;
    case H_Reverb+22  : rvb.IIR_DEST_A1=(short)val;    break;
    case H_Reverb+24  : rvb.ACC_SRC_A0=(short)val;     break;
    case H_Reverb+26  : rvb.ACC_SRC_A1=(short)val;     break;
    case H_Reverb+28  : rvb.ACC_SRC_B0=(short)val;     break;
    case H_Reverb+30  : rvb.ACC_SRC_B1=(short)val;     break;
    case H_Reverb+32  : rvb.IIR_SRC_A0=(short)val;     break;
    case H_Reverb+34  : rvb.IIR_SRC_A1=(short)val;     break;
    case H_Reverb+36  : rvb.IIR_DEST_B0=(short)val;    break;
    case H_Reverb+38  : rvb.IIR_DEST_B1=(short)val;    break;
    case H_Reverb+40  : rvb.ACC_SRC_C0=(short)val;     break;
    case H_Reverb+42  : rvb.ACC_SRC_C1=(short)val;     break;
    case H_Reverb+44  : rvb.ACC_SRC_D0=(short)val;     break;
    case H_Reverb+46  : rvb.ACC_SRC_D1=(short)val;     break;
    case H_Reverb+48  : rvb.IIR_SRC_B1=(short)val;     break;
    case H_Reverb+50  : rvb.IIR_SRC_B0=(short)val;     break;
    case H_Reverb+52  : rvb.MIX_DEST_A0=(short)val;    break;
    case H_Reverb+54  : rvb.MIX_DEST_A1=(short)val;    break;
    case H_Reverb+56  : rvb.MIX_DEST_B0=(short)val;    break;
    case H_Reverb+58  : rvb.MIX_DEST_B1=(short)val;    break;
    case H_Reverb+60  : rvb.IN_COEF_L=(short)val;      break;
    case H_Reverb+62  : rvb.IN_COEF_R=(short)val;      break;
   }

 iSpuAsyncWait=0;
}

////////////////////////////////////////////////////////////////////////
// READ REGISTER: called by main emu
////////////////////////////////////////////////////////////////////////

unsigned short CALLBACK SPU_readRegister(unsigned long reg)
{
 const unsigned long r=reg&0xfff;
        
 iSpuAsyncWait=0;

 if(r>=0x0c00 && r<0x0d80)
  {
   switch(r&0x0f)
    {
     case 12:                                          // get adsr vol
      {
       const int ch=(r>>4)-0xc0;
       if(s_chan[ch].bNew) return 1;                   // we are started, but not processed? return 1
       if(s_chan[ch].ADSRX.lVolume &&                  // same here... we haven't decoded one sample yet, so no envelope yet. return 1 as well
          !s_chan[ch].ADSRX.EnvelopeVol)                   
        return 1;
       return (unsigned short)(s_chan[ch].ADSRX.EnvelopeVol>>16);
      }

     case 14:                                          // get loop address
      {
       const int ch=(r>>4)-0xc0;
       if(s_chan[ch].pLoop==NULL) return 0;
       return (unsigned short)((s_chan[ch].pLoop-spuMemC)>>3);
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
