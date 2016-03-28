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

/*
 * PSX assembly interpreter.
 */

#include "psxcommon.h"
#include "r3000a.h"
#include "gte.h"
#include "psxhle.h"
#include "profiler.h"
#include "debug.h"

#define L_Op_     		((code >> 26)       )
#define L_Funct_  		((code      ) & 0x3F)
#define L_Rd_     		((code >> 11) & 0x1F)
#define L_Rt_     		((code >> 16) & 0x1F)
#define L_Rs_     		((code >> 21) & 0x1F)
#define L_Sa_     		((code >>  6) & 0x1F)
#define L_Imm_	  		((s16)code)
#define L_ImmU_	  		(code&0xffff)
#define L_rFs_    		psxRegs.CP0.r[L_Rd_]
#define L_rRs_   		regs[L_Rs_]
#define L_rRt_   		regs[L_Rt_]
#define L_rRd_   		regs[L_Rd_]
#define L_SetLink(x)   	regs[x] = psxRegs.pc + 4
#define L_JumpTarget_   (((code & 0x03ffffff) * 4) + (psxRegs.pc & 0xf0000000))
#define L_BranchTarget_ ((s16)((u16)code) * 4 + psxRegs.pc)

static unsigned int branch = 0;
static unsigned int block=0;

static unsigned int autobias_read=1;
static unsigned int autobias_write=2;
static unsigned int autobias_mul=11;
static unsigned int autobias_div=34;
static unsigned int autobias_RTPS=15;
static unsigned int autobias_NCLIP=8;
static unsigned int autobias_OP=6;
static unsigned int autobias_NCDS=19;
static unsigned int autobias_CDP=13;
static unsigned int autobias_NCDT=44;
static unsigned int autobias_NCCS=17;
static unsigned int autobias_NCS=14;
static unsigned int autobias_NCT=30;
static unsigned int autobias_SQR=5;
static unsigned int autobias_RTPT=23;
static unsigned int autobias_NCCT=39;

INLINE void INSTRUCTION(u32 *regs, unsigned int code);

#ifdef DEBUG_FAST_MEMORY
#define MemRead8(MEM) psxMemRead8(MEM)
#define MemRead16(MEM) psxMemRead16(MEM)
#define MemRead32(MEM) psxMemRead32(MEM)
#define MemWrite8(MEM,VAL) psxMemWrite8(MEM,VAL)
#define MemWrite16(MEM,VAL) psxMemWrite16(MEM,VAL)
#define MemWrite32(MEM,VAL) psxMemWrite32(MEM,VAL)
#else
INLINE u16 MemRead8(u32 mem) {
	psxRegs.cycle+=autobias_read;
	if ((mem&0x1fffffff)<0x800000)
		return *((u8 *)&psxM[mem&0x1fffff]);
	return psxMemRead8(mem);
}
INLINE u16 MemRead16(u32 mem) {
	psxRegs.cycle+=autobias_read;
	if ((mem&0x1fffffff)<0x800000)
		return *((u16 *)&psxM[mem&0x1fffff]);
	return psxMemRead16(mem);
}
INLINE u32 MemRead32(u32 mem) {
	psxRegs.cycle+=autobias_read;
	if ((mem&0x1fffffff)<0x800000)
		return *((u32 *)&psxM[mem&0x1fffff]);
	return psxMemRead32(mem);
}
INLINE void MemWrite8(u32 mem, u8 value) {
	psxRegs.cycle+=autobias_write;
	if (((mem&0x1fffffff)<0x800000)&&(Config.HLE))
		*((u8 *)&psxM[mem&0x1fffff]) = value;
	else
		psxMemWrite8(mem,value);
}
INLINE void MemWrite16(u32 mem, u16 value) {
	psxRegs.cycle+=autobias_write;
	if (((mem&0x1fffffff)<0x800000)&&(Config.HLE))
		*((u16 *)&psxM[mem&0x1fffff]) = value;
	else
		psxMemWrite16(mem,value);
}
INLINE void MemWrite32(u32 mem, u32 value) {
	psxRegs.cycle+=autobias_write;
	if (((mem&0x1fffffff)<0x800000)&&(Config.HLE))
		*((u32 *)&psxM[mem&0x1fffff]) = value;
	else
		psxMemWrite32(mem,value);
}
#endif

