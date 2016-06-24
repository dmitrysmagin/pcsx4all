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

//senquack - May 22 2016 NOTE: 
// These have all been updated to use new PSXINT_* interrupts enum and intCycle
// struct from PCSX Reloaded/Rearmed (much cleaner, no magic numbers)

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

//senquack - Adapted pcsxReARMed SPU to PCSX4ALL:
#ifdef spu_pcsxrearmed
//senquack - Added so spu_pcsxrearmed can read current cycle value. Some of its SPU*() functions
//           take a new "cycles" parameter, and because of circuluar header dependency problems
//           I must provide a simple pointer to the current cycle value, so the wrapped
//           functions can be passed it from anywhere in code.
const u32 * const psxRegs_cycle_valptr = &psxRegs.cycle;
#endif //spu_pcsxrearmed

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
	dbgf("psxException %X %X PC=%p\n",code,bd,psxRegs.pc);
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
#ifdef DEBUG_CPU
		dbg("bd set!!!");
#endif
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

#ifdef DEBUG_BIOS
	dbgf("!psxException 0x%X\n",psxRegs.CP0.n.Cause & 0x3c);
	if (!dbg_biosin() && isdbg() && (!Config.HLE || !(psxRegs.CP0.n.Cause & 0x3c))){
		if ((!Config.HLE) && !(psxRegs.CP0.n.Cause &0x3c))
			dbg_biosset(psxRegs.CP0.n.EPC);
		else
			dbg_biosset(psxRegs.CP0.n.EPC+4);
		dbg_disable();
	}
#endif
	if (autobias)
		psxRegs.cycle += 11;
	if (Config.HLE) {
		psxBiosException();
#ifdef DEBUG_BIOS
		if (!dbg_biosin() && isdbg() && (psxRegs.CP0.n.Cause & 0x3c)){
			dbg("RETURN BIOS");
			dbgpsxregs();
			fflush(stdout);
			psxRegs.cycle-=autobias?2:3;
//			psxBranchTest();
		}
#endif
	}
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);
//	pcsx4all_prof_start(PCSX4ALL_PROF_CPU);
#if defined(DEBUG_CPU_OPCODES) && !defined(DEBUG_BIOS)
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
		if ((psxRegs.interrupt & (1 << PSXINT_SIO)) { // sio
			unsigned n = psxRegs.intCycle[PSXINT_SIO].sCycle + psxRegs.intCycle[PSXINT_SIO].cycle;
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[PSXINT_SIO].sCycle=%u + iC[PSXINT_SIO].cycle=%u\n", n,
					psxRegs.intCycle[PSXINT_SIO].sCycle, psxRegs.intCycle[PSXINT_SIO].cycle);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDR)) { // cdr
			unsigned n = psxRegs.intCycle[PSXINT_CDR].sCycle + psxRegs.intCycle[PSXINT_CDR].cycle;
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[PSXINT_CDR].sCycle=%u + iC[PSXINT_CDR].cycle=%u\n", n,
					psxRegs.intCycle[PSXINT_CDR].sCycle, psxRegs.intCycle[PSXINT_CDR].cycle);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDREAD)) { // cdr read
			unsigned n = psxRegs.intCycle[PSXINT_CDREAD].sCycle + psxRegs.intCycle[PSXINT_CDREAD].cycle;
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[PSXINT_CDREAD].sCycle=%u + iC[PSXINT_CDREAD].cycle=%u\n", n,
					psxRegs.intCycle[PSXINT_CDREAD].sCycle, psxRegs.intCycle[PSXINT_CDREAD].cycle);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & (1 << PSXINT_GPUDMA)) { // gpu dma
			unsigned n = psxRegs.intCycle[PSXINT_GPUDMA].sCycle + psxRegs.intCycle[PSXINT_GPUDMA].cycle;
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[PSXINT_GPUDMA].sCycle=%u + iC[PSXINT_GPUDMA].cycle=%u\n", n,
					psxRegs.intCycle[PSXINT_GPUDMA].sCycle, psxRegs.intCycle[PSXINT_GPUDMA].cycle);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & (1 << PSXINT_MDECOUTDMA)) { // mdec out dma
			unsigned n = psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle + psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle;
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[PSXINT_MDECOUTDMA].sCycle=%u + iC[PSXINT_MDECOUTDMA].cycle=%u\n", n,
					psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle, psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle);
