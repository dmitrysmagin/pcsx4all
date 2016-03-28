#if 1
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
		MIPS_STR_IMM(MIPS_POINTER, rsrt, PERM_REG_1, 128);
		MIPS_STR_IMM(MIPS_POINTER, rsrt, PERM_REG_1, 132);		
		regBranchUnlock(rsrt);
	}
	else
	{
		u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
		u32	rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
		
		MIPS_EMIT(MIPS_POINTER, 0x00000018 | (rs << 21) | (rt << 16)); /* mult rs, rt */
		MIPS_EMIT(MIPS_POINTER, 0x00000012 | (TEMP_1 << 11)); /* mflo temp1 */
		MIPS_EMIT(MIPS_POINTER, 0x00000010 | (TEMP_2 << 11)); /* mfhi temp2 */
		
		MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, 128);
		MIPS_STR_IMM(MIPS_POINTER, TEMP_2, PERM_REG_1, 132);
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
		MIPS_STR_IMM(MIPS_POINTER, rsrt, PERM_REG_1, 128);
		MIPS_STR_IMM(MIPS_POINTER, rsrt, PERM_REG_1, 132);		
		regBranchUnlock(rsrt);
	}
	else
	{
		u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
		u32 rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
		
		MIPS_EMIT(MIPS_POINTER, 0x00000019 | (rs << 21) | (rt << 16)); /* multu rs, rt */
		MIPS_EMIT(MIPS_POINTER, 0x00000012 | (TEMP_1 << 11)); /* mflo temp1 */
		MIPS_EMIT(MIPS_POINTER, 0x00000010 | (TEMP_2 << 11)); /* mfhi temp2 */
		
		MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, 128);
		MIPS_STR_IMM(MIPS_POINTER, TEMP_2, PERM_REG_1, 132);
		regBranchUnlock(rs);
		regBranchUnlock(rt);
	}
}

REC_FUNC_TEST(DIV);	/* FIXME: implement natively */
REC_FUNC_TEST(DIVU);	/* dto */

static void recMFHI() {
// Rd = Hi
	if (!_Rd_) return;
	psxRegs->iRegs[_Rd_] = -1;
	u32 rd = regMipsToArm(_Rd_, REG_FIND, REG_REGISTER);

	MIPS_LDR_IMM(MIPS_POINTER, rd, PERM_REG_1, 132);
	regMipsChanged(_Rd_);
	regBranchUnlock(rd);
}

static void recMTHI() {
// Hi = Rs
	u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
	MIPS_STR_IMM(MIPS_POINTER, rs, PERM_REG_1, 132);
	regBranchUnlock(rs);
}

static void recMFLO() {
// Rd = Lo
	if (!_Rd_) return;
	psxRegs->iRegs[_Rd_] = -1;
	u32 rd = regMipsToArm(_Rd_, REG_FIND, REG_REGISTER);

	MIPS_LDR_IMM(MIPS_POINTER, rd, PERM_REG_1, 128);
	regMipsChanged(_Rd_);
	regBranchUnlock(rd);
}

static void recMTLO() {
// Lo = Rs
	u32 rs = regMipsToArm(_Rs_, REG_LOAD, REG_REGISTER);
	MIPS_STR_IMM(MIPS_POINTER, rs, PERM_REG_1, 128);
	regBranchUnlock(rs);
}
#else


REC_FUNC_TEST(MULT);
REC_FUNC_TEST(MULTU);

REC_FUNC_TEST(MTLO);
REC_FUNC_TEST(MTHI);
REC_FUNC_TEST(MFLO);
REC_FUNC_TEST(MFHI);

REC_FUNC_TEST(DIV);
REC_FUNC_TEST(DIVU);

#endif