INLINE u32 _psxTestLoadDelay(u32 reg, u32 code) {
	if (code == 0) return 0; // NOP
	switch (L_Op_) {
		case 0x00: // SPECIAL
			switch (L_Funct_) {
				case 0x00: case 0x02: case 0x03: if (L_Rd_ == reg && L_Rt_ == reg) return 1; else if (L_Rt_ == reg) return 2; break;
				case 0x08: case 0x11: case 0x13: if (L_Rs_ == reg) return 2; break;
				case 0x09: if (L_Rd_ == reg && L_Rs_ == reg) return 1; else if (L_Rs_ == reg) return 2; break;
				case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x2a: case 0x2b: case 0x04: case 0x06: case 0x07: 
					if (L_Rd_ == reg && (L_Rt_ == reg || L_Rs_ == reg)) return 1; else if (L_Rt_ == reg || L_Rs_ == reg) return 2; break;
				case 0x18: case 0x19: case 0x1a: case 0x1b: if (L_Rt_ == reg || L_Rs_ == reg) return 2; break;
			}
			break;
		case 0x01: switch (L_Rt_) { case 0x00: case 0x02: case 0x10: case 0x12: if (L_Rs_ == reg) return 2; break; } break;
		case 0x04: case 0x05: if (L_Rs_ == reg || L_Rt_ == reg) return 2; break;
		case 0x06: case 0x07: case 0x22: case 0x26: case 0x32: case 0x3a: if (L_Rs_ == reg) return 2; break;
		case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: if (L_Rt_ == reg && L_Rs_ == reg) return 1; else if (L_Rs_ == reg) return 2; break;
		case 0x10: switch (L_Funct_) { case 0x04: case 0x06: if (L_Rt_ == reg) return 2; break; } break;
		case 0x12: switch (L_Funct_) { case 0x00: switch (L_Rs_) { case 0x04: case 0x06: if (L_Rt_ == reg) return 2; break; } break; } break;
		case 0x20: case 0x21: case 0x23: case 0x24: case 0x25: if (L_Rt_ == reg && L_Rs_ == reg) return 1; else if (L_Rs_ == reg) return 2; break;
		case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2e: if (L_Rt_ == reg || L_Rs_ == reg) return 2; break;
	}
	return 0;
}

// CHUI: Para no llamar a psxBranchTest mas de la cuenta
INLINE void _psxBranchTest(void) {
#ifdef DEBUG_IO_CYCLE_COUNTER
	psxBranchTest();
#else
	if (!Config.HLE)
		psxBranchTest();
	else
		if (psxRegs.cycle>=psxRegs.io_cycle_counter)
			psxBranchTestCalculate();
#endif
}

// CHUI: Se hace global para poderlo llamar desde el recompilador
void _psxDelayTest(u32 *regs, u32 code, u32 bpc) {
	branch = 1;
#ifdef DEBUG_CPU_OPCODES
	dbgf("DelayTest %X %p\n",code,bpc);
#endif
//	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	switch (_psxTestLoadDelay(L_Rt_, PSXMu32(bpc))) {
		case 1: branch = 0; psxRegs.pc = bpc; _psxBranchTest(); break;
		case 2:	{ u32 rold,rnew; rold=L_rRt_; INSTRUCTION(regs,code); rnew=L_rRt_; psxRegs.pc=bpc; _psxBranchTest(); L_rRt_=rold;
				  code=PSXMu32(psxRegs.pc); psxRegs.pc += 4; psxRegs.cycle += BIAS; INSTRUCTION(regs,code); L_rRt_=rnew; branch=0; } break;
		default: INSTRUCTION(regs,code); branch = 0; psxRegs.pc = bpc; _psxBranchTest(); break;
	}
