// Generate inline psxMemRead/Write or call them as-is
#define USE_DIRECT_MEM_ACCESS

/* Helper for generating rec*** function that calls psx*** one */
#define REC_FUNC(f) \
extern void psx##f(); \
void rec##f() \
{ \
	regClearJump(); \
	LI32(TEMP_1, pc); \
	SW(TEMP_1, PERM_REG_1, off(pc)); \
	LI32(TEMP_1, psxRegs.code); \
	SW(TEMP_1, PERM_REG_1, off(code)); \
	CALLFunc((u32)psx##f); \
}

#define OPCODE(insn, rt, rn, imm) \
	write32((insn) | ((rn) << 21) | ((rt) << 16) | ((imm) & 0xffff))

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
			OPCODE(insn, r1, TEMP_2, imm16);
			regMipsChanged(rt);
			regBranchUnlock(r1);
			regBranchUnlock(r2);
			iRegs[_Rt_] = -1;
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
			OPCODE(insn, r1, TEMP_2, imm16);
			regBranchUnlock(r1);
			regBranchUnlock(r2);
			return 1;
		}
	}

	return 0;
}

#ifdef USE_DIRECT_MEM_ACCESS
static void LoadFromAddr(u32 insn)
{
	// Rt = [Rs + imm16]
	s32 imm16 = (s32)(s16)_Imm_;
	u32 rs = _Rs_;
	u32 rt = _Rt_;
	u32 *backpatch1, *backpatch2, *backpatch3, *backpatch5, *backpatch6;

	u32 r1 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);
	u32 r2 = regMipsToArm(rt, REG_FIND, REG_REGISTER);
	ADDIU(MIPSREG_A0, r1, imm16);

	SRL(TEMP_1, MIPSREG_A0, 16);
	LI16(TEMP_2, 0x1f80);
	backpatch1 = (u32 *)recMem;
	BEQ(TEMP_1, TEMP_2, 0); // beq temp_1, temp_2, label_1f80
	NOP();

	LI32(TEMP_2, (u32)psxMemRLUT);
	SLL(TEMP_1, TEMP_1, 2);
	ADDU(TEMP_1, TEMP_2, TEMP_1);
	LW(TEMP_1, TEMP_1, 0);

	backpatch3 = (u32 *)recMem;
	BEQZ(TEMP_1, 0); // beqz temp_1, label_exit
	NOP();

	ANDI(TEMP_2, MIPSREG_A0, 0xffff);
	ADDU(TEMP_1, TEMP_1, TEMP_2);
	OPCODE(insn, r2, TEMP_1, 0);

	backpatch2 = (u32 *)recMem;
	B(0); // b label_exit
	NOP();

	// label_1f80:
	*backpatch1 |= mips_relative_offset(backpatch1, (u32)recMem, 4);
	ANDI(TEMP_2, MIPSREG_A0, 0xffff);
	SLTIU(TEMP_3, TEMP_2, 0x1000);
	backpatch5 = (u32 *)recMem;
	BEQZ(TEMP_3, 0); // beqz temp_3, label_call_hle
	NOP();

	LI32(TEMP_1, (u32)psxH);
	ADDU(TEMP_1, TEMP_1, TEMP_2);
	OPCODE(insn, r2, TEMP_1, 0);

	backpatch6 = (u32 *)recMem;
	B(0); // b label_exit
	NOP();

	// label_call_hle:
	*backpatch5 |= mips_relative_offset(backpatch5, (u32)recMem, 4);
	switch (insn) {
	case 0x80000000: CALLFunc((u32)psxHwRead8); SEB(r2, MIPSREG_V0); break; // LB
	case 0x90000000: CALLFunc((u32)psxHwRead8); MOV(r2, MIPSREG_V0); break; // LBU
	case 0x84000000: CALLFunc((u32)psxHwRead16); SEH(r2, MIPSREG_V0); break; // LH
	case 0x94000000: CALLFunc((u32)psxHwRead16); MOV(r2, MIPSREG_V0); break; // LHU
	case 0x8c000000: CALLFunc((u32)psxHwRead32); MOV(r2, MIPSREG_V0); break; // LW
	}

	// label_exit:
	*backpatch2 |= mips_relative_offset(backpatch2, (u32)recMem, 4);
	*backpatch3 |= mips_relative_offset(backpatch3, (u32)recMem, 4);
	*backpatch6 |= mips_relative_offset(backpatch6, (u32)recMem, 4);

	regMipsChanged(rt);
	regBranchUnlock(r1);
	regBranchUnlock(r2);
}

