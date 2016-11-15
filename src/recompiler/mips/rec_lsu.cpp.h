//TODO: Adapt all code here to use guard 'HAVE_MIPS32R2_EXT_INS',
//      with fallbacks to pre-R2 instructions. Only some has been.

// Generate inline psxMemRead/Write or call them as-is
#define USE_DIRECT_MEM_ACCESS
#define USE_CONST_ADDRESSES

// Upper/lower 64K of PSX RAM (psxM[]) is now mirrored to virtual address
//  regions surrounding psxM[]. This allows skipping mirror-region boundary
//  check which special-cased loads/stores that crossed the boundary, the
//  Einhander game fix. See notes in psxmem.cpp psxMemInit().
#define SKIP_SAME_2MB_REGION_CHECK

#define OPCODE(insn, rt, rn, imm) \
	write32((insn) | ((rn) << 21) | ((rt) << 16) | ((imm) & 0xffff))

//#define LOG_WL_WR
//#define LOG_LOADS
//#define LOG_STORES

static void disasm_psx(u32 pc)
{
	static char buffer[512];
	u32 opcode = *(u32 *)((char *)PSXM(pc));
	disasm_mips_instruction(opcode, buffer, pc, 0, 0);
	printf("%08x: %08x %s\n", pc, opcode, buffer);
}

s32 imm_max, imm_min;

/* NOTE: psxM must be mmap'ed, not malloc'ed, otherwise segfault */
static int LoadFromConstAddr(int count)
{
#ifdef USE_CONST_ADDRESSES
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].r + imm_min;

#ifndef SKIP_SAME_2MB_REGION_CHECK
		/* Check if addr and addr+imm are in the same 2MB region */
		if ((addr & 0xe00000) != (iRegs[_Rs_].r & 0xe00000))
			return 0;
#endif

		// Is address in lower 8MB region? (2MB mirrored x4)
		if ((addr & 0x1fffffff) < 0x800000) {
			u32 r2 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
			u32 PC = pc - 4;

			#ifdef WITH_DISASM
			for (int i = 0; i < count-1; i++)
				DISASM_PSX(pc + i * 4);
			#endif

			if ((u32)psxM == 0x10000000) {
				// psxM base is mmap'd at virtual address 0x10000000
				LUI(TEMP_2, 0x1000);
				INS(TEMP_2, r2, 0, 0x15); // TEMP_2 = 0x10000000 | (r2 & 0x1fffff)
			} else {
				LW(TEMP_2, PERM_REG_1, off(psxM));
				EXT(TEMP_1, r2, 0, 0x15);
				ADDU(TEMP_2, TEMP_2, TEMP_1);
			}

			do {
				u32 opcode = *(u32 *)((char *)PSXM(PC));
				s32 imm = _fImm_(opcode);
				u32 rt = _fRt_(opcode);
				u32 r1 = regMipsToHost(rt, REG_FIND, REG_REGISTER);

				OPCODE(opcode & 0xfc000000, r1, TEMP_2, imm);

				SetUndef(rt);
				regMipsChanged(rt);
				regUnlock(r1);
				PC += 4;
			} while (--count);

			pc = PC;
			regUnlock(r2);

			return 1;
		}
	}
#endif // USE_CONST_ADDRESSES

	return 0;
}

static int StoreToConstAddr(int count)
{
#ifdef USE_CONST_ADDRESSES
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].r + imm_min;

#ifndef SKIP_SAME_2MB_REGION_CHECK
		/* Check if addr and addr+imm are in the same 2MB region */
		if ((addr & 0xe00000) != (iRegs[_Rs_].r & 0xe00000))
			return 0;
#endif

		// Is address in lower 8MB region? (2MB mirrored x4)
		if ((addr & 0x1fffffff) < 0x800000) {
			u32 r2 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
			u32 PC = pc - 4;

			#ifdef WITH_DISASM
			for (int i = 0; i < count-1; i++)
				DISASM_PSX(pc + i * 4);
			#endif

			if ((u32)psxM == 0x10000000) {
				// psxM base is mmap'd at virtual address 0x10000000
				LUI(TEMP_2, 0x1000);
				INS(TEMP_2, r2, 0, 0x15); // TEMP_2 = 0x10000000 | (r2 & 0x1fffff)
			} else {
				LW(TEMP_2, PERM_REG_1, off(psxM));
				EXT(TEMP_1, r2, 0, 0x15);
				ADDU(TEMP_2, TEMP_2, TEMP_1);
			}

			do {
				u32 opcode = *(u32 *)((char *)PSXM(PC));
				s32 imm = _fImm_(opcode);
				u32 rt = _fRt_(opcode);
				u32 r1 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

				OPCODE(opcode & 0xfc000000, r1, TEMP_2, imm);

				regUnlock(r1);
				PC += 4;
			} while (--count);

			pc = PC;
			regUnlock(r2);
			return 1;
		}
	}