//	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
}

INLINE void doBranch(u32 *regs, u32 branchPC)
{
	u32 code;
	branch = 1;
	code=PSXMu32(psxRegs.pc);
	psxRegs.code = code;
	psxRegs.pc += 4;
	psxRegs.cycle += BIAS;
	switch (L_Op_)
	{
		case 0x10: switch (L_Rs_) { case 0x00: case 0x02: _psxDelayTest(regs, code, branchPC); return; } break;
		case 0x12: switch (L_Funct_) { case 0x00: switch (L_Rs_) { case 0x00: case 0x02: _psxDelayTest(regs, code, branchPC); return; } break; } break;
		case 0x32: _psxDelayTest(regs, code, branchPC); return;
		default: if (L_Op_ >= 0x20 && L_Op_ <= 0x26) { _psxDelayTest(regs, code, branchPC); return; } break;
	}
	INSTRUCTION(regs,code);
	branch = 0;
	psxRegs.pc = branchPC;
	_psxBranchTest();
}

INLINE void _psxTestSWInts(void) {
	if (psxRegs.CP0.n.Cause & psxRegs.CP0.n.Status & 0x0300 &&	psxRegs.CP0.n.Status & 0x1) { psxException(psxRegs.CP0.n.Cause, branch); }
}

static const u32 LWL_MASK[4] = { 0xffffff, 0xffff, 0xff, 0 };
static const u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };
static const u32 LWR_MASK[4] = { 0, 0xff000000, 0xffff0000, 0xffffff00 };
static const u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };
static const u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0 };
static const u32 SWL_SHIFT[4] = { 24, 16, 8, 0 };
static const u32 SWR_MASK[4] = { 0, 0xff, 0xffff, 0xffffff };
static const u32 SWR_SHIFT[4] = { 0, 8, 16, 24 };

