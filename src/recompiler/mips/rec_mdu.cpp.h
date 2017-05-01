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
	//  0..+7FFFFFFFh   0   -->  Rs            -1
	//  -80000000h..-1  0   -->  Rs            +1
	//  -80000000h      -1  -->  0             -80000000h

	if (IsConst(_Rt_) && iRegs[_Rt_].r != 0) {
		DIV(rs, rt);
		MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */
		MFHI(TEMP_2);
	} else {
		u32 *backpatch1, *backpatch2;
		backpatch1 = (u32*)recMem;
		BEQZ(rt, 0);
		ADDIU(TEMP_2, 0, -1);  // $t1 = -1  <BD slot>

		DIV(rs, rt);
		MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */

		backpatch2 = (u32*)recMem;
		B(0);
		MFHI(TEMP_2);  // <BD slot>

		// division by zero
		fixup_branch(backpatch1);

		SLT(TEMP_1, rs, 0); // $t0 = (rs < 0 ? 1 : 0)
		MOVZ(TEMP_1, TEMP_2, TEMP_1); // if $t0 == 0 then $t0 = $t1
		MOV(TEMP_2, rs);

		// exit
		fixup_branch(backpatch2);
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
	// NOTE: we don't bother checking for signed division overflow (the
	//       last entry in this list), as it seems to work the same on
	//       modern MIPS32 CPUs like the jz4770 of GCW Zero. Unfortunately,
	//       division-by-zero results only match PS1 in the 'hi' reg.
	//  Rs              Rt       Hi/Remainder  Lo/Result
	//  0..+7FFFFFFFh   0   -->  Rs            -1
	//  -80000000h..-1  0   -->  Rs            +1
	//  -80000000h      -1  -->  0             -80000000h

	if (IsConst(_Rt_) && iRegs[_Rt_].r != 0) {
		DIVU(rs, rt);
		MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */
		MFHI(TEMP_2);
	} else {
		u32 *backpatch1, *backpatch2;
		backpatch1 = (u32*)recMem;
		BEQZ(rt, 0);
		ADDIU(TEMP_1, 0, -1);  // $t0 = -1  <BD slot>

		DIVU(rs, rt);
		MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */

		backpatch2 = (u32*)recMem;
		B(0);
		MFHI(TEMP_2);  // <BD slot>

		// division by zero
		fixup_branch(backpatch1);

		MOV(TEMP_2, rs);

		// exit
		fixup_branch(backpatch2);
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