#endif // USE_CONST_ADDRESSES

	return 0;
}

static void LoadFromAddr(int count)
{
	// Rt = [Rs + imm16]
	u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 PC = pc - 4;

	#ifdef WITH_DISASM
	for (int i = 0; i < count-1; i++)
		DISASM_PSX(pc + i * 4);
	#endif

#ifdef USE_DIRECT_MEM_ACCESS
	regPushState();

	// Is address in lower 8MB region? (2MB mirrored x4)
	//  We check only the effective address of first load in the series,
	// seeing if bits 27:24 are unset to determine if it is in lower 8MB.
	// See comments in StoreToAddr().

	// Get the effective address of first load in the series.
	// ---> NOTE: leave value in MIPSREG_A0, it will be used later!
	ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef HAVE_MIPS32R2_EXT_INS
	EXT(TEMP_2, MIPSREG_A0, 24, 4);
#else
	LUI(TEMP_2, 0x0f00);
	AND(TEMP_2, TEMP_2, MIPSREG_A0);
#endif
	u32 *backpatch_label_hle_1 = (u32 *)recMem;
	BGTZ(TEMP_2, 0); // beqz temp_2, label_hle
	//NOTE: Delay slot of branch is harmlessly occupied by the first op
	// of the branch-not-taken section below (writing to TEMP_2)

#ifndef SKIP_SAME_2MB_REGION_CHECK
	/* Check if addr and addr+imm are in the same 2MB region */
	EXT(TEMP_2, MIPSREG_A0, 21, 3); // <BD slot> TEMP_2 = (MIPSREG_A0 >> 21) & 0x7
	EXT(TEMP_3, r1, 21, 3);         //           TEMP_3 = (r1 >> 21) & 0x7
	u32 *backpatch_label_hle_2 = (u32 *)recMem;
	BNE(TEMP_2, TEMP_3, 0);         // goto label_hle if not in same 2MB region
	//NOTE: Delay slot of branch is harmlessly occupied by the first op
	// of the branch-not-taken section below (writing to a temp reg)
#endif

	if ((u32)psxM == 0x10000000) {
		// psxM base is mmap'd at virtual address 0x10000000
		LUI(TEMP_2, 0x1000);      // <BD slot>
		INS(TEMP_2, r1, 0, 0x15); // TEMP_2 = 0x10000000 | (r1 & 0x1fffff)
	} else {
		EXT(TEMP_1, r1, 0, 0x15); // <BD slot> TEMP_1 = r1 & 0x1fffff
		LW(TEMP_2, PERM_REG_1, off(psxM));
		ADDU(TEMP_2, TEMP_2, TEMP_1);
	}

	int icount = count;
	u32 *backpatch_label_exit_1 = 0;
	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		s32 imm = _fImm_(opcode);
		u32 rt = _fRt_(opcode);
		u32 r2 = regMipsToHost(rt, REG_FIND, REG_REGISTER);

		if (icount == 1) {
			// This is the end of the loop
			backpatch_label_exit_1 = (u32 *)recMem;
			B(0); // b label_exit
			// NOTE: Branch delay slot will contain the instruction below
		}
		// Important: this should be the last opcode in the loop (see note above)
		OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

		SetUndef(rt);
		regMipsChanged(rt);
		regUnlock(r2);

		PC += 4;
	} while (--icount);

	PC = pc - 4;

	regPopState();

	// label_hle:
	fixup_branch(backpatch_label_hle_1);
#ifndef SKIP_SAME_2MB_REGION_CHECK
	fixup_branch(backpatch_label_hle_2);
#endif

#endif //USE_DIRECT_MEM_ACCESS

	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		u32 rt = _fRt_(opcode);
		s32 imm = _fImm_(opcode);
		u32 r2 = regMipsToHost(rt, REG_FIND, REG_REGISTER);

		switch (opcode & 0xfc000000) {
		case 0x80000000: // LB
			JAL(psxMemRead8);
			ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
			SEB(r2, MIPSREG_V0);
			break;
		case 0x90000000: // LBU
			JAL(psxMemRead8);
			ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
			MOV(r2, MIPSREG_V0);
			break;
		case 0x84000000: // LH
			JAL(psxMemRead16);
			ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
			SEH(r2, MIPSREG_V0);
			break;
		case 0x94000000: // LHU
			JAL(psxMemRead16);
			ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
			MOV(r2, MIPSREG_V0);
			break;
		case 0x8c000000: // LW
			JAL(psxMemRead32);
			ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
			MOV(r2, MIPSREG_V0);
			break;
		}

		SetUndef(rt);
		regMipsChanged(rt);
		regUnlock(r2);

		PC += 4;
	} while (--count);

