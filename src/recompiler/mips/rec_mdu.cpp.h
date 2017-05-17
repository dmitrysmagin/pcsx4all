// If your platform gives the same results as PS1 did in the HI register
//   when dividing by zero, define this to eliminate some fixup code, which
//   MIPS32 jz-series CPUs seem to. (See notes in recDIV()/recDIVU())
#if defined(GCW_ZERO)
	#define OMIT_DIV_BY_ZERO_HI_FIXUP
#endif

// See notes in recDIV()/recDIVU (disable for debugging purposes)
#define OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND

static void recMULT() {
// Lo/Hi = Rs * Rt (signed)
	if (!(_Rs_) || !(_Rt_)) {
		// If either operand is $r0, just store 0 in both LO/HI regs
		SW(0, PERM_REG_1, offGPR(32)); // LO
		SW(0, PERM_REG_1, offGPR(33)); // HI
	} else {
		u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
		u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

		MULT(rs, rt);
		MFLO(TEMP_1);
		MFHI(TEMP_2);

		SW(TEMP_1, PERM_REG_1, offGPR(32));
		SW(TEMP_2, PERM_REG_1, offGPR(33));
		regUnlock(rs);
		regUnlock(rt);
	}
}

static void recMULTU() {
// Lo/Hi = Rs * Rt (unsigned)
	if (!(_Rs_) || !(_Rt_)) {
		// If either operand is $r0, just store 0 in both LO/HI regs
		SW(0, PERM_REG_1, offGPR(32)); // LO
		SW(0, PERM_REG_1, offGPR(33)); // HI
	} else {
		u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
		u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

		MULTU(rs, rt);
		MFLO(TEMP_1);
		MFHI(TEMP_2);

		SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
		SW(TEMP_2, PERM_REG_1, offGPR(33)); // HI
		regUnlock(rs);
		regUnlock(rt);
	}
}

static void recDIV()
{
// Hi, Lo = rs / rt signed
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	// Test if divisor is 0, emulating correct results for PS1 CPU.
	// NOTE: we don't bother checking for signed division overflow (the
	//       last entry in this list), as it seems to work the same on
	//       modern MIPS32 CPUs like the jz4770 of GCW Zero. Unfortunately,
	//       division-by-zero results only match PS1 in the 'hi' reg.
	//  Rs              Rt       Hi/Remainder  Lo/Result
	//  0..+7FFFFFFFh    0   -->  Rs           -1
	//  -80000000h..-1   0   -->  Rs           +1
	//  -80000000h      -1   -->  0           -80000000h

	bool omit_div_by_zero_fixup = false;

	if (IsConst(_Rt_) && iRegs[_Rt_].r != 0) {
		// If divisor is known-const val and isn't 0, no need to fixup
		omit_div_by_zero_fixup = true;
	} else if (!branch) {
#ifdef OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND
		// If we're not inside a branch-delay slot, we scan ahead for a
		//  PS1 GCC auto-generated code sequence that was inserted in most
		//  binaries after a DIV/DIVU instruction. The sequence checked for
		//  div-by-zero (and signed overflow, for DIV) and issued an
		//  exception that went unhandled by PS1 BIOS, which would crash.
		//  If found, don't bother to emulate PS1 div-by-zero result.
		//
		// Sequence is sometimes preceeded by one or two MFHI/MFLO opcodes.
		u32 code_loc = pc;
		code_loc += 4 * rec_scan_for_MFHI_MFLO_sequence(code_loc);
		omit_div_by_zero_fixup = (rec_scan_for_div_by_zero_check_sequence(code_loc) > 0);
#endif
	}

	if (omit_div_by_zero_fixup) {
		DIV(rs, rt);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);
	} else {
		DIV(rs, rt);
		ADDIU(MIPSREG_A1, 0, -1);
		SLT(TEMP_3, rs, 0);        // TEMP_3 = (rs < 0 ? 1 : 0)
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);

		// If divisor was 0, set LO result (quotient) to 1 if dividend was < 0
		// If divisor was 0, set LO result (quotient) to -1 if dividend was >= 0
		MOVN(MIPSREG_A0, TEMP_3, TEMP_3);      // if (TEMP_3 != 0) then MIPSREG_A1 = TEMP_3
		MOVZ(MIPSREG_A0, MIPSREG_A1, TEMP_3);  // if (TEMP_3 == 0) then MIPSREG_A1 = MIPSREG_A0
		MOVZ(TEMP_1, MIPSREG_A0, rt);          // if (rt == 0) then TEMP_1 = MIPSREG_A0

#ifndef OMIT_DIV_BY_ZERO_HI_FIXUP
		// If divisor was 0, set HI result (remainder) to rs
		MOVZ(TEMP_2, rs, rt);
#endif
	}

	SW(TEMP_1, PERM_REG_1, offGPR(32));
	SW(TEMP_2, PERM_REG_1, offGPR(33));
	regUnlock(rs);
	regUnlock(rt);
}

static void recDIVU()
{
// Hi, Lo = rs / rt unsigned
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	// Test if divisor is 0, emulating correct results for PS1 CPU.
	//  Rs              Rt       Hi/Remainder  Lo/Result
	//  0..FFFFFFFFh    0   -->  Rs            FFFFFFFFh

	bool omit_div_by_zero_fixup = false;

	if (IsConst(_Rt_) && iRegs[_Rt_].r != 0) {
		// If divisor is known-const val and isn't 0, no need to fixup
		omit_div_by_zero_fixup = true;
	} else if (!branch) {
#ifdef OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND
		// See notes in recDIV() emitter
		u32 code_loc = pc;
		code_loc += 4 * rec_scan_for_MFHI_MFLO_sequence(code_loc);
		omit_div_by_zero_fixup = (rec_scan_for_div_by_zero_check_sequence(code_loc) > 0);
#endif
	}

	if (omit_div_by_zero_fixup) {
		DIVU(rs, rt);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);
	} else {
		DIVU(rs, rt);
		ADDIU(TEMP_3, 0, -1);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);

		// If divisor was 0, set LO result (quotient) to 0xffff_ffff
		MOVZ(TEMP_1, TEMP_3, rt);  // if (rt == 0) then TEMP_1 = TEMP_3

#ifndef OMIT_DIV_BY_ZERO_HI_FIXUP
		// If divisor was 0, set HI result (remainder) to rs
		MOVZ(TEMP_2, rs, rt);
#endif
	}

	SW(TEMP_1, PERM_REG_1, offGPR(32));
	SW(TEMP_2, PERM_REG_1, offGPR(33));
	regUnlock(rs);
	regUnlock(rt);
}

static void recMFHI() {
// Rd = Hi
	if (!_Rd_) return;
	SetUndef(_Rd_);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(33));
	regMipsChanged(_Rd_);
	regUnlock(rd);
}

static void recMTHI() {
// Hi = Rs
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(33));
	regUnlock(rs);
}

static void recMFLO() {
// Rd = Lo
	if (!_Rd_) return;
	SetUndef(_Rd_);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(32));
	regMipsChanged(_Rd_);
	regUnlock(rd);
}

static void recMTLO() {
// Lo = Rs
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(32));
	regUnlock(rs);
}
