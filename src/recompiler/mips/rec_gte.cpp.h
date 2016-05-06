#ifdef gte_old

#define CP2_FUNC(f, cycle) \
void gte##f(u32 code, u32 pc); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	/* TODO: Remove */ regClearJump(); \
	LI32(MIPSREG_A0, psxRegs.code); \
	LI32(MIPSREG_A1, pc); \
	CALLFunc((u32)gte##f); \
} \

#define CP2_FUNC2(f, cycle) \
void gte##f(u32 code); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	/* TODO: Remove */ regClearJump(); \
	LI32(MIPSREG_A0, psxRegs.code); \
	CALLFunc((u32)gte##f); \
}

#define CP2_FUNC3(f, cycle) \
void gte##f(); void rec##f() \
{ \
	if (autobias) cycles_pending += cycle; \
	/* TODO: Remove */ regClearJump(); \
	CALLFunc((u32)gte##f); \
}

//CP2_FUNC2(MFC2, 2);
//CP2_FUNC2(MTC2, 2);
CP2_FUNC(LWC2, 3);
CP2_FUNC(SWC2, 4);
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

static void recMFC2()
{
	if (autobias) cycles_pending += 2;
	if (!_Rt_) return;

	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	if (_Rd_ == 29) {
		LW(rt, PERM_REG_1, off(CP2D.r[9])); // gteIR1
		SRL(rt, rt, 7);
		ANDI(rt, rt, 0x1f);
		LW(TEMP_1, PERM_REG_1, off(CP2D.r[10])); // gteIR2
		SRL(TEMP_1, TEMP_1, 7);
		ANDI(TEMP_1, TEMP_1, 0x1f);
		SLL(TEMP_1, TEMP_1, 5);
		OR(rt, rt, TEMP_1);
		LW(TEMP_1, PERM_REG_1, off(CP2D.r[11])); // gteIR3
		SRL(TEMP_1, TEMP_1, 7);
		ANDI(TEMP_1, TEMP_1, 0x1f);
		SLL(TEMP_1, TEMP_1, 10);
		OR(rt, rt, TEMP_1);
		SW(rt, PERM_REG_1, off(CP2D.r[29]));
	} else {
		LW(rt, PERM_REG_1, off(CP2D.r[_Rd_]));
	}

	regMipsChanged(_Rt_);
	regUnlock(rt);
}

// defined in gte_old/gte.cpp
// TODO: expand it inline here
void MTC2(unsigned long value, int reg);

static void recMTC2()
{
	if (autobias) cycles_pending += 2;
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	MOV(MIPSREG_A0, rt);
	LI32(MIPSREG_A1, _Rd_);
	CALLFunc((u32)MTC2);
	regUnlock(rt);
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
