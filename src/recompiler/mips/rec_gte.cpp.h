#ifdef gte_old

#define CP2_FUNC(f, cycle) \
void gte##f(u32 code, u32 pc); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	LI32(MIPSREG_A0, psxRegs.code); \
	LI32(MIPSREG_A1, pc); \
	CALLFunc((u32)gte##f); \
} \

#define CP2_FUNC2(f, cycle) \
void gte##f(u32 code); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	LI32(MIPSREG_A0, psxRegs.code); \
	CALLFunc((u32)gte##f); \
}

#define CP2_FUNC3(f, cycle) \
void gte##f(); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	CALLFunc((u32)gte##f); \
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
		SLL(TEMP_1, TEMP_1, 5);
		OR(rt, rt, TEMP_1);
		LW(TEMP_1, PERM_REG_1, off(CP2D.r[11])); // gteIR3
		EXT(TEMP_1, TEMP_1, 7, 5);
		SLL(TEMP_1, TEMP_1, 10);
		OR(rt, rt, TEMP_1);
		SW(rt, PERM_REG_1, off(CP2D.r[29]));
	} else {
		LW(rt, PERM_REG_1, off(CP2D.r[reg]));
	}
}

// defined in gte_old/gte.cpp
// TODO: expand it inline here
void MTC2(unsigned long value, int reg);

/* TODO: emit inline, don't call MTC2() */
/* move from host rt to cp2 reg */
static void emitMTC2(u32 rt, u32 reg)
{
	MOV(MIPSREG_A0, rt);
	LI32(MIPSREG_A1, reg);
	CALLFunc((u32)MTC2);
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

	ADDIU(MIPSREG_A0, rs, _Imm_);
	CALLFunc((u32)psxMemRead32);

	emitMTC2(MIPSREG_V0, _Rt_);

	regUnlock(rs);
}

static void recSWC2()
{
	if (autobias) cycles_pending += 4;
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

	emitMFC2(MIPSREG_A1, _Rt_);

	ADDIU(MIPSREG_A0, rs, _Imm_);
	CALLFunc((u32)psxMemWrite32);

	regUnlock(rs);
}

#elif defined(gte_new) || defined(gte_pcsx)

#define CP2_FUNC(f, cycle) \
extern void gte##f(); \
void rec##f() \
{ \
	if (autobias) cycles_pending+=cycle; \
	regClearJump(); \
	LI32(TEMP_1, pc); \
	SW(TEMP_1, PERM_REG_1, off(pc)); \
	LI32(TEMP_1, psxRegs.code); \
	SW(TEMP_1, PERM_REG_1, off(code)); \
	CALLFunc((u32)gte##f); \
} \

CP2_FUNC(MFC2, 2);
CP2_FUNC(MTC2, 2);
CP2_FUNC(LWC2, 3);
CP2_FUNC(SWC2, 4);
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

static void recCTC2()
{
	if (autobias) cycles_pending += 2;
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	SW(rt, PERM_REG_1, offCP2C(_Rd_));
	regUnlock(rt);
}

#endif