#endif
			if (n<value)
				value=n;
		}
		if (psxRegs.interrupt & (1 << PSXINT_SPUDMA)) { // spu dma
			unsigned n = psxRegs.intCycle[PSXINT_SPUDMA].sCycle + psxRegs.intCycle[PSXINT_SPUDMA].cycle;
#ifdef DEBUG_CPU_OPCODES
			dbgf("\t%u iC[PSXINT_SPUDMA].sCycle=%u + iC[PSXINT_SPUDMA].cycle=%u\n", n,
					psxRegs.intCycle[PSXINT_SPUDMA].sCycle, psxRegs.intCycle[PSXINT_SPUDMA].cycle);
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

	//senquack - NOTE: This DEBUG block has been updated to use new PSXINT_*
	// interrupts enum and intCycle struct from PCSX Reloaded/Rearmed.
	// This older DEBUG code is not a part of newer PCSXR code, so could be
	// removed in the future (TODO):
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
		if (psxRegs.interrupt & (1 << PSXINT_SIO)) { // sio
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SIO].sCycle) >= psxRegs.intCycle[PSXINT_SIO].cycle)
		{
			printf("ERROR: cycle (%i) - intCycle[PSXINT_SIO].sCycle (%i) >= intCycle[PSXINT_SIO].cycle (%i). io=%i\n",
					psxRegs.cycle, psxRegs.intCycle[PSXINT_SIO].sCycle, psxRegs.intCycle[PSXINT_SIO].cycle, psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDR)) { // cdr
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDR].sCycle) >= psxRegs.intCycle[PSXINT_CDR].cycle)
		{
			printf("ERROR: cycle (%i) - intCycle[PSXINT_CDR].sCycle (%i) >= intCycle[PSXINT_CDR].cycle (%i). io=%i\n",
					psxRegs.cycle, psxRegs.intCycle[PSXINT_CDR].sCycle, psxRegs.intCycle[PSXINT_CDR].cycle, psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDREAD)) { // cdr read
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDREAD].sCycle) >= psxRegs.intCycle[PSXINT_CDREAD].cycle)
		{
			printf("ERROR: cycle (%i) - intCycle[PSXINT_CDREAD].sCycle (%i) >= intCycle[PSXINT_CDREAD].cycle (%i). io=%i\n",
					psxRegs.cycle, psxRegs.intCycle[PSXINT_CDREAD].sCycle, psxRegs.intCycle[PSXINT_CDREAD].cycle, psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & (1 << PSXINT_GPUDMA)) { // gpu dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_GPUDMA].sCycle) >= psxRegs.intCycle[PSXINT_GPUDMA].cycle)
		{
			printf("ERROR: cycle (%i) - intCycle[PSXINT_GPUDMA].sCycle (%i) >= intCycle[PSXINT_GPUDMA].cycle (%i). io=%i\n",
					psxRegs.cycle, psxRegs.intCycle[PSXINT_GPUDMA].sCycle, psxRegs.intCycle[PSXINT_GPUDMA].cycle, psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & (1 << PSXINT_MDECOUTDMA)) { // mdec out dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle) >= psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle)
		{
			printf("ERROR: cycle (%i) - intCycle[PSXINT_MDECOUTDMA].sCycle (%i) >= intCycle[PSXINT_MDECOUTDMA].cycle (%i). io=%i\n",
					psxRegs.cycle, psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle, psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle, psxRegs.io_cycle_counter);
			pcsx4all_exit();
		}
		if (psxRegs.interrupt & (1 << PSXINT_SPUDMA)) { // spu dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SPUDMA].sCycle) >= psxRegs.intCycle[PSXINT_SPUDMA].cycle)
		{
			printf("ERROR: cycle (%i) - intCycle[PSXINT_SPUDMA].sCycle (%i) >= intCycle[PSXINT_SPUDMA].cycle (%i). io=%i\n",
					psxRegs.cycle, psxRegs.intCycle[PSXINT_SPUDMA].sCycle, psxRegs.intCycle[PSXINT_SPUDMA].cycle, psxRegs.io_cycle_counter);
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

//senquack - NOTE: This DEBUG block has been updated to use new PSXINT_*
// interrupts enum and intCycle struct from PCSX Reloaded/Rearmed.
// This older DEBUG code is not a part of newer PCSXR code, so could be
// removed in the future (TODO):
#ifdef DEBUG_BIOS
	dbgf("Counters %u %u, IntCycle:",psxNextCounter,psxNextsCounter);
	for(unsigned i=0;i<PSXINT_COUNT;i++){
		dbgf("\n\t%.2u:", i);
		dbgf(" sCycle:%u cycle:%u", psxRegs.intCycle[i].sCycle, psxRegs.intCycle[i].cycle);
	}
	dbg("");
#endif

	//senquack - Update SPU plugin & feed audio backend. This used to be done
	// in psxcounters.cpp VSync rootcounter, but moved here after rootcounter
	// code was updated to match pcsx_rearmed.
	// NOTE: PSXINT_SPU_UPDATE is always active and checked, it does not
	//  set or check a flag of its own in psxRegs.interrupts.
	if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SPU_UPDATE].sCycle) >= psxRegs.intCycle[PSXINT_SPU_UPDATE].cycle) {
#ifdef spu_pcsxrearmed
		//Clear any scheduled SPUIRQ, as HW SPU IRQ will end up handled with
		// this call to SPU_async(), and new SPUIRQ scheduled if necessary.
		psxRegs.interrupt &= ~(1 << PSXINT_SPUIRQ);

		SPU_async(psxRegs.cycle, 1);
#else
		SPU_async();
#endif
		SCHEDULE_SPU_UPDATE(spu_upd_interval);
	}

	if (psxRegs.interrupt) {
		//senquack - TODO: add support for new Config.Sio option of PCSXR?
		if ((psxRegs.interrupt & (1 << PSXINT_SIO)) /* && !Config.Sio */ ) { // sio
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SIO].sCycle) >= psxRegs.intCycle[PSXINT_SIO].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_SIO);
				sioInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n = psxRegs.intCycle[PSXINT_SIO].sCycle + psxRegs.intCycle[PSXINT_SIO].cycle;
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDR)) { // cdr
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDR].sCycle) >= psxRegs.intCycle[PSXINT_CDR].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDR);
				cdrInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n = psxRegs.intCycle[PSXINT_CDR].sCycle + psxRegs.intCycle[PSXINT_CDR].cycle;
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDREAD)) { // cdr read
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDREAD].sCycle) >= psxRegs.intCycle[PSXINT_CDREAD].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDREAD);
				cdrReadInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n = psxRegs.intCycle[PSXINT_CDREAD].sCycle + psxRegs.intCycle[PSXINT_CDREAD].cycle;
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & (1 << PSXINT_GPUDMA)) { // gpu dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_GPUDMA].sCycle) >= psxRegs.intCycle[PSXINT_GPUDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_GPUDMA);
				gpuInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n = psxRegs.intCycle[PSXINT_GPUDMA].sCycle + psxRegs.intCycle[PSXINT_GPUDMA].cycle;
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & (1 << PSXINT_MDECOUTDMA)) { // mdec out dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle) >= psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_MDECOUTDMA);
				mdec1Interrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n = psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle + psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle;
			if (n<value)
				value=n;
#endif
		}
		if (psxRegs.interrupt & (1 << PSXINT_SPUDMA)) { // spu dma
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SPUDMA].sCycle) >= psxRegs.intCycle[PSXINT_SPUDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_SPUDMA);
				spuInterrupt();
			}
