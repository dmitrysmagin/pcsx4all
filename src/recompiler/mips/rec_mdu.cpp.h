static void recMULT() {
// Lo/Hi = Rs * Rt (signed)
	if (autobias) cycles_pending += 11;

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
	if (autobias) cycles_pending += 11;

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
	if (autobias) cycles_pending += 34;
	if (!_Rt_) return;
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	DIV(rs, rt);
	MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */
	SW(TEMP_1, PERM_REG_1, offGPR(32));
	MFHI(TEMP_1);
	SW(TEMP_1, PERM_REG_1, offGPR(33));
	regUnlock(rs);
	regUnlock(rt);
}

static void recDIVU()
{
// Hi, Lo = rs / rt unsigned
	if (autobias) cycles_pending += 34;
	if (!_Rt_) return;
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	DIVU(rs, rt);
	MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */
	SW(TEMP_1, PERM_REG_1, offGPR(32));
	MFHI(TEMP_1);
	SW(TEMP_1, PERM_REG_1, offGPR(33));
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
