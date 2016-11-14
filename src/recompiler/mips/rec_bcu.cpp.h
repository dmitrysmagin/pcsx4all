/* This is from old interpreter_pcsx.cpp */
/* Maybe use _psxTestLoadDelay() from interpreter_new.cpp ?? */

#if defined (interpreter_new) || defined (interpreter_none)

// this defines shall be used with the tmp
// of the next func (instead of _Funct_...)
#define _tFunct_  ((tmp      ) & 0x3F)  // The funct part of the instruction register
#define _tRd_     ((tmp >> 11) & 0x1F)  // The rd part of the instruction register
#define _tRt_     ((tmp >> 16) & 0x1F)  // The rt part of the instruction register
#define _tRs_     ((tmp >> 21) & 0x1F)  // The rs part of the instruction register
#define _tSa_     ((tmp >>  6) & 0x1F)  // The sa part of the instruction register

int psxTestLoadDelay(int reg, u32 tmp) {
	if (tmp == 0) return 0; // NOP
	switch (tmp >> 26) {
		case 0x00: // SPECIAL
			switch (_tFunct_) {
				case 0x00: // SLL
				case 0x02: case 0x03: // SRL/SRA
					if (_tRd_ == reg && _tRt_ == reg) return 1; else
					if (_tRt_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				case 0x08: // JR
					if (_tRs_ == reg) return 2;
					break;
				case 0x09: // JALR
					if (_tRd_ == reg && _tRs_ == reg) return 1; else
					if (_tRs_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				// SYSCALL/BREAK just a break;

				case 0x20: case 0x21: case 0x22: case 0x23:
				case 0x24: case 0x25: case 0x26: case 0x27:
				case 0x2a: case 0x2b: // ADD/ADDU...
				case 0x04: case 0x06: case 0x07: // SLLV...
					if (_tRd_ == reg && (_tRt_ == reg || _tRs_ == reg)) return 1; else
					if (_tRt_ == reg || _tRs_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				case 0x10: case 0x12: // MFHI/MFLO
					if (_tRd_ == reg) return 3;
					break;
				case 0x11: case 0x13: // MTHI/MTLO
					if (_tRs_ == reg) return 2;
					break;

				case 0x18: case 0x19:
				case 0x1a: case 0x1b: // MULT/DIV...
					if (_tRt_ == reg || _tRs_ == reg) return 2;
					break;
			}
			break;

		case 0x01: // REGIMM
			switch (_tRt_) {
				case 0x00: case 0x02:
				case 0x10: case 0x12: // BLTZ/BGEZ...
					if (_tRs_ == reg) return 2;
					break;
			}
			break;

		// J would be just a break;
		case 0x03: // JAL
			if (31 == reg) return 3;
			break;

		case 0x04: case 0x05: // BEQ/BNE
			if (_tRs_ == reg || _tRt_ == reg) return 2;
			break;

		case 0x06: case 0x07: // BLEZ/BGTZ
			if (_tRs_ == reg) return 2;
			break;

		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: // ADDI/ADDIU...
			if (_tRt_ == reg && _tRs_ == reg) return 1; else
			if (_tRs_ == reg) return 2; else
			if (_tRt_ == reg) return 3;
			break;

		case 0x0f: // LUI
			if (_tRt_ == reg) return 3;
			break;

		case 0x10: // COP0
			switch (_tFunct_) {
				case 0x00: // MFC0
					if (_tRt_ == reg) return 3;
					break;
				case 0x02: // CFC0
					if (_tRt_ == reg) return 3;
					break;
				case 0x04: // MTC0
					if (_tRt_ == reg) return 2;
					break;
				case 0x06: // CTC0
					if (_tRt_ == reg) return 2;
					break;
				// RFE just a break;
			}
			break;

		case 0x12: // COP2
			switch (_tFunct_) {
				case 0x00:
					switch (_tRs_) {
						case 0x00: // MFC2
							if (_tRt_ == reg) return 3;
							break;
						case 0x02: // CFC2
							if (_tRt_ == reg) return 3;
							break;
						case 0x04: // MTC2
							if (_tRt_ == reg) return 2;
							break;
						case 0x06: // CTC2
							if (_tRt_ == reg) return 2;
							break;
					}
					break;
				// RTPS... break;
			}
			break;

		case 0x22: case 0x26: // LWL/LWR
			if (_tRt_ == reg) return 3; else
			if (_tRs_ == reg) return 2;
			break;

		case 0x20: case 0x21: case 0x23:
		case 0x24: case 0x25: // LB/LH/LW/LBU/LHU
			if (_tRt_ == reg && _tRs_ == reg) return 1; else
			if (_tRs_ == reg) return 2; else
			if (_tRt_ == reg) return 3;
			break;

		case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2e: // SB/SH/SWL/SW/SWR
			if (_tRt_ == reg || _tRs_ == reg) return 2;
			break;

		case 0x32: case 0x3a: // LWC2/SWC2
			if (_tRs_ == reg) return 2;
			break;
	}

	return 0;
}
#endif

static void recSYSCALL()
{
	regClearJump();

	LI32(TEMP_1, pc - 4);
	SW(TEMP_1, PERM_REG_1, off(pc));

	LI16(MIPSREG_A1, (branch == 1 ? 1 : 0));
	JAL(psxException);
	LI16(MIPSREG_A0, 0x20); // <BD> Load first param using BD slot of JAL()

	rec_recompile_end_part1();
	LW(MIPSREG_V0, PERM_REG_1, off(pc)); // Block retval $v0 = new PC set by psxException()
	rec_recompile_end_part2();

	cycles_pending = 0;

	end_block = 1;
}

/* Set a pending branch */
static void SetBranch()
{
	branch = 1;
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX(pc);
	pc+=4;

	recBSC[psxRegs.code>>26]();
	branch = 0;
}

static void iJumpNormal(u32 branchPC)
{
	branch = 1;
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX(pc);
	pc+=4;

	recBSC[psxRegs.code>>26]();

	branch = 0;

	rec_recompile_end_part1();
	regClearJump();
	LI32(MIPSREG_V0, branchPC); // Block retval $v0 = new PC val
	rec_recompile_end_part2();

	cycles_pending = 0;

	end_block = 1;
}

static void iJumpAL(u32 branchPC, u32 linkpc)
{
	branch = 1;
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX(pc);
	pc+=4;

	recBSC[psxRegs.code>>26]();

	branch = 0;

	rec_recompile_end_part1();
	regClearJump();
	if ((branchPC >> 16) == (linkpc >> 16)) {
		// Both PCs share an upper half, can save an instruction:
		LUI(MIPSREG_V0, (branchPC >> 16));
		ORI(TEMP_1, MIPSREG_V0, (linkpc & 0xffff));
		ORI(MIPSREG_V0, MIPSREG_V0, (branchPC & 0xffff)); // Block retval $v0 = new PC val
	} else {
		LI32(MIPSREG_V0, branchPC); // Block retval $v0 = new PC val
		LI32(TEMP_1, linkpc);
	}

	SW(TEMP_1, PERM_REG_1, offGPR(31));
	rec_recompile_end_part2();

	cycles_pending = 0;

	end_block = 1;
}

static void recBLTZ()
{
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		SetBranch();
		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	BGEZ(br1, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
}

static void recBGTZ()
{
// Branch if Rs > 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		SetBranch();
		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	BLEZ(br1, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
}

static void recBLTZAL()
{
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		SetBranch();
		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	BGEZ(br1, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	LI32(TEMP_2, nbpc);
	SW(TEMP_2, PERM_REG_1, offGPR(31));
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
}

static void recBGEZAL()
{
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		iJumpAL(bpc, (pc + 4));
		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	BLTZ(br1, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	LI32(TEMP_2, nbpc);
	SW(TEMP_2, PERM_REG_1, offGPR(31));
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
}

static void recJ()
{
// j target

	iJumpNormal(_Target_ * 4 + (pc & 0xf0000000));
}

static void recJAL()
{
// jal target

	iJumpAL(_Target_ * 4 + (pc & 0xf0000000), (pc + 4));
}

static void recJR()
{
// jr Rs
	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();

	rec_recompile_end_part1();
	regClearJump();
	MOV(MIPSREG_V0, br1); // Block retval $v0 = new PC val
	regUnlock(br1);
	rec_recompile_end_part2();

	cycles_pending = 0;

	end_block = 1;
}

static void recJALR()
{
// jalr Rs
	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);
	LI32(rd, pc + 4);
	regMipsChanged(_Rd_);
	SetBranch();

	rec_recompile_end_part1();
	regClearJump();
	MOV(MIPSREG_V0, br1); // Block retval $v0 = new PC val
	regUnlock(br1);
	rec_recompile_end_part2();

	cycles_pending = 0;

	end_block = 1;
}

static void recBEQ()
{
// Branch if Rs == Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (_Rs_ == _Rt_) {
		iJumpNormal(bpc);
		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 br2 = regMipsToHost(_Rt_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	BNE(br1, br2, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
	regUnlock(br2);
}

static void recBNE()
{
// Branch if Rs != Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_) && !(_Rt_)) {
		SetBranch(); return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 br2 = regMipsToHost(_Rt_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32* backpatch = (u32*)recMem;
	BEQ(br1, br2, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
	regUnlock(br2);
}

static void recBLEZ()
{
// Branch if Rs <= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		iJumpNormal(bpc);
		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	BGTZ(br1, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
}

static void recBGEZ()
{
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		iJumpNormal(bpc);
		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	SetBranch();
	u32 *backpatch = (u32*)recMem;
	BLTZ(br1, 0);
	// Use BD slot of branch above to load upper part of branch-taken PC val
	LUI(TEMP_1, (bpc >> 16)); // <BD>

	rec_recompile_end_part1();
	regClearBranch();
	ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff)); // Block retval $v0 = branch-taken PC val
	rec_recompile_end_part2();

	cycles_pending = 0;

	fixup_branch(backpatch);
	regUnlock(br1);
}

static void recBREAK() { }

/* senquack - This is the one function I was unsure about during improvements
 *  to block dispatch in August 2016. I haven't found a single game yet that
 *  seems to use this opcode. I believe it is correct with new changes, though.
 */
static void recHLE()
{
	regClearJump();

	LI32(TEMP_1, pc);
	JAL(((u32)psxHLEt[psxRegs.code & 0x7]));
	SW(TEMP_1, PERM_REG_1, off(pc));        // <BD> BD slot of JAL() above

	rec_recompile_end_part1();
	LW(MIPSREG_V0, PERM_REG_1, off(pc)); // <BD> Block retval $v0 = psxRegs.pc
	rec_recompile_end_part2();

	cycles_pending = 0;

	end_block = 1;
}
