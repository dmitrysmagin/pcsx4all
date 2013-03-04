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

#ifndef __MDEC_H__
#define __MDEC_H__

#include "psxcommon.h"
#include "r3000a.h"
#include "psxhw.h"
#include "psxdma.h"

extern void mdecInit(void);
extern void mdecWrite0(u32 data);
extern void mdecWrite1(u32 data);
extern u32  mdecRead0(void);
extern u32  mdecRead1(void);
extern void psxDma0(u32 madr, u32 bcr, u32 chcr);
extern void psxDma1(u32 madr, u32 bcr, u32 chcr);
extern void mdec1Interrupt(void);
extern int  mdecFreeze(gzFile f, int Mode);

#endif
