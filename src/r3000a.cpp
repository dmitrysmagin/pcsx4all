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
* R3000A CPU functions.
*/

#include "r3000a.h"
#include "cdrom.h"
#include "mdec.h"
#include "gte.h"
#include "profiler.h"
#include "debug.h"

PcsxConfig Config;
R3000Acpu *psxCpu=NULL;
psxRegisters psxRegs;
u32 BIAS=3; /* 2 */
int autobias=0;
u32 PSXCLK=33868800; /* 33.8688 Mhz */

int psxInit() {
	printf("Running PCSX Version %s (%s).\n", PACKAGE_VERSION, __DATE__);

// CHUI: Añado el inicio del profiler
	pcsx4all_prof_init();
	pcsx4all_prof_add("CPU"); // PCSX4ALL_PROF_CPU
	pcsx4all_prof_add("HW_READ"); // PCSX4ALL_PROF_HW_READ
	pcsx4all_prof_add("HW_WRITE"); // PCSX4ALL_PROF_HW_WRITE
	pcsx4all_prof_add("COUNTERS"); // PCSX4ALL_PROF_COUNTERS
	pcsx4all_prof_add("HLE"); // PCSX4ALL_PROF_HLE
	pcsx4all_prof_add("GTE"); // PCSX4ALL_PROF_GTE
	pcsx4all_prof_add("GPU"); // PCSX4ALL_PROF_GPU
	pcsx4all_prof_add("RECOMPILER"); // PCSX4ALL_PROF_REC
	pcsx4all_prof_add("BTEST"); // PCSX4ALL_PROF_TEST
#ifdef PSXREC
	#ifndef interpreter_none
	if (Config.Cpu == CPU_INTERPRETER) {
		psxCpu = &psxInt;
	} else
	#endif
	psxCpu = &psxRec;
#else
	psxCpu = &psxInt;
#endif

	if (psxMemInit() == -1) return -1;

	return psxCpu->Init();
}

void psxReset() {

	psxCpu->Reset();

	psxMemReset();

	memset(&psxRegs, 0, sizeof(psxRegs));

	psxRegs.pc = 0xbfc00000; // Start in bootstrap
// CHUI: Añadimos el puntero a la memoria PSX
	psxRegs.psxM = psxM;	// PSX Memory
	psxRegs.psxP = psxP;	// PSX Memory
	psxRegs.psxR = psxR;	// PSX Memory
	psxRegs.psxH = psxH;	// PSX Memory

	psxRegs.CP0.r[12] = 0x10900000; // COP0 enabled | BEV = 1 | TS = 1
	psxRegs.CP0.r[15] = 0x00000002; // PRevID = Revision ID, same as R3000A

	psxHwReset();
	psxBiosInit();

	if (!Config.HLE)
		psxExecuteBios();

#ifdef EMU_LOG
	EMU_LOG("*BIOS END*\n");
#endif
}

void psxShutdown() {
	psxMemShutdown();
	psxBiosShutdown();

	psxCpu->Shutdown();
}

void psxException(u32 code, u32 bd) {
#ifdef DEBUG_CPU_OPCODES
	dbgf("psxException %X %X\n",code,bd);
	dbgregs((void *)&psxRegs);
#endif
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_psxException++;
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);
//	_pcsx4all_prof_end(PCSX4ALL_PROF_CPU,0);
	// Set the Cause
	psxRegs.CP0.n.Cause = code;

	// Set the EPC & PC
	if (bd) {
#ifdef PSXCPU_LOG
		PSXCPU_LOG("bd set!!!\n");
#endif
		printf("bd set!!!\n");
		psxRegs.CP0.n.Cause|= 0x80000000;
		psxRegs.CP0.n.EPC = (psxRegs.pc - 4);
	} else
		psxRegs.CP0.n.EPC = (psxRegs.pc);

	if (psxRegs.CP0.n.Status & 0x400000)
		psxRegs.pc = 0xbfc00180;
	else
		psxRegs.pc = 0x80000080;

	// Set the Status
	psxRegs.CP0.n.Status = (psxRegs.CP0.n.Status &~0x3f) |
						  ((psxRegs.CP0.n.Status & 0xf) << 2);
// CHUI: Añado ResetIoCycle para permite que en el proximo salto entre en psxBranchTest
	ResetIoCycle();

	if (!Config.HLE && (((PSXMu32(psxRegs.CP0.n.EPC) >> 24) & 0xfe) == 0x4a)) {
		// "hokuto no ken" / "Crash Bandicot 2" ... fix
		PSXMu32ref(psxRegs.CP0.n.EPC)&= SWAPu32(~0x02000000);
	}

	if (Config.HLE) psxBiosException();
	if (autobias) psxRegs.cycle += 11;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);