INLINE void INSTRUCTION(u32 *regs, unsigned int code)
{
#if defined(DEBUG_CPU_OPCODES) || defined(DEBUG_ANALYSIS)
//dbgsum("CHK",(void *)&psxM[0],0x200000);
//if (psxRegs.pc>=0x80070000 && psxRegs.pc<=0x80090000) { dbgpsxregs(); }
	dbg_opcode(psxRegs.pc,code,0,block);
#endif
	switch(L_Op_)
	{
		case  0: 
			switch(L_Funct_)
			{
				case  0: if (L_Rd_) { _u32(L_rRd_) = _u32(L_rRt_) << L_Sa_; } break;
				case  2: if (L_Rd_) { _u32(L_rRd_) = _u32(L_rRt_) >> L_Sa_; } break;
				case  3: if (L_Rd_) { _i32(L_rRd_) = _i32(L_rRt_) >> L_Sa_; } break;
				case  4: if (L_Rd_) { _u32(L_rRd_) = _u32(L_rRt_) << _u32(L_rRs_); } break;
				case  6: if (L_Rd_) { _u32(L_rRd_) = _u32(L_rRt_) >> _u32(L_rRs_); } break;
				case  7: if (L_Rd_) { _i32(L_rRd_) = _i32(L_rRt_) >> _u32(L_rRs_); } break;
				case  8: doBranch(regs,_u32(L_rRs_)); break;
				case  9: { u32 temp = _u32(L_rRs_); if (L_Rd_) { L_SetLink(L_Rd_); } doBranch(regs,temp); } break;
				case 12: psxRegs.pc -= 4; psxException(0x20, branch); break;
				case 16: if (L_Rd_) { L_rRd_ = _rHi_; } break;
				case 17: _rHi_ = L_rRs_; break;
				case 18: if (L_Rd_) { L_rRd_ = _rLo_; } break;
				case 19: _rLo_ = L_rRs_; break;
				case 24: { u64 res = (s64)((s64)_i32(L_rRs_) * (s64)_i32(L_rRt_)); psxRegs.GPR.n.lo = (u32)(res & 0xffffffff); psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff); } psxRegs.cycle +=autobias_mul; break;
				case 25: { u64 res = (u64)((u64)_u32(L_rRs_) * (u64)_u32(L_rRt_)); psxRegs.GPR.n.lo = (u32)(res & 0xffffffff); psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff); } psxRegs.cycle +=autobias_mul; break;
				case 26: if (_i32(L_rRt_)) { _i32(_rLo_) = _i32(L_rRs_) / _i32(L_rRt_); _i32(_rHi_) = _i32(L_rRs_) % _i32(L_rRt_); } psxRegs.cycle +=autobias_div; break;
				case 27: if (L_rRt_) { _rLo_ = L_rRs_ / L_rRt_; _rHi_ = L_rRs_ % L_rRt_; } psxRegs.cycle +=autobias_div; break;
				case 32: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) + _u32(L_rRt_); } break;
				case 33: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) + _u32(L_rRt_); } break;
				case 34: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) - _u32(L_rRt_); } break;
				case 35: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) - _u32(L_rRt_); } break;
				case 36: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) & _u32(L_rRt_); } break;
				case 37: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) | _u32(L_rRt_); } break;
				case 38: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) ^ _u32(L_rRt_); } break;
				case 39: if (L_Rd_) { L_rRd_ =~(_u32(L_rRs_) | _u32(L_rRt_)); } break;
				case 42: if (L_Rd_) { L_rRd_ = _i32(L_rRs_) < _i32(L_rRt_); } break;
				case 43: if (L_Rd_) { L_rRd_ = _u32(L_rRs_) < _u32(L_rRt_); } break;
				default: break;
			}
			break;
		case  1:
			switch(L_Rt_)
			{
				case  0: if(_i32(L_rRs_)<0) doBranch(regs,L_BranchTarget_); break;
				case  1: if(_i32(L_rRs_)>=0) doBranch(regs,L_BranchTarget_); break;
				case 16: if(_i32(L_rRs_)<0) { L_SetLink(31); doBranch(regs,L_BranchTarget_); } break;
				case 17: if(_i32(L_rRs_)>=0) { L_SetLink(31); doBranch(regs,L_BranchTarget_); } break;
				default: break;
			}
			break;
		case  2: doBranch(regs,L_JumpTarget_); break;
		case  3: L_SetLink(31); doBranch(regs,L_JumpTarget_); break;
		case  4: if(_i32(L_rRs_)==_i32(L_rRt_)) doBranch(regs,L_BranchTarget_); break;
		case  5: if(_i32(L_rRs_)!=_i32(L_rRt_)) doBranch(regs,L_BranchTarget_); break;
		case  6: if(_i32(L_rRs_)<=0) doBranch(regs,L_BranchTarget_); break;
		case  7: if(_i32(L_rRs_)>0) doBranch(regs,L_BranchTarget_); break;
		case  8: if (L_Rt_) { L_rRt_ = _u32(L_rRs_) + L_Imm_ ; } break;
		case  9: if (L_Rt_) { L_rRt_ = _u32(L_rRs_) + L_Imm_ ; } break;
		case 10: if (L_Rt_) { L_rRt_ = _i32(L_rRs_) < L_Imm_ ; } break;
		case 11: if (L_Rt_) { L_rRt_ = _u32(L_rRs_) < ((u32)L_Imm_); } break;
		case 12: if (L_Rt_) { L_rRt_ = _u32(L_rRs_) & L_ImmU_; } break;
		case 13: if (L_Rt_) { L_rRt_ = _u32(L_rRs_) | L_ImmU_; } break;
		case 14: if (L_Rt_) { L_rRt_ = _u32(L_rRs_) ^ L_ImmU_; } break;
		case 15: if (L_Rt_) { _u32(L_rRt_) = code << 16; } break;
		case 16: 
			switch(L_Rs_)
			{
				case 0: case 2: if (L_Rt_) { _i32(L_rRt_) = (int)L_rFs_; } break;
				case 4: case 6:
					switch (L_Rd_) {
						case 12: psxRegs.CP0.r[12] = _u32(L_rRt_); _psxTestSWInts(); break;
						case 13: psxRegs.CP0.n.Cause = _u32(L_rRt_) & ~(0xfc00); _psxTestSWInts(); break;
						default: psxRegs.CP0.r[L_Rd_] = _u32(L_rRt_); break;
					}
					break;
				case 16: psxRegs.CP0.n.Status = (psxRegs.CP0.n.Status & 0xfffffff0) | ((psxRegs.CP0.n.Status & 0x3c) >> 2); break;
				default: break;
			}
			break;
		case 18:
			psxRegs.code = code;		
			switch(L_Funct_)
			{
				case  0: 
					switch(L_Rs_)
					{
#ifdef gte_new
						case 0: _gteMFC2((psxRegisters *)regs); break;
						case 2: _gteCFC2((psxRegisters *)regs); break;
						case 4: _gteMTC2((psxRegisters *)regs); break;
						case 6: _gteCTC2((psxRegisters *)regs); break;
#else
						case 0: gteMFC2(); break;
						case 2: gteCFC2(); break;
						case 4: gteMTC2(); break;
						case 6: gteCTC2(); break;
#endif
						default: break;
					}
					psxRegs.cycle +=autobias_write;
					break;
#ifdef gte_new
				case  1: _gteRTPS((psxRegisters *)regs); psxRegs.cycle +=autobias_RTPS; break;
				case  6: _gteNCLIP((psxRegisters *)regs); psxRegs.cycle +=autobias_NCLIP; break;
				case 12: _gteOP((psxRegisters *)regs); psxRegs.cycle +=autobias_OP; break;
				case 16: _gteDPCS((psxRegisters *)regs); psxRegs.cycle +=autobias_NCLIP; break;
				case 17: _gteINTPL((psxRegisters *)regs); psxRegs.cycle +=autobias_NCLIP; break;
				case 18: _gteMVMVA((psxRegisters *)regs); psxRegs.cycle +=autobias_NCLIP; break;
				case 19: _gteNCDS((psxRegisters *)regs); psxRegs.cycle +=autobias_NCDS; break;
				case 20: _gteCDP((psxRegisters *)regs); psxRegs.cycle +=autobias_CDP; break;
				case 22: _gteNCDT((psxRegisters *)regs); psxRegs.cycle +=autobias_NCDT; break;
				case 27: _gteNCCS((psxRegisters *)regs); psxRegs.cycle +=autobias_NCCS; break;
				case 28: _gteCC((psxRegisters *)regs); psxRegs.cycle +=autobias_mul; break;
				case 30: _gteNCS((psxRegisters *)regs); psxRegs.cycle +=autobias_NCS; break;
				case 32: _gteNCT((psxRegisters *)regs); psxRegs.cycle +=autobias_NCT; break;
				case 40: _gteSQR((psxRegisters *)regs); psxRegs.cycle +=autobias_SQR; break;
				case 41: _gteDCPL((psxRegisters *)regs); psxRegs.cycle +=autobias_NCLIP; break;
				case 42: _gteDPCT((psxRegisters *)regs); psxRegs.cycle +=autobias_NCCS; break;
				case 45: _gteAVSZ3((psxRegisters *)regs); psxRegs.cycle +=autobias_SQR; break;
				case 46: _gteAVSZ4((psxRegisters *)regs); psxRegs.cycle +=autobias_OP; break;
				case 48: _gteRTPT((psxRegisters *)regs); psxRegs.cycle +=autobias_RTPT; break;
				case 61: _gteGPF((psxRegisters *)regs); psxRegs.cycle +=autobias_SQR; break;
				case 62: _gteGPL((psxRegisters *)regs); psxRegs.cycle +=autobias_SQR; break;
				case 63: _gteNCCT((psxRegisters *)regs); psxRegs.cycle +=autobias_NCCT; break;
#else
				case  1: gteRTPS(); psxRegs.cycle +=autobias_RTPS; break;
				case  6: gteNCLIP(); psxRegs.cycle +=autobias_NCLIP; break;
				case 12: gteOP(); psxRegs.cycle +=autobias_OP; break;
				case 16: gteDPCS(); psxRegs.cycle +=autobias_NCLIP; break;
				case 17: gteINTPL(); psxRegs.cycle +=autobias_NCLIP; break;
				case 18: gteMVMVA(); psxRegs.cycle +=autobias_NCLIP; break;
				case 19: gteNCDS(); psxRegs.cycle +=autobias_NCDS; break;
				case 20: gteCDP(); psxRegs.cycle +=autobias_CDP; break;
				case 22: gteNCDT(); psxRegs.cycle +=autobias_NCDT; break;
				case 27: gteNCCS(); psxRegs.cycle +=autobias_NCCS; break;
				case 28: gteCC(); psxRegs.cycle +=autobias_mul; break;
				case 30: gteNCS(); psxRegs.cycle +=autobias_NCS; break;
				case 32: gteNCT(); psxRegs.cycle +=autobias_NCT; break;
				case 40: gteSQR(); psxRegs.cycle +=autobias_SQR; break;
				case 41: gteDCPL(); psxRegs.cycle +=autobias_NCLIP; break;
				case 42: gteDPCT(); psxRegs.cycle +=autobias_NCCS; break;
				case 45: gteAVSZ3(); psxRegs.cycle +=autobias_SQR; break;
				case 46: gteAVSZ4(); psxRegs.cycle +=autobias_OP; break;
				case 48: gteRTPT(); psxRegs.cycle +=autobias_RTPT; break;
				case 61: gteGPF(); psxRegs.cycle +=autobias_SQR; break;
				case 62: gteGPL(); psxRegs.cycle +=autobias_SQR; break;
				case 63: gteNCCT(); psxRegs.cycle +=autobias_NCCT; break;
#endif
				default: break;
//default: printf("%p [NULL]\n", psxRegs.pc); break;
			}
