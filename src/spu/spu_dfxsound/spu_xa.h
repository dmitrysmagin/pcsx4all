/***************************************************************************
                            xa.c  -  description
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
// XA GLOBALS
////////////////////////////////////////////////////////////////////////

xa_decode_t   * xapGlobal=0;

uint32_t * XAFeed  = NULL;
uint32_t * XAPlay  = NULL;
uint32_t * XAStart = NULL;
uint32_t * XAEnd   = NULL;

uint32_t   XARepeat  = 0;
uint32_t   XALastVal = 0;

uint32_t * CDDAFeed  = NULL;
uint32_t * CDDAPlay  = NULL;
uint32_t * CDDAStart = NULL;
uint32_t * CDDAEnd   = NULL;

int             iLeftXAVol  = 32767;
int             iRightXAVol = 32767;

////////////////////////////////////////////////////////////////////////
// MIX XA & CDDA
////////////////////////////////////////////////////////////////////////

INLINE void MixXA(void)
{
 int ns;
 uint32_t l;

 for(ns=0;ns<NSSIZE && XAPlay!=XAFeed;ns++)
  {
   XALastVal=*XAPlay++;
   if(XAPlay==XAEnd) XAPlay=XAStart;
#ifdef XA_HACK
   SSumL[ns]+=(((short)(XALastVal&0xffff))       * iLeftXAVol)/32768;
   SSumR[ns]+=(((short)((XALastVal>>16)&0xffff)) * iRightXAVol)/32768;
#else
   SSumL[ns]+=(((short)(XALastVal&0xffff))       * iLeftXAVol)/32767;
   SSumR[ns]+=(((short)((XALastVal>>16)&0xffff)) * iRightXAVol)/32767;
#endif
  }

 if(XAPlay==XAFeed && XARepeat)
  {
   XARepeat--;
   for(;ns<NSSIZE;ns++)
    {
#ifdef XA_HACK
     SSumL[ns]+=(((short)(XALastVal&0xffff))       * iLeftXAVol)/32768;
     SSumR[ns]+=(((short)((XALastVal>>16)&0xffff)) * iRightXAVol)/32768;
#else
     SSumL[ns]+=(((short)(XALastVal&0xffff))       * iLeftXAVol)/32767;
     SSumR[ns]+=(((short)((XALastVal>>16)&0xffff)) * iRightXAVol)/32767;
#endif
    }
  }

 for(ns=0;ns<NSSIZE && CDDAPlay!=CDDAFeed && (CDDAPlay!=CDDAEnd-1||CDDAFeed!=CDDAStart);ns++)
  {
   l=*CDDAPlay++;
   if(CDDAPlay==CDDAEnd) CDDAPlay=CDDAStart;
#ifdef XA_HACK
   SSumL[ns]+=(((short)(l&0xffff))       * iLeftXAVol)/32768;
   SSumR[ns]+=(((short)((l>>16)&0xffff)) * iRightXAVol)/32768;
#else
   SSumL[ns]+=(((short)(l&0xffff))       * iLeftXAVol)/32767;
   SSumR[ns]+=(((short)((l>>16)&0xffff)) * iRightXAVol)/32767;
#endif
  }
}

////////////////////////////////////////////////////////////////////////
// FEED XA 
////////////////////////////////////////////////////////////////////////

INLINE void FeedXA(xa_decode_t *xap)
{
 int sinc,spos,i,iSize,iPlace;

 xapGlobal = xap;                                      // store info for save states
 XARepeat  = 100;                                      // set up repeat

#ifdef XA_HACK
 iSize=((45500*xap->nsamples)/xap->freq);              // get size
#else
 iSize=((44100*xap->nsamples)/xap->freq);              // get size
#endif
 if(!iSize) return;                                    // none? bye

 if(XAFeed<XAPlay) iPlace=XAPlay-XAFeed;               // how much space in my buf?
 else              iPlace=(XAEnd-XAFeed) + (XAPlay-XAStart);

 if(iPlace==0) return;                                 // no place at all

 spos=0x10000L;
 sinc = (xap->nsamples << 16) / iSize;                 // calc freq by num / size

 if(xap->stereo)
{
   uint32_t * pS=(uint32_t *)xap->pcm;
   uint32_t l=0;

    
     for(i=0;i<iSize;i++)
      {
         while(spos>=0x10000L)
          {
           l = *pS++;
           spos -= 0x10000L;
          }

       *XAFeed++=l;

       if(XAFeed==XAEnd) XAFeed=XAStart;
       if(XAFeed==XAPlay) 
        {
         if(XAPlay!=XAStart) XAFeed=XAPlay-1;
         break;
        }

       spos += sinc;
      }
    
  }
 else
 {
   unsigned short * pS=(unsigned short *)xap->pcm;
   uint32_t l;short s=0;

   for(i=0;i<iSize;i++)
      {
         while(spos>=0x10000L)
          {
           s = *pS++;
           spos -= 0x10000L;
          }
         l=s;

       *XAFeed++=(l|(l<<16));

       if(XAFeed==XAEnd) XAFeed=XAStart;
       if(XAFeed==XAPlay) 
        {
         if(XAPlay!=XAStart) XAFeed=XAPlay-1;
         break;
        }

       spos += sinc;
      }
  }
}

////////////////////////////////////////////////////////////////////////
// FEED CDDA
////////////////////////////////////////////////////////////////////////

INLINE void FeedCDDA(unsigned char *pcm, int nBytes)
{
 while(nBytes>0)
  {
   if(CDDAFeed==CDDAEnd) CDDAFeed=CDDAStart;
   if(CDDAFeed==CDDAPlay-1||
         (CDDAFeed==CDDAEnd-1&&CDDAPlay==CDDAStart))
   {
    return;
   }
   *CDDAFeed++=(*pcm | (*(pcm+1)<<8) | (*(pcm+2)<<16) | (*(pcm+3)<<24));
   nBytes-=4;
   pcm+=4;
  }
}
