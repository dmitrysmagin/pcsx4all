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
			s_chan[ch].bStop=false;
			s_chan[ch].pCurr=s_chan[ch].pStart;
			dwNewChannel|=(1<<ch);
			dwChannelOn|=1<<ch;
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
			s_chan[ch].bStop=true;
			dwNewChannel &= ~(1<<ch);
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
			if(ch>0&&s_chan[ch-1].bFMod==2) s_chan[ch-1].bFMod=0;
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
		s_chan[ch].bNoise=((val&1)!=0);                            // -> noise on/off
	}
}

////////////////////////////////////////////////////////////////////////
// LEFT VOLUME register write
////////////////////////////////////////////////////////////////////////

/* right bit shift according to the volume level 0..511 */
static const unsigned char sound_scale[512] =
{
	16, 8, 7, 7, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// please note: sweep and phase invert are wrong... but I've never seen
// them used

INLINE void SetVolumeL(unsigned char ch,short vol)            // LEFT VOLUME
{
	if(vol&0x8000)                                        // sweep?
	{
		short sInc=1;                                       // -> sweep up?
		if(vol&0x2000) sInc=-1;                             // -> or down?
		if(vol&0x1000) vol^=0xffff;                         // -> mmm... phase inverted? have to investigate this
		vol=((vol&0x7f)+1)/2;                               // -> sweep: 0..127 -> 0..64
		if (sInc==1) vol+=vol>>1;
		else vol-=vol>>1;
		//vol+=vol/(2*sInc);                                  // -> we just raise/lower the volume by the half!
		vol*=128;
	}
	else                                                  // no sweep:
	{
		if(vol&0x4000) vol=0x3fff-(vol&0x3fff);
	}

	vol&=0x3fff;
	s_chan[ch].iLeftVolume=sound_scale[vol>>5];                           // store volume
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
// WRITE REGISTERS: called by main emu
////////////////////////////////////////////////////////////////////////

void SPU_writeRegister(unsigned long reg, unsigned short val)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_writeRegister++;
#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);

	const unsigned long r=reg&0xfff;
	regArea[(r-0xc00)>>1] = val;

	if(r>=0x0c00 && r<0x0d80)                             // some channel info?
	{
		if (!nullspu)
		{
			int ch=(r>>4)-0xc0;                                 // calc channel
			switch(r&0x0f)
			{
			case 0: SetVolumeL((unsigned char)ch,val); break;
			case 4: SetPitch(ch,val); break;
			case 6: s_chan[ch].pStart=spuMemC+(unsigned long)((val<<3)&~0xf); break;
			case 8: // level with pre-calcs
				{
					const unsigned long lval=val;
					s_chan[ch].ADSRX.AttackModeExp=(lval&0x8000)?1:0; 
					s_chan[ch].ADSRX.AttackRate=(lval>>8) & 0x007f;
					s_chan[ch].ADSRX.DecayRate=(lval>>4) & 0x000f;
					s_chan[ch].ADSRX.SustainLevel=lval & 0x000f;
				}
				break;
			case 10: // adsr times with pre-calcs
				{
					const unsigned long lval=val;
					s_chan[ch].ADSRX.SustainModeExp = (lval&0x8000)?1:0;
					s_chan[ch].ADSRX.SustainIncrease= (lval&0x4000)?0:1;
					s_chan[ch].ADSRX.SustainRate = (lval>>6) & 0x007f;
					s_chan[ch].ADSRX.ReleaseModeExp = (lval&0x0020)?1:0;
					s_chan[ch].ADSRX.ReleaseRate = lval & 0x001f;
				}
				break;
			case 14: // loop?
				s_chan[ch].pLoop=spuMemC+((unsigned long)((val<<3)&~0xf));
				break;
			}
		}
		pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
		return;
	}

	switch(r)
	{
	case H_SPUaddr: spuAddr = (unsigned long) val<<3; break;
	case H_SPUdata: spuMem[spuAddr>>1] = val; spuAddr+=2; if(spuAddr>0x7ffff) spuAddr=0; break;
	case H_SPUctrl: spuCtrl=val; break;
	case H_SPUstat: spuStat=val & 0xf800; break;
	case H_SPUirqAddr: spuIrq = val; pSpuIrq=spuMemC+((unsigned long) val<<3); break;
	case H_SPUon1: if (!nullspu) SoundOn(0,16,val); break;
	case H_SPUon2: if (!nullspu) SoundOn(16,24,val); break;
	case H_SPUoff1: if (!nullspu) SoundOff(0,16,val); break;
	case H_SPUoff2: if (!nullspu) SoundOff(16,24,val); break;
	case H_CDLeft: if (!nullspu) iLeftXAVol=sound_scale[(val&0x7fff)>>6]; break;
	case H_FMod1: if (!nullspu) FModOn(0,16,val); break;
	case H_FMod2: if (!nullspu) FModOn(16,24,val); break;
	case H_Noise1: if (!nullspu) NoiseOn(0,16,val); break;
	case H_Noise2: if (!nullspu) NoiseOn(16,24,val); break;
	}
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
}

////////////////////////////////////////////////////////////////////////
// READ REGISTER: called by main emu
////////////////////////////////////////////////////////////////////////

unsigned short SPU_readRegister(unsigned long reg)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SPU_readRegister++;
#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	const unsigned long r=reg&0xfff;
	
	if(r>=0x0c00 && r<0x0d80)
	{
		switch(r&0x0f)
		{
		case 12:                                          // get adsr vol
			{
				if (!nullspu)
				{
					const int ch=(r>>4)-0xc0;
					if(dwNewChannel&(1<<ch)) return 1;              // we are started, but not processed? return 1
					if((dwChannelOn&(1<<ch)) &&                     // same here... we haven't decoded one sample yet, so no envelope yet. return 1 as well
						!s_chan[ch].ADSRX.EnvelopeVol) return 1;
					unsigned short ret=(unsigned short)(s_chan[ch].ADSRX.EnvelopeVol>>16);
					pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
					return ret;
				}
				else
				{
					static unsigned short adsr_dummy_vol=0;
					adsr_dummy_vol=!adsr_dummy_vol;
					unsigned short ret=adsr_dummy_vol;
					pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
					return ret;
				}
			}

		case 14:                                          // get loop address
			{
				if (!nullspu)
				{
					const int ch=(r>>4)-0xc0;
					unsigned short ret;
					ret=(unsigned short)((s_chan[ch].pLoop-spuMemC)>>3);
					pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
					return ret;
				}
				else
				{
					pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
					return 0;
				}
			}
		}
	}

	switch(r)
	{
	case H_SPUctrl:
		pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
		return spuCtrl;
	case H_SPUstat:
		pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
		return spuStat;
	case H_SPUaddr:
		pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
		return (unsigned short)(spuAddr>>3);
	case H_SPUdata: { unsigned short s=spuMem[spuAddr>>1]; spuAddr+=2; if(spuAddr>0x7ffff) spuAddr=0;
			pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
			return s; }
	case H_SPUirqAddr:
		pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
		return spuIrq;
	}

	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
	return regArea[(r-0xc00)>>1];
}
