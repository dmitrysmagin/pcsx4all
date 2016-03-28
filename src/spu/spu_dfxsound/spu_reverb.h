/***************************************************************************
                          reverb.c  -  description
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

////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

// REVERB info and timing vars...

int *          sRVBPlay      = 0;
int *          sRVBEnd       = 0;
int *          sRVBStart     = 0;
int            iReverbOff    = -1;                          // some delay factor for reverb
int            iReverbRepeat = 0;
int            iReverbNum    = 1;    

////////////////////////////////////////////////////////////////////////
// SET REVERB
////////////////////////////////////////////////////////////////////////

INLINE void SetREVERB(unsigned short val)
{
 switch(val)
  {
   case 0x0000: iReverbOff=-1;  break;                                         // off
   case 0x007D: iReverbOff=32;  iReverbNum=2; iReverbRepeat=128;  break;       // ok room

   case 0x0033: iReverbOff=32;  iReverbNum=2; iReverbRepeat=64;   break;       // studio small
   case 0x00B1: iReverbOff=48;  iReverbNum=2; iReverbRepeat=96;   break;       // ok studio medium
   case 0x00E3: iReverbOff=64;  iReverbNum=2; iReverbRepeat=128;  break;       // ok studio large ok

   case 0x01A5: iReverbOff=128; iReverbNum=4; iReverbRepeat=32;   break;       // ok hall
   case 0x033D: iReverbOff=256; iReverbNum=4; iReverbRepeat=64;   break;       // space echo
   case 0x0001: iReverbOff=184; iReverbNum=3; iReverbRepeat=128;  break;       // echo/delay
   case 0x0017: iReverbOff=128; iReverbNum=2; iReverbRepeat=128;  break;       // half echo
   default:     iReverbOff=32;  iReverbNum=1; iReverbRepeat=0;    break;
  }
}

////////////////////////////////////////////////////////////////////////
// START REVERB
////////////////////////////////////////////////////////////////////////

INLINE void StartREVERB(int ch)
{
 if(s_chan[ch].bReverb && (spuCtrl&0x80))              // reverb possible?
  {
   if(iUseReverb && iReverbOff>0)                   // -> fake reverb used?
    {
     s_chan[ch].bRVBActive=1;                            // -> activate it
     s_chan[ch].iRVBOffset=iReverbOff*45;
     s_chan[ch].iRVBRepeat=iReverbRepeat*45;
     s_chan[ch].iRVBNum   =iReverbNum;
    }
  }
 else s_chan[ch].bRVBActive=0;                         // else -> no reverb
}

////////////////////////////////////////////////////////////////////////
// STORE REVERB
////////////////////////////////////////////////////////////////////////

INLINE void StoreREVERB(int ch,int ns)
{
 if(iUseReverb==0) return;
 else
  {
   int * pN;int iRn,iRr=0;

   // we use the half channel volume (/0x8000) for the first reverb effects, quarter for next and so on

   int iRxl=(s_chan[ch].sval*s_chan[ch].iLeftVolume)/0x8000;
   int iRxr=(s_chan[ch].sval*s_chan[ch].iRightVolume)/0x8000;
 
   for(iRn=1;iRn<=s_chan[ch].iRVBNum;iRn++,iRr+=s_chan[ch].iRVBRepeat,iRxl/=2,iRxr/=2)
    {
     pN=sRVBPlay+((s_chan[ch].iRVBOffset+iRr+ns)<<1);
     if(pN>=sRVBEnd) pN=sRVBStart+(pN-sRVBEnd);

     (*pN)+=iRxl;
     pN++;
     (*pN)+=iRxr;
    }
  }
}

////////////////////////////////////////////////////////////////////////

INLINE int MixREVERBLeft(int ns)
{
 if(iUseReverb==0) return 0;
 else                                                  // easy fake reverb:
  {
   const int iRV=*sRVBPlay;                            // -> simply take the reverb mix buf value
   *sRVBPlay++=0;                                      // -> init it after
   if(sRVBPlay>=sRVBEnd) sRVBPlay=sRVBStart;           // -> and take care about wrap arounds
   return iRV;                                         // -> return reverb mix buf val
  }
}

////////////////////////////////////////////////////////////////////////

INLINE int MixREVERBRight(void)
{
 if(iUseReverb==0) return 0;
 else
  {
   const int iRV=*sRVBPlay;                            // -> simply take the reverb mix buf value
   *sRVBPlay++=0;                                      // -> init it after
   if(sRVBPlay>=sRVBEnd) sRVBPlay=sRVBStart;           // -> and take care about wrap arounds
   return iRV;                                         // -> return reverb mix buf val
  }
}