//	pcsx4all_prof_start(PCSX4ALL_PROF_CPU);
#ifdef DEBUG_CPU_OPCODES
	dbgf("!psxException PC=%p\n",psxRegs.pc);
	dbgregs((void *)&psxRegs);
#endif
}

#ifdef USE_BRANCH_TEST_CALCULATE
// CHUI: Funcion que calcula la proxima entrada en psxBranchTest
INLINE void psxCalculateIoCycle() {
	unsigned value=psxNextCounter+psxNextsCounter;
#ifdef DEBUG_CPU_OPCODES
	dbgf("CalculateIoCycle next=%u, nexts=%u, int=%X\n",psxNextCounter,psxNextsCounter,psxRegs.interrupt);
#endif

	if (psxRegs.interrupt) {
		if ((psxRegs.interrupt & 0x80) && (!Config.Sio)) {
			unsigned n=psxRegs.intCycle[7]+psxRegs.intCycle[7+1];
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[7]=%u+ iC[8]=%u\n",n,psxRegs.intCycle[7],psxRegs.intCycle[7+1]);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & 0x04) { // cdr
			unsigned n=psxRegs.intCycle[2]+psxRegs.intCycle[2+1];
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[2]=%u+ iC[3]=%u\n",n,psxRegs.intCycle[2],psxRegs.intCycle[2+1]);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & 0x040000) { // cdr read
			unsigned n=psxRegs.intCycle[2+16]+psxRegs.intCycle[2+16+1];
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[18]=%u+ iC[19]=%u\n",n,psxRegs.intCycle[2+16],psxRegs.intCycle[2+16+1]);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & 0x01000000) { // gpu dma
			unsigned n=psxRegs.intCycle[3+24]+psxRegs.intCycle[3+24+1];
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[27]=%u+ iC[28]=%u\n",n,psxRegs.intCycle[3+24],psxRegs.intCycle[3+24+1]);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & 0x02000000) { // mdec out dma
			unsigned n=psxRegs.intCycle[5+24]+psxRegs.intCycle[5+24+1];
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[29]=%u+ iC[30]=%u\n",n,psxRegs.intCycle[5+24],psxRegs.intCycle[5+24+1]);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & 0x04000000) { // spu dma
			unsigned n=psxRegs.intCycle[1+24]+psxRegs.intCycle[1+24+1];
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[25]=%u+ iC[26]=%u\n",n,psxRegs.intCycle[1+24],psxRegs.intCycle[1+24+1]);
#endif
			if (n<value)
				value=n;
        	}
	}
	if (psxHu32(0x1070) & psxHu32(0x1074)) {
		if ((psxRegs.CP0.n.Status & 0x401) == 0x401) {
#ifdef DEBUG_CPU_OPCODES
			dbgf("\tException Hu32[1070]=%X & Hu32[1074]=%X & Stat=X\n",psxHu32(0x1070),psxHu32(0x1074),psxRegs.CP0.n.Status);
#endif
			value=0;
		}
	}
	if ((value > (unsigned)-0x10000)||(value<0x10000)){
#ifdef DEBUG_CPU_OPCODES
		if (value<0x10000) {
			dbgf("\tValue %u Lower\n",value);
		} else {
			dbgf("\tValue %u Higher\n",value);
		}
#endif
		psxRegs.io_cycle_counter=0;
	} else {
#ifdef DEBUG_CPU_OPCODES
		dbgf("\tIO_CYCLE=%u\n",value);
#endif
		psxRegs.io_cycle_counter=value;
	}
}
#endif

#if defined(DEBUG_CPU_OPCODES) && defined(USE_BRANCH_TEST_CALCULATE)
static int yet_branch_tested=0;
#endif