#ifdef USE_DIRECT_MEM_ACCESS
	// label_exit:
	fixup_branch(backpatch_label_exit_1);
#endif

	pc = PC;

	regUnlock(r1);
}

static void StoreToAddr(int count)
{
	int icount;
	u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 PC = pc - 4;

	#ifdef WITH_DISASM
	for (int i = 0; i < count-1; i++)
		DISASM_PSX(pc + i * 4);
	#endif

#ifdef USE_DIRECT_MEM_ACCESS
	regPushState();

	// Check if memory is writable and also check if address is in lower 8MB:
	//
	//   This is just slightly tricky. All addresses in lower 8MB of RAM will
	//  have bits 27:24 unset, and all other valid addresses have them set.
	//  The check below only passes when 'writeok' is 1 and bits 27:24 are 0.
	//   It is not possible for a series of stores sharing a base register to
	//  write both inside and outside of lower-8MB RAM region: signed 16bit imm
	//  offset is not large enough to reach a valid mapped address in both.
	//   Writes to 0xFFFF_xxxx and 0x8000_xxxx regions would cause an exception
	//  on a real PS1, so no need to worry about imm offsets reaching them.
	//  Base addresses with offsets wrapping to next/prior mirror region are
	//  handled either with masking further below, or by emulation of mirrors
	//  using mmap'd virtual mem (SKIP_SAME_2MB_REGION_CHECK, see psxmem.cpp).
	//    MIPSREG_A0 is left set to the eff address of first store in series,
	//  saving emitting an instruction in first loop iterations further below.
	// ---- Equivalent C code: ----
	// if ( !((r1+imm_of_first_store) & 0x0f00_0000) < writeok) )
	//    goto label_hle_1;

	LW(TEMP_1, PERM_REG_1, off(writeok));

	// Get the effective address of first store in the series.
	// ---> NOTE: leave value in MIPSREG_A0, it will be used later!
	ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef HAVE_MIPS32R2_EXT_INS
	EXT(TEMP_2, MIPSREG_A0, 24, 4);
#else
	LUI(TEMP_2, 0x0f00);
	AND(TEMP_2, TEMP_2, MIPSREG_A0);
#endif
	SLTU(TEMP_2, TEMP_2, TEMP_1);
	u32 *backpatch_label_hle_1 = (u32 *)recMem;
	BEQZ(TEMP_2, 0);
	//NOTE: Delay slot of branch is harmlessly occupied by the first op
	// of the branch-not-taken section below (writing to a temp var)

#ifndef SKIP_SAME_2MB_REGION_CHECK
	/* Check if addr and addr+imm are in the same 2MB region */
	EXT(TEMP_2, MIPSREG_A0, 21, 3); // <BD slot> TEMP_2 = (MIPSREG_A0 >> 21) & 0x7
	EXT(TEMP_3, r1, 21, 3);         //           TEMP_3 = (r1 >> 21) & 0x7
	u32 *backpatch_label_hle_2 = (u32 *)recMem;
	BNE(TEMP_2, TEMP_3, 0);         // goto label_hle if not in same 2MB region
	//NOTE: Delay slot of branch is harmlessly occupied by the first op
	// of the branch-not-taken section below (writing to a temp reg)
#endif

	if ((u32)psxM == 0x10000000) {
		// psxM base is mmap'd at virtual address 0x10000000
		LUI(TEMP_2, 0x1000);      // <BD slot>
		INS(TEMP_2, r1, 0, 0x15); // TEMP_2 = 0x10000000 | (r1 & 0x1fffff)
	} else {
		EXT(TEMP_1, r1, 0, 0x15); // <BD slot> TEMP_1 = r1 & 0x1fffff
		LW(TEMP_2, PERM_REG_1, off(psxM));
		ADDU(TEMP_2, TEMP_2, TEMP_1);
	}

	icount = count;
	u32 *backpatch_label_exit_1 = 0;
	LUI(TEMP_3, ADR_HI(recRAM)); // temp_3 = upper code block ptr array addr
	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		s32 imm = _fImm_(opcode);
		u32 rt = _fRt_(opcode);
		u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

		OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

		/* Invalidate recRAM[addr+imm16] pointer */
		if (icount != count) {
			// No need to do this for the first store of the series,
			//  as it was already done for us during initial checks.
			ADDIU(MIPSREG_A0, r1, imm);
		}
		EXT(TEMP_1, MIPSREG_A0, 0, 0x15); // and 0x1fffff
		if ((opcode & 0xfc000000) != 0xac000000) {
			// Not a SW, clear lower 2 bits to ensure addr is aligned:
			INS(TEMP_1, 0, 0, 2);
		}
		ADDU(TEMP_1, TEMP_1, TEMP_3);

		backpatch_label_exit_1 = 0;
		if (icount == 1) {
			// This is the end of the loop
			backpatch_label_exit_1 = (u32 *)recMem;
			B(0); // b label_exit
			// NOTE: Branch delay slot will contain the instruction below
		}
		// Important: this should be the last opcode in the loop (see note above)
		SW(0, TEMP_1, ADR_LO(recRAM));  // set code block ptr to NULL

		PC += 4;

		regUnlock(r2);
	} while (--icount);

	PC = pc - 4;

	regPopState();

	// label_hle:
	fixup_branch(backpatch_label_hle_1);
