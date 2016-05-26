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

int LoadPlugins(void) {
	int ret;
	const char *cdrfilename=NULL;

	ReleasePlugins();

	LoadMcds(Config.Mcd1, Config.Mcd2);

	ret = CDR_init();
	if (ret < 0) { printf ("Error initializing CD-ROM plugin: %d\n", ret); return -1; }

	ret = GPU_init();
	if (ret < 0) { printf ("Error initializing GPU plugin: %d\n", ret); return -1; }

	ret = SPU_init();
	if (ret < 0) { printf ("Error initializing SPU plugin: %d\n", ret); return -1; }

#ifdef spu_pcsxrearmed
	//senquack - NOTE: this is an important function to call, as SPU
	// IRQs will not be acknowledged otherwise, leading to repeating sound
	// problems in games like NFS3, Grandia, Fifa98, and some games will
	// lack music like Chrono Cross FMV music, THPS2 etc.
	// Only spu_pcsxrearmed supports this (older SPU plugins all had
	// problems with sound in these games.. TODO: add support?)
	SPU_registerCallback(AcknowledgeSPUIRQ);

	//senquack - pcsx_rearmed SPU plugin schedules its own updates:
	SPU_registerScheduleCb(ScheduleSPUUpdate);
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

#ifdef __cplusplus
extern "C" {
#endif

//senquack - A generic function SPU plugins can use to acknowledge SPU interrupts:
void CALLBACK AcknowledgeSPUIRQ(void) {
	psxHu32ref(0x1070) |= SWAPu32(0x200);
}

//senquack - A generic function SPU plugins can use to schedule an SPU update:
void CALLBACK ScheduleSPUUpdate(unsigned int cycles_after) {
	psxRegs.interrupt |= (1 << PSXINT_SPU_UPDATE);
	psxRegs.intCycle[PSXINT_SPU_UPDATE].cycle = cycles_after;
	psxRegs.intCycle[PSXINT_SPU_UPDATE].sCycle = psxRegs.cycle;
}

#ifdef __cplusplus
}
#endif
