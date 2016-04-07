static void recMFC0()
{
// Rt = Cop0->Rd
	if (!_Rt_) return;
	u32 rt = regMipsToArm(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP0(_Rd_));
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
	SW(rt, PERM_REG_1, offCP0(_Rd_));
	regBranchUnlock(rt);
}

static void recCTC0()
{
// Cop0->Rd = Rt

	recMTC0();
}

static void recRFE()
{
	write32(0x3c00ffff | (TEMP_2 << 16)); /* lui 0xffff */
	write32(0x3400fff0 | (TEMP_2 << 21) | (TEMP_2 << 16)); /* ori 0xfff0 */

	LW(TEMP_1, PERM_REG_1, offCP0(12));
	AND(TEMP_2, TEMP_1, TEMP_2);
	write32(0x30000000 | (TEMP_1 << 21) | (TEMP_1 << 16) | 0x3c); /* andi t1, t1, 0x3c */
	write32(0x00000002 | (TEMP_1 << 16) | (TEMP_1 << 11) | (2 << 6)); /* srl t1, t1, 2 */
	write32(0x00000025 | (TEMP_2 << 21) | (TEMP_1 << 16) | (TEMP_1 << 11)); /* or t1, t2, t1 */

	SW(TEMP_1, PERM_REG_1, offCP0(12));
}