void psxMemWrite32_error(u32 mem, u32 value);

static void StoreToAddr(u32 insn)
{
	s32 imm16 = (s32)(s16)_Imm_;
	u32 rs = _Rs_;
	u32 rt = _Rt_;
	u32 *backpatch1, *backpatch2, *backpatch3, *backpatch4=0;
	u32 *backpatch5, *backpatch6;

	u32 r1 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);
	u32 r2 = regMipsToArm(rt, REG_LOAD, REG_REGISTER);

	ADDIU(MIPSREG_A0, r1, imm16);
	MOV(MIPSREG_A1, r2);

	SRL(TEMP_1, MIPSREG_A0, 16);
	LI16(TEMP_2, 0x1f80);
	backpatch1 = (u32 *)recMem;
	BEQ(TEMP_1, TEMP_2, 0); // beq temp_1, temp_2, label_1f80
	NOP();

	LI32(TEMP_2, (u32)psxMemWLUT);
	SLL(TEMP_1, TEMP_1, 2);
	ADDU(TEMP_1, TEMP_2, TEMP_1);
	LW(TEMP_1, TEMP_1, 0);

	backpatch3 = (u32 *)recMem;
	BEQZ(TEMP_1, 0); // beqz temp_1, label_exit or label_error
	NOP();

	ANDI(TEMP_2, MIPSREG_A0, 0xffff);
	ADDU(TEMP_1, TEMP_1, TEMP_2);
	OPCODE(insn, r2, TEMP_1, 0);

	SRL(TEMP_1, MIPSREG_A0, 16);
	SLL(TEMP_1, TEMP_1, 2);
	LI32(TEMP_2, (u32)psxRecLUT);
	ADDU(TEMP_1, TEMP_1, TEMP_2);
	LW(TEMP_1, TEMP_1, 0);
	ANDI(TEMP_2, MIPSREG_A0, (~3));
	ADDU(TEMP_1, TEMP_1, TEMP_2);
	SW(0, TEMP_1, 0);

	backpatch2 = (u32 *)recMem;
	B(0); // b label_exit
	NOP();

	/* Generate psxMemWrite32_error() for SW only */
	/* This is needed for cache control in bios */
	// label_error:
	if (insn == 0xac000000) {
		*backpatch3 |= mips_relative_offset(backpatch3, (u32)recMem, 4);
		CALLFunc((u32)psxMemWrite32_error);
		backpatch4 = (u32 *)recMem;
		B(0); // b label_exit
		NOP();
	}

	// label_1f80:
	*backpatch1 |= mips_relative_offset(backpatch1, (u32)recMem, 4);
	ANDI(TEMP_2, MIPSREG_A0, 0xffff);
	SLTIU(TEMP_3, TEMP_2, 0x1000);
	backpatch5 = (u32 *)recMem;
	BEQZ(TEMP_3, 0); // beqz temp_3, label_call_hle
	NOP();

	LI32(TEMP_1, (u32)psxH);
	ADDU(TEMP_1, TEMP_1, TEMP_2);
	OPCODE(insn, r2, TEMP_1, 0);

	backpatch6 = (u32 *)recMem;
	B(0); // b label_exit
	NOP();

	// label_call_hle:
	*backpatch5 |= mips_relative_offset(backpatch5, (u32)recMem, 4);
	switch (insn) {
	case 0xa0000000: CALLFunc((u32)psxHwWrite8); break;
	case 0xa4000000: CALLFunc((u32)psxHwWrite16); break;
	case 0xac000000: CALLFunc((u32)psxHwWrite32); break;
	default: break;
	}

	// label2_exit
	*backpatch2 |= mips_relative_offset(backpatch2, (u32)recMem, 4);
	if (insn == 0xac000000)
		*backpatch4 |= mips_relative_offset(backpatch4, (u32)recMem, 4);
	else
		*backpatch3 |= mips_relative_offset(backpatch3, (u32)recMem, 4);
	*backpatch6 |= mips_relative_offset(backpatch6, (u32)recMem, 4);

	regBranchUnlock(r1);
	regBranchUnlock(r2);
}

