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
// XA GLOBALS
////////////////////////////////////////////////////////////////////////

xa_decode_t   * xapGlobal=0;

uint16_t * XAFeed  = NULL;
uint16_t * XAPlay  = NULL;
uint16_t * XAStart = NULL;
uint16_t * XAEnd   = NULL;

uint32_t   XARepeat  = 0;

uint16_t * CDDAFeed  = NULL;
uint16_t * CDDAPlay  = NULL;
uint16_t * CDDAStart = NULL;
uint16_t * CDDAEnd   = NULL;

unsigned int iLeftXAVol  = 0;

////////////////////////////////////////////////////////////////////////
// MIX XA & CDDA
////////////////////////////////////////////////////////////////////////

INLINE void MixXA(int nssize,signed int *sum)
{
	signed int   XALastVal = 0;
	int ns;
	unsigned int vol=iLeftXAVol;

	{
		uint16_t *feed=XAFeed;
		uint16_t *play=XAPlay;
		uint16_t *start=XAStart;
		uint16_t *end=XAEnd;
		for(ns=0;ns<nssize && play!=feed;ns++)
		{
			XALastVal=(signed int)(*(signed short *)play++);
			if(play==end) play=start;
			sum[ns]+=(XALastVal >> vol);
		}

		if(play==feed && XARepeat)
		{
			XARepeat--;
			for(;ns<nssize;ns++)
			{
				sum[ns]+=(XALastVal >> vol);
			}
		}
		XAPlay=play;
	}

	{
		uint16_t *feed=CDDAFeed;
		uint16_t *play=CDDAPlay;
		uint16_t *start=CDDAStart;
		uint16_t *end=CDDAEnd;
		if (play==feed)
		{
			extern void playCDDA(void);
			playCDDA();
		}
		
		for(ns=0;ns<nssize && play!=feed;ns++)
		{
			XALastVal=(signed int)(*(signed short *)play++);
			if(play==end) play=start;
			sum[ns]+=(XALastVal >> vol);
		}
		CDDAPlay=play;
	}
}

////////////////////////////////////////////////////////////////////////
// FEED XA 
////////////////////////////////////////////////////////////////////////

INLINE void FeedXA(xa_decode_t *xap)
{
	uint16_t *feed=XAFeed;
	uint16_t *play=XAPlay;
	uint16_t *start=XAStart;
	uint16_t *end=XAEnd;
	int sinc,spos,i,iSize;
	xapGlobal = xap;                                      // store info for save states
	XARepeat  = 100;                                      // set up repeat

	iSize=((22050*xap->nsamples)/xap->freq);              // get size
	if(!iSize) return;                                    // none? bye

	spos=0x10000L;
	sinc = (xap->nsamples << 16) / iSize;                 // calc freq by num / size

	if(xap->stereo)
	{
		uint32_t * ps=(uint32_t *)xap->pcm;
		uint16_t l=0;
		
		for(i=0;i<iSize;i++)
		{
			while(spos>=0x10000L)
			{
				l = *ps++;
				spos -= 0x10000L;
			}

			*feed++=l;

			if(feed==end) feed=start;
			if(feed==play) 
			{
				if(play!=start) feed=play-1;
				break;
			}

			spos += sinc;
		}
		
	}
	else
	{
		unsigned short * ps=(unsigned short *)xap->pcm;
		uint16_t l=0;

		for(i=0;i<iSize;i++)
		{
			while(spos>=0x10000L)
			{
				l = *ps++;
				spos -= 0x10000L;
			}

			*feed++=l;

			if(feed==end) feed=start;
			if(feed==play) 
			{
				if(play!=start) feed=play-1;
				break;
			}

			spos += sinc;
		}
	}
	XAFeed=feed;
}

////////////////////////////////////////////////////////////////////////
// FEED CDDA
////////////////////////////////////////////////////////////////////////

INLINE void FeedCDDA(unsigned char *pcm, int nBytes)
{
	uint16_t *feed=CDDAFeed;
	uint16_t *start=CDDAStart;
	uint16_t *end=CDDAEnd;
	if ((unsigned int)pcm&1)
	{
		while(nBytes>0)
		{
			*feed++=(((uint16_t)*pcm) | (((uint16_t)*(pcm+1))<<8));
			nBytes-=8;
			pcm+=8;
			if(feed==end) feed=start;
		}
	}
	else
	{
		uint16_t *pcm16=(uint16_t *)pcm;
		while(nBytes>0)
		{
			*feed++=*pcm16;
			nBytes-=8;
			pcm16+=4;
			if(feed==end) feed=start;
		}
	}
	CDDAFeed=feed;
}
