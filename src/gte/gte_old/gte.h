/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2003  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __GTE_H__
#define __GTE_H__

#if 0
void gteMFC2();
void gteCFC2();
void gteMTC2();
void gteCTC2();
void gteLWC2();
void gteSWC2();

void gteRTPS();
void gteOP();
void gteNCLIP();
void gteDPCS();
void gteINTPL();
void gteMVMVA();
void gteNCDS();
void gteNCDT();
void gteCDP();
void gteNCCS();
void gteCC();
void gteNCS();
void gteNCT();
void gteSQR();
void gteDCPL();
void gteDPCT();
void gteAVSZ3();
void gteAVSZ4();
void gteRTPT();
void gteGPF();
void gteGPL();
void gteNCCT();
#endif
void gteMFC2(u32 code);
void gteCFC2(u32 code);
void gteMTC2(u32 code);
void gteCTC2(u32 code);
void gteLWC2(u32 code, u32 pc);
void gteSWC2(u32 code, u32 pc);

void gteRTPS();
void gteOP(u32 code);
void gteNCLIP();
void gteDPCS();
void gteINTPL();
void gteMVMVA(u32 code);
void gteNCDS();
void gteNCDT();
void gteCDP();
void gteNCCS();
void gteCC();
void gteNCS();
void gteNCT();
void gteSQR(u32 code);
void gteDCPL();
void gteDPCT();
void gteAVSZ3();
void gteAVSZ4();
void gteRTPT();
void gteGPF(u32 code);
void gteGPL(u32 code);
void gteNCCT();

#endif /* __GTE_H__ */
