/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

/*
* Internal PSX HLE functions.
*/

#include "psxhle.h"
#include "profiler.h"

static void hleDummy(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_hleDummy++;
#endif
	if (autobias) psxRegs.cycle += 5;
#ifdef DEBUG_HLE
	{
		char msg[50];
		sprintf((char *)&msg[0],"hleDummy (ret=%p) MEMORY SUM",psxRegs.GPR.n.ra);
		dbgsum((char *)&msg[0],(void *)&psxM[0],0x200000);
		dbgregs((void *)&psxRegs);
	}
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
	psxRegs.pc = psxRegs.GPR.n.ra;

//	psxBranchTest();
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
}

static void hleA0(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_hleA0++;
#endif
	if (autobias) psxRegs.cycle += 51;
#ifdef DEBUG_HLE
	{
		char msg[50];
		sprintf((char *)&msg[0],"hleA0 (call=%p) MEMORY SUM",psxRegs.GPR.n.t1 & 0xff);
		dbgsum((char *)&msg[0],(void *)&psxM[0],0x200000);
		dbgregs((void *)&psxRegs);
	}
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
	u32 call = psxRegs.GPR.n.t1 & 0xff;

	if (biosA0[call]) biosA0[call]();

#ifdef DEBUG_BIOS
	psxRegs.cycle+=3;
	dbg_bioscheckopcode(psxRegs.pc);
	psxRegs.cycle-=3;
#else
	psxBranchTest();
#endif
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
}

static void hleB0(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_hleB0++;
#endif
	if (autobias) psxRegs.cycle += 51;
#ifdef DEBUG_HLE
	{
		char msg[50];
		sprintf((char *)&msg[0],"hleB0 (call=%p) MEMORY SUM",psxRegs.GPR.n.t1 & 0xff);
		dbgsum((char *)&msg[0],(void *)&psxM[0],0x200000);
		dbgregs((void *)&psxRegs);
	}
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
	u32 call = psxRegs.GPR.n.t1 & 0xff;

	if (biosB0[call]) biosB0[call]();

#ifdef DEBUG_BIOS
	psxRegs.cycle+=3;
	dbg_bioscheckopcode(psxRegs.pc);
	psxRegs.cycle-=3;
#else
	psxBranchTest();
#endif
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
}

static void hleC0(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_hleC0++;
#endif
	if (autobias) psxRegs.cycle += 51;
#ifdef DEBUG_HLE
	{
		char msg[50];
		sprintf((char *)&msg[0],"hleC0 (call=%p) MEMORY SUM",psxRegs.GPR.n.t1 & 0xff);
		dbgsum((char *)&msg[0],(void *)&psxM[0],0x200000);
		dbgregs((void *)&psxRegs);
	}
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
	u32 call = psxRegs.GPR.n.t1 & 0xff;

	if (biosC0[call]) biosC0[call]();

#ifdef DEBUG_BIOS
	psxRegs.cycle+=3;
	dbg_bioscheckopcode(psxRegs.pc);
	psxRegs.cycle-=3;
#else
//	psxBranchTest();
#endif
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
}

static void hleBootstrap(void) { // 0xbfc00000
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_hleBootstrap++;
#endif
	if (autobias) psxRegs.cycle += 15;
#ifdef DEBUG_HLE
	{
		char msg[50];
		sprintf((char *)&msg[0],"hleBootstrap MEMORY SUM");
		dbgsum((char *)&msg[0],(void *)&psxM[0],0x200000);
		dbgregs((void *)&psxRegs);
	}
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
	printf("hleBootstrap\n");
	CheckCdrom();
	LoadCdrom();
	printf("CdromLabel: \"%s\": PC = %8.8lx (SP = %8.8lx)\n", CdromLabel, psxRegs.pc, psxRegs.GPR.n.sp);
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
}

typedef struct {                   
	u32 _pc0;      
	u32 gp0;      
	u32 t_addr;   
	u32 t_size;   
	u32 d_addr;   
	u32 d_size;   
	u32 b_addr;   
	u32 b_size;   
	u32 S_addr;
	u32 s_size;
	u32 _sp,_fp,_gp,ret,base;
} EXEC;

static void hleExecRet(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_hleExecRet++;
#endif
	if (autobias) psxRegs.cycle += 15;
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
	EXEC *header = (EXEC*)PSXM(psxRegs.GPR.n.s0);

	//printf("ExecRet %x: %x\n", psxRegs.GPR.n.s0, header->ret);
#ifdef DEBUG_HLE
	{
		char msg[50];
		sprintf((char *)&msg[0],"hleBExecRet %x: %x MEMORY SUM", psxRegs.GPR.n.s0, header->ret);
		dbgsum((char *)&msg[0],(void *)&psxM[0],0x200000);
		dbgregs((void *)&psxRegs);
	}
#endif

	psxRegs.GPR.n.ra = header->ret;
	psxRegs.GPR.n.sp = header->_sp;
	psxRegs.GPR.n.s8 = header->_fp;
	psxRegs.GPR.n.gp = header->_gp;
	psxRegs.GPR.n.s0 = header->base;

	psxRegs.GPR.n.v0 = 1;
	psxRegs.pc = psxRegs.GPR.n.ra;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_HLE,PCSX4ALL_PROF_CPU);
}

void (*psxHLEt[256])(void) = {
	hleDummy, hleA0, hleB0, hleC0,
	hleBootstrap, hleExecRet,
	hleDummy, hleDummy
};