#ifndef USE_BRANCH_TEST_CALCULATE
			unsigned n = psxRegs.intCycle[PSXINT_SPUDMA].sCycle + psxRegs.intCycle[PSXINT_SPUDMA].cycle;
			if (n<value)
				value=n;
#endif
		}

		//senquack - NOTE: all the remaining PSXINT_* cases here are new ones
		// introduced from PCSX Reloaded/Rearmed:

		if (psxRegs.interrupt & (1 << PSXINT_MDECINDMA)) { // mdec in
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_MDECINDMA].sCycle) >= psxRegs.intCycle[PSXINT_MDECINDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_MDECINDMA);
				mdec0Interrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_GPUOTCDMA)) { // gpu otc
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_GPUOTCDMA].sCycle) >= psxRegs.intCycle[PSXINT_GPUOTCDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_GPUOTCDMA);
				gpuotcInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDRDMA)) { // cdrom
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDRDMA].sCycle) >= psxRegs.intCycle[PSXINT_CDRDMA].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDRDMA);
				cdrDmaInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDRPLAY)) { // cdr play timing
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDRPLAY].sCycle) >= psxRegs.intCycle[PSXINT_CDRPLAY].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDRPLAY);
				cdrPlayInterrupt();
			}
		}
		if (psxRegs.interrupt & (1 << PSXINT_CDRLID)) { // cdr lid states
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_CDRLID].sCycle) >= psxRegs.intCycle[PSXINT_CDRLID].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_CDRLID);
				cdrLidSeekInterrupt();
			}
		}
#ifdef spu_pcsxrearmed
		//senquack - spu_pcsxrearmed plugin schedules this update when a game
		// is using the HW SPU IRQ (like Metal Gear Solid). The '0' parameter
		// to SPU_async() tells SPU plugin to just scan for upcoming HW IRQs,
		// but not to feed audio backend.
		if (psxRegs.interrupt & (1 << PSXINT_SPUIRQ)) { // scheduled spu HW IRQ update
			if ((psxRegs.cycle - psxRegs.intCycle[PSXINT_SPUIRQ].sCycle) >= psxRegs.intCycle[PSXINT_SPUIRQ].cycle) {
				psxRegs.interrupt &= ~(1 << PSXINT_SPUIRQ);
				SPU_async(psxRegs.cycle, 0);
			}
		}
#endif
	}

	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_TEST,PCSX4ALL_PROF_CPU);

	if (psxHu32(0x1070) & psxHu32(0x1074)) {
		if ((psxRegs.CP0.n.Status & 0x401) == 0x401) {
#ifdef DEBUG_CPU
			dbgf("Interrupt: %x %x\n", psxHu32(0x1070), psxHu32(0x1074));
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
#ifdef DEBUG_CPU
	int back=isdbg();
	dbg_disable();
#endif
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_psxExecuteBios++;
#endif
	while (psxRegs.pc != 0x80030000)
		psxCpu->ExecuteBlock(0x80030000);
#ifdef DEBUG_CPU
	if (back)
		dbg_enable();
#endif
}

