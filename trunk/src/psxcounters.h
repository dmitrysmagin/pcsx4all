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

#ifndef __PSXCOUNTERS_H__
#define __PSXCOUNTERS_H__

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"
#include "plugins.h"

extern u32 psxNextCounter, psxNextsCounter;

typedef struct Rcnt
{
    u16 mode, target;
    u32 rate, irq, counterState, irqState;
    u32 cycle, cycleStart;
} Rcnt;
extern Rcnt rcnts[];

extern void psxRcntUVtarget(void);

extern void psxRcntInit(void);
extern void psxRcntUpdate(void);

extern void psxRcntWcount(u32 index, u32 value);
extern void psxRcntWmode(u32 index, u32 value);
extern void psxRcntWtarget(u32 index, u32 value);

extern u32 psxRcntRcount(u32 index);
extern u32 psxRcntRmode(u32 index);
extern u32 psxRcntRtarget(u32 index);

extern s32 psxRcntFreeze(gzFile f, s32 Mode);

extern void psxSetSyncs(unsigned h_sync, unsigned s_sync);
extern unsigned psxGetHSync(void);
extern unsigned psxGetSpuSync(void);

#endif /* __PSXCOUNTERS_H__ */