#ifdef DEBUG_CPU_OPCODES
//			dbgpsxregsCop();
#endif
			break;
		case 32: if (L_Rt_) { _i32(L_rRt_) = (signed char)MemRead8((_u32(L_rRs_) + L_Imm_)); } else { MemRead8((_u32(L_rRs_) + L_Imm_)); } break;
		case 33: if (L_Rt_) { _i32(L_rRt_) = (short)MemRead16((_u32(L_rRs_) + L_Imm_)); } else { MemRead16((_u32(L_rRs_) + L_Imm_)); } break;
		case 34: if (L_Rt_) { u32 addr=(_u32(L_rRs_)+L_Imm_); u32 shift=addr&3; _u32(L_rRt_)=(_u32(L_rRt_)&LWL_MASK[shift])|(MemRead32(addr&~3)<<LWL_SHIFT[shift]); } break;
		case 35: if (L_Rt_) { _u32(L_rRt_) = MemRead32((_u32(L_rRs_) + L_Imm_)); } else { MemRead32((_u32(L_rRs_) + L_Imm_)); } break;
		case 36: if (L_Rt_) { _u32(L_rRt_) = MemRead8((_u32(L_rRs_) + L_Imm_)); } else { MemRead8((_u32(L_rRs_) + L_Imm_)); } break;
		case 37: if (L_Rt_) { _u32(L_rRt_) = MemRead16((_u32(L_rRs_) + L_Imm_)); } else { MemRead16((_u32(L_rRs_) + L_Imm_)); } break;
		case 38: if (L_Rt_) { u32 addr=(_u32(L_rRs_)+L_Imm_); u32 shift=addr&3; _u32(L_rRt_)=(_u32(L_rRt_)&LWR_MASK[shift])|(MemRead32(addr&~3)>>LWR_SHIFT[shift]); } break;
		case 40: MemWrite8 ((_u32(L_rRs_) + L_Imm_), _u8 (L_rRt_)); break;
		case 41: MemWrite16((_u32(L_rRs_) + L_Imm_), _u16(L_rRt_)); break;
		case 42: { u32 addr=(_u32(L_rRs_)+L_Imm_); u32 shift=addr&3; addr=addr&~3; MemWrite32(addr,(_u32(L_rRt_)>>SWL_SHIFT[shift])|(MemRead32(addr)&SWL_MASK[shift])); } break;
		case 43: MemWrite32((_u32(L_rRs_) + L_Imm_), _u32(L_rRt_)); break;
		case 46: { u32 addr=(_u32(L_rRs_)+L_Imm_); u32 shift=addr&3; addr=addr&~3; MemWrite32(addr,(_u32(L_rRt_)<<SWR_SHIFT[shift])|(MemRead32(addr)&SWR_MASK[shift])); } break;
