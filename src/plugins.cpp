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
* Plugin library callback/access functions.
*/

#include "plugins.h"
#include "psxevents.h"

int LoadPlugins(void) {
	int ret;
	const char *cdrfilename=NULL;

	ReleasePlugins();

	if ( LoadMcd(1, Config.Mcd1) < 0 ||
	     LoadMcd(2, Config.Mcd2) < 0 ) {
		printf("Error initializing memcards\n");
		return -1;
	}

	ret = CDR_init();
	if (ret < 0) { printf ("Error initializing CD-ROM plugin: %d\n", ret); return -1; }

	ret = GPU_init();
	if (ret < 0) { printf ("Error initializing GPU plugin: %d\n", ret); return -1; }

	ret = SPU_init();
	if (ret < 0) { printf ("Error initializing SPU plugin: %d\n", ret); return -1; }

#ifdef spu_pcsxrearmed
	//senquack - PCSX Rearmed SPU supports handling SPU hardware IRQ through
	// two callback functions. Needed by games like NFS3, Grandia, Fifa98,
	// Chrono Cross, THPS2 etc.
	// Only spu_pcsxrearmed supports this (older SPU plugins all had
	// problems with sound in these games.. TODO: add support?)
	SPU_registerCallback(Trigger_SPU_IRQ);

	//senquack - pcsx_rearmed SPU plugin schedules its own updates to scan
	// for upcoming SPU HW interrupts, calling above function when needed.
	SPU_registerScheduleCb(Schedule_SPU_IRQ);
#endif

	cdrfilename=GetIsoFile();
	if (cdrfilename[0] != '\0') {
		ret=CDR_open();
		if (ret < 0) { printf ("Error opening CD-ROM: %s\n", cdrfilename); return -1; }
	}
	
	printf("Plugins loaded.\n");
	return 0;
}

void ReleasePlugins(void) {
	CDR_shutdown();
	GPU_shutdown();
	SPU_shutdown();
}

//senquack - UpdateSPU() provides a void(void) function that wraps
// call to SPU_async(), allowing generic handling of event handlers
// in r3000a.cpp psxBranchTest()
void UpdateSPU(void)
{
#ifdef spu_pcsxrearmed
	//Clear any scheduled SPUIRQ, as HW SPU IRQ will end up handled with
	// this call to SPU_async(), and new SPUIRQ scheduled if necessary.
	psxEventQueue.dequeue(PSXINT_SPUIRQ);

	SPU_async(psxRegs.cycle, 1);
#else
	SPU_async();
#endif
	SCHEDULE_SPU_UPDATE(spu_upd_interval);
}

//senquack - HandleSPU_IRQ() provides a void(void) function that wraps
// call to SPU_async(), allowing generic handling of event handlers
// in r3000a.cpp psxBranchTest(). NOTE: Only spu_pcsxrearmed actually
// implements handling of SPU HW IRQs.
void HandleSPU_IRQ(void)
{
#ifdef spu_pcsxrearmed
	SPU_async(psxRegs.cycle, 0);
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

//senquack - A generic function SPU plugins can use to set the hardware
// SPU interrupt bit (currently only used by spu_pcsxrearmed)
void CALLBACK Trigger_SPU_IRQ(void) {
	psxHu32ref(0x1070) |= SWAPu32(0x200);
	// Ensure psxBranchTest() is called soon when IRQ is pending:
	ResetIoCycle();
}

//senquack - A generic function SPU plugins can use to schedule an update
// that scans for upcoming SPU HW IRQs. If one is encountered, above
// function will be called.
// (This is used by games that use the actual hardware SPU IRQ like
//  Need for Speed 3, Metal Gear Solid, Chrono Cross, etc. and is currently
//  only implemented in spu_pcsxrearmed)
void CALLBACK Schedule_SPU_IRQ(unsigned int cycles_after) {
	psxEventQueue.enqueue(PSXINT_SPUIRQ, cycles_after);
}

#ifdef __cplusplus
}
#endif
