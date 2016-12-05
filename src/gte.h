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

#ifndef __GTE_H__
#define __GTE_H__

#include "psxcommon.h"
#include "r3000a.h"

extern void gteMFC2(void);
extern void gteCFC2(void);
extern void gteMTC2(void);
extern void gteCTC2(void);
extern void gteLWC2(void);
extern void gteSWC2(void);

extern void gteRTPS(void);
extern void gteOP(void);
extern void gteNCLIP(void);
extern void gteDPCS(void);
extern void gteINTPL(void);
extern void gteMVMVA(void);
extern void gteNCDS(void);
extern void gteNCDT(void);
extern void gteCDP(void);
extern void gteNCCS(void);
extern void gteCC(void);
extern void gteNCS(void);
extern void gteNCT(void);
extern void gteSQR(void);
extern void gteDCPL(void);
extern void gteDPCT(void);
extern void gteAVSZ3(void);
extern void gteAVSZ4(void);
extern void gteRTPT(void);
extern void gteGPF(void);
extern void gteGPL(void);
extern void gteNCCT(void);

// for the recompiler
extern u32 gtecalcMFC2(int reg);
extern void gtecalcMTC2(u32 value, int reg);
extern void gtecalcCTC2(u32 value, int reg);

#endif /* __GTE_H__ */
