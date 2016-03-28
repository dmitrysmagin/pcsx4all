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

unsigned int RateTable[160];

INLINE void InitADSR(void)                                    // INIT ADSR
{
 unsigned int r,rs,rd;int i;

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

INLINE int MixADSR(int ch)                             // MIX ADSR
{
	static const char ratetable_offset[8] = { 0, 4, 6, 8, 9, 10, 11, 12 };
	int rto;
	int EnvelopeVol=s_chan[ch].ADSRX.EnvelopeVol;

	if(s_chan[ch].bStop)                                  // should be stopped:
	{                                                    // do release
		if(s_chan[ch].ADSRX.ReleaseModeExp) rto=ratetable_offset[(EnvelopeVol>>28)&0x7];
		else rto=12;
		EnvelopeVol-=((RateTable[((s_chan[ch].ADSRX.ReleaseRate^0x1F)<<2)-0x18 + rto + 32])<<1);

		if(EnvelopeVol<0) { EnvelopeVol=0; dwChannelOn &= ~(1<<ch); }

		goto done;
	}

	switch(s_chan[ch].ADSRX.State)                       // not stopped yet
	{
		case 0:                                             // -> attack
			if(s_chan[ch].ADSRX.AttackModeExp&&EnvelopeVol>=0x60000000) rto = 0;
			else rto=8;
			EnvelopeVol+=((RateTable[(s_chan[ch].ADSRX.AttackRate^0x7F)-0x18 + rto + 32])<<1);

			if(EnvelopeVol<0) 
			{
				EnvelopeVol=0x7FFFFFFF;
				s_chan[ch].ADSRX.State=1;
			}
			break;

			//--------------------------------------------------//
		case 1:                                             // -> decay
			rto=ratetable_offset[(EnvelopeVol>>28)&0x7];
			EnvelopeVol-=((RateTable[((s_chan[ch].ADSRX.DecayRate^0x1F)<<2)-0x18+ rto + 32])<<1);

			if(EnvelopeVol<0) EnvelopeVol=0;
			if(((EnvelopeVol>>27)&0xF) <= s_chan[ch].ADSRX.SustainLevel) s_chan[ch].ADSRX.State=2;
			break;

			//--------------------------------------------------//
		case 2:                                             // -> sustain
			if(s_chan[ch].ADSRX.SustainIncrease)
			{
				if(s_chan[ch].ADSRX.SustainModeExp&&EnvelopeVol>=0x60000000) rto=0;
				else rto=8;
				EnvelopeVol+=((RateTable[(s_chan[ch].ADSRX.SustainRate^0x7F)-0x18 + rto + 32])<<1);

				if(EnvelopeVol<0) EnvelopeVol=0x7FFFFFFF;
			}
			else
			{
				if(s_chan[ch].ADSRX.SustainModeExp) rto=ratetable_offset[(EnvelopeVol>>28)&0x7];
				else rto=12;
				EnvelopeVol-=((RateTable[((s_chan[ch].ADSRX.SustainRate^0x7F))-0x1B + rto + 32])<<1);

				if(EnvelopeVol<0) { EnvelopeVol=0; dwChannelOn &= ~(1<<ch); }
			}
			break;
	}

	done:
	s_chan[ch].ADSRX.EnvelopeVol=EnvelopeVol;
	return EnvelopeVol>>21;
}
