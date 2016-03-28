#if 1
static void recMFC0()
{
// Rt = Cop0->Rd
	if (!_Rt_) return;
	u32 rt = regMipsToArm(_Rt_, REG_FIND, REG_REGISTER);

	MIPS_LDR_IMM(MIPS_POINTER, rt, PERM_REG_1, CalcDispCP0(_Rd_));
	regMipsChanged(_Rt_);
	regBranchUnlock(rt);
}

static void recCFC0()
{
// Rt = Cop0->Rd

	recMFC0();
}

static void recMTC0()
{
// Cop0->Rd = Rt

	u32 rt = regMipsToArm(_Rt_, REG_LOAD, REG_REGISTER);
	MIPS_STR_IMM(MIPS_POINTER, rt, PERM_REG_1, CalcDispCP0(_Rd_));
	regBranchUnlock(rt);
}

static void recCTC0()
{
// Cop0->Rd = Rt

	recMTC0();
}

static void recRFE()
{
	MIPS_EMIT(MIPS_POINTER, 0x3c00ffff | (TEMP_2 << 16)); /* lui 0xffff */
	MIPS_EMIT(MIPS_POINTER, 0x3400fff0 | (TEMP_2 << 21) | (TEMP_2 << 16)); /* ori 0xfff0 */
	
	MIPS_LDR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, 184);
	/* DEBUG LSR or ASR ? */
	MIPS_AND_REG_REG(MIPS_POINTER, TEMP_2, TEMP_1, TEMP_2);
	MIPS_EMIT(MIPS_POINTER, 0x30000000 | (TEMP_1 << 21) | (TEMP_1 << 16) | 0x3c); /* andi t1, t1, 0x3c */
	MIPS_EMIT(MIPS_POINTER, 0x00000002 | (TEMP_1 << 16) | (TEMP_1 << 11) | (2 << 6)); /* srl t1, t1, 2 */
	MIPS_EMIT(MIPS_POINTER, 0x00000025 | (TEMP_2 << 21) | (TEMP_1 << 16) | (TEMP_1 << 11)); /* or t1, t2, t1 */
	
	MIPS_STR_IMM(MIPS_POINTER, TEMP_1, PERM_REG_1, 184);
}
#else


REC_FUNC_TEST(RFE);
REC_FUNC_TEST(MTC0);
REC_FUNC_TEST(CTC0);
REC_FUNC_TEST(MFC0);
REC_FUNC_TEST(CFC0);

#endif
