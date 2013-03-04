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

#include "psxcommon.h"
#include "psxmem.h"
#include "r3000a.h"
#include "profiler.h"
#include "debug.h"

typedef struct tagGlobalData
{
    uint8_t CurByte1;
    uint8_t CmdLen1;
    uint8_t CurByte2;
    uint8_t CmdLen2;
} GLOBALDATA;

static GLOBALDATA g={0,0,0,0};

unsigned char PAD1_startPoll(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_PAD1_startPoll++;
#endif
	g.CurByte1 = 0; return 0xFF;
}

unsigned char PAD2_startPoll(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_PAD2_startPoll++;
#endif
	g.CurByte2 = 0; return 0xFF;
}

unsigned char PAD1_poll(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_PAD1_poll++;
#endif
	static uint8_t		buf[8] = {0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80};

	if (g.CurByte1 == 0) {
		uint16_t			n;
		g.CurByte1++;

		n = pad_read(0);
#ifdef AUTOEVENTS
		static unsigned event=0;
		static unsigned short _pad1_=0xffff;
		static clock_t t_init;
		if (!event) t_init=clock();
		event++;
#ifdef SHOW_BIOS_CALLS
		printf("EVENT %i\n",event);
#endif
#ifdef DEBUG_START
		if (event>=DEBUG_START)
			dbg_enable();
#endif
#ifdef AUTOEVENTS_MAX
		if (event>AUTOEVENTS_MAX) {
#ifndef DEBUG_PCSX4ALL
			printf("TIME %.2fsec\n",((double)(clock()-t_init))/1000000.0);
#endif
			pcsx4all_exit();
		}
#endif
#ifdef PROFILER_PCSX4ALL_RESET
		static int prof_started=0;
		if (event==PROFILER_PCSX4ALL_RESET)
		{
//puts("RESET!!!");fflush(stdout);
			prof_started=1;
			pcsx4all_prof_reset();
			pcsx4all_prof_start(PCSX4ALL_PROF_CPU);
		}
#endif
#ifdef DEBUG_CPU_OPCODES
		{
			char msg[50];
			sprintf((char *)&msg[0],"Event %i MEMORY SUM",event);
			dbgsum((char *)&msg[0],(void *)&psxM[0],0x200000);
			dbgregsCop((void *)&psxRegs);
		}
#endif
//printf("EVENTO %i\n",event); fflush(stdout);
//		switch ((event>>1)&31)
		switch (event&31)
		{
// /*
			case 0: _pad1_ &= ~(1 << 12); break;
			case 3: _pad1_ |= (1 << 12); break;
// */
// /*
			case 12: _pad1_ |= (1 << 14); break;
			case 15: _pad1_ &= ~(1 << 14); break;
// */
// /*
			case 18: _pad1_ |= (1 << 15); break;
			case 21: _pad1_ &= ~(1 << 15); break;
// */
// /*
			case 6: _pad1_ &= ~(1 << 13); break;
			case 9: _pad1_ |= (1 << 13); break;
// */
			case 24:
#ifdef PROFILER_PCSX4ALL_RESET
				 if (!prof_started)
#endif
				 _pad1_ &= ~(1 << 3);
				 break;
			case 26:
#ifdef PROFILER_PCSX4ALL_RESET
				 if (!prof_started)
#endif
				 _pad1_ |= (1 << 3);
				 break;
		}
		n=_pad1_;
#endif

		buf[2] = n & 0xFF;
		buf[3] = n >> 8;

		g.CmdLen1 = 4;

		return 0x41;
	}

	if (g.CurByte1 >= g.CmdLen1) return 0xFF;
	return buf[g.CurByte1++];
}

unsigned char PAD2_poll(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_PAD2_poll++;
#endif
	static uint8_t		buf[8] = {0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80};

	if (g.CurByte2 == 0) {
		uint16_t			n;
		g.CurByte2++;

#ifdef AUTOEVENTS
		n = 0xffff;
#else
		n = pad_read(1);
#endif

		buf[2] = n & 0xFF;
		buf[3] = n >> 8;

		g.CmdLen2 = 4;

		return 0x41;
	}

	if (g.CurByte2 >= g.CmdLen2) return 0xFF;
	return buf[g.CurByte2++];
}
