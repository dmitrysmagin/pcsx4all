#ifdef gte_old

#define CP2_FUNC(f, cycle) \
void gte##f(u32 code, u32 pc); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	LI32(MIPSREG_A0, psxRegs.code); \
	LUI(MIPSREG_A1, (pc >> 16)); \
	JAL(gte##f); \
	ORI(MIPSREG_A1, MIPSREG_A1, (pc & 0xffff)); /* <BD> Branch delay slot */ \
}

#define CP2_FUNC2(f, cycle) \
void gte##f(u32 code); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	LUI(MIPSREG_A0, (psxRegs.code >> 16)); \
	JAL(gte##f); \
	ORI(MIPSREG_A0, MIPSREG_A0, (psxRegs.code & 0xffff)); /* <BD> Branch delay slot */ \
}

#define CP2_FUNC3(f, cycle) \
void gte##f(); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	JAL(gte##f); \
	NOP(); /* <BD> Branch delay slot */ \
}

//CP2_FUNC2(MFC2, 2);
//CP2_FUNC2(MTC2, 2);
//CP2_FUNC(LWC2, 3);
//CP2_FUNC(SWC2, 4);
CP2_FUNC3(DCPL, 8);
CP2_FUNC3(RTPS, 15);
CP2_FUNC2(OP, 6);
CP2_FUNC3(NCLIP, 8);
CP2_FUNC3(DPCS, 8);
CP2_FUNC3(INTPL, 8);
CP2_FUNC2(MVMVA, 8);
CP2_FUNC3(NCDS, 19);
CP2_FUNC3(NCDT, 44);
CP2_FUNC3(CDP, 13);
CP2_FUNC3(NCCS, 17);
CP2_FUNC3(CC, 11);
CP2_FUNC3(NCS, 14);
CP2_FUNC3(NCT, 30);
CP2_FUNC2(SQR, 3);
CP2_FUNC3(DPCT, 17);
CP2_FUNC3(AVSZ3, 5);
CP2_FUNC3(AVSZ4, 6);
CP2_FUNC3(RTPT, 23);
CP2_FUNC2(GPF, 5);
CP2_FUNC2(GPL, 5);
CP2_FUNC3(NCCT, 39);

static void recCFC2()
{
	if (autobias) cycles_pending += 2;
	if (!_Rt_) return;

	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP2C(_Rd_));
	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void recCTC2()
{
	if (autobias) cycles_pending += 2;
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	SW(rt, PERM_REG_1, offCP2C(_Rd_));
	regUnlock(rt);
}

/* move from cp2 reg to host rt */
static void emitMFC2(u32 rt, u32 reg)
{
	if (reg == 29) {
		LW(rt, PERM_REG_1, off(CP2D.r[9])); // gteIR1
		EXT(rt, rt, 7, 5);
		LW(TEMP_1, PERM_REG_1, off(CP2D.r[10])); // gteIR2
		EXT(TEMP_1, TEMP_1, 7, 5);
		INS(rt, TEMP_1, 5, 5);
		LW(TEMP_1, PERM_REG_1, off(CP2D.r[11])); // gteIR3
		EXT(TEMP_1, TEMP_1, 7, 5);
		INS(rt, TEMP_1, 10, 5);
		SW(rt, PERM_REG_1, off(CP2D.r[29]));
	} else {
		LW(rt, PERM_REG_1, off(CP2D.r[reg]));
	}
}

/* move from host rt to cp2 reg */
static void emitMTC2(u32 rt, u32 reg)
{
	switch (reg) {
	case 8: case 9: case 10: case 11:
		SEH(TEMP_1, rt);
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[reg]));
		break;
	case 15:
		LW(TEMP_1, PERM_REG_1, off(CP2D.p[13]));
		SW(TEMP_1, PERM_REG_1, off(CP2D.p[12])); // gteSXY0 = gteSXY1;
		LW(TEMP_1, PERM_REG_1, off(CP2D.p[14]));
		SW(TEMP_1, PERM_REG_1, off(CP2D.p[13])); // gteSXY1 = gteSXY2;

		SW(rt, PERM_REG_1, off(CP2D.p[14])); // gteSXY2 = value;
		SW(rt, PERM_REG_1, off(CP2D.p[15])); // gteSXYP = value;
		break;
	case 16: case 17: case 18: case 19:
		ANDI(TEMP_1, rt, 0xffff);
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[reg]));
		break;
	case 28:
		SW(rt, PERM_REG_1, off(CP2D.r[reg]));
		EXT(TEMP_1, rt, 0, 5);
		SLL(TEMP_1, TEMP_1, 7);
		// gteIR1 = ((value      ) & 0x1f) << 7;
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[9]));
		EXT(TEMP_1, rt, 5, 5);
		SLL(TEMP_1, TEMP_1, 7);
		// gteIR2 = ((value >>  5) & 0x1f) << 7;
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[10]));
		EXT(TEMP_1, rt, 10, 5);
		SLL(TEMP_1, TEMP_1, 7);
		// gteIR3 = ((value >> 10) & 0x1f) << 7;
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[11]));
		break;
	case 30:
		u32 *backpatch;
		SW(rt, PERM_REG_1, off(CP2D.r[30]));
		SLT(1, rt, 0);
		backpatch = (u32 *)recMem;
		BEQZ(1, 0);
		MOV(TEMP_1, rt); // delay slot

		NOR(TEMP_1, 0, rt); // temp_1 = rt ^ -1
		fixup_branch(backpatch);
		CLZ(TEMP_1, TEMP_1);
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[31]));
		break;
	default:
		SW(rt, PERM_REG_1, off(CP2D.r[reg]));
		break;
	}
}

