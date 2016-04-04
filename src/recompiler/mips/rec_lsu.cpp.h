/* Fast reads/writes */
/* TODO: Implement in generated asm code */
static u16 MemRead8(u32 mem) {
	if ((mem&0x1fffffff)<0x800000)
		return *((u8 *)&psxM[mem&0x1fffff]);
	return psxMemRead8(mem);
}
static u16 MemRead16(u32 mem) {
	if ((mem&0x1fffffff)<0x800000)
		return *((u16 *)&psxM[mem&0x1fffff]);
	return psxMemRead16(mem);
}
static u32 MemRead32(u32 mem) {
	if ((mem&0x1fffffff)<0x800000)
		return *((u32 *)&psxM[mem&0x1fffff]);
	return psxMemRead32(mem);
}
static void MemWrite8(u32 mem, u8 value) {
	if (((mem&0x1fffffff)<0x800000)&&(Config.HLE))
		*((u8 *)&psxM[mem&0x1fffff]) = value;
	else
		psxMemWrite8(mem,value);
}
static void MemWrite16(u32 mem, u16 value) {
	if (((mem&0x1fffffff)<0x800000)&&(Config.HLE))
		*((u16 *)&psxM[mem&0x1fffff]) = value;
	else
		psxMemWrite16(mem,value);
}
static void MemWrite32(u32 mem, u32 value) {
	if (((mem&0x1fffffff)<0x800000)&&(Config.HLE))
		*((u32 *)&psxM[mem&0x1fffff]) = value;
	else
		psxMemWrite32(mem,value);
}

static void AddrToA0()
{
	s32 imm16 = (s32)(s16)_Imm_;
	u32 rs = _Rs_;
	if (rs) {
		u32 r1 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);
		ADDIU(MIPSREG_A0, r1, imm16);
		regBranchUnlock(r1);
	} else {
		LI16(MIPSREG_A0, imm16);
	}
}

/* NOTE: psxM must be mmap'ed, not malloc'ed, otherwise segfault */
static int LoadFromConstAddr(u32 insn)
{
	if (iRegs[_Rs_] != -1) {
		u32 addr = iRegs[_Rs_] + ((s32)(s16)_Imm_);
		/* DEBUGF("known address 0x%x", addr); */
		if ((addr & 0x1fffffff) < 0x200000) {
			u32 rt = _Rt_;
			u32 rs = _Rs_;
			u32 r2 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);
			u32 r1;
			if (rt != rs)
				r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);
			else
				r1 = r2;
			s32 imm16 = (s32)(s16)_Imm_;
			/* DEBUGF("r1 %d r2 %d imm16 %d", r1, r2, imm16); */
			if (addr < 0x200000) {
				LUI(TEMP_1, 0x1000);
			} else if (addr >= 0xa0000000) {
				LUI(TEMP_1, 0xb000);
			} else {
				LUI(TEMP_1, 0x9000);
			}

			XOR(TEMP_2, TEMP_1, r2);
			write32((insn) | (TEMP_2 << 21) | (r1 << 16) | (imm16 & 0xffff));
			regMipsChanged(rt); \
			regBranchUnlock(r1); \
			regBranchUnlock(r2); \
			iRegs[_Rt_] = -1; \
			return 1;
		}
	}

	return 0;
}

static int StoreToConstAddr(u32 insn)
{
	if (iRegs[_Rs_] != -1) {
		u32 addr = iRegs[_Rs_] + ((s32)(s16)_Imm_);
		/* DEBUGF("known address 0x%x", addr); */
		if ((addr & 0x1fffffff) < 0x200000) {
			u32 rt = _Rt_;
			u32 rs = _Rs_;
			u32 r2 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);
			u32 r1;
			if (rt != rs)
				r1 = regMipsToArm(rt, REG_LOAD, REG_REGISTER);
			else
				r1 = r2;
			s32 imm16 = (s32)(s16)_Imm_;
			/* DEBUGF("r1 %d r2 %d imm16 %d", r1, r2, imm16); */
			if (addr < 0x200000) {
				LUI(TEMP_1, 0x1000);
			} else if (addr >= 0xa0000000) {
				LUI(TEMP_1, 0xb000);
			} else {
				LUI(TEMP_1, 0x9000);
			}

			XOR(TEMP_2, TEMP_1, r2);
			write32((insn) | (TEMP_2 << 21) | (r1 << 16) | (imm16 & 0xffff));
			regBranchUnlock(r1);
			regBranchUnlock(r2);
			return 1;
		}
	}

	return 0;
}

static void recLB()
{
// Rt = mem[Rs + Im] (signed)
	u32 rt = _Rt_;

	if (LoadFromConstAddr(0x80000000))
		return;

	AddrToA0();
	iRegs[_Rt_] = -1;
	CALLFunc((u32)MemRead8);

	/* Sign extend */
	SLL(MIPSREG_V0, MIPSREG_V0, 24);
	SRA(MIPSREG_V0, MIPSREG_V0, 24);

	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);
		MOV(r1, MIPSREG_V0);
		regMipsChanged(rt);
		regBranchUnlock(r1);
	}
}

static void recLBU()
{
// Rt = mem[Rs + Im] (unsigned)
	u32 rt = _Rt_;

	if (LoadFromConstAddr(0x90000000))
		return;

	AddrToA0();
	iRegs[_Rt_] = -1;

	CALLFunc((u32)MemRead8);
	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);
		MOV(r1, MIPSREG_V0);
		regMipsChanged(rt);
		regBranchUnlock(r1);
	}
}

