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


#ifndef _SIO_H_
#define _SIO_H_

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"
#include "plugins.h"
#include "psemu_plugin_defs.h"

#define MCD_SIZE	(1024 * 8 * 16)

extern char Mcd1Data[MCD_SIZE], Mcd2Data[MCD_SIZE];

extern void sioInit(void);

extern void sioWrite8(unsigned char value);
extern void sioWriteStat16(unsigned short value);
extern void sioWriteMode16(unsigned short value);
extern void sioWriteCtrl16(unsigned short value);
extern void sioWriteBaud16(unsigned short value);

extern unsigned char sioRead8(void);
extern unsigned short sioReadStat16(void);
extern unsigned short sioReadMode16(void);
extern unsigned short sioReadCtrl16(void);
extern unsigned short sioReadBaud16(void);

void netError();

extern void sioInterrupt(void);
extern int sioFreeze(gzFile f, int Mode);

extern void LoadMcd(int mcd, char *str);
extern void LoadMcds(char *mcd1, char *mcd2);
extern void SaveMcd(char *mcd, char *data, uint32_t adr, int size);
extern void CreateMcd(char *mcd);
extern void ConvertMcd(char *mcd, char *data);

typedef struct {
	char Title[48 + 1]; // Title in ASCII
	char sTitle[48 * 2 + 1]; // Title in Shift-JIS
	char ID[12 + 1];
	char Name[16 + 1];
	int IconCount;
	short Icon[16*16*3];
	unsigned char Flags;
} McdBlock;

extern void GetMcdBlockInfo(int mcd, int block, McdBlock *info);

#endif
