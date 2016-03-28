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

INLINE int MixADSR(SPUCHAN *l_chan)                             // MIX ADSR
{
	if(l_chan->bStop)                                  // should be stopped:
	{                                                    // do release
		if(l_chan->ADSRX.ReleaseModeExp)
		{
			switch((l_chan->ADSRX.EnvelopeVol>>28)&0x7)
			{
			case 0: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +0 + 32]; break;
			case 1: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +4 + 32]; break;
			case 2: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +6 + 32]; break;
			case 3: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +8 + 32]; break;
			case 4: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +9 + 32]; break;
			case 5: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +10+ 32]; break;
			case 6: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +11+ 32]; break;
			case 7: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x18 +12+ 32]; break;
			}
		}
		else
		{
			l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.ReleaseRate^0x1F))-0x0C + 32];
		}

		if(l_chan->ADSRX.EnvelopeVol<0) 
		{
			l_chan->ADSRX.EnvelopeVol=0;
			l_chan->bOn=false;
		}

		l_chan->ADSRX.lVolume=l_chan->ADSRX.EnvelopeVol>>21;
		return l_chan->ADSRX.lVolume;
	}
	else                                                  // not stopped yet?
	{
		if(l_chan->ADSRX.State==0)                       // -> attack
		{
			if(l_chan->ADSRX.AttackModeExp)
			{
				if(l_chan->ADSRX.EnvelopeVol<0x60000000) 
				l_chan->ADSRX.EnvelopeVol+=RateTable[(l_chan->ADSRX.AttackRate^0x7F)-0x10 + 32];
				else
				l_chan->ADSRX.EnvelopeVol+=RateTable[(l_chan->ADSRX.AttackRate^0x7F)-0x18 + 32];
			}
			else
			{
				l_chan->ADSRX.EnvelopeVol+=RateTable[(l_chan->ADSRX.AttackRate^0x7F)-0x10 + 32];
			}

			if(l_chan->ADSRX.EnvelopeVol<0) 
			{
				l_chan->ADSRX.EnvelopeVol=0x7FFFFFFF;
				l_chan->ADSRX.State=1;
			}

			l_chan->ADSRX.lVolume=l_chan->ADSRX.EnvelopeVol>>21;
			return l_chan->ADSRX.lVolume;
		}
		//--------------------------------------------------//
		if(l_chan->ADSRX.State==1)                       // -> decay
		{
			switch((l_chan->ADSRX.EnvelopeVol>>28)&0x7)
			{
			case 0: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+0 + 32]; break;
			case 1: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+4 + 32]; break;
			case 2: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+6 + 32]; break;
			case 3: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+8 + 32]; break;
			case 4: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+9 + 32]; break;
			case 5: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+10+ 32]; break;
			case 6: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+11+ 32]; break;
			case 7: l_chan->ADSRX.EnvelopeVol-=RateTable[(4*(l_chan->ADSRX.DecayRate^0x1F))-0x18+12+ 32]; break;
			}

			if(l_chan->ADSRX.EnvelopeVol<0) l_chan->ADSRX.EnvelopeVol=0;
			if(((l_chan->ADSRX.EnvelopeVol>>27)&0xF) <= l_chan->ADSRX.SustainLevel)
			{
				l_chan->ADSRX.State=2;
			}

			l_chan->ADSRX.lVolume=l_chan->ADSRX.EnvelopeVol>>21;
			return l_chan->ADSRX.lVolume;
		}
		//--------------------------------------------------//
		if(l_chan->ADSRX.State==2)                       // -> sustain
		{
			if(l_chan->ADSRX.SustainIncrease)
			{
				if(l_chan->ADSRX.SustainModeExp)
				{
					if(l_chan->ADSRX.EnvelopeVol<0x60000000) 
					l_chan->ADSRX.EnvelopeVol+=RateTable[(l_chan->ADSRX.SustainRate^0x7F)-0x10 + 32];
					else
					l_chan->ADSRX.EnvelopeVol+=RateTable[(l_chan->ADSRX.SustainRate^0x7F)-0x18 + 32];
				}
				else
				{
					l_chan->ADSRX.EnvelopeVol+=RateTable[(l_chan->ADSRX.SustainRate^0x7F)-0x10 + 32];
				}

				if(l_chan->ADSRX.EnvelopeVol<0) 
				{
					l_chan->ADSRX.EnvelopeVol=0x7FFFFFFF;
				}
			}
			else
			{
				if(l_chan->ADSRX.SustainModeExp)
				{
					switch((l_chan->ADSRX.EnvelopeVol>>28)&0x7)
					{
					case 0: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +0 + 32];break;
					case 1: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +4 + 32];break;
					case 2: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +6 + 32];break;
					case 3: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +8 + 32];break;
					case 4: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +9 + 32];break;
					case 5: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +10+ 32];break;
					case 6: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +11+ 32];break;
					case 7: l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x1B +12+ 32];break;
					}
				}
				else
				{
					l_chan->ADSRX.EnvelopeVol-=RateTable[((l_chan->ADSRX.SustainRate^0x7F))-0x0F + 32];
				}

				if(l_chan->ADSRX.EnvelopeVol<0) 
				{
					l_chan->ADSRX.EnvelopeVol=0;
				}
			}
			l_chan->ADSRX.lVolume=l_chan->ADSRX.EnvelopeVol>>21;
			return l_chan->ADSRX.lVolume;
		}
	}
	return 0;
}