#ifndef SKIP_SAME_2MB_REGION_CHECK
	fixup_branch(backpatch_label_hle_2);
#endif

#endif // USE_DIRECT_MEM_ACCESS

	icount = count;
	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		u32 rt = _fRt_(opcode);
		s32 imm = _fImm_(opcode);
		u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

#ifdef USE_DIRECT_MEM_ACCESS
		if (icount != count) {
			// No need to do this for the first store of the series,
			//  as it was already done for us during initial checks.
			ADDIU(MIPSREG_A0, r1, imm);
		}
#else
			ADDIU(MIPSREG_A0, r1, imm);
#endif

		switch (opcode & 0xfc000000) {
		case 0xa0000000:
			JAL(psxMemWrite8);
			MOV(MIPSREG_A1, r2); // <BD> Branch delay slot
			break;
		case 0xa4000000:
			JAL(psxMemWrite16);
			MOV(MIPSREG_A1, r2); // <BD> Branch delay slot
			break;
		case 0xac000000:
			JAL(psxMemWrite32);
			MOV(MIPSREG_A1, r2); // <BD> Branch delay slot
			break;
		default: break;
		}

		PC += 4;

		regUnlock(r2);
	} while (--icount);

#ifdef USE_DIRECT_MEM_ACCESS
	// label_exit:
	fixup_branch(backpatch_label_exit_1);
#endif

	pc = PC;

	regUnlock(r1);
}

static int calc_loads()
{
	int count = 0;
	u32 PC = pc;
	u32 opcode = psxRegs.code;
	u32 rs = _Rs_;

	imm_min = imm_max = _fImm_(opcode);

	/* If in delay slot, set count to 1 */
	if (branch)
		return 1;

	/* Allow LB, LBU, LH, LHU and LW */
	/* rs should be the same, imm and rt could be different */
	while ((_fOp_(opcode) == 0x20 || _fOp_(opcode) == 0x24 ||
	        _fOp_(opcode) == 0x21 || _fOp_(opcode) == 0x25 ||
	        _fOp_(opcode) == 0x23) && (rs == _fRs_(opcode))) {

		/* Update min and max immediate values */
		if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
		if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

		opcode = *(u32 *)((char *)PSXM(PC));

		PC += 4;
		count++;

		/* Extra paranoid check if rt == rs */
		if (_fRt_(opcode) == _fRs_(opcode))
			return count;

	}

#ifdef LOG_LOADS
	if (count) {
		printf("\nFOUND %d loads, min: %d, max: %d\n", count, imm_min, imm_max);
		u32 dpc = pc - 4;
		for (; dpc < PC - 4; dpc += 4)
			disasm_psx(dpc);
	}
#endif

	return count;
}

static int calc_stores()
{
	int count = 0;
	u32 PC = pc;
	u32 opcode = psxRegs.code;
	u32 rs = _Rs_;

	imm_min = imm_max = _fImm_(opcode);

	/* If in delay slot, set count to 1 */
	if (branch)
		return 1;

	/* Allow SB, SH and SW */
	/* rs should be the same, imm and rt could be different */
	while (((_fOp_(opcode) == 0x28) || (_fOp_(opcode) == 0x29) ||
	        (_fOp_(opcode) == 0x2b)) && (rs == _fRs_(opcode))) {

		/* Update min and max immediate values */
		if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
		if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

		opcode = *(u32 *)((char *)PSXM(PC));

		PC += 4;
		count++;
	}

#ifdef LOG_STORES
	if (count) {
		printf("\nFOUND %d stores, min: %d, max: %d\n", count, imm_min, imm_max);
		u32 dpc = pc - 4;
		for (; dpc < PC - 4; dpc += 4)
			disasm_psx(dpc);
	}
#endif

	return count;
}

static void recLB()
{
	int count = calc_loads();

	if (autobias) cycles_pending += count;

	// Rt = mem[Rs + Im] (signed)
	if (LoadFromConstAddr(count))
		return;

	LoadFromAddr(count);
}