void psxBranchTest() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_psxBranchTest++;
#endif
#ifdef DEBUG_IO_CYCLE_COUNTER
//	printf("%i (%i)\n",psxRegs.cycle,psxRegs.io_cycle_counter);
//CHUI: Si no supera el umbral no comprueba nada
	if (psxRegs.cycle< psxRegs.io_cycle_counter)
	{
		extern unsigned iocycle_saltado;
		iocycle_saltado++;
		if ((psxRegs.cycle - psxNextsCounter) >= psxNextCounter)
		{
			printf("ERROR: cycle (%i) - nexts (%i) >= next (%i). io=%i\n",psxRegs.cycle,psxNextsCounter,psxNextCounter,psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if ((psxRegs.interrupt & 0x80) && (!Config.Sio)) // sio
			if ((psxRegs.cycle - psxRegs.intCycle[7]) >= psxRegs.intCycle[7+1])
		{
			printf("ERROR: cycle (%i) - intCycle[7] (%i) >= intCycle[8] (%i). io=%i\n",psxRegs.cycle,psxRegs.intCycle[7],psxRegs.intCycle[7+1],psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & 0x04) // cdr
			if ((psxRegs.cycle - psxRegs.intCycle[2]) >= psxRegs.intCycle[2+1])
		{
			printf("ERROR: cycle (%i) - intCycle[2] (%i) >= intCycle[3] (%i). io=%i\n",psxRegs.cycle,psxRegs.intCycle[2],psxRegs.intCycle[2+1],psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & 0x040000) // cdr read
			if ((psxRegs.cycle - psxRegs.intCycle[2+16]) >= psxRegs.intCycle[2+16+1])
		{
			printf("ERROR: cycle (%i) - intCycle[18] (%i) >= intCycle[19] (%i). io=%i\n",psxRegs.cycle,psxRegs.intCycle[2+16],psxRegs.intCycle[2+16+1],psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & 0x01000000) // gpu dma
			if ((psxRegs.cycle - psxRegs.intCycle[3+24]) >= psxRegs.intCycle[3+24+1])
		{
			printf("ERROR: cycle (%i) - intCycle[27] (%i) >= intCycle[28] (%i). io=%i\n",psxRegs.cycle,psxRegs.intCycle[3+24],psxRegs.intCycle[3+24+1],psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & 0x02000000) // mdec out dma
			if ((psxRegs.cycle - psxRegs.intCycle[5+24]) >= psxRegs.intCycle[5+24+1])
		{
			printf("ERROR: cycle (%i) - intCycle[29] (%i) >= intCycle[30] (%i). io=%i\n",psxRegs.cycle,psxRegs.intCycle[5+24],psxRegs.intCycle[5+24+1],psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
        	if (psxRegs.interrupt & 0x04000000) // spu dma
            		if ((psxRegs.cycle - psxRegs.intCycle[1+24]) >= psxRegs.intCycle[1+24+1])
		{
			printf("ERROR: cycle (%i) - intCycle[25] (%i) >= intCycle[26] (%i). io=%i\n",psxRegs.cycle,psxRegs.intCycle[1+24],psxRegs.intCycle[1+24+1],psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxHu32(0x1070) & psxHu32(0x1074))
			if ((psxRegs.CP0.n.Status & 0x401) == 0x401)
		{
			printf("ERROR: cycle (%i) psxHu32[0x1070] (%i) psxHu32[0x1074] (%i) Status (%X). io=%i\n",psxRegs.cycle,psxHu32(0x1070),psxHu32(0x1074),psxRegs.CP0.n.Status,psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		return;
	}
	else
	{
		extern unsigned iocycle_ok;
		iocycle_ok++;
	}
#else
#ifdef DEBUG_CPU_OPCODES
#ifdef USE_BRANCH_TEST_CALCULATE
	int back_yet_branch_tested=yet_branch_tested;
	if (!yet_branch_tested) {
		if (psxRegs.cycle< psxRegs.io_cycle_counter)
			return;
		dbgf("Btest (%u>=%u) PC=%p\n",psxRegs.cycle,psxRegs.io_cycle_counter,psxRegs.pc);
	}
	yet_branch_tested=0;
#else
	dbgf("Btest (%u>=%u) PC=%p\n",psxRegs.cycle,psxRegs.io_cycle_counter,psxRegs.pc);
#endif
#endif
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);
	
	if ((psxRegs.cycle - psxNextsCounter) >= psxNextCounter)
		psxRcntUpdate();
#ifndef USE_BRANCH_TEST_CALCULATE
	unsigned value=psxNextCounter+psxNextsCounter;
#endif

	if (psxRegs.interrupt) {
		if ((psxRegs.interrupt & 0x80) && (!Config.Sio)) { // sio
			if ((psxRegs.cycle - psxRegs.intCycle[7]) >= psxRegs.intCycle[7+1]) {
				psxRegs.interrupt&=~0x80;
				sioInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n=psxRegs.intCycle[7]+psxRegs.intCycle[7+1];
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & 0x04) { // cdr
			if ((psxRegs.cycle - psxRegs.intCycle[2]) >= psxRegs.intCycle[2+1]) {
				psxRegs.interrupt&=~0x04;
				cdrInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n=psxRegs.intCycle[2]+psxRegs.intCycle[2+1];
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & 0x040000) { // cdr read
			if ((psxRegs.cycle - psxRegs.intCycle[2+16]) >= psxRegs.intCycle[2+16+1]) {
				psxRegs.interrupt&=~0x040000;
				cdrReadInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n=psxRegs.intCycle[2+16]+psxRegs.intCycle[2+16+1];
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & 0x01000000) { // gpu dma
			if ((psxRegs.cycle - psxRegs.intCycle[3+24]) >= psxRegs.intCycle[3+24+1]) {
				psxRegs.interrupt&=~0x01000000;
				gpuInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n=psxRegs.intCycle[3+24]+psxRegs.intCycle[3+24+1];
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & 0x02000000) { // mdec out dma
			if ((psxRegs.cycle - psxRegs.intCycle[5+24]) >= psxRegs.intCycle[5+24+1]) {
				psxRegs.interrupt&=~0x02000000;
				mdec1Interrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n=psxRegs.intCycle[5+24]+psxRegs.intCycle[5+24+1];
			if (n<value)
				value=n;
#endif
		}
        	if (psxRegs.interrupt & 0x04000000) { // spu dma
            		if ((psxRegs.cycle - psxRegs.intCycle[1+24]) >= psxRegs.intCycle[1+24+1]) {
                		psxRegs.interrupt&=~0x04000000;
                		spuInterrupt();
        		}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n=psxRegs.intCycle[5+24]+psxRegs.intCycle[5+24+1];
			if (n<value)
				value=n;
#endif
		}
	}
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);

	if (psxHu32(0x1070) & psxHu32(0x1074)) {
		if ((psxRegs.CP0.n.Status & 0x401) == 0x401) {
#ifdef PSXCPU_LOG
			PSXCPU_LOG("Interrupt: %x %x\n", psxHu32(0x1070), psxHu32(0x1074));
#endif
//			SysPrintf("Interrupt (%x): %x %x\n", psxRegs.cycle, psxHu32(0x1070), psxHu32(0x1074));
			psxException(0x400, 0);
#ifndef USE_BRANCH_TEST_CALCULATE
			value=0;
#endif
		}
	}
#ifndef USE_BRANCH_TEST_CALCULATE
	if ((value > (unsigned)-0x10000)||(value<0x10000)){
#ifdef DEBUG_CPU_OPCODES
		if (value<0x10000) {
			dbgf("\tValue %u Lower\n",value);
		} else {
			dbgf("\tValue %u Higher\n",value);
		}
#endif
		psxRegs.io_cycle_counter=0;
	} else {
#ifdef DEBUG_CPU_OPCODES
		dbgf("\tIO_CYCLE=%u\n",value);
#endif
		psxRegs.io_cycle_counter=value;
	}
#else
#ifdef DEBUG_IO_CYCLE_COUNTER
	psxCalculateIoCycle();
#endif
#endif
#if defined(DEBUG_CPU_OPCODES) && defined(USE_BRANCH_TEST_CALCULATE) && !defined(DEBUG_IO_CYCLE_COUNTER)
	if (!back_yet_branch_tested) {
		psxCalculateIoCycle();
#ifdef DEBUG_CPU_OPCODES
//		dbgsum("SUM ",(void *)&psxM[0],0x200000);
		dbgregs((void *)&psxRegs);
		dbgf("!Btest (%u) PC=%p\n",psxRegs.io_cycle_counter,psxRegs.pc);
#endif
	}
#endif
}

#ifdef USE_BRANCH_TEST_CALCULATE
void psxBranchTestCalculate() {
#ifdef DEBUG_CPU_OPCODES
	yet_branch_tested=1;
	dbgf("Btest (%u>=%u) PC=%p\n",psxRegs.cycle,psxRegs.io_cycle_counter,psxRegs.pc);
#endif
//	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	psxRegs.cycle_add=0;
#endif
	psxBranchTest();
	psxCalculateIoCycle();
//	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);
#ifdef DEBUG_CPU_OPCODES
	yet_branch_tested=0;
//	dbgsum("SUM ",(void *)&psxM[0],0x200000);
	dbgregs((void *)&psxRegs);
	dbgf("!Btest (%u) PC=%p\n",psxRegs.io_cycle_counter,psxRegs.pc);
#endif
}
#endif

void psxExecuteBios() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_psxExecuteBios++;
#endif
	while (psxRegs.pc != 0x80030000)
		psxCpu->ExecuteBlock(0x80030000);
}