static void recLH()
{
// Rt = mem[Rs + Im] (signed)
	u32 rt = _Rt_;

	if (LoadFromConstAddr(0x84000000))
		return;

	AddrToA0();
	iRegs[_Rt_] = -1;
	CALLFunc((u32)MemRead16);

	/* Sign extend */
	SLL(MIPSREG_V0, MIPSREG_V0, 16);
	SRA(MIPSREG_V0, MIPSREG_V0, 16);

	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);
		MOV(r1, MIPSREG_V0);
		regMipsChanged(rt);
		regBranchUnlock(r1);
	}
}

static void recLHU()
{
// Rt = mem[Rs + Im] (unsigned)
	u32 rt = _Rt_;

	if (LoadFromConstAddr(0x94000000))
		return;

	AddrToA0();
	iRegs[_Rt_] = -1;
	CALLFunc((u32)MemRead16);
	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);
		MOV(r1, MIPSREG_V0);
		regMipsChanged(rt);
		regBranchUnlock(r1);
	}
}

static void recLW()
{
// Rt = mem[Rs + Im] (unsigned)
	u32 rt = _Rt_;

	if (LoadFromConstAddr(0x8c000000))
		return;

	iRegs[_Rt_] = -1;
	AddrToA0();
	CALLFunc((u32)MemRead32);
	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_FIND, REG_REGISTER);
		MOV(r1, MIPSREG_V0);
		regMipsChanged(rt);
		regBranchUnlock(r1);
	}
}

static void recSB()
{
// mem[Rs + Im] = Rt
	u32 rt = _Rt_;

	if (StoreToConstAddr(0xa0000000))
		return;

	AddrToA0();
	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_LOAD, REG_REGISTER);
		MOV(MIPSREG_A1, r1);
		regBranchUnlock(r1);
	} else {
		LI16(MIPSREG_A1, 0);
	}
	CALLFunc((u32)MemWrite8);
}

static void recSH()
{
// mem[Rs + Im] = Rt
	u32 rt = _Rt_;

	if (StoreToConstAddr(0xa4000000))
		return;

	AddrToA0();
	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_LOAD, REG_REGISTER);
		MOV(MIPSREG_A1, r1);
		regBranchUnlock(r1);
	} else {
		LI16(MIPSREG_A1, 0);
	}
	CALLFunc((u32)MemWrite16);
}

static void recSW()
{
// mem[Rs + Im] = Rt
	u32 rt = _Rt_;

	if (StoreToConstAddr(0xac000000))
		return;

	AddrToA0();
	if (rt) {
		u32 r1 = regMipsToArm(rt, REG_LOAD, REG_REGISTER);
		MOV(MIPSREG_A1, r1);
		regBranchUnlock(r1);
	} else {
		LUI(MIPSREG_A1, 0);
	}
	CALLFunc((u32)MemWrite32);
}

#if defined (interpreter_new) || defined (interpreter_none)

#define _oB_ (_u32(_rRs_) + _Imm_)
static u32 LWL_MASK[4] = { 0xffffff, 0xffff, 0xff, 0 };
static u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };

void psxLWL() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	if (!_Rt_) return;
	(_rRt_) = ( _u32(_rRt_) & LWL_MASK[shift]) |
					( mem << LWL_SHIFT[shift]);

	/*
	Mem = 1234.  Reg = abcd

	0   4bcd   (mem << 24) | (reg & 0x00ffffff)
	1   34cd   (mem << 16) | (reg & 0x0000ffff)
	2   234d   (mem <<  8) | (reg & 0x000000ff)
	3   1234   (mem      ) | (reg & 0x00000000)
	*/
}

static u32 LWR_MASK[4] = { 0, 0xff000000, 0xffff0000, 0xffffff00 };
static u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };

void psxLWR() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	if (!_Rt_) return;
	(_rRt_) = ( _u32(_rRt_) & LWR_MASK[shift]) |
					( mem >> LWR_SHIFT[shift]);

	/*
	Mem = 1234.  Reg = abcd

	0   1234   (mem      ) | (reg & 0x00000000)
	1   a123   (mem >>  8) | (reg & 0xff000000)
	2   ab12   (mem >> 16) | (reg & 0xffff0000)
	3   abc1   (mem >> 24) | (reg & 0xffffff00)
	*/
}

static u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0 };
static u32 SWL_SHIFT[4] = { 24, 16, 8, 0 };

void psxSWL() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	psxMemWrite32(addr & ~3,  (_u32(_rRt_) >> SWL_SHIFT[shift]) |
			     (  mem & SWL_MASK[shift]) );
	/*
	Mem = 1234.  Reg = abcd

	0   123a   (reg >> 24) | (mem & 0xffffff00)
	1   12ab   (reg >> 16) | (mem & 0xffff0000)
	2   1abc   (reg >>  8) | (mem & 0xff000000)
	3   abcd   (reg      ) | (mem & 0x00000000)
	*/
}

static u32 SWR_MASK[4] = { 0, 0xff, 0xffff, 0xffffff };
static u32 SWR_SHIFT[4] = { 0, 8, 16, 24 };

void psxSWR() {
	u32 addr = _oB_;
	u32 shift = addr & 3;
	u32 mem = psxMemRead32(addr & ~3);

	psxMemWrite32(addr & ~3,  (_u32(_rRt_) << SWR_SHIFT[shift]) |
			     (  mem & SWR_MASK[shift]) );

	/*
	Mem = 1234.  Reg = abcd

	0   abcd   (reg      ) | (mem & 0x00000000)
	1   bcd4   (reg <<  8) | (mem & 0x000000ff)
	2   cd34   (reg << 16) | (mem & 0x0000ffff)
	3   d234   (reg << 24) | (mem & 0x00ffffff)
	*/
}
#endif

REC_FUNC_TEST(LWL);
REC_FUNC_TEST(LWR);
REC_FUNC_TEST(SWL);
REC_FUNC_TEST(SWR);