static void recLBU()
{
	int count = calc_loads();

	if (autobias) cycles_pending += count;

	// Rt = mem[Rs + Im] (unsigned)
	if (LoadFromConstAddr(count))
		return;

	LoadFromAddr(count);
}

static void recLH()
{
	int count = calc_loads();

	if (autobias) cycles_pending += count;

	// Rt = mem[Rs + Im] (signed)
	if (LoadFromConstAddr(count))
		return;

	LoadFromAddr(count);
}

static void recLHU()
{
	int count = calc_loads();

	if (autobias) cycles_pending += count;

	// Rt = mem[Rs + Im] (unsigned)
	if (LoadFromConstAddr(count))
		return;

	LoadFromAddr(count);
}

static void recLW()
{
	int count = calc_loads();

	if (autobias) cycles_pending += count;

	// Rt = mem[Rs + Im] (unsigned)
	if (LoadFromConstAddr(count))
		return;

	LoadFromAddr(count);
}

static void recSB()
{
	int count = calc_stores();

	if (autobias) cycles_pending += count;

	// mem[Rs + Im] = Rt
	if (StoreToConstAddr(count))
		return;

	StoreToAddr(count);
}

static void recSH()
{
	int count = calc_stores();

	if (autobias) cycles_pending += count;

	// mem[Rs + Im] = Rt
	if (StoreToConstAddr(count))
		return;

	StoreToAddr(count);
}

static void recSW()
{
	int count = calc_stores();

	if (autobias) cycles_pending += count;

	// mem[Rs + Im] = Rt
	if (StoreToConstAddr(count))
		return;

	StoreToAddr(count);
}

static u32 LWL_MASKSHIFT[8] = { 0xffffff, 0xffff, 0xff, 0,
                                24, 16, 8, 0 };
static u32 LWR_MASKSHIFT[8] = { 0, 0xff000000, 0xffff0000, 0xffffff00,
                                0, 8, 16, 24 };

static void gen_LWL_LWR(int count)
{
	int icount;
	u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 PC = pc - 4;

	#ifdef WITH_DISASM
	for (int i = 0; i < count-1; i++)
		DISASM_PSX(pc + i * 4);
	#endif

	// Get the effective address of first store in the series.
	// ---> NOTE: leave value in MIPSREG_A0, it will be used later!
	ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef USE_DIRECT_MEM_ACCESS
	regPushState();

	// Is address in lower 8MB region? (2MB mirrored x4)
	//  We check only the effective address of first load in the series,
	// seeing if bits 27:24 are unset to determine if it is in lower 8MB.
	// See comments in StoreToAddr() for explanation of check.

#ifdef HAVE_MIPS32R2_EXT_INS
	EXT(TEMP_2, MIPSREG_A0, 24, 4);
#else
	LUI(TEMP_2, 0x0f00);
	AND(TEMP_2, TEMP_2, MIPSREG_A0);
#endif
	u32 *backpatch_label_hle_1 = (u32 *)recMem;
	BGTZ(TEMP_2, 0);
	//NOTE: Delay slot of branch is harmlessly occupied by the first op
	// of the branch-not-taken section below (writing to temp reg)

	if ((u32)psxM == 0x10000000) {
		// psxM base is mmap'd at virtual address 0x10000000
		LUI(TEMP_2, 0x1000);      // <BD slot>
		INS(TEMP_2, r1, 0, 0x15); // TEMP_2 = 0x10000000 | (r1 & 0x1fffff)
	} else {
		EXT(TEMP_1, r1, 0, 0x15); // <BD slot> TEMP_1 = r1 & 0x1fffff
		LW(TEMP_2, PERM_REG_1, off(psxM));
		ADDU(TEMP_2, TEMP_2, TEMP_1);
	}

	icount = count;
	u32 *backpatch_label_exit_1 = 0;
	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		s32 imm = _fImm_(opcode);
		u32 rt = _fRt_(opcode);
		u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

		if (icount == 1) {
			// This is the end of the loop
			backpatch_label_exit_1 = (u32 *)recMem;
			B(0); // b label_exit
			// NOTE: Branch delay slot will contain the instruction below
		}
		// Important: this should be the last instruction in the loop (see note above)
		OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

		SetUndef(rt);
		regMipsChanged(rt);
		regUnlock(r2);

		PC += 4;
	} while (--icount);

	PC = pc - 4;

	regPopState();

	// label_hle:
	fixup_branch(backpatch_label_hle_1);
