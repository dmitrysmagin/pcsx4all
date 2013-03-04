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
	
    LoadMcds(MCD1_FILE,MCD2_FILE);

	ret = CDR_init();
	if (ret < 0) { printf ("Error initializing CD-ROM plugin: %d\n", ret); return -1; }

	ret = GPU_init();
	if (ret < 0) { printf ("Error initializing GPU plugin: %d\n", ret); return -1; }

	ret = SPU_init();
	if (ret < 0) { printf ("Error initializing SPU plugin: %d\n", ret); return -1; }

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
