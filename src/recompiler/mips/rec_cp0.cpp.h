static void recMFC0()
{
// Rt = Cop0->Rd
	if (!_Rt_) return;
	SetUndef(_Rt_);
	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP0(_Rd_));
	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void recCFC0()
{
// Rt = Cop0->Rd

	recMFC0();
}

static void recMTC0()
{
// Cop0->Rd = Rt

	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	SW(rt, PERM_REG_1, offCP0(_Rd_));
	regUnlock(rt);
}

static void recCTC0()
{
// Cop0->Rd = Rt

	recMTC0();
}

static void recRFE()
{
// 'Return from exception' opcode
//  Inside CP0 Status register (12), RFE atomically copies bits 5:2 to
//  bits 3:0 , unwinding the exception 'stack'

	LW(TEMP_1, PERM_REG_1, offCP0(12));

	// Reset psxRegs.io_cycle_counter, so that psxBranchTest() is called as
	//  soon as possible to handle any pending interrupts/events
	SW(0, PERM_REG_1, off(io_cycle_counter));

#ifdef HAVE_MIPS32R2_EXT_INS
	EXT(TEMP_2, TEMP_1, 2, 4);   // Copy bits 5:2 of TEMP_1 to 3:0 of TEMP_2
	INS(TEMP_1, TEMP_2, 0, 4);   // Copy those bits back to 3:0 of TEMP_1
#else
	SRL(TEMP_2, TEMP_1, 4);
	SLL(TEMP_2, TEMP_2, 4);      // TEMP_2 = orig SR value with bits 3:0 cleared

	ANDI(TEMP_1, TEMP_1, 0x3c);  // TEMP_1 = just bits 5:2 from orig SR value
	SRL(TEMP_1, TEMP_1, 2);      // Shift them right two places
	OR(TEMP_1, TEMP_2, TEMP_1);  // TEMP_1 = new SR value
#endif

	SW(TEMP_1, PERM_REG_1, offCP0(12));
}