#endif // USE_DIRECT_MEM_ACCESS

	icount = count;
	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		u32 insn = opcode & 0xfc000000;
		u32 rt = _fRt_(opcode);
		s32 imm = _fImm_(opcode);
		u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

		if (icount != count) {
			// No need to do this for the first load of the series, as value
			//  is already in $a0 from earlier direct-mem address range check.
			ADDIU(MIPSREG_A0, r1, imm);
		}
		JAL(psxMemRead32);          // result in MIPSREG_V0
		INS(MIPSREG_A0, 0, 0, 2);   // <BD> clear 2 lower bits of $a0 (using branch delay slot)

		ADDIU(MIPSREG_A0, r1, imm);

		if (insn == 0x88000000) // LWL
			LUI(TEMP_2, ADR_HI(LWL_MASKSHIFT));
		else                    // LWR
			LUI(TEMP_2, ADR_HI(LWR_MASKSHIFT));

		// Lower 2 bits of dst addr are index into u32 mask/shift arrays:
#ifdef HAVE_MIPS32R2_EXT_INS
		INS(TEMP_2, MIPSREG_A0, 2, 2);
#else
		ANDI(TEMP_1, MIPSREG_A0, 3);    // temp_1 = addr & 3
		SLL(TEMP_1, TEMP_1, 2);         // temp_1 *= 4
		OR(TEMP_2, TEMP_2, TEMP_1);
#endif

		ADDIU(TEMP_3, TEMP_2, 16);      // array entry of shift amount is
		                                // 16 bytes past mask entry
		if (insn == 0x88000000) { // LWL
			LW(TEMP_2, TEMP_2, ADR_LO(LWL_MASKSHIFT)); // temp_2 = mask
			LW(TEMP_3, TEMP_3, ADR_LO(LWL_MASKSHIFT)); // temp_3 = shift
		} else {                  // LWR
			LW(TEMP_2, TEMP_2, ADR_LO(LWR_MASKSHIFT)); // temp_2 = mask
			LW(TEMP_3, TEMP_3, ADR_LO(LWR_MASKSHIFT)); // temp_3 = shift
		}

		AND(r2, r2, TEMP_2);            // mask pre-existing contents of r2

		if (insn == 0x88000000) // LWL
			SLLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read << shift
		else                    // LWR
			SRLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read >> shift

		OR(r2, r2, TEMP_3);

		SetUndef(rt);
		regMipsChanged(rt);
		regUnlock(r2);

		PC += 4;
	} while (--icount);

#ifdef USE_DIRECT_MEM_ACCESS
	// label_exit:
	fixup_branch(backpatch_label_exit_1);
#endif

	pc = PC;

	regUnlock(r1);
}

static u32 SWL_MASKSHIFT[8] = { 0xffffff00, 0xffff0000, 0xff000000, 0,
                                24, 16, 8, 0 };
static u32 SWR_MASKSHIFT[8] = { 0, 0xff, 0xffff, 0xffffff,
                                0, 8, 16, 24 };

static void gen_SWL_SWR(int count)
{
	int icount;
	u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 PC = pc - 4;

	#ifdef WITH_DISASM
	for (int i = 0; i < count-1; i++)
		DISASM_PSX(pc + i * 4);
	#endif

#ifdef USE_DIRECT_MEM_ACCESS
	regPushState();

	// Check if memory is writable and also check if address is in lower 8MB:
	// See comments in StoreToAddr() for explanation of check.
	// ---- Equivalent C code: ----
	// if ( !((r1+imm_of_first_store) & 0x0f00_0000) < writeok) )
	//    goto label_hle_1;

	LW(TEMP_1, PERM_REG_1, off(writeok));

	// Get the effective address of first store in the series.
	// ---> NOTE: leave value in MIPSREG_A0, it will be used later!
	ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef HAVE_MIPS32R2_EXT_INS
	EXT(TEMP_2, MIPSREG_A0, 24, 4);
#else
	LUI(TEMP_2, 0x0f00);
	AND(TEMP_2, TEMP_2, MIPSREG_A0);
#endif
	SLTU(TEMP_2, TEMP_2, TEMP_1);
	u32 *backpatch_label_hle_1 = (u32 *)recMem;
	BEQZ(TEMP_2, 0);
	//NOTE: Delay slot of branch is harmlessly occupied by the first op
	// of the branch-not-taken section below (writing to a temp reg)

	if ((u32)psxM == 0x10000000) {
		// psxM base is mmap'd at virtual address 0x10000000
		LUI(TEMP_2, 0x1000);      // <BD slot>
		INS(TEMP_2, r1, 0, 0x15); // TEMP_2 = 0x10000000 | (r1 & 0x1fffff)
	} else {
		EXT(TEMP_1, r1, 0, 0x15); // <BD slot> TEMP_1 = r1 & 0x1fffff
		LW(TEMP_2, PERM_REG_1, off(psxM));
		ADDU(TEMP_2, TEMP_2, TEMP_1);
	}

	icount = count;
	u32 *backpatch_label_exit_1 = 0;

	LUI(TEMP_3, ADR_HI(recRAM)); // temp_3 = upper code block ptr array addr
	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		s32 imm = _fImm_(opcode);
		u32 rt = _fRt_(opcode);
		u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

		OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

		/* Invalidate recRAM[addr+imm16] pointer */
		if (icount != count) {
			// No need to do this for the first store of the series,
			//  as it was already done for us during initial checks.
			ADDIU(MIPSREG_A0, r1, imm);
		}
		EXT(TEMP_1, MIPSREG_A0, 0, 0x15); // and 0x1fffff
		INS(TEMP_1, 0, 0, 2); // clear 2 lower bits
		ADDU(TEMP_1, TEMP_1, TEMP_3);

		if (icount == 1) {
			// This is the end of the loop
			backpatch_label_exit_1 = (u32 *)recMem;
			B(0); // b label_exit
			// NOTE: Branch delay slot will contain the instruction below
		}
		// Important: this should be the last instruction in the loop (is BD slot of exit branch)
		SW(0, TEMP_1, ADR_LO(recRAM));  // set code block ptr to NULL

		PC += 4;

		regUnlock(r2);
	} while (--icount);

	PC = pc - 4;

	regPopState();

	// label_hle:
	fixup_branch(backpatch_label_hle_1);