static void recMFC2()
{
	if (autobias) cycles_pending += 2;
	if (!_Rt_) return;

	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	emitMFC2(rt, _Rd_);

	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void recMTC2()
{
	if (autobias) cycles_pending += 2;
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	emitMTC2(rt, _Rd_);

	regUnlock(rt);
}

static void recLWC2()
{
	if (autobias) cycles_pending += 3;
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

	JAL(psxMemRead32);
	ADDIU(MIPSREG_A0, rs, _Imm_); /* <BD> Branch delay slot of JAL() */

	emitMTC2(MIPSREG_V0, _Rt_);

	regUnlock(rs);
}

static void recSWC2()
{
	if (autobias) cycles_pending += 4;
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

	emitMFC2(MIPSREG_A1, _Rt_);

	JAL(psxMemWrite32);
	ADDIU(MIPSREG_A0, rs, _Imm_); /* <BD> Branch delay slot of JAL() */

	regUnlock(rs);
}

#elif defined(gte_new) || defined(gte_pcsx)

#define CP2_FUNC(f, cycle) \
extern void gte##f(); \
void rec##f() \
{ \
	if (autobias) cycles_pending+=cycle; \
	LI32(TEMP_1, psxRegs.code); \
	JAL(gte##f); \
	SW(TEMP_1, PERM_REG_1, off(code)); /* <BD> Branch delay slot of JAL() */ \
} \

CP2_FUNC(DCPL, 8);
CP2_FUNC(RTPS, 15);
CP2_FUNC(OP, 6);
CP2_FUNC(NCLIP, 8);
CP2_FUNC(DPCS, 8);
CP2_FUNC(INTPL, 8);
CP2_FUNC(MVMVA, 8);
CP2_FUNC(NCDS, 19);
CP2_FUNC(NCDT, 44);
CP2_FUNC(CDP, 13);
CP2_FUNC(NCCS, 17);
CP2_FUNC(CC, 11);
CP2_FUNC(NCS, 14);
CP2_FUNC(NCT, 30);
CP2_FUNC(SQR, 3);
CP2_FUNC(DPCT, 17);
CP2_FUNC(AVSZ3, 5);
CP2_FUNC(AVSZ4, 6);
CP2_FUNC(RTPT, 23);
CP2_FUNC(GPF, 5);
CP2_FUNC(GPL, 5);
CP2_FUNC(NCCT, 39);

static void recCFC2()
{
	if (autobias) cycles_pending += 2;
	if (!_Rt_) return;

	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP2C(_Rd_));
	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void emitCTC2(u32 rt, u32 reg)
{
	switch (reg) {
	case 4: case 12: case 20: case 26: case 27: case 29: case 30:
		SEH(TEMP_1, rt);
		SW(TEMP_1, PERM_REG_1, offCP2C(reg));
		break;

	case 31:
		//value = value & 0x7ffff000;
		//if (value & 0x7f87e000) value |= 0x80000000;
		LI32(TEMP_1, 0x7ffff000);
		AND(TEMP_1, rt, TEMP_1);	// $t0 = rt & $t0
		LI32(TEMP_2, 0x7f87e000);
		AND(TEMP_2, TEMP_1, TEMP_2);	// $t1 = $t0 & $t2
		LUI(TEMP_3, 0x8000);		// $t2 = 0x80000000
		OR(TEMP_3, TEMP_1, TEMP_3);	// $t2 = $t0 | $t2
		MOVN(TEMP_1, TEMP_3, TEMP_2);   // if ($t1) $t0 = $t2

		SW(TEMP_1, PERM_REG_1, offCP2C(reg));
		break;

	default:
		SW(rt, PERM_REG_1, offCP2C(reg));
		break;
	}
}

static void recCTC2()
{
	if (autobias) cycles_pending += 2;
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	emitCTC2(rt, _Rd_);
	regUnlock(rt);
}

/* Limit rt to [0 .. 0x1f] */
static void emitLIM(u32 rt)
{
	u32 *patch;

	SLT(1, rt, 0);			// $at = (rt < 0 ? 1 : 0)
	patch = (u32 *)recMem;
	BNE(1, 0, 0);			// if ($at != 0) goto end
	MOVN(rt, 0, 1);			// <BD> if ($at != 0) rt = 0

	LI16(MIPSREG_V0, 0x1f);		// $v0 = 0x1f
	SLTI(1, rt, 0x20);		// $at = (rt < 0x20 ? 1 : 0)
	MOVZ(rt, MIPSREG_V0, 1);	// if ($at == 0) rt = $v0

	// end:
	fixup_branch(patch);
}

/* move from cp2 reg to host rt */
static void emitMFC2(u32 rt, u32 reg)
{
	switch (reg) {
	case 1: case 3: case 5: case 8: case 9: case 10: case 11:
		LH(rt, PERM_REG_1, off(CP2D.r[reg]));
		SW(rt, PERM_REG_1, off(CP2D.r[reg]));
		break;

	case 7: case 16: case 17: case 18: case 19:
		LHU(rt, PERM_REG_1, off(CP2D.r[reg]));
		SW(rt, PERM_REG_1, off(CP2D.r[reg]));
		break;

	case 15:
		LW(rt, PERM_REG_1, off(CP2D.r[14])); // gteSXY2
		SW(rt, PERM_REG_1, off(CP2D.r[reg]));
		break;

	case 28: case 30:
		MOV(rt, 0);
		break;

	case 29:
		LW(rt, PERM_REG_1, off(CP2D.r[9])); // gteIR1
		SRA(rt, rt, 7);
		emitLIM(rt);
		LW(TEMP_1, PERM_REG_1, off(CP2D.r[10])); // gteIR2
		SRA(TEMP_1, TEMP_1, 7);
		emitLIM(TEMP_1);
		INS(rt, TEMP_1, 5, 5);
		LW(TEMP_1, PERM_REG_1, off(CP2D.r[11])); // gteIR3
		SRA(TEMP_1, TEMP_1, 7);
		emitLIM(TEMP_1);
		INS(rt, TEMP_1, 10, 5);
		SW(rt, PERM_REG_1, off(CP2D.r[29]));
		/*
		psxRegs.CP2D.r[reg] = LIM(gteIR1 >> 7, 0x1f, 0, 0) |
				(LIM(gteIR2 >> 7, 0x1f, 0, 0) << 5) |
				(LIM(gteIR3 >> 7, 0x1f, 0, 0) << 10);
		*/
		break;
	default:
		LW(rt, PERM_REG_1, off(CP2D.r[reg]));
		break;
	}
}

/* move from host rt to cp2 reg */
static void emitMTC2(u32 rt, u32 reg)
{
	switch (reg) {
	case 15:
		LW(TEMP_1, PERM_REG_1, off(CP2D.p[13]));
		SW(TEMP_1, PERM_REG_1, off(CP2D.p[12])); // gteSXY0 = gteSXY1;
		LW(TEMP_1, PERM_REG_1, off(CP2D.p[14]));
		SW(TEMP_1, PERM_REG_1, off(CP2D.p[13])); // gteSXY1 = gteSXY2;

		SW(rt, PERM_REG_1, off(CP2D.p[14])); // gteSXY2 = value;
		SW(rt, PERM_REG_1, off(CP2D.p[15])); // gteSXYP = value;
		break;

	case 28:
		SW(rt, PERM_REG_1, off(CP2D.r[reg]));
		EXT(TEMP_1, rt, 0, 5);
		SLL(TEMP_1, TEMP_1, 7);
		// gteIR1 = ((value      ) & 0x1f) << 7;
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[9]));
		EXT(TEMP_1, rt, 5, 5);
		SLL(TEMP_1, TEMP_1, 7);
		// gteIR2 = ((value >>  5) & 0x1f) << 7;
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[10]));
		EXT(TEMP_1, rt, 10, 5);
		SLL(TEMP_1, TEMP_1, 7);
		// gteIR3 = ((value >> 10) & 0x1f) << 7;
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[11]));
		break;

	case 30:
		u32 *backpatch;
		SW(rt, PERM_REG_1, off(CP2D.r[30]));
		SLT(1, rt, 0);
		backpatch = (u32 *)recMem;
		BEQZ(1, 0);
		MOV(TEMP_1, rt); // delay slot

		NOR(TEMP_1, 0, rt); // temp_1 = rt ^ -1
		fixup_branch(backpatch);
		CLZ(TEMP_1, TEMP_1);
		SW(TEMP_1, PERM_REG_1, off(CP2D.r[31]));
		break;

	case 7: case 29: case 31:
		return;

	default:
		SW(rt, PERM_REG_1, off(CP2D.r[reg]));
		break;
	}
}

static void recMFC2()
{
	if (autobias) cycles_pending += 2;
	if (!_Rt_) return;

	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	emitMFC2(rt, _Rd_);

	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void recMTC2()
{
	if (autobias) cycles_pending += 2;
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	emitMTC2(rt, _Rd_);

	regUnlock(rt);
}

static void recLWC2()
{
	if (autobias) cycles_pending += 3;
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

	JAL(psxMemRead32);
	ADDIU(MIPSREG_A0, rs, _Imm_); /* <BD> Branch delay slot of JAL() */

	emitMTC2(MIPSREG_V0, _Rt_);

	regUnlock(rs);
}

static void recSWC2()
{
	if (autobias) cycles_pending += 4;
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

	emitMFC2(MIPSREG_A1, _Rt_);

	JAL(psxMemWrite32);
	ADDIU(MIPSREG_A0, rs, _Imm_); /* <BD> Branch delay slot of JAL() */

	regUnlock(rs);
}

#endif