#ifdef gte_new
		case 50: psxRegs.code = code; _gteLWC2((psxRegisters *)regs); psxRegs.cycle +=autobias_read+autobias_read; break;
		case 58: psxRegs.code = code; _gteSWC2((psxRegisters *)regs); psxRegs.cycle +=autobias_read+autobias_write; break;
#else
		case 50: psxRegs.code = code; gteLWC2(); psxRegs.cycle +=autobias_read+autobias_read; break;
		case 58: psxRegs.code = code; gteSWC2(); psxRegs.cycle +=autobias_read+autobias_write; break;
#endif
		case 59: psxHLEt[code & 0x07](); break;
		default: break;
	}
}

///////////////////////////////////////////

static int intInit(void) { return 0; }

static void intReset(void) { }

static void setAutoBias(void) {
	if (autobias) {
		autobias_read=1;
		autobias_write=2;
		autobias_mul=11;
		autobias_div=34;
		autobias_RTPS=15;
		autobias_NCLIP=8;
		autobias_OP=6;
		autobias_NCDS=19;
		autobias_CDP=13;
		autobias_NCDT=44;
		autobias_NCCS=17;
		autobias_NCS=14;
		autobias_NCT=30;
		autobias_SQR=5;
		autobias_RTPT=23;
		autobias_NCCT=39;
	} else {
		autobias_read=autobias_write=autobias_mul=autobias_div=autobias_RTPS=autobias_NCLIP=autobias_OP=autobias_NCDS=autobias_CDP=autobias_NCDT=autobias_NCCS=autobias_NCS=autobias_NCT=autobias_SQR=autobias_RTPT=autobias_NCCT=0;
	}
}
static void intExecute(void)
{
	u32 *regs=psxRegs.GPR.r;
	u32 code;
	setAutoBias();
	while (1)
	{
		code=PSXMu32(psxRegs.pc);
		psxRegs.pc += 4;
		psxRegs.cycle += BIAS;
		INSTRUCTION(regs,code);
	}
}

static void intExecuteBlock(unsigned target_pc)
{
	u32 *regs=psxRegs.GPR.r;
	u32 code;
	block=1;
	setAutoBias();
	do {
		code=PSXMu32(psxRegs.pc);
		psxRegs.pc += 4;
		psxRegs.cycle += BIAS;
		INSTRUCTION(regs,code);
	} while(psxRegs.pc!=target_pc);
	block=0;
//	ResetIoCycle();
}

static void intClear(u32 Addr, u32 Size) { }

static void intShutdown(void) { }

R3000Acpu psxInt = { intInit, intReset, intExecute, intExecuteBlock, intClear, intShutdown };
