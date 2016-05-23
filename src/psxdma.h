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

#ifndef __PSXDMA_H__
#define __PSXDMA_H__

#include "psxcommon.h"
#include "r3000a.h"
#include "psxhw.h"
#include "psxmem.h"

//senquack - NOTE: These macros have been updated to use new PSXINT_*
// interrupts enum and intCycle struct (much cleaner than before)
// from PCSX Reloaded/Rearmed

// CHUI: Añado ResetIoCycle para permite que en el proximo salto entre en psxBranchTest
#define GPUDMA_INT(eCycle) { \
	ResetIoCycle(); \
	psxRegs.interrupt |= (1 << PSXINT_GPUDMA); \
	psxRegs.intCycle[PSXINT_GPUDMA].cycle = eCycle; \
	psxRegs.intCycle[PSXINT_GPUDMA].sCycle = psxRegs.cycle; \
}

// CHUI: Añado ResetIoCycle para permite que en el proximo salto entre en psxBranchTest
#define SPUDMA_INT(eCycle) { \
    ResetIoCycle(); \
	psxRegs.interrupt |= (1 << PSXINT_SPUDMA); \
	psxRegs.intCycle[PSXINT_SPUDMA].cycle = eCycle; \
	psxRegs.intCycle[PSXINT_SPUDMA].sCycle = psxRegs.cycle; \
}

// CHUI: Añado ResetIoCycle para permite que en el proximo salto entre en psxBranchTest
#define MDECOUTDMA_INT(eCycle) { \
	ResetIoCycle(); \
	psxRegs.interrupt |= (1 << PSXINT_MDECOUTDMA); \
	psxRegs.intCycle[PSXINT_MDECOUTDMA].cycle = eCycle; \
	psxRegs.intCycle[PSXINT_MDECOUTDMA].sCycle = psxRegs.cycle; \
}

#define MDECINDMA_INT(eCycle) { \
	psxRegs.interrupt |= (1 << PSXINT_MDECINDMA); \
	psxRegs.intCycle[PSXINT_MDECINDMA].cycle = eCycle; \
	psxRegs.intCycle[PSXINT_MDECINDMA].sCycle = psxRegs.cycle; \
}

#define GPUOTCDMA_INT(eCycle) { \
	psxRegs.interrupt |= (1 << PSXINT_GPUOTCDMA); \
	psxRegs.intCycle[PSXINT_GPUOTCDMA].cycle = eCycle; \
	psxRegs.intCycle[PSXINT_GPUOTCDMA].sCycle = psxRegs.cycle; \
}

#define CDRDMA_INT(eCycle) { \
	psxRegs.interrupt |= (1 << PSXINT_CDRDMA); \
	psxRegs.intCycle[PSXINT_CDRDMA].cycle = eCycle; \
	psxRegs.intCycle[PSXINT_CDRDMA].sCycle = psxRegs.cycle; \
}

extern void psxDma2(u32 madr, u32 bcr, u32 chcr);
extern void psxDma3(u32 madr, u32 bcr, u32 chcr);
extern void psxDma4(u32 madr, u32 bcr, u32 chcr);
extern void psxDma6(u32 madr, u32 bcr, u32 chcr);
extern void gpuInterrupt(void);
extern void spuInterrupt(void);

#endif /* __PSXDMA_H__ */
