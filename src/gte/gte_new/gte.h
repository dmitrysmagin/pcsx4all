/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis and Chui                                   *
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
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#ifndef __GTE_H__
#define __GTE_H__

#include "psxcommon.h"
#include "r3000a.h"

extern void gteMFC2(void);
extern void _gteMFC2(psxRegisters *regs);
extern void gteCFC2(void);
extern void _gteCFC2(psxRegisters *regs);
extern void gteMTC2(void);
extern void _gteMTC2(psxRegisters *regs);
extern void gteCTC2(void);
extern void _gteCTC2(psxRegisters *regs);
extern void gteLWC2(void);
extern void _gteLWC2(psxRegisters *regs);
extern void gteSWC2(void);
extern void _gteSWC2(psxRegisters *regs);

extern void gteRTPS(void);
extern void _gteRTPS(psxRegisters *regs);
extern void gteOP(void);
extern void _gteOP(psxRegisters *regs);
extern void gteNCLIP(void);
extern void _gteNCLIP(psxRegisters *regs);
extern void gteDPCS(void);
extern void _gteDPCS(psxRegisters *regs);
extern void gteINTPL(void);
extern void _gteINTPL(psxRegisters *regs);
extern void gteMVMVA(void);
extern void _gteMVMVA(psxRegisters *regs);
extern void gteNCDS(void);
extern void _gteNCDS(psxRegisters *regs);
extern void gteNCDT(void);
extern void _gteNCDT(psxRegisters *regs);
extern void gteCDP(void);
extern void _gteCDP(psxRegisters *regs);
extern void gteNCCS(void);
extern void _gteNCCS(psxRegisters *regs);
extern void gteCC(void);
extern void _gteCC(psxRegisters *regs);
extern void gteNCS(void);
extern void _gteNCS(psxRegisters *regs);
extern void gteNCT(void);
extern void _gteNCT(psxRegisters *regs);
extern void gteSQR(void);
extern void _gteSQR(psxRegisters *regs);
extern void gteDCPL(void);
extern void _gteDCPL(psxRegisters *regs);
extern void gteDPCT(void);
extern void _gteDPCT(psxRegisters *regs);
extern void gteAVSZ3(void);
extern void _gteAVSZ3(psxRegisters *regs);
extern void gteAVSZ4(void);
extern void _gteAVSZ4(psxRegisters *regs);
extern void gteRTPT(void);
extern void _gteRTPT(psxRegisters *regs);
extern void gteGPF(void);
extern void _gteGPF(psxRegisters *regs);
extern void gteGPL(void);
extern void _gteGPL(psxRegisters *regs);
extern void gteNCCT(void);
extern void _gteNCCT(psxRegisters *regs);

// for the recompiler
extern u32 gtecalcMFC2(int reg);
extern u32 _gtecalcMFC2(int reg,psxRegisters *regs);
extern void gtecalcMTC2(u32 value, int reg);
extern void _gtecalcMTC2(u32 value, int reg,psxRegisters *regs);
extern void gtecalcCTC2(u32 value, int reg);
extern void _gtecalcCTC2(u32 value, int reg,psxRegisters *regs);
extern void _gteMVMVA_cv0_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx3_s12(psxRegisters *regs);
extern void _gteMVMVA_cv0_mx3_s0(psxRegisters *regs);
extern void _gteMVMVA_cv1_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx3_s12(psxRegisters *regs);
extern void _gteMVMVA_cv1_mx3_s0(psxRegisters *regs);
extern void _gteMVMVA_cv2_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx3_s12(psxRegisters *regs);
extern void _gteMVMVA_cv2_mx3_s0(psxRegisters *regs);
extern void _gteMVMVA_cv3_mx0_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx0_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx1_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx1_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx2_s12(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx2_s0(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx3_s12(psxRegisters *regs);
extern void _gteMVMVA_cv3_mx3_s0(psxRegisters *regs);
extern void _gteOP_s12(psxRegisters *regs);
extern void _gteOP_s0(psxRegisters *regs);
extern void _gteDPCS_s12(psxRegisters *regs);
extern void _gteDPCS_s0(psxRegisters *regs);
extern void _gteGPF_s12(psxRegisters *regs);
extern void _gteGPF_s0(psxRegisters *regs);
extern void _gteSQR_s12(psxRegisters *regs);
extern void _gteSQR_s0(psxRegisters *regs);
extern void _gteINTPL_s12(psxRegisters *regs);
extern void _gteINTPL_s0(psxRegisters *regs);
extern void _gteDCPL_(psxRegisters *regs);
extern void _gteGPL_s12(psxRegisters *regs);
extern void _gteGPL_s0(psxRegisters *regs);
extern void _gteNCDS_(psxRegisters *regs);
extern void _gteNCDT_(psxRegisters *regs);
extern void _gteCDP_(psxRegisters *regs);
extern void _gteNCCS_(psxRegisters *regs);
extern void _gteCC_(psxRegisters *regs);
extern void _gteNCS_(psxRegisters *regs);
extern void _gteNCT_(psxRegisters *regs);
extern void _gteDPCT_(psxRegisters *regs);
extern void _gteNCCT_(psxRegisters *regs);
extern void _gteRTPT_(psxRegisters *regs);
#endif /* __GTE_H__ */
