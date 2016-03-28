////////////////////////////////////////////////////////////////////////
// ADSR func
////////////////////////////////////////////////////////////////////////

static unsigned long RateTable[160];

INLINE void InitADSR(void)                                    // INIT ADSR
{
 unsigned long r,rs,rd;int i;

 memset(RateTable,0,sizeof(unsigned long)*160);        // build the rate table according to Neill's rules (see at bottom of file)

 r=3;rs=1;rd=0;

 for(i=32;i<160;i++)                                   // we start at pos 32 with the real values... everything before is 0
  {
   if(r<0x3FFFFFFF)
    {
     r+=rs;
     rd++;if(rd==5) {rd=1;rs*=2;}
    }
   if(r>0x3FFFFFFF) r=0x3FFFFFFF;

   RateTable[i]=r;
  }
}

////////////////////////////////////////////////////////////////////////

INLINE void StartADSR(int ch)                          // MIX ADSR
{
 s_chan[ch].ADSRX.lVolume=1;                           // and init some adsr vars
 s_chan[ch].ADSRX.State=0;
 s_chan[ch].ADSRX.EnvelopeVol=0;
}

////////////////////////////////////////////////////////////////////////

INLINE int MixADSR(int ch)                             // MIX ADSR
{    
 if(s_chan[ch].bStop)                                  // should be stopped:
  {                                                    // do release
   if(s_chan[ch].ADSRX.ReleaseModeExp)
    {
     switch((s_chan[ch].ADSRX.EnvelopeVol>>28)&0x7)
      {
       case 0: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +0 + 32]; break;
       case 1: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +4 + 32]; break;
       case 2: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +6 + 32]; break;
       case 3: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +8 + 32]; break;
       case 4: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +9 + 32]; break;
       case 5: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +10+ 32]; break;
       case 6: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +11+ 32]; break;
       case 7: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x18 +12+ 32]; break;
      }
    }
   else
    {
     s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.ReleaseRate^0x1F))-0x0C + 32];
    }

   if(s_chan[ch].ADSRX.EnvelopeVol<0) 
    {
     s_chan[ch].ADSRX.EnvelopeVol=0;
     s_chan[ch].bOn=0;
     //s_chan[ch].bReverb=0;
     //s_chan[ch].bNoise=0;
    }

   s_chan[ch].ADSRX.lVolume=s_chan[ch].ADSRX.EnvelopeVol>>21;
   return s_chan[ch].ADSRX.lVolume;
  }
 else                                                  // not stopped yet?
  {
   if(s_chan[ch].ADSRX.State==0)                       // -> attack
    {
     if(s_chan[ch].ADSRX.AttackModeExp)
      {
       if(s_chan[ch].ADSRX.EnvelopeVol<0x60000000) 
        s_chan[ch].ADSRX.EnvelopeVol+=RateTable[(s_chan[ch].ADSRX.AttackRate^0x7F)-0x10 + 32];
       else
        s_chan[ch].ADSRX.EnvelopeVol+=RateTable[(s_chan[ch].ADSRX.AttackRate^0x7F)-0x18 + 32];
      }
     else
      {
       s_chan[ch].ADSRX.EnvelopeVol+=RateTable[(s_chan[ch].ADSRX.AttackRate^0x7F)-0x10 + 32];
      }

     if(s_chan[ch].ADSRX.EnvelopeVol<0) 
      {
       s_chan[ch].ADSRX.EnvelopeVol=0x7FFFFFFF;
       s_chan[ch].ADSRX.State=1;
      }

     s_chan[ch].ADSRX.lVolume=s_chan[ch].ADSRX.EnvelopeVol>>21;
     return s_chan[ch].ADSRX.lVolume;
    }
   //--------------------------------------------------//
   if(s_chan[ch].ADSRX.State==1)                       // -> decay
    {
     switch((s_chan[ch].ADSRX.EnvelopeVol>>28)&0x7)
      {
       case 0: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+0 + 32]; break;
       case 1: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+4 + 32]; break;
       case 2: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+6 + 32]; break;
       case 3: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+8 + 32]; break;
       case 4: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+9 + 32]; break;
       case 5: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+10+ 32]; break;
       case 6: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+11+ 32]; break;
       case 7: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[(4*(s_chan[ch].ADSRX.DecayRate^0x1F))-0x18+12+ 32]; break;
      }

     if(s_chan[ch].ADSRX.EnvelopeVol<0) s_chan[ch].ADSRX.EnvelopeVol=0;
     if(((s_chan[ch].ADSRX.EnvelopeVol>>27)&0xF) <= s_chan[ch].ADSRX.SustainLevel)
      {
       s_chan[ch].ADSRX.State=2;
      }

     s_chan[ch].ADSRX.lVolume=s_chan[ch].ADSRX.EnvelopeVol>>21;
     return s_chan[ch].ADSRX.lVolume;
    }
   //--------------------------------------------------//
   if(s_chan[ch].ADSRX.State==2)                       // -> sustain
    {
     if(s_chan[ch].ADSRX.SustainIncrease)
      {
       if(s_chan[ch].ADSRX.SustainModeExp)
        {
         if(s_chan[ch].ADSRX.EnvelopeVol<0x60000000) 
          s_chan[ch].ADSRX.EnvelopeVol+=RateTable[(s_chan[ch].ADSRX.SustainRate^0x7F)-0x10 + 32];
         else
          s_chan[ch].ADSRX.EnvelopeVol+=RateTable[(s_chan[ch].ADSRX.SustainRate^0x7F)-0x18 + 32];
        }
       else
        {
         s_chan[ch].ADSRX.EnvelopeVol+=RateTable[(s_chan[ch].ADSRX.SustainRate^0x7F)-0x10 + 32];
        }

       if(s_chan[ch].ADSRX.EnvelopeVol<0) 
        {
         s_chan[ch].ADSRX.EnvelopeVol=0x7FFFFFFF;
        }
      }
     else
      {
       if(s_chan[ch].ADSRX.SustainModeExp)
        {
         switch((s_chan[ch].ADSRX.EnvelopeVol>>28)&0x7)
          {
           case 0: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +0 + 32];break;
           case 1: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +4 + 32];break;
           case 2: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +6 + 32];break;
           case 3: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +8 + 32];break;
           case 4: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +9 + 32];break;
           case 5: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +10+ 32];break;
           case 6: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +11+ 32];break;
           case 7: s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B +12+ 32];break;
          }
        }
       else
        {
         s_chan[ch].ADSRX.EnvelopeVol-=RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x0F + 32];
        }

       if(s_chan[ch].ADSRX.EnvelopeVol<0) 
        {
         s_chan[ch].ADSRX.EnvelopeVol=0;
        }
      }
     s_chan[ch].ADSRX.lVolume=s_chan[ch].ADSRX.EnvelopeVol>>21;
     return s_chan[ch].ADSRX.lVolume;
    }
  }
 return 0;
}
