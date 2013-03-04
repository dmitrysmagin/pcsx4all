/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Chui                                               *
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

#include "profiler.h"


#ifdef PROFILER_PCSX4ALL

unsigned pcsx4all_prof_frames=0;
static unsigned pcsx4all_prof_initial[PCSX4ALL_PROFILER_MAX];
static unsigned pcsx4all_prof_paused[PCSX4ALL_PROFILER_MAX];
static unsigned pcsx4all_prof_sum[PCSX4ALL_PROFILER_MAX];
static unsigned pcsx4all_prof_executed[PCSX4ALL_PROFILER_MAX];

static unsigned pcsx4all_prof_total_initial=0;
static unsigned pcsx4all_prof_total=0;
static const char *pcsx4all_prof_msg[PCSX4ALL_PROFILER_MAX];

void pcsx4all_prof_init(void)
{
	unsigned i;
	for(i=0;i<PCSX4ALL_PROFILER_MAX;i++)
	{
		pcsx4all_prof_initial[i]=0;
		pcsx4all_prof_paused[i]=0;
		pcsx4all_prof_sum[i]=0;
		pcsx4all_prof_executed[i]=0;
		if (!pcsx4all_prof_total)
			pcsx4all_prof_msg[i]=NULL;
	}
	pcsx4all_prof_total_initial=get_ticks();
}

void pcsx4all_prof_reset(void)
{
	unsigned i;
	for(i=0;i<PCSX4ALL_PROFILER_MAX;i++)
	{
		pcsx4all_prof_initial[i]=0;
		pcsx4all_prof_paused[i]=0;
		pcsx4all_prof_sum[i]=0;
		pcsx4all_prof_executed[i]=0;
	}
	pcsx4all_prof_total_initial=get_ticks();
	pcsx4all_prof_frames=0;
}


int pcsx4all_prof_add(const char *msg)
{
	if (pcsx4all_prof_total<PCSX4ALL_PROFILER_MAX)
	{
		pcsx4all_prof_msg[pcsx4all_prof_total]=msg;	
		return pcsx4all_prof_total++;
	}
	return -1;
}

void pcsx4all_prof_show(void)
{
	unsigned i;
	double toper=0;
	unsigned to=(get_ticks()-pcsx4all_prof_total_initial)>>10;

	for(i=0;i<pcsx4all_prof_total;i++)
		_pcsx4all_prof_end(i,0);
	puts("\n\n\n\n");
	puts("--------------------------------------------");
	for(i=0;i<pcsx4all_prof_total;i++)
	{
		unsigned t0=pcsx4all_prof_sum[i]>>10;
		double percent=(double)t0;
		percent*=100.0;
		percent/=(double)to;
		toper+=percent;
#ifdef PCSX4ALL_PROFILER_ONE
		if (i!=PCSX4ALL_PROFILER_ONE)
			continue;
#endif
		if (pcsx4all_prof_executed[i]>10000)
			printf("%s: %.2f%% -> Ticks=%i -> %iK veces\n",pcsx4all_prof_msg[i],percent,((unsigned)t0),(unsigned)(pcsx4all_prof_executed[i]>>10));
		else
			printf("%s: %.2f%% -> Ticks=%i -> %i veces\n",pcsx4all_prof_msg[i],percent,((unsigned)t0),(unsigned)(pcsx4all_prof_executed[i]));
			
	}
	printf("TOTAL: %.2f%% -> Ticks=%u, Frames=%u\n",toper,(unsigned)to,pcsx4all_prof_frames);
	puts("--------------------------------------------"); fflush(stdout);
}

int _pcsx4all_prof_start(unsigned a)
{
	pcsx4all_prof_executed[a]++;
	if (pcsx4all_prof_paused[a]) {
		pcsx4all_prof_initial[a]+=get_ticks()-pcsx4all_prof_paused[a]+1;
		pcsx4all_prof_paused[a]=0;
		return 0;
	}
	if (!pcsx4all_prof_initial[a]) {
		pcsx4all_prof_initial[a]=get_ticks();
		return 0;
	}
	return 2;
}

void _pcsx4all_prof_end(unsigned a, int yet)
{
	if ((pcsx4all_prof_initial[a])&&(!yet))
	{
		unsigned ticks=get_ticks();
		if (pcsx4all_prof_paused[a])
			pcsx4all_prof_initial[a]+=ticks-pcsx4all_prof_paused[a]+1;
		pcsx4all_prof_sum[a]+=ticks-pcsx4all_prof_initial[a];
		pcsx4all_prof_initial[a]=pcsx4all_prof_paused[a]=0;
	}
}

void _pcsx4all_prof_resume(unsigned a, int yet)
{
	if (pcsx4all_prof_paused[a] && pcsx4all_prof_initial[a] && !yet)
	{
		pcsx4all_prof_initial[a]+=get_ticks()-pcsx4all_prof_paused[a]+1;
		pcsx4all_prof_paused[a]=0;
	}
}

int _pcsx4all_prof_pause(unsigned a)
{
	if (pcsx4all_prof_initial[a] && !pcsx4all_prof_paused[a])
	{
		pcsx4all_prof_paused[a]=get_ticks();
		return 0;
	}
	return 1;
}

void _pcsx4all_prof_pause_with_resume(unsigned a, unsigned b)
{
	unsigned ticks=get_ticks();
	if (pcsx4all_prof_paused[b] && pcsx4all_prof_initial[b]) {
		pcsx4all_prof_initial[b]+=ticks-pcsx4all_prof_paused[b]+1;
		pcsx4all_prof_paused[b]=0;
	}
	if (pcsx4all_prof_initial[a] && !pcsx4all_prof_paused[a]) {
		pcsx4all_prof_paused[a]=ticks;
	}
}

int _pcsx4all_prof_start_with_pause(unsigned a, unsigned b)
{
	int ret=0;
	unsigned ticks=get_ticks();
	pcsx4all_prof_executed[a]++;
	if (pcsx4all_prof_paused[a]) {
		pcsx4all_prof_initial[a]+=ticks-pcsx4all_prof_paused[a]+1;
		pcsx4all_prof_paused[a]=0;
	}
	else
	if (!pcsx4all_prof_initial[a])
		pcsx4all_prof_initial[a]=ticks;
	else
		ret=2;
	if (pcsx4all_prof_initial[b] && !pcsx4all_prof_paused[b])
	{
		pcsx4all_prof_paused[b]=ticks;
		return 0|ret;
	}
	return 1|ret;
}

void _pcsx4all_prof_end_with_resume(unsigned a, unsigned b, int yet)
{
	unsigned ticks=get_ticks();
	if ((pcsx4all_prof_initial[a])&&(!(yet&2)))
	{
		if (pcsx4all_prof_paused[a])
			pcsx4all_prof_initial[a]+=ticks-pcsx4all_prof_paused[a]+1;
		pcsx4all_prof_sum[a]+=ticks-pcsx4all_prof_initial[a];
		pcsx4all_prof_initial[a]=pcsx4all_prof_paused[a]=0;
	}
	if (pcsx4all_prof_paused[b] && pcsx4all_prof_initial[b] && !(yet&1))
	{
		pcsx4all_prof_initial[b]+=ticks-pcsx4all_prof_paused[b]+1;
		pcsx4all_prof_paused[b]=0;
	}
}

#endif