#endif // USE_DIRECT_MEM_ACCESS

	icount = count;
	do {
		u32 opcode = *(u32 *)((char *)PSXM(PC));
		u32 insn = opcode & 0xfc000000;
		u32 rt = _fRt_(opcode);
		s32 imm = _fImm_(opcode);
		u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

#ifdef USE_DIRECT_MEM_ACCESS
		if (icount != count) {
			// No need to do this for the first store of the series, as value
			//  is already in $a0 from earlier direct-mem address range check.
			ADDIU(MIPSREG_A0, r1, imm);
		}
#else
			ADDIU(MIPSREG_A0, r1, imm);
#endif

#ifdef HAVE_MIPS32R2_EXT_INS
		JAL(psxMemRead32);              // result in MIPSREG_V0
		INS(MIPSREG_A0, 0, 0, 2);       // <BD> clear 2 lower bits of $a0
#else
		SRL(MIPSREG_A0, MIPSREG_A0, 2);
		JAL(psxMemRead32);              // result in MIPSREG_V0
		SLL(MIPSREG_A0, MIPSREG_A0, 2); // <BD> clear lower 2 bits of $a0
#endif

		ADDIU(MIPSREG_A0, r1, imm);

		if (insn == 0xa8000000)   // SWL
			LUI(TEMP_2, ADR_HI(SWL_MASKSHIFT));
		else                      // SWR
			LUI(TEMP_2, ADR_HI(SWR_MASKSHIFT));

		// Lower 2 bits of dst addr are index into u32 mask/shift arrays:
#ifdef HAVE_MIPS32R2_EXT_INS
		INS(TEMP_2, MIPSREG_A0, 2, 2);
		INS(MIPSREG_A0, 0, 0, 2);       // clear 2 lower bits of addr
#else
		ANDI(TEMP_1, MIPSREG_A0, 3);    // temp_1 = addr & 3
		SLL(TEMP_1, TEMP_1, 2);         // temp_1 *= 4
		OR(TEMP_2, TEMP_2, TEMP_1);

		SRL(MIPSREG_A0, MIPSREG_A0, 2); // clear 2 lower bits of addr
		SLL(MIPSREG_A0, MIPSREG_A0, 2);
#endif

		ADDIU(TEMP_3, TEMP_2, 16);      // array entry of shift amount is
		                                // 16 bytes past mask entry

		if (insn == 0xa8000000) { // SWL
			LW(TEMP_2, TEMP_2, ADR_LO(SWL_MASKSHIFT)); // temp_2 = mask
			LW(TEMP_3, TEMP_3, ADR_LO(SWL_MASKSHIFT)); // temp_3 = shift
		} else {                  // SWR
			LW(TEMP_2, TEMP_2, ADR_LO(SWR_MASKSHIFT)); // temp_2 = mask
			LW(TEMP_3, TEMP_3, ADR_LO(SWR_MASKSHIFT)); // temp_3 = shift
		}

		AND(MIPSREG_A1, MIPSREG_V0, TEMP_2); // $a1 = mem_val & mask

		if (insn == 0xa8000000) // SWL
			SRLV(TEMP_1, r2, TEMP_3);        // temp_1 = new_data >> shift
		else                    // SWR
			SLLV(TEMP_1, r2, TEMP_3);        // temp_1 = new_data << shift

		JAL(psxMemWrite32);
		OR(MIPSREG_A1, MIPSREG_A1, TEMP_1);  // <BD> $a1 |= temp_1

		PC += 4;

		regUnlock(r2);
	} while (--icount);

