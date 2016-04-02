static void recMULT() {
// Lo/Hi = Rs * Rt (signed)

//	iFlushRegs();
	if(!(_Rt_) || !(_Rs_))
	{
		u32 rsrt;
		if( !(_Rt_) )
		{
			rsrt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
		}
		else
		{
			rsrt = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
		}
		SW(rsrt, PERM_REG_1, offGPR(32));
		SW(rsrt, PERM_REG_1, offGPR(33));
		regBranchUnlock(rsrt);
	}
	else
	{
		u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
		u32	rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
		
		write32(0x00000018 | (rs << 21) | (rt << 16)); /* mult rs, rt */
		write32(0x00000012 | (TEMP_1 << 11)); /* mflo temp1 */
		write32(0x00000010 | (TEMP_2 << 11)); /* mfhi temp2 */
		
		SW(TEMP_1, PERM_REG_1, offGPR(32));
		SW(TEMP_2, PERM_REG_1, offGPR(33));
		regBranchUnlock(rs);
		regBranchUnlock(rt);
	}
}

static void recMULTU() {
// Lo/Hi = Rs * Rt (unsigned)

//	iFlushRegs();

	if(!(_Rt_) || !(_Rs_))
	{
		u32 rsrt;
		if( !(_Rt_) )
		{
			rsrt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
		}
		else
		{
			rsrt = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
		}
		SW(rsrt, PERM_REG_1, offGPR(32)); // LO
		SW(rsrt, PERM_REG_1, offGPR(33)); // HI
		regBranchUnlock(rsrt);
	}
	else
	{
		u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
		u32 rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
		
		write32(0x00000019 | (rs << 21) | (rt << 16)); /* multu rs, rt */
		write32(0x00000012 | (TEMP_1 << 11)); /* mflo temp1 */
		write32(0x00000010 | (TEMP_2 << 11)); /* mfhi temp2 */
		
		SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
		SW(TEMP_2, PERM_REG_1, offGPR(33)); // HI
		regBranchUnlock(rs);
		regBranchUnlock(rt);
	}
}

/* From interpreter_pcsx.cpp */
#if defined (interpreter_new) || defined (interpreter_none)
void psxDIV() {
	if (_i32(_rRt_) != 0) {
		(_rLo_) = _i32(_rRs_) / _i32(_rRt_);
		(_rHi_) = _i32(_rRs_) % _i32(_rRt_);
	}
}

void psxDIVU() {
	if (_rRt_ != 0) {
		_rLo_ = _rRs_ / _rRt_;
		_rHi_ = _rRs_ % _rRt_;
	}
}
#endif

REC_FUNC_TEST(DIV);	/* FIXME: implement natively */
REC_FUNC_TEST(DIVU);	/* dto */

static void recMFHI() {
// Rd = Hi
	if (!_Rd_) return;
	iRegs[_Rd_] = -1;
	u32 rd = regMipsToArm(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(33));
	regMipsChanged(_Rd_);
	regBranchUnlock(rd);
}

static void recMTHI() {
// Hi = Rs
	u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(33));
	regBranchUnlock(rs);
}

static void recMFLO() {
// Rd = Lo
	if (!_Rd_) return;
	iRegs[_Rd_] = -1;
	u32 rd = regMipsToArm(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(32));
	regMipsChanged(_Rd_);
	regBranchUnlock(rd);
}

static void recMTLO() {
// Lo = Rs
	u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(32));
	regBranchUnlock(rs);
}