#else

static void LoadFromAddr(u32 insn)
{
	// Rt = [Rs + imm16]
	s32 imm16 = (s32)(s16)_Imm_;
	u32 rs = _Rs_;
	u32 rt = _Rt_;

	u32 r1 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);
	u32 r2 = regMipsToArm(rt, REG_FIND, REG_REGISTER);

	ADDIU(MIPSREG_A0, r1, imm16);

	switch (insn) {
	case 0x80000000: CALLFunc((u32)psxMemRead8); SEB(r2, MIPSREG_V0); break; // LB
	case 0x90000000: CALLFunc((u32)psxMemRead8); MOV(r2, MIPSREG_V0); break; // LBU
	case 0x84000000: CALLFunc((u32)psxMemRead16); SEH(r2, MIPSREG_V0); break; // LH
	case 0x94000000: CALLFunc((u32)psxMemRead16); MOV(r2, MIPSREG_V0); break; // LHU
	case 0x8c000000: CALLFunc((u32)psxMemRead32); MOV(r2, MIPSREG_V0); break; // LW
	}

	regMipsChanged(rt);
	regBranchUnlock(r1);
	regBranchUnlock(r2);
}

static void StoreToAddr(u32 insn)
{
	// mem[Rs + Im] = Rt
	s32 imm16 = (s32)(s16)_Imm_;
	u32 rs = _Rs_;
	u32 rt = _Rt_;

	u32 r1 = regMipsToArm(rs, REG_LOAD, REG_REGISTER);
	u32 r2 = regMipsToArm(rt, REG_LOAD, REG_REGISTER);

	ADDIU(MIPSREG_A0, r1, imm16);
	MOV(MIPSREG_A1, r2);

	switch (insn) {
	case 0xa0000000: CALLFunc((u32)psxMemWrite8); break;
	case 0xa4000000: CALLFunc((u32)psxMemWrite16); break;
	case 0xac000000: CALLFunc((u32)psxMemWrite32); break;
	default: break;
	}

	regBranchUnlock(r1);
	regBranchUnlock(r2);
}
#endif

static void recLB()
{
	// Rt = mem[Rs + Im] (signed)
	if (LoadFromConstAddr(0x80000000))
		return;

	iRegs[_Rt_] = -1;

	LoadFromAddr(0x80000000);
}

static void recLBU()
{
	// Rt = mem[Rs + Im] (unsigned)
	if (LoadFromConstAddr(0x90000000))
		return;

	iRegs[_Rt_] = -1;

	LoadFromAddr(0x90000000);
}

static void recLH()
{
	// Rt = mem[Rs + Im] (signed)
	if (LoadFromConstAddr(0x84000000))
		return;

	iRegs[_Rt_] = -1;

	LoadFromAddr(0x84000000);
}

static void recLHU()
{
	// Rt = mem[Rs + Im] (unsigned)
	if (LoadFromConstAddr(0x94000000))
		return;

	iRegs[_Rt_] = -1;

	LoadFromAddr(0x94000000);
}

static void recLW()
{
	// Rt = mem[Rs + Im] (unsigned)
	if (LoadFromConstAddr(0x8c000000))
		return;

	iRegs[_Rt_] = -1;

	LoadFromAddr(0x8c000000);
}

static void recSB()
{
	// mem[Rs + Im] = Rt
	if (StoreToConstAddr(0xa0000000))
		return;

	StoreToAddr(0xa0000000);
}

static void recSH()
{
	// mem[Rs + Im] = Rt
	if (StoreToConstAddr(0xa4000000))
		return;

	StoreToAddr(0xa4000000);
}

static void recSW()
{
	// mem[Rs + Im] = Rt
	if (StoreToConstAddr(0xac000000))
		return;

	StoreToAddr(0xac000000);
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

REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(SWL);
REC_FUNC(SWR);