#ifdef USE_DIRECT_MEM_ACCESS
	// label_exit:
	fixup_branch(backpatch_label_exit_1);
#endif

	pc = PC;

	regUnlock(r1);
}

/* Calculate number of lwl/lwr or swl/swr opcodes */
static int calc_wl_wr(u32 op1, u32 op2)
{
	int count = 0;
	u32 PC = pc;
	u32 opcode = psxRegs.code;
	u32 rs = _Rs_; // base reg

	imm_min = imm_max = _fImm_(opcode);

	/* If in delay slot, set count to 1 */
	if (branch)
		return 1;

	while ((_fOp_(opcode) == op1 || _fOp_(opcode) == op2) && (_fRs_(opcode) == rs)) {

		/* Update min and max immediate values */
		if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
		if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

		opcode = *(u32 *)((char *)PSXM(PC));
		PC += 4;
		count++;
	}

#ifdef LOG_WL_WR
	if (count) {
		printf("\nFOUND %d opcodes, min: %d, max: %d\n", count, imm_min, imm_max);
		u32 dpc = pc - 4;
		for (; dpc < PC - 4; dpc += 4)
			disasm_psx(dpc);
	}
#endif

	return count;
}

static void recLWL()
{
	int count = calc_wl_wr(0x22, 0x26);

	if (autobias) cycles_pending += count;

#ifdef USE_CONST_ADDRESSES
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].r + imm_min;
		// Is address in lower 8MB region? (2MB mirrored x4)
		if ((addr & 0x1fffffff) < 0x800000) {
			u32 r2 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
			u32 PC = pc - 4;

			#ifdef WITH_DISASM
			for (int i = 0; i < count-1; i++)
				DISASM_PSX(pc + i * 4);
			#endif

			if ((u32)psxM == 0x10000000) {
				// psxM base is mmap'd at virtual address 0x10000000
				LUI(TEMP_2, 0x1000);
				INS(TEMP_2, r2, 0, 0x15); // TEMP_2 = 0x10000000 | (r2 & 0x1fffff)
			} else {
				LW(TEMP_2, PERM_REG_1, off(psxM));
				EXT(TEMP_1, r2, 0, 0x15);
				ADDU(TEMP_2, TEMP_2, TEMP_1);
			}

			do {
				u32 opcode = *(u32 *)((char *)PSXM(PC));
				s32 imm = _fImm_(opcode);
				u32 rt = _fRt_(opcode);
				u32 r1 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

				OPCODE(opcode & 0xfc000000, r1, TEMP_2, imm);

				SetUndef(rt);
				regMipsChanged(rt);
				regUnlock(r1);
				PC += 4;
			} while (--count);

			pc = PC;
			regUnlock(r2);

			return;
		}
	}
#endif // USE_CONST_ADDRESSES

	gen_LWL_LWR(count);
}

static void recLWR()
{
	if (autobias) cycles_pending += 1;

	gen_LWL_LWR(1);
}

static void recSWL()
{
	int count = calc_wl_wr(0x2a, 0x2e);

	if (autobias) cycles_pending += count;

#ifdef USE_CONST_ADDRESSES
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].r + imm_min;
		// Is address in lower 8MB region? (2MB mirrored x4)
		if ((addr & 0x1fffffff) < 0x800000) {
			u32 r2 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
			u32 PC = pc - 4;

			#ifdef WITH_DISASM
			for (int i = 0; i < count-1; i++)
				DISASM_PSX(pc + i * 4);
			#endif

			if ((u32)psxM == 0x10000000) {
				// psxM base is mmap'd at virtual address 0x10000000
				LUI(TEMP_2, 0x1000);
				INS(TEMP_2, r2, 0, 0x15); // TEMP_2 = 0x10000000 | (r2 & 0x1fffff)
			} else {
				LW(TEMP_2, PERM_REG_1, off(psxM));
				EXT(TEMP_1, r2, 0, 0x15);
				ADDU(TEMP_2, TEMP_2, TEMP_1);
			}

			do {
				u32 opcode = *(u32 *)((char *)PSXM(PC));
				s32 imm = _fImm_(opcode);
				u32 rt = _fRt_(opcode);
				u32 r1 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

				OPCODE(opcode & 0xfc000000, r1, TEMP_2, imm);

				regUnlock(r1);
				PC += 4;
			} while (--count);

			pc = PC;
			regUnlock(r2);
			return;
		}
	}
#endif // USE_CONST_ADDRESSES

	gen_SWL_SWR(count);
}

static void recSWR()
{
	if (autobias) cycles_pending += 1;

	gen_SWL_SWR(1);
}
